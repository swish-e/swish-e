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

$HTTP::URI_CLASS = "URI";   # prevent loading default URI::URL
                            # so we don't store long list of base items
                            # and eat up memory with >= URI 1.13
use LWP::RobotUA;
use HTML::LinkExtor;

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


use constant MAX_SIZE       => 5_000_000;   # Max size of document to fetch

    

    
#-----------------------------------------------------------------------

    use vars '@servers';

    my $config = shift || 'SwishSpiderConfig.pl';

    if ( lc( $config ) eq 'default' ) {
        @servers = default_urls();
    } else {
        do $config or die "Failed to read $0 configuration parameters '$config' $! $@";
    }


    print STDERR "$0: Reading parameters from '$config'\n";

    my $abort;
    local $SIG{HUP} = sub { $abort++ };

    my %visited;  # global -- I suppose would be smarter to localize it per server.

    process_server($_) for @servers;


#-----------------------------------------------------------------------


sub process_server {
    my $server = shift;

    # set defaults

    $server->{debug} ||= 0;
    $server->{debug} = 0 unless $server->{debug} =~ /^\d+$/;

    $server->{max_size} ||= MAX_SIZE;
    die "max_size parameter '$server->{max_size}' must be a number\n" unless $server->{max_size} =~ /^\d+$/;

    $server->{link_tags} = ['a'] unless ref $server->{link_tags} eq 'ARRAY';
    my %seen;
    $server->{link_tags} = [ grep { !$seen{$_}++} map { lc } @{$server->{link_tags}} ];

    die "max_depth parameter '$server->{max_depth}' must be a number\n" if defined $server->{max_depth} && $server->{max_depth} !~ /^\d+/;


    if ( $server->{keep_alive} ) {

        eval 'require LWP::Protocol::http11;';
        if ( $@ ) {
            warn "Cannot use keep alives -- $@\n";
        } else {
            LWP::Protocol::implementor('http', 'LWP::Protocol::http11');
        }
    } else {
        require LWP::Protocol::http;
        LWP::Protocol::implementor('http', 'LWP::Protocol::http');
    }


    for ( qw/ test_url test_response filter_content / ) {
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
        print STDERR "Skipping: $server->{base_url}\n";
        return;
    }

    require "HTTP/Cookies.pm" if $server->{use_cookies};
    require "Digest/MD5.pm" if $server->{use_md5};
            

    # set starting URL, and remove any specified fragment
    my $uri = URI->new( $server->{base_url} );
    $uri->fragment(undef);

    print STDERR "\n -- Starting to spider: $uri --\n" if $server->{debug};

    

    # set the starting server name (including port) -- will only spider on server:port
    
    $server->{authority} = $uri->authority;
    $server->{same} = [ $uri->authority ];
    push @{$server->{same}}, @{$server->{same_hosts}} if ref $server->{same_hosts};


    # set time to end
    
    $server->{max_time} = $server->{max_time} * 60 + time
        if $server->{max_time};


    # set default agent for log files

    $server->{agent} ||= 'swish-e spider 2.2 http://sunsite.berkeley.edu/SWISH-E/';


    # get a user agent object
    
    my $ua;
    if ( $server->{ignore_robots_file} ) {
        $ua = LWP::UserAgent->new();
        return unless $ua;
        $ua->agent( $server->{agent} );
        $ua->from( $server->{email} );

    } else {
        $ua = LWP::RobotUA->new( $server->{agent}, $server->{email} );
        return unless $ua;
        $ua->delay( $server->{delay_min} || 0.1 );
    }
        
    $server->{ua} = $ua;  # save it for fun.
    # $ua->parse_head(0);   # Don't parse the content

    $ua->cookie_jar( HTTP::Cookies->new ) if $server->{use_cookies};


    # uri, parent, depth
    eval { spider( $server, $uri ) };
    print STDERR $@ if $@;

    $start = time - $start;
    $start++ unless $start;

    my $max_width = 0;
    my $max_num = 0;
    for ( keys %{$server->{counts}} ) {
        $max_width = length if length > $max_width;
        my $val = commify( $server->{counts}{$_} );
        $max_num = length $val if length $val > $max_num;
    }

    printf STDERR "\n$0: Summary for: $server->{base_url}\n";

    for ( sort keys %{$server->{counts}} ) {
        printf STDERR "%${max_width}s: %${max_num}s  (%0.1f/sec)\n",
            $_,
            commify( $server->{counts}{$_} ),
            $server->{counts}{$_}/$start;
    }
}

#----------- Non recursive spidering ---------------------------

sub spider {
    my ( $server, $uri ) = @_;

    my @link_array = [ $uri, '', 0 ];

    $visited{ $uri->canonical }++;  # so we don't extract this link again

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


    return unless check_user_function( 'test_url', $uri, $server );

    

    # make request
    my $ua = $server->{ua};
    my $request = HTTP::Request->new('GET', $uri );


    my $content = '';

    # Really should just subclass the response object!
    $server->{no_contents} = 0;
    $server->{no_index} = 0;
    $server->{no_spider} = 0;

    my $been_here;
    my $callback = sub {

        die "test_response" if !$been_here++ && !check_user_function( 'test_response', $uri, $server, $_[1], \$_[0]  );
            

        if ( length( $content ) + length( $_[0] ) > $server->{max_size} ) {
            print STDERR "-Skipped $uri: Document exceeded $server->{max_size} bytes\n" if $server->{debug}&DEBUG_SKIPPED;
            die "too big!\n";
        }

        $content .= $_[0];

    };

    my $response = $ua->simple_request( $request, $callback, 4096 );


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
    

    # skip excluded by robots.txt
    
    if ( !$response->is_success && $response->status_line =~ 'robots.txt' ) {
        print STDERR "-Skipped $depth $uri: ", $response->status_line,"\n" if $server->{debug}&DEBUG_SKIPPED;
        $server->{counts}{'robots.txt'}++;
        return;
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

            $visited{ $u->canonical }++;  # say that we say this one.
            return [$u];
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
    



    # Index the file
    
    if ( $server->{no_index} ) {
        $server->{counts}{Skipped}++;
        print STDERR "-Skipped indexing $uri some callback set 'no_index' flag\n" if $server->{debug}&DEBUG_SKIPPED;

    } else {
        return unless check_user_function( 'filter_content', $uri, $server, $response, \$content );

        output_content( $server, \$content, $response )
            unless $server->{no_index};
    }


    # Allow skipping of this file for spidering.

    return if defined $server->{max_depth} && $depth >= $server->{max_depth};

    return extract_links( $server, \$content, $response );
}


sub check_user_function {
    my ( $fn, $uri, $server ) = ( shift, shift, shift );
    
    return 1 unless $server->{$fn};

    my $tests = ref $server->{$fn} eq 'ARRAY' ? $server->{$fn} : [ $server->{$fn} ];

    my $cnt;

    for my $sub ( @$tests ) {
        $cnt++;
        print STDERR "?Testing '$fn' user supplied function #$cnt\n" if $server->{debug} & DEBUG_INFO;

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

    
sub extract_links {
    my ( $server, $content, $response ) = @_;

    return [] unless $response->header('content-type') &&
                     $response->header('content-type') =~ m[^text/html];


    # allow skipping.
    if ( $server->{no_spider} ) {
        print STDERR '-Links not extracted: ', $response->request->uri->canonical, " some callback set 'no_spider' flag\n" if $server->{debug}&DEBUG_SKIPPED;
        return [];
    }

    $server->{Spidered}++;

    my @links;


    my $base = $response->base;

    my $p = HTML::LinkExtor->new;
    $p->parse( $$content );

    my %skipped_tags;
    for ( $p->links ) {
        my ( $tag, %attr ) = @$_;

        # which tags to use ( not reported in debug )

        print STDERR " ?? Looking at extracted tag '$tag'\n" if $server->{debug} & DEBUG_LINKS;
        
        unless ( grep { $tag eq $_ } @{$server->{link_tags}} ) {

            # each tag is reported only once per page
            print STDERR
                " ?? <$tag> skipped because not one of (",
                join( ',', @{$server->{link_tags}} ),
                ")\n"
                     if $server->{debug} & DEBUG_LINKS && !$skipped_tags{$tag}++;
                     
            next;
        }
        

        my $links = $HTML::Tagset::linkElements{$tag};
        $links = [$links] unless ref $links;

        my $found;
        my %seen;
        for ( @$links ) {
            if ( $attr{ $_ } ) {  # ok tag

                my $u = URI->new_abs( $attr{$_},$base );

                $u->fragment( undef );

                # Ignore duplicates on the same page
                if ( $seen{$u}++ ) {
                    $server->{counts}{Duplicates}++;
                    return;
                }

                unless ( $u->host ) {
                    print STDERR qq[ ?? <$tag $_="$u"> skipped because missing host name\n] if $server->{debug} & DEBUG_LINKS;
                    next;
                }

                unless ( grep { $u->authority eq $_ } @{$server->{same}} ) {
                    print STDERR qq[ ?? <$tag $_="$u"> skipped because different authority (server:port)\n] if $server->{debug} & DEBUG_LINKS;
                    $server->{counts}{'Off-site links'}++;
                    next;
                }
                
                $u->authority( $server->{authority} );  # Force all the same host name

                # Don't add the link if already seen  - these are so common that we don't report
                if ( $visited{ $u->canonical }++ ) {
                    #$server->{counts}{Skipped}++;
                    $server->{counts}{Duplicates}++;
                    next;
                }

                push @links, $u;
                print STDERR qq[ ++ <$tag $_="$u"> Added to list of links to follow\n] if $server->{debug} & DEBUG_LINKS;
                $found++;
            }
        }

        if ( !$found && $server->{debug} & DEBUG_LINKS ) {
            my $s = "<$tag";
            $s .= ' ' . qq[$_="$attr{$_}"] for sort keys %attr;
            $s .= '>';
                
            print STDERR " ?? tag $s did not include any links to follow\n";
        }
        
    }

    print STDERR "! Found ", scalar @links, " links in ", $response->base, "\n" if $server->{debug} & DEBUG_INFO;


    return \@links;
}

sub output_content {
    my ( $server, $content, $response ) = @_;

    $server->{indexed}++;

    $server->{counts}{'Total Bytes'} += length $$content;
    $server->{counts}{'Total Docs'}++;


    my $headers = join "\n",
        'Path-Name: ' .  $response->request->uri,
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
    die "$0: Must list URLs when using 'default'\n" unless @ARGV;

    my @content_types  = qw{ text/html text/plain };

    return map {
        {
            base_url        => $_,
            email           => 'swish@domain.invalid',
            delay_min       => .0001,
            link_tags       => [qw/ a frame /],
            test_url        => sub { $_[0]->path !~ /\.(?:gif|jpeg|.png)$/i },

            test_response   => sub {
                my $content_type = $_[2]->content_type;
                my $ok = grep { $_ eq $content_type } @content_types;
                return 1 if $ok;
                print STDERR "$_[0] $content_type != (@content_types)\n";
                return;

            }
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

  begin indexing:
    swish-e -S prog -c swish.config

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
is not the same configuration file that swish-e uses.

The example configuration file C<SwishSpiderConfig.pl> is
included in the C<prog-bin> directory along with this script.  Please just use it as an
example, as it contains more settings than you probably want to use.  Start with a tiny config file
and add settings as required by your situation.

The available configuration parameters are discussed below.

If all that sounds confusing, then you can run the spider with default settings.  In fact, you can
run the spider without using swish just to make sure it works.  Just run

    ./spider.pl default http://someserver.com/sometestdoc.html

And you should see F<sometestdoc.html> dumped to your screen.  Get ready to kill the script
if the file you request contains links!

If the first parameter passed to the spider is the word "default" then the spider uses
the default parameters, and the following parameter(s) are expected to be URL(s) to spider.
Otherwise, the first parameter is considered to be the name of the configuration file (as described
below).  When using C<-S prog>, the swish-e configuration setting
C<SwishProgParameters> is used to pass parameters to the program specified
with C<IndexDir> or the C<-i> switch.

The spider does require Perl's LWP library and a few other reasonably common modules.
Most well maintained systems should have these modules installed.  If not, start here:

    http://search.cpan.org/search?dist=libwww-perl
    http://search.cpan.org/search?dist=HTML-Parser

See more below in C<REQUIREMENTS>.    

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
        base_url    => 'http://sunsite.berkeley.edu/SWISH-E/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu/ ],
        email       => 'my@email.address',
    );
    @servers = ( \%serverA, \%serverB, );

=head1 CONFIGURATION OPTIONS

This describes the required and optional keys in the server configuration hash, in random order...

=over 4

=item base_url

This required setting is the starting URL for spidering.

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

Note: This should probably be called B<same_authority> because it compares the URI C<authority>
against the list of host names in C<same_hosts>.  So, if you specify a port name in you will
probably want to specify the port name in the the list of hosts in C<same_hosts>:

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

=back

=head1 CALLBACK FUNCTIONS

Three callback functions can be defined in your parameter hash.
These optional settings are I<callback> subroutines that are called while
processing URLs.

The first routine allows you to skip processing of urls based on the url before the request
to the server is made.  The second function allows you to filter based on the response from the
remote server (such as by content-type).  And the third function allows you to filter the returned
content from the remote server.

The function calls are wrapped in an eval, so calling die (or doing something that dies) will just cause
that URL to be skipped.  If you really want to stop processing you need to set $server->{abort} in your
subroutine (or kill -HUP yourself).

The first two parameters passed are a URI object (to have access to the current URL), and
a reference to the current server hash.  Other parameters may be passes, as described below.

Note that you can create your own counters to display in the summary list when spidering
is finished by adding a value to the hash pointed to by C<$server->{counts}>.  See the example below.

Each callback function B<must> return true to continue processing the URL.  Returning false will
cause processing of I<the current> URL to be skipped.
Setting flags in the passed in C<$server> hash can be used for finer
control over the spidering.  Currently there are four flags:

    no_contents -- index only the title (or file name), and not the contents
    no_index    -- do not index this file, but continue to spider if HTML
    no_spider   -- index, but do not spider this file for links to follow
    abort       -- stop spidering any more files

Swish (not this spider) has a configuration directive C<NoContents> that will instruct swish to
index only the title (or file name), and not the contents.  This is often used when
indexing binary files such as image files, but can also be used with html
files to index only the document titles.

You can turn this feature on for specific documents by setting a flag in
the server hash passed into the C<test_response> or C<filter_contents> subroutines.
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
to be ignored.  Therefore, it would be smart to use a C<filter_contents> callback routine to
replace the contents with single character (you cannot use the empty string at this time).

A similar flag may be set to prevent indexing a document at all, but still allow spidering.
In general, if you want completely skip spidering a file you return false from one of the three
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
file will not be processed by swish.

B<Note:> If C<no_index> is set in a C<test_response> callback function then
the document I<will not be filtered>.  That is, your C<filter_content>
callback function will not be called.  In general, this is what you want.
The exception might be where you need to filter a C<text/html> file to
fix up links before following links in that file (spidering that file).  In
this case you would want to set the C<no_index> flag in the C<filter_content>
function instead of the C<test_response> callback.

The C<no_spider> flag can be set to avoid spiderering an HTML file.  The file will still be indexed unless
C<no_index> is also set.  But if you do not want to index and spider, then simply return false from one of the three
callback funtions.


=over 4

=item test_url

Test URL is called before a request is made to the server for this URL.
Your callback function is passed two parameters, a URI object and a reference
to the server parameter hash.  If this function returns true the URL will be
requested from the web server.  Returning false will cause the spider to skip this
URL.

This callback function allows you to filter out URLs before taking the time to
request the resource (document, image, whatever) from the web server.  This might
be a good place, for example, to filter out requests for images.

For example:

    $server{test_url} = sub {
        my ( $uri, $server ) = @_;
        return $uri->path !~ /\.(gif|jpeg|jpg|png)$/;
    }

=item test_response

C<test_response> is similar to C<test_url>, but it is called I<after> the URI
has been requested from the web server (and some content has been returned).
In addition to the URI, and server hash passed, the HTTP::Response object is passed as
a third parameter.
Again, returning false will cause the spider to ignore this resource.

The spider requests a document in "chunks" or 4096 bytes.  4096 is only a suggestion
of how many bytes to return in each chunk.  The C<test_response> routine is
called when the first chunk is received only.  This allows ignoring (aborting)
reading of a very large file, for example, without having to read the entire file.
Although not much use, a reference to this chunk is passed as the forth parameter.

This subroutine is useful for filtering documents based on data returned in the
headers of the response.  For example, if you have lots of large images on your
web server, and they cannot be easily identified by the URL (as above), then
you could use this callback subroutine to check the Content-Type: header.

    $server{test_response} = sub {
        my ( $uri, $server, $response ) = @_;
        return $response->content_type !~ m[^image/];
    }


=item filter_content

This callback subroutine can filter the contents before it gets
passed to swish for indexing.  For example, you could add meta tags to HTML
documents or convert PDF or Word documents into text, HTML, or XML.

Again, your routine must return true to continue processing this resource.

The HTTP::Response object is passed as the third parameter, and a reference to the
content is passed as the forth parameter.

For example:

    use pdf2xml;  # included example pdf converter module
    $server{filter_content} = sub {
       my ( $uri, $server, $response, $content_ref ) = @_;

       return 1 if $response->content_type eq 'text/html';
       return 0 unless $response->content_type eq 'application/pdf';

       # for logging counts
       $server->{counts}{'PDF transformed'}++;

       $$content_ref = ${pdf2xml( $content_ref )};
       $$content_ref =~ tr/ / /s;  # squash extra space
       return 1;
    }

=back

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

