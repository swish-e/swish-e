#!/usr/local/bin/perl -w
use strict;


# $Id$
#
# "prog" document source for spidering web servers
#
# For documentation, type:
#
#       perldoc spider.pl
#
# Apr 7, 2001 -- added quite a bit of bulk for easier debugging
#
# Nov 19, 2001 -- to do, build a server object so we are not using the passed in hash,
#                 and so we can warn on invalid config settings.

$HTTP::URI_CLASS = "URI";   # prevent loading default URI::URL
                            # so we don't store long list of base items
                            # and eat up memory with >= URI 1.13
use LWP::RobotUA;
use HTML::LinkExtor;
use HTML::Tagset;

use vars '$VERSION';
$VERSION = sprintf '%d.%02d', q$Revision$ =~ /: (\d+)\.(\d+)/;

use vars '$bit';
use constant DEBUG_ERRORS   => $bit = 1;    # program errors
use constant DEBUG_URL      => $bit <<= 1;  # print out every URL processes
use constant DEBUG_HEADERS  => $bit <<= 1;  # prints the response headers
use constant DEBUG_FAILED   => $bit <<= 1;  # failed to return a 200
use constant DEBUG_SKIPPED  => $bit <<= 1;  # didn't index for some reason
use constant DEBUG_INFO     => $bit <<= 1;  # more verbose
use constant DEBUG_LINKS    => $bit <<= 1;  # prints links as they are extracted

my %DEBUG_MAP = (
    errors  => DEBUG_ERRORS,
    url     => DEBUG_URL,
    headers => DEBUG_HEADERS,
    failed  => DEBUG_FAILED,
    skipped => DEBUG_SKIPPED,
    info    => DEBUG_INFO,
    links   => DEBUG_LINKS,
);
    


use constant MAX_SIZE       => 5_000_000;   # Max size of document to fetch
use constant MAX_WAIT_TIME  => 30;          # request time.

#Can't locate object method "host" via package "URI::mailto" at ../prog-bin/spider.pl line 473.
#sub URI::mailto::host { return '' };


# This is not the right way to do this.
sub UNIVERSAL::host { '' };
sub UNIVERSAL::port { '' };
sub UNIVERSAL::host_port { '' };


    
#-----------------------------------------------------------------------

    use vars '@servers';

    my $config = shift || 'SwishSpiderConfig.pl';

    if ( lc( $config ) eq 'default' ) {
        @servers = default_urls();
    } else {
        do $config or die "Failed to read $0 configuration parameters '$config' $! $@";
    }


    print STDERR "$0: Reading parameters from '$config'\n" unless $ENV{SPIDER_QUIET};

    my $abort;
    local $SIG{HUP} = sub { warn "Caught SIGHUP\n"; $abort++ } unless $^O =~ /Win32/i;

    my %visited;  # global -- I suppose would be smarter to localize it per server.

    my %validated;
    my %bad_links;

    for my $s ( @servers ) {
        if ( !$s->{base_url} ) {
            die "You must specify 'base_url' in your spider config settings\n";
        }

        my @urls = ref $s->{base_url} eq 'ARRAY' ? @{$s->{base_url}} :( $s->{base_url});
        for my $url ( @urls ) {
            $s->{base_url} = $url;
            process_server( $s );
        }
    }


    if ( %bad_links ) {
        print STDERR "\nBad Links:\n\n";
        foreach my $page ( sort keys %bad_links ) {
            print STDERR "On page: $page\n";
            printf(STDERR " %-40s  %s\n", $_, $validated{$_} ) for @{$bad_links{$page}};
            print STDERR "\n";
        }
    }


#-----------------------------------------------------------------------


sub process_server {
    my $server = shift;

    # set defaults

    if ( $ENV{SPIDER_DEBUG} ) {
        $server->{debug} = 0;

        $server->{debug} |= (exists $DEBUG_MAP{lc $_} ? $DEBUG_MAP{lc $_} : die "Bad debug setting passed in environment '$_'\nOptions are: " . join( ', ', keys %DEBUG_MAP) ."\n")
            for split /\s*,\s*/, $ENV{SPIDER_DEBUG};

    } else {
        $server->{debug} ||= 0;
        die "debug parameter '$server->{debug}' must be a number\n" unless $server->{debug} =~ /^\d+$/;
    }

    $server->{quiet} ||= $ENV{SPIDER_QUIET} || 0;


    $server->{max_size} ||= MAX_SIZE;
    die "max_size parameter '$server->{max_size}' must be a number\n" unless $server->{max_size} =~ /^\d+$/;


    $server->{max_wait_time} ||= MAX_WAIT_TIME;
    die "max_wait_time parameter '$server->{max_wait_time}' must be a number\n" if $server->{max_wait_time} !~ /^\d+/;



    $server->{link_tags} = ['a'] unless ref $server->{link_tags} eq 'ARRAY';
    $server->{link_tags_lookup} = { map { lc, 1 } @{$server->{link_tags}} };

    die "max_depth parameter '$server->{max_depth}' must be a number\n" if defined $server->{max_depth} && $server->{max_depth} !~ /^\d+/;


    for ( qw/ test_url test_response filter_content/ ) {
        next unless $server->{$_};
        $server->{$_} = [ $server->{$_} ] unless ref $server->{$_} eq 'ARRAY';
        my $n;
        for my $sub ( @{$server->{$_}} ) {
            $n++;
            die "Entry number $n in $_ is not a code reference\n" unless ref $sub eq 'CODE';
        }
    }

    

    my $start = time;

    if ( $server->{skip} ) {
        print STDERR "Skipping: $server->{base_url}\n" unless $server->{quiet};
        return;
    }

    require "HTTP/Cookies.pm" if $server->{use_cookies};
    require "Digest/MD5.pm" if $server->{use_md5};
            

    # set starting URL, and remove any specified fragment
    my $uri = URI->new( $server->{base_url} );
    $uri->fragment(undef);

    if ( $uri->userinfo ) {
        die "Can't specify parameter 'credentials' because base_url defines them\n"
            if $server->{credentials};
        $server->{credentials} = $uri->userinfo;
        $uri->userinfo( undef );
    }


    print STDERR "\n -- Starting to spider: $uri --\n" if $server->{debug};

    

    # set the starting server name (including port) -- will only spider on server:port
    

    # All URLs will end up with this host:port
    $server->{authority} = $uri->canonical->authority;

    # All URLs must match this scheme ( Jan 22, 2002 - spot by Darryl Friesen )
    $server->{scheme} = $uri->scheme;



    # Now, set the OK host:port names
    $server->{same} = [ $uri->canonical->authority ];
    
    push @{$server->{same}}, @{$server->{same_hosts}} if ref $server->{same_hosts};

    $server->{same_host_lookup} = { map { $_, 1 } @{$server->{same}} };

    


    # set time to end
    
    $server->{max_time} = $server->{max_time} * 60 + time
        if $server->{max_time};


    # set default agent for log files

    $server->{agent} ||= 'swish-e spider 2.2 http://swish-e.org/';


    # get a user agent object


    my $ua;

    if ( $server->{ignore_robots_file} ) {
        $ua = LWP::UserAgent->new;
        return unless $ua;
        $ua->agent( $server->{agent} );
        $ua->from( $server->{email} );

    } else {
        $ua = LWP::RobotUA->new( $server->{agent}, $server->{email} );
        return unless $ua;
        $ua->delay( $server->{delay_min} || 0.1 );
    }

    # Set the timeout on the server and using Windows.
    $ua->timeout( $server->{max_wait_time} ) if $^O =~ /Win32/i;

        
    $server->{ua} = $ua;  # save it for fun.
    # $ua->parse_head(0);   # Don't parse the content

    $ua->cookie_jar( HTTP::Cookies->new ) if $server->{use_cookies};

    if ( $server->{keep_alive} ) {

        if ( $ua->can( 'conn_cache' ) ) {
            my $keep_alive = $server->{keep_alive} =~ /^\d+$/ ? $server->{keep_alive} : 1;
            $ua->conn_cache( { total_capacity => $keep_alive } );

        } else {
            warn "Can't use keep-alive: conn_cache method not available\n";
        }
    }


    # uri, parent, depth
    eval { spider( $server, $uri ) };
    print STDERR $@ if $@;


    # provide a way to call a function in the config file when all done
    check_user_function( 'spider_done', undef, $server );
    

    return if $server->{quiet};


    $start = time - $start;
    $start++ unless $start;

    my $max_width = 0;
    my $max_num = 0;
    for ( keys %{$server->{counts}} ) {
        $max_width = length if length > $max_width;
        my $val = commify( $server->{counts}{$_} );
        $max_num = length $val if length $val > $max_num;
    }


    printf STDERR "\nSummary for: $server->{base_url}\n";

    for ( sort keys %{$server->{counts}} ) {
        printf STDERR "%${max_width}s: %${max_num}s  (%0.1f/sec)\n",
            $_,
            commify( $server->{counts}{$_} ),
            $server->{counts}{$_}/$start;
    }
}


#-----------------------------------------------------------------------
# Deal with Basic Authen



# Thanks Gisle!
sub get_basic_credentials {
    my($uri, $server, $realm ) = @_;
    my $netloc = $uri->canonical->host_port;

    my ($user, $password);

    eval {
        local $SIG{ALRM} = sub { die "timed out\n" };
        alarm( $server->{credential_timeout} || 30 ) unless $^O =~ /Win32/i;

        if (  $uri->userinfo ) {
            print STDERR "\nSorry: invalid username/password\n";
            $uri->userinfo( undef );
        }
            

        print STDERR "Need Authentication for $uri at realm '$realm'\n(<Enter> skips)\nUsername: ";
        $user = <STDIN>;
        chomp($user);
        die "No Username specified\n" unless length $user;

        alarm( $server->{credential_timeout} || 30 ) unless $^O =~ /Win32/i;

        print STDERR "Password: ";
        system("stty -echo");
        $password = <STDIN>;
        system("stty echo");
        print STDERR "\n";  # because we disabled echo
        chomp($password);

        alarm( 0 ) unless $^O =~ /Win32/i;
    };

    return if $@;

    return join ':', $user, $password;


}
            

        

#----------- Non recursive spidering ---------------------------

sub spider {
    my ( $server, $uri ) = @_;

    # Validate the first link, just in case
    return unless check_link( $uri, $server, '', '(Base URL)' );

    my @link_array = [ $uri, '', 0 ];

    while ( @link_array ) {

        die if $abort || $server->{abort};

        my ( $uri, $parent, $depth ) = @{shift @link_array};
        
        my $new_links = process_link( $server, $uri, $parent, $depth );

        push @link_array, map { [ $_, $uri, $depth+1 ] } @$new_links if $new_links;
            
    }
}    
        

#----------- Process a url and return links -----------------------
sub process_link {
    my ( $server, $uri, $parent, $depth ) = @_;


    $server->{counts}{'Unique URLs'}++;

    die "$0: Max files Reached\n"
        if $server->{max_files} && $server->{counts}{'Unique URLs'} > $server->{max_files};

    die "$0: Time Limit Exceeded\n"
        if $server->{max_time} && $server->{max_time} < time;



    # make request
    my $ua = $server->{ua};
    my $request = HTTP::Request->new('GET', $uri );


    my $content = '';

    # Really should just subclass the response object!
    $server->{no_contents} = 0;
    $server->{no_index} = 0;
    $server->{no_spider} = 0;


    # Set basic auth if defined - use URI specific first, then credentials
    if ( my ( $user, $pass ) = split /:/, ( $uri->userinfo || $server->{credentials} || '' ) ) {
        $request->authorization_basic( $user, $pass );
    }


    

    my $been_here;
    my $callback = sub {

        # Reset alarm;
        alarm( $server->{max_wait_time} ) unless $^O =~ /Win32/i;


        # Cache user/pass
        if ( $server->{cur_realm} && $uri->userinfo ) {
             my $key = $uri->canonical->host_port . ':' . $server->{cur_realm};
             $server->{auth_cache}{$key} =  $uri->userinfo;
        }

        $uri->userinfo( undef ) unless $been_here;

        die "test_response" if !$been_here++ && !check_user_function( 'test_response', $uri, $server, $_[1], \$_[0]  );
            

        if ( length( $content ) + length( $_[0] ) > $server->{max_size} ) {
            print STDERR "-Skipped $uri: Document exceeded $server->{max_size} bytes\n" if $server->{debug}&DEBUG_SKIPPED;
            die "too big!\n";
        }

        $content .= $_[0];

    };

    my $response;

    eval {
        local $SIG{ALRM} = sub { die "timed out\n" };
        alarm( $server->{max_wait_time} ) unless $^O =~ /Win32/i;
        $response = $ua->simple_request( $request, $callback, 4096 );
        alarm( 0 ) unless $^O =~ /Win32/i;
    };


    return if $server->{abort};


    if ( $response && $response->code == 401 && $response->header('WWW-Authenticate') && $response->header('WWW-Authenticate') =~ /realm="([^"]+)"/i ) {
        my $realm = $1;

        my $user_pass;

        # Do we have a cached user/pass for this realm?
        my $key = $uri->canonical->host_port . ':' . $realm;

        if ( $user_pass = $server->{auth_cache}{$key} ) {

            # If we didn't just try it, try again
            unless( $uri->userinfo && $user_pass eq $uri->userinfo ) {
                $uri->userinfo( $user_pass );
                return process_link( $server, $uri, $parent, $depth );
            }
        }

        # otherwise, prompt:


        if ( $user_pass = get_basic_credentials( $uri, $server, $realm ) ) {
            $uri->userinfo( $user_pass );

            $server->{cur_realm} = $realm;  # save so we can cache
            my $links = process_link( $server, $uri, $parent, $depth );
            delete $server->{cur_realm};

            return $links;
        }
        print STDERR "Skipping $uri\n";
    }

    $uri->userinfo( undef );
        


    # Log the response
    
    if ( ( $server->{debug} & DEBUG_URL ) || ( $server->{debug} & DEBUG_FAILED && !$response->is_success)  ) {
        print STDERR '>> ',
          join( ' ',
                ( $response->is_success ? '+Fetched' : '-Failed' ),
                $depth,
                "Cnt: $server->{counts}{'Unique URLs'}",
                $uri,
                ( $response->status_line || $response->status || 'unknown status' ),
                ( $response->content_type || 'Unknown content type'),
                ( $response->content_length || '???' ),
                "parent:$parent",
           ),"\n";
    }




    # If the LWP callback aborts

    if ( $response->header('client-aborted') ) {
        $server->{counts}{Skipped}++;
        return;
    }

    $response->request->uri->userinfo( undef ) if $response->request;


    # skip excluded by robots.txt
    
    if ( !$response->is_success && $response->status_line =~ 'robots.txt' ) {
        print STDERR "-Skipped $depth $uri: ", $response->status_line,"\n" if $server->{debug}&DEBUG_SKIPPED;
        $server->{counts}{'robots.txt'}++;
        return;
    }


    # Report bad links (excluding those skipped by robots.txt

    if ( $server->{validate_links} && !$response->is_success ) {
        validate_link( $server, $uri, $parent, $response );
    }



    # And check for meta robots tag
    # -- should probably be done in request sub to avoid fetching docs that are not needed

    unless ( $server->{ignore_robots_file} ) {
        if ( my $directives = $response->header('X-Meta-ROBOTS') ) {
            my %settings = map { lc $_, 1 } split /\s*,\s*/, $directives;
            $server->{no_contents}++ if exists $settings{nocontents};  # an extension for swish
            $server->{no_index}++    if exists $settings{noindex};
            $server->{no_spider}++   if exists $settings{nofollow};
        }
    }




    print STDERR "\n----HEADERS for $uri ---\n", $response->headers_as_string,"-----END HEADERS----\n\n"
       if $server->{debug} & DEBUG_HEADERS;


    unless ( $response->is_success ) {

        # look for redirect
        if ( $response->is_redirect && $response->header('location') ) {
            my $u = URI->new_abs( $response->header('location'), $response->base );

            if ( $u->canonical eq $uri->canonical ) {
                print STDERR "Warning: $uri redirects to itself!.\n";
                return;
            }

            return [$u] if check_link( $u, $server, $response->base, '(redirect)','Location' );
        }
        return;
    }

    return unless $content;  # $$$ any reason to index empty files?
    

    # make sure content is unique - probably better to chunk into an MD5 object above

    if ( $server->{use_md5} ) {
        my $digest =  Digest::MD5::md5($content);

        if ( $visited{ $digest } ) {

            print STDERR "-Skipped $uri has same digest as $visited{ $digest }\n"
                if $server->{debug} & DEBUG_SKIPPED;
        
            $server->{counts}{Skipped}++;
            $server->{counts}{'MD5 Duplicates'}++;
            return;
        }
        $visited{ $digest } = $uri;
    }


    # Extract out links (if not too deep)

    my $links_extracted = extract_links( $server, \$content, $response )
        unless defined $server->{max_depth} && $depth >= $server->{max_depth};


    # Index the file
    
    if ( $server->{no_index} ) {
        $server->{counts}{Skipped}++;
        print STDERR "-Skipped indexing $uri some callback set 'no_index' flag\n" if $server->{debug}&DEBUG_SKIPPED;

    } else {
        return $links_extracted unless check_user_function( 'filter_content', $uri, $server, $response, \$content );

        output_content( $server, \$content, $uri, $response )
            unless $server->{no_index};
    }



    return $links_extracted;
}

#===================================================================================================
#  Calls a user-defined function
#
#---------------------------------------------------------------------------------------------------

sub check_user_function {
    my ( $fn, $uri, $server ) = ( shift, shift, shift );
    
    return 1 unless $server->{$fn};

    my $tests = ref $server->{$fn} eq 'ARRAY' ? $server->{$fn} : [ $server->{$fn} ];

    my $cnt;

    for my $sub ( @$tests ) {
        $cnt++;
        print STDERR "?Testing '$fn' user supplied function #$cnt '$uri'\n" if $server->{debug} & DEBUG_INFO;

        my $ret;
        
        eval { $ret = $sub->( $uri, $server, @_ ) };

        if ( $@ ) {
            print STDERR "-Skipped $uri due to '$fn' user supplied function #$cnt death '$@'\n" if $server->{debug} & DEBUG_SKIPPED;
            $server->{counts}{Skipped}++;
            return;
        }
            
        next if $ret;
        
        print STDERR "-Skipped $uri due to '$fn' user supplied function #$cnt\n" if $server->{debug} & DEBUG_SKIPPED;
        $server->{counts}{Skipped}++;
        return;
    }
    print STDERR "+Passed all $cnt tests for '$fn' user supplied function\n" if $server->{debug} & DEBUG_INFO;
    return 1;
}


#==============================================================================================
#  Extract links from a text/html page
#
#   Call with:
#       $server - server object
#       $content - ref to content
#       $response - response object
#
#----------------------------------------------------------------------------------------------
    
sub extract_links {
    my ( $server, $content, $response ) = @_;

    return unless $response->header('content-type') &&
                     $response->header('content-type') =~ m[^text/html];

    # allow skipping.
    if ( $server->{no_spider} ) {
        print STDERR '-Links not extracted: ', $response->request->uri->canonical, " some callback set 'no_spider' flag\n" if $server->{debug}&DEBUG_SKIPPED;
        return;
    }

    $server->{Spidered}++;

    my @links;


    my $base = $response->base;

    print STDERR "\nExtracting links from ", $response->request->uri, ":\n" if $server->{debug} & DEBUG_LINKS;

    my $p = HTML::LinkExtor->new;
    $p->parse( $$content );

    my %skipped_tags;

    for ( $p->links ) {
        my ( $tag, %attr ) = @$_;

        # which tags to use ( not reported in debug )

        my $attr = join ' ', map { qq[$_="$attr{$_}"] } keys %attr;

        print STDERR "\nLooking at extracted tag '<$tag $attr>'\n" if $server->{debug} & DEBUG_LINKS;

        unless ( $server->{link_tags_lookup}{$tag} ) {
        
            # each tag is reported only once per page
            print STDERR
                "   <$tag> skipped because not one of (",
                join( ',', @{$server->{link_tags}} ),
                ")\n" if $server->{debug} & DEBUG_LINKS && !$skipped_tags{$tag}++;

            if ( $server->{validate_links} && $tag eq 'img' && $attr{src} ) {
                my $img = URI->new_abs( $attr{src}, $base );
                validate_link( $server, $img, $base );
            }
                     
            next;
        }
        
        # Grab which attribute(s) which might contain links for this tag
        my $links = $HTML::Tagset::linkElements{$tag};
        $links = [$links] unless ref $links;


        my $found;


        # Now, check each attribut to see if a link exists
        
        for my $attribute ( @$links ) {
            if ( $attr{ $attribute } ) {  # ok tag

                # Create a URI object
                
                my $u = URI->new_abs( $attr{$attribute},$base );

                next unless check_link( $u, $server, $base, $tag, $attribute );
                
                push @links, $u;
                print STDERR qq[   $attribute="$u" Added to list of links to follow\n] if $server->{debug} & DEBUG_LINKS;
                $found++;
            }
        }


        if ( !$found && $server->{debug} & DEBUG_LINKS ) {
            print STDERR "  tag did not include any links to follow or is a duplicate\n";
        }
        
    }

    print STDERR "! Found ", scalar @links, " links in ", $response->base, "\n\n" if $server->{debug} & DEBUG_INFO;


    return \@links;
}




#=============================================================================
# This function check's if a link should be added to the list to spider
#
#   Pass:
#       $u - URI object
#       $server - the server hash
#       $base - the base or parent of the link
#
#   Returns true if a valid link
#
#   Calls the user function "test_url".  Link rewriting before spider
#   can be done here.
#
#------------------------------------------------------------------------------
sub check_link {
    my ( $u, $server, $base, $tag, $attribute ) = @_;
    
    $tag ||= '';
    $attribute ||= '';


    # Kill the fragment
    $u->fragment( undef );


    # This should not happen, but make sure we have a host to check against

    unless ( $u->host ) {
        print STDERR qq[ ?? <$tag $attribute="$u"> skipped because missing host name\n] if $server->{debug} & DEBUG_LINKS;
        return;
    }


    # Here we make sure we are looking at a link pointing to the correct (or equivalent) host

    unless ( $server->{scheme} eq $u->scheme && $server->{same_host_lookup}{$u->canonical->authority} ) {

        print STDERR qq[ ?? <$tag $attribute="$u"> skipped because different host\n] if $server->{debug} & DEBUG_LINKS;
        $server->{counts}{'Off-site links'}++;
        validate_link( $server, $u, $base ) if $server->{validate_links};
        return;
    }
    
    $u->host_port( $server->{authority} );  # Force all the same host name

    # Allow rejection of this URL by user function

    return unless check_user_function( 'test_url', $u, $server );


    # Don't add the link if already seen  - these are so common that we don't report

    if ( $visited{ $u->canonical }++ ) {
        #$server->{counts}{Skipped}++;
        $server->{counts}{Duplicates}++;


        # Just so it's reported for all pages 
        if ( $server->{validate_links} && $validated{$u->canonical} ) {
            push @{$bad_links{ $base->canonical }}, $u->canonical;
        }

        return;
    }

    return 1;
}


#=============================================================================
# This function is used to validate links that are off-site.
#
#   It's just a very basic link check routine that lets you validate the
#   off-site links at the same time as indexing.  Just because we can.
#
#------------------------------------------------------------------------------
sub validate_link {
    my ($server, $uri, $base, $response ) = @_;

   # Already checked? 

    if ( exists $validated{ $uri->canonical } )
    {
        # Add it to the list of bad links on that page if it's a bad link.
        push @{$bad_links{ $base->canonical }}, $uri->canonical
            if $validated{ $uri->canonical };

        return;            
    }

    $validated{ $uri->canonical } = 0;  # mark as checked and ok.

    unless ( $response ) {
        my $ua = LWP::UserAgent->new;
        my $request = HTTP::Request->new('HEAD', $uri->canonical );

        eval {
            local $SIG{ALRM} = sub { die "timed out\n" };
            alarm( $server->{max_wait_time} ) unless $^O =~ /Win32/i;
            $response = $ua->simple_request( $request );
            alarm( 0 ) unless $^O =~ /Win32/i;
        };

        if ( $@ ) {
            $server->{counts}{'Bad Links'}++;
            my $msg = $@;
            $msg =~ tr/\n//s;
            $validated{ $uri->canonical } = $msg;
            push @{$bad_links{ $base->canonical }}, $uri->canonical;
            return;
        }
    }

    return if $response->is_success;

    my $error = $response->status_line || $response->status || 'unknown status';

    $error .= ' ' . URI->new_abs( $response->header('location'), $response->base )->canonical
        if $response->is_redirect && $response->header('location');

    $validated{ $uri->canonical } = $error;
    push @{$bad_links{ $base->canonical }}, $uri->canonical;
}
    

sub output_content {
    my ( $server, $content, $uri, $response ) = @_;

    $server->{indexed}++;

    unless ( length $$content ) {
        print STDERR "Warning: document '", $response->request->uri, "' has no content\n";
        $$content = ' ';
    }


    $server->{counts}{'Total Bytes'} += length $$content;
    $server->{counts}{'Total Docs'}++;


    my $headers = join "\n",
        'Path-Name: ' .  $uri,
        'Content-Length: ' . length $$content,
        '';

    $headers .= 'Last-Mtime: ' . $response->last_modified . "\n"
        if $response->last_modified;


    $headers .= "No-Contents: 1\n" if $server->{no_contents};
    print "$headers\n$$content";

    die "$0: Max indexed files Reached\n"
        if $server->{max_indexed} && $server->{counts}{'Total Docs'} >= $server->{max_indexed};
}
        


sub commify {
    local $_  = shift;
    1 while s/^([-+]?\d+)(\d{3})/$1,$2/;
    return $_;
}

sub default_urls {

    my $validate = 0;
    if ( $ARGV[0] eq 'validate' ) {
        shift @ARGV;
        $validate = 1;
    }

    die "$0: Must list URLs when using 'default'\n" unless @ARGV;


    my @content_types  = qw{ text/html text/plain };

    return map {
        {
            #debug => DEBUG_HEADERS,
            #debug => DEBUG_URL|DEBUG_SKIPPED|DEBUG_INFO,
            base_url        => \@ARGV,
            email           => 'swish@domain.invalid',
            delay_min       => .0001,
            link_tags       => [qw/ a frame /],
            test_url        => sub { $_[0]->path !~ /\.(?:gif|jpeg|png)$/i },

            test_response   => sub {
                my $content_type = $_[2]->content_type;
                my $ok = grep { $_ eq $content_type } @content_types;
                return 1 if $ok;
                print STDERR "$_[0] $content_type != (@content_types)\n";
                return;
            },
            validate_links => $validate,

        }
    } @ARGV;
}

            

__END__

=head1 NAME

spider.pl - Example Perl program to spider web servers

=head1 SYNOPSIS

  swish.config:
    IndexDir ./spider.pl
    SwishProgParameters spider.config
    # other swish-e settings

  spider.config:
    @servers = (
        {
            base_url    => 'http://myserver.com/',
            email       => 'me@myself.com',
            # other spider settings described below
        },
    );

Begin indexing:

    swish-e -S prog -c swish.config

Note: When running on some versions of Windows (e.g. Win ME and Win 98 SE)
you may need to index using the command:

    perl spider.pl | swish-e -S prog -c swish.conf -i stdin

This pipes the output of the spider directly into swish.
   

=head1 DESCRIPTION

This is a swish-e "prog" document source program for spidering
web servers.  It can be used instead of the C<http> method for
spidering with swish.

The spider typically uses a configuration
file that lists the URL(s) to spider, and configuration parameters that control
the behavior of the spider.  In addition, you may define I<callback> perl functions
in the configuration file that can dynamically change the behavior of the spider
based on URL, HTTP response headers, or the content of the fetched document.  These
callback functions can also be used to filter or convert documents (e.g. PDF, gzip, MS Word)
into a format that swish-e can parse.  Some examples are provided.

You define "servers" to spider, set a few parameters,
create callback routines, and start indexing as the synopsis above shows.
The spider requires its own configuration file (unless you want the default values).  This
is NOT the same configuration file that swish-e uses.

The example configuration file C<SwishSpiderConfig.pl> is
included in the C<prog-bin> directory along with this script.  Please just use it as an
example, as it contains more settings than you probably want to use.  Start with a tiny config file
and add settings as required by your situation.

The available configuration parameters are discussed below.

If all that sounds confusing, then you can run the spider with default settings.  In fact, you can
run the spider without using swish just to make sure it works.  Just run

    ./spider.pl default http://someserver.com/sometestdoc.html

And you should see F<sometestdoc.html> dumped to your screen.  Get ready to kill the script
if the file you request contains links as the output from the fetched pages will be displayed.

    ./spider.pl default http://someserver.com/sometestdoc.html > output.file

might be more friendly.    

If the first parameter passed to the spider is the word "default" (as in the preceeding example)
then the spider uses the default parameters,
and the following parameter(s) are expected to be URL(s) to spider.
Otherwise, the first parameter is considered to be the name of the configuration file (as described
below).  When using C<-S prog>, the swish-e configuration setting
C<SwishProgParameters> is used to pass parameters to the program specified
with C<IndexDir> or the C<-i> switch.

If you do not specify any parameters the program will look for the file

    SwishSpiderConfig.pl

in the current directory.    

The spider does require Perl's LWP library and a few other reasonably common modules.
Most well maintained systems should have these modules installed.  If not, start here:

    http://search.cpan.org/search?dist=libwww-perl
    http://search.cpan.org/search?dist=HTML-Parser

See more below in C<REQUIREMENTS>.  It's a good idea to check that you are running
a current version of these modules.

=head2 Robots Exclusion Rules and being nice

This script will not spider files blocked by F<robots.txt>.  In addition,
The script will check for <meta name="robots"..> tags, which allows finer
control over what files are indexed and/or spidered.
See http://www.robotstxt.org/wc/exclusion.html for details.

This spider provides an extension to the <meta> tag exclusion, by adding a
C<NOCONTENTS> attribute.  This attribute turns on the C<no_contents> setting, which
asks swish-e to only index the document's title (or file name if not title is found).

For example:

      <META NAME="ROBOTS" CONTENT="NOCONTENTS, NOFOLLOW">

says to just index the document's title, but don't index its contents, and don't follow
any links within the document.  Granted, it's unlikely that this feature will ever be used...

If you are indexing your own site, and know what you are doing, you can disable robot exclusion by
the C<ignore_robots_file> configuration parameter, described below.  This disables both F<robots.txt>
and the meta tag parsing.

This script only spiders one file at a time, so load on the web server is not that great.
And with libwww-perl-5.53_91 HTTP/1.1 keep alive requests can reduce the load on
the server even more (and potentially reduce spidering time considerably!)

Still, discuss spidering with a site's administrator before beginning.
Use the C<delay_min> to adjust how fast the spider fetches documents.
Consider running a second web server with a limited number of children if you really
want to fine tune the resources used by spidering.

=head2 Duplicate Documents

The spider program keeps track of URLs visited, so a document is only indexed
one time.  

The Digest::MD5 module can be used to create a "fingerprint" of every page
indexed and this fingerprint is used in a hash to find duplicate pages.
For example, MD5 will prevent indexing these as two different documents:

    http://localhost/path/to/some/index.html
    http://localhost/path/to/some/

But note that this may have side effects you don't want.  If you want this
file indexed under this URL:

    http://localhost/important.html

But the spider happens to find the exact content in this file first:

    http://localhost/developement/test/todo/maybeimportant.html

Then only that URL will be indexed.    

MD5 may slow down indexing a tiny bit, so test with and without if speed is an
issue (which it probably isn't since you are spidering in the first place).
This feature will also use more memory.

Note: the "prog" document source in swish bypasses many swish-e configuration settings.
For example, you cannot use the C<IndexOnly> directive with the "prog" document
source.  This is by design to limit the overhead when using an external program
for providing documents to swish; after all, with "prog", if you don't want to index a file, then
don't give it to swish to index in the first place.

So, for spidering, if you do not wish to index images, for example, you will
need to either filter by the URL or by the content-type returned from the web
server.  See L<CALLBACK FUNCTIONS|CALLBACK FUNCTIONS> below for more information.

=head1 REQUIREMENTS

Perl 5 (hopefully at least 5.00503) or later.

You must have the LWP Bundle on your computer.  Load the LWP::Bundle via the CPAN.pm shell,
or download libwww-perl-x.xx from CPAN (or via ActiveState's ppm utility).
Also required is the the HTML-Parser-x.xx bundle of modules also from CPAN
(and from ActiveState for Windows).

    http://search.cpan.org/search?dist=libwww-perl
    http://search.cpan.org/search?dist=HTML-Parser

You will also need Digest::MD5 if you wish to use the MD5 feature.
HTML::Tagset is also required.
Other modules may be required (for example, the pod2xml.pm module
has its own requirementes -- see perldoc pod2xml for info).

The spider.pl script, like everyone else, expects perl to live in /usr/local/bin.
If this is not the case then either add a symlink at /usr/local/bin/perl
to point to where perl is installed
or modify the shebang (#!) line at the top of the spider.pl program.

Note that the libwww-perl package does not support SSL (Secure Sockets Layer) (https)
by default.  See F<README.SSL> included in the libwww-perl package for information on
installing SSL support.

=head1 CONFIGURATION FILE

Configuration is not very fancy.  The spider.pl program simply does a
C<do "path";> to read in the parameters and create the callback subroutines.
The C<path> is the first parameter passed to the spider script, which is set
by the Swish-e configuration setting C<SwishProgParameters>.

For example, if in your swish-e configuration file you have

    SwishProgParameters /path/to/config.pl
    IndexDir /home/moseley/swish-e/prog-bin/spider.pl

And then run swish as

    swish-e -c swish.config -S prog

swish will run C</home/moseley/swish-e/prog-bin/spider.pl> and the spider.pl
program will receive as its first parameter C</path/to/config.pl>, and
spider.pl will read C</path/to/config.pl> to get the spider configuration
settings.  If C<SwishProgParameters> is not set, the program will try to
use C<SwishSpiderConfig.pl> by default.

There is a special case of:

    SwishProgParameters default http://www.mysite/index.html ...

Where default parameters are used.  This will only index documents of type
C<text/html> or C<text/plain>, and will skip any file with an extension that matches
the pattern:

    /\.(?:gif|jpeg|.png)$/i

This can be useful for indexing just your web documnts, but you will probably want finer
control over your spidering by using a configuration file.

The configuration file must set a global variable C<@servers> (in package main).
Each element in C<@servers> is a reference to a hash.  The elements of the has
are described next.  More than one server hash may be defined -- each server
will be spidered in order listed in C<@servers>, although currently a I<global> hash is
used to prevent spidering the same URL twice.

Examples:

    my %serverA = (
        base_url    => 'http://swish-e.org/',
        same_hosts  => [ qw/www.swish-e.org/ ],
        email       => 'my@email.address',
    );
    my %serverB = (
        ...
        ...
    );
    @servers = ( \%serverA, \%serverB, );

=head1 CONFIGURATION OPTIONS

This describes the required and optional keys in the server configuration hash, in random order...

=over 4

=item base_url

This required setting is the starting URL for spidering.

Typically, you will just list one URL for the base_url.  You may specify more than one
URL as a reference to a list

    base_url => [qw! http://swish-e.org/ http://othersite.org/other/index.html !],

You may specify a username and password:

    base_url => 'http://user:pass@swish-e.org/index.html',

but you may find that to be a security issue.  If a URL is protected by Basic Authentication
you will be prompted for a username and password.  This might be a slighly safer way to go.

The parameter C<max_wait_time> controls how long to wait for user entry before skipping the
current URL.

See also C<credentials> below.


=item same_hosts

This optional key sets equivalent B<authority> name(s) for the site you are spidering.
For example, if your site is C<www.mysite.edu> but also can be reached by
C<mysite.edu> (with or without C<www>) and also C<web.mysite.edu> then:


Example:

    $serverA{base_url} = 'http://www.mysite.edu/index.html';
    $serverA{same_hosts} = ['mysite.edu', 'web.mysite.edu'];

Now, if a link is found while spidering of:

    http://web.mysite.edu/path/to/file.html

it will be considered on the same site, and will actually spidered and indexed
as:

    http://www.mysite.edu/path/to/file.html

Note: This should probably be called B<same_host_port> because it compares the URI C<host:port>
against the list of host names in C<same_hosts>.  So, if you specify a port name in you will
want to specify the port name in the the list of hosts in C<same_hosts>:

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu:4444/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu:4444/ ],
        email       => 'my@email.address',
    );


=item email

This required key sets the email address for the spider.  Set this to
your email address.

=item agent

This optional key sets the name of the spider.

=item link_tags

This optional tag is a reference to an array of tags.  Only links found in these tags will be extracted.
The default is to only extract links from C<a> tags.

For example, to extract tags from C<a> tags and from C<frame> tags:

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu:4444/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu:4444/ ],
        email       => 'my@email.address',
        link_tags   => [qw/ a frame /],
    );


=item delay_min

This optional key sets the delay in minutes to wait between requests.  See the
LWP::RobotUA man page for more information.  The default is 0.1 (6 seconds),
but in general you will probably want it much smaller.  But, check with
the webmaster before using too small a number.

=item max_wait_time

This setting is the number of seconds to wait for data to be returned from
the request.  Data is returned in chunks to the spider, and the timer is reset each time
a new chunk is reported.  Therefore, documents (requests) that take longer than this setting
should not be aborted as long as some data is received every max_wait_time seconds.
The default it 30 seconds.

NOTE: This option has no effect on Windows.

=item max_time

This optional key will set the max minutes to spider.   Spidering
for this host will stop after C<max_time> minutes, and move on to the
next server, if any.  The default is to not limit by time.

=item max_files

This optional key sets the max number of files to spider before aborting.
The default is to not limit by number of files.  This is the number of requests
made to the remote server, not the total number of files to index (see C<max_indexed>).
This count is displayted at the end of indexing as C<Unique URLs>.

This feature can (and perhaps should) be use when spidering a web site where dynamic
content may generate unique URLs to prevent run-away spidering.

=item max_indexed

This optional key sets the max number of files that will be indexed.
The default is to not limit.  This is the number of files sent to
swish for indexing (and is reported by C<Total Docs> when spidering ends).

=item max_size

This optional key sets the max size of a file read from the web server.
This B<defaults> to 5,000,000 bytes.  If the size is exceeded the resource is
skipped and a message is written to STDERR if the DEBUG_SKIPPED debug flag is set.

=item keep_alive

This optional parameter will enable keep alive requests.  This can dramatically speed
up searching and reduce the load on server being spidered.  The default is to not use
keep alives, although enabling it will probably be the right thing to do.

To get the most out of keep alives, you may want to set up your web server to
allow a lot of requests per single connection (i.e MaxKeepAliveRequests on Apache).
Apache's default is 100, which should be good.  (But, in general, don't enable KeepAlives
on a mod_perl server.)

Note: try to filter as many documents as possible B<before> making the request to the server.  In
other words, use C<test_url> to look for files ending in C<.html> instead of using C<test_response> to look
for a content type of C<text/html> if possible.
Do note that aborting a request from C<test_response> will break the
current keep alive connection.

Note: you must have at least libwww-perl-5.53_90 installed to use this feature.

=item skip

This optional key can be used to skip the current server.  It's only purpose
is to make it easy to disable a server in a configuration file.

=item debug

Set this to a number to display different amounts of info while spidering.  Writes info
to STDERR.  Zero/undefined is no debug output.

The following constants are defined for debugging.  They may be or'ed together to
get the individual debugging of your choice.

Here are basically the levels:

    DEBUG_ERRORS   general program errors (not used at this time)
    DEBUG_URL      print out every URL processes
    DEBUG_HEADERS  prints the response headers
    DEBUG_FAILED   failed to return a 200
    DEBUG_SKIPPED  didn't index for some reason
    DEBUG_INFO     more verbose
    DEBUG_LINKS    prints links as they are extracted

For example, to display the urls processed, failed, and skipped use:

    debug => DEBUG_URL | DEBUG_FAILED | DEBUG_SKIPPED,

To display the returned headers

    debug => DEBUG_HEADERS,

You can easily run the spider without using swish for debugging purposes:

    ./spider.pl test.config > spider.out

And you will see debugging info as it runs, and the fetched documents will be saved
in the C<spider.out> file.

Debugging can be also be set by an environment variable when running swish.  This will
override any setting in the configuration file.  Set the variable SPIDER_DEBUG when running
the spider.  You can specify any of the above debugging options, separated by a comma.

For example with Bourne type shell:

    SPIDER_DEBUG=url,links

=item quiet

If this is true then normal, non-error messages will be supressed.  Quiet mode can also
be set by setting the environment variable SPIDER_QUIET to any true value.

    SPIDER_QUIET=1

=item max_depth

The C<max_depth> parameter can be used to limit how deeply to recurse a web site.
The depth is just a count of levels of web pages decended, and not related to
the number of path elements in a URL.

A max_depth of zero says to only spider the page listed as the C<base_url>.  A max_depth of one will
spider the C<base_url> page, plus all links on that page, and no more.  The default is to spider all
pages.


=item ignore_robots_file

If this is set to true then the robots.txt file will not be checked when spidering
this server.  Don't use this option unless you know what you are doing.

=item use_cookies

If this is set then a "cookie jar" will be maintained while spidering.  Some
(poorly written ;) sites require cookies to be enabled on clients.

This requires the HTTP::Cookies module.

=item use_md5

If this setting is true, then a MD5 digest "fingerprint" will be made from the content of every
spidered document.  This digest number will be used as a hash key to prevent
indexing the same content more than once.  This is helpful if different URLs
generate the same content.

Obvious example is these two documents will only be indexed one time:

    http://localhost/path/to/index.html
    http://localhost/path/to/

This option requires the Digest::MD5 module.  Spidering with this option might
be a tiny bit slower.

=item validate_links

Just a hack.  If you set this true the spider will do HEAD requests all links (e.g. off-site links), just
to make sure that all your links work.

=item credentials

You may specify a username and password to be used automatically when spidering:

    credentials => 'username:password',

A username and password supplied in a URL will override this setting.

=item credential_timeout

Sets the number of seconds to wait for user input when prompted for a username or password.
The default is 30 seconds.

=back

=head1 CALLBACK FUNCTIONS

Three callback functions can be defined in your parameter hash.
These optional settings are I<callback> subroutines that are called while
processing URLs.

A little perl discussion is in order:

In perl, a scalar variable can contain a reference to a subroutine.  The config example above shows
that the configuration parameters are stored in a perl I<hash>.

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu:4444/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu:4444/ ],
        email       => 'my@email.address',
        link_tags   => [qw/ a frame /],
    );

There's two ways to add a reference to a subroutine to this hash:

sub foo {
    return 1;
}

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu:4444/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu:4444/ ],
        email       => 'my@email.address',
        link_tags   => [qw/ a frame /],
        test_url    => \&foo,  # a reference to a named subroutine
    );

Or the subroutine can be coded right in place:    

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu:4444/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu:4444/ ],
        email       => 'my@email.address',
        link_tags   => [qw/ a frame /],
        test_url    => sub { reutrn 1; },
    );

The above example is not very useful as it just creates a user callback function that
always returns a true value (the number 1).  But, it's just an example.

The function calls are wrapped in an eval, so calling die (or doing something that dies) will just cause
that URL to be skipped.  If you really want to stop processing you need to set $server->{abort} in your
subroutine (or send a kill -HUP to the spider).

The first two parameters passed are a URI object (to have access to the current URL), and
a reference to the current server hash.  The C<server> hash is just a global hash for holding data, and
useful for setting flags as describe belwo.

Other parameters may be also passes, as described below.
In perl parameters are passed in an array called "@_".  The first element (first parameter) of
that array is $_[0], and the second is $_[1], and so on.  Depending on how complicated your
function is you may wish to shift your parameters off of the @_ list to make working with them
easier.  See the examples below.


To make use of these routines you need to understand when they are called, and what changes
you can make in your routines.  Each routine deals with a given step, and returning false from
your routine will stop processing for the current URL.

=over 4

=item test_url

C<test_url> allows you to skip processing of urls based on the url before the request
to the server is made.  This function is called for the C<base_url> links (links you define in
the spider configuration file) and for every link extracted from a fetched web page.

This function is a good place to skip links that you are not interested in following.  For example,
if you know there's no point in requesting images then you can exclude them like:

    test_url => sub {
        my $uri = shift;
        return 0 if $uri->path =~ /\.(gif|jpeg|png)$/;
        return 1;
    },

Or to write it another way:

    test_url => sub { $_[0]->path !~ /\.(gif|jpeg|png)$/ },

Another feature would be if you were using a web server where path names are
NOT case sensitive (e.g. Windows).  You can normalize all links in this situation
using something like

    test_url => sub {
        my $uri = shift;
        return 0 if $uri->path =~ /\.(gif|jpeg|png)$/;

        $uri->path( lc $uri->path ); # make all path names lowercase
        return 1;
    },

The important thing about C<test_url> (compared to the other callback functions) is that
it is called while I<extracting> links, not while actually fetching that page from the web
server.  Returning false from C<test_url> simple says to not add the URL to the list of links to
spider.

You may set a flag in the server hash (second parameter) to tell the spider to abort processing.

    test_url => sub {
        my $server = $_[1];
        $server->{abort}++ if $_[0]->path =~ /foo\.html/;
        return 1;
    },

You cannot use the server flags:

    no_contents
    no_index
    no_spider


This is discussed below.

=item test_response

This function allows you to filter based on the response from the
remote server (such as by content-type).  This function is called while the
web pages is being fetched from the remote server, typically after just enought
data has been returned to read the response from the web server.

The spider requests a document in "chunks" of 4096 bytes.  4096 is only a suggestion
of how many bytes to return in each chunk.  The C<test_response> routine is
called when the first chunk is received only.  This allows ignoring (aborting)
reading of a very large file, for example, without having to read the entire file.
Although not much use, a reference to this chunk is passed as the forth parameter.

Web servers use a Content-Type: header to define the type of data returned from the server.
On a web server you could have a .jpeg file be a web page -- file extensions may not always
indicate the type of the file.  The third parameter ($_[2]) returned is a reference to a
HTTP::Response object:

For example, to only index true HTML (text/html) pages:

    test_response => sub {
        my $content_type = $_[2]->content_type;
        return $content_type =~ m!text/html!;
    },

You can also set flags in the server hash (the second parameter) to control indexing:

    no_contents -- index only the title (or file name), and not the contents
    no_index    -- do not index this file, but continue to spider if HTML
    no_spider   -- index, but do not spider this file for links to follow
    abort       -- stop spidering any more files

For example, to avoid index the contents of "private.html", yet still follow any links
in that file:

    test_response => sub {
        my $server = $_[1];
        $server->{no_index}++ if $_[0]->path =~ /private\.html$/;
        return 1;
    },

Note: Do not modify the URI object in this call back function.
    

=item filter_content

This callback function is called right before sending the content to swish.
Like the other callback function, returning false will cause the URL to be skipped.
Setting the C<abort> server flag and returning false will abort spidering.

You can also set the C<no_contents> flag.

This callback function is passed four parameters.
The URI object, server hash, the HTTP::Response object,
and a reference to the content.

You can modify the content as needed.  For example you might not like upper case:

    filter_content => sub {
        my $content_ref = $_[3];

        $$content_ref = lc $$content_ref;
        return 1;
    },

I more reasonable example would be converting PDF or MS Word documents for parsing by swish.
Examples of this are provided in the F<prog-bin> directory of the swish-e distribution.

You may also modify the URI object to change the path name passed to swish for indexing.

    filter_content => sub {
        my $uri = $_[0];
        $uri->host('www.other.host') ;
        return 1;
    },

Swish-e's ReplaceRules feature can also be used for modifying the path name indexed.

Here's a bit more advanced example of indexing text/html and PDF files only:

    use pdf2xml;  # included example pdf converter module
    $server{filter_content} = sub {
       my ( $uri, $server, $response, $content_ref ) = @_;

       return 1 if $response->content_type eq 'text/html';
       return 0 unless $response->content_type eq 'application/pdf';

       # for logging counts
       $server->{counts}{'PDF transformed'}++;

       $$content_ref = ${pdf2xml( $content_ref )};
       return 1;
    }



=back

Note that you can create your own counters to display in the summary list when spidering
is finished by adding a value to the hash pointed to by C<$server->{counts}>.

    test_url => sub {
        my $server = $_[1];
        $server->{no_index}++ if $_[0]->path =~ /private\.html$/;
        $server->{counts}{'Private Files'}++;
        return 1;
    },


Each callback function B<must> return true to continue processing the URL.  Returning false will
cause processing of I<the current> URL to be skipped.

=head2 More on setting flags

Swish (not this spider) has a configuration directive C<NoContents> that will instruct swish to
index only the title (or file name), and not the contents.  This is often used when
indexing binary files such as image files, but can also be used with html
files to index only the document titles.

As shown above, you can turn this feature on for specific documents by setting a flag in
the server hash passed into the C<test_response> or C<filter_content> subroutines.
For example, in your configuration file you might have the C<test_response> callback set
as:

   
    test_response => sub {
        my ( $uri, $server, $response ) = @_;
        # tell swish not to index the contents if this is of type image
        $server->{no_contents} = $response->content_type =~ m[^image/];
        return 1;  # ok to index and spider this document
    }

The entire contents of the resource is still read from the web server, and passed
on to swish, but swish will also be passed a C<No-Contents> header which tells
swish to enable the NoContents feature for this document only.

Note: Swish will index the path name only when C<NoContents> is set, unless the document's
type (as set by the swish configuration settings C<IndexContents> or C<DefaultContents>) is
HTML I<and> a title is found in the html document.

Note: In most cases you probably would not want to send a large binary file to swish, just
to be ignored.  Therefore, it would be smart to use a C<filter_content> callback routine to
replace the contents with single character (you cannot use the empty string at this time).

A similar flag may be set to prevent indexing a document at all, but still allow spidering.
In general, if you want completely skip spidering a file you return false from one of the 
callback routines (C<test_url>, C<test_response>, or C<filter_content>).  Returning false from any of those
three callbacks will stop processing of that file, and the file will B<not> be spidered.

But there may be some cases where you still want to spider (extract links) yet, not index the file.  An example
might be where you wish to index only PDF files, but you still need to spider all HTML files to find
the links to the PDF files.

    $server{test_response} = sub {
        my ( $uri, $server, $response ) = @_;
        $server->{no_index} = $response->content_type ne 'application/pdf';
        return 1;  # ok to spider, but don't index
    }

So, the difference between C<no_contents> and C<no_index> is that C<no_contents> will still index the file
name, just not the contents.  C<no_index> will still spider the file (if it's C<text/html>) but the
file will not be processed by swish at all.

B<Note:> If C<no_index> is set in a C<test_response> callback function then
the document I<will not be filtered>.  That is, your C<filter_content>
callback function will not be called.

The C<no_spider> flag can be set to avoid spiderering an HTML file.  The file will still be indexed unless
C<no_index> is also set.  But if you do not want to index and spider, then simply return false from one of the three
callback funtions.


=head1 SIGNALS

Sending a SIGHUP to the running spider will cause it to stop spidering.  This is a good way to abort spidering, but
let swish index the documents retrieved so far.

=head1 SEE ALSO

L<URI> L<LWP::RobotUA> L<WWW::RobotRules> L<Digest::MD5>

=head1 COPYRIGHT

Copyright 2001 Bill Moseley

This program is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=head1 SUPPORT

Send all questions to the The SWISH-E discussion list.

See http://sunsite.berkeley.edu/SWISH-E.

=cut   

