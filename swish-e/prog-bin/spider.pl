#!/usr/local/bin/perl -w
use strict;

use LWP::RobotUA;
use HTML::LinkExtor;
use HTTP::Cookies;

#use LWP::Debug qw(+);

#--------------------- Global Config ----------------------------
=pod

  Set the base_url to the first page to index
  same_hosts is a reference to an array of host names that are the same
  this is to keep from indexing the same doc by different host names

  This program only spiders one file at a time, so you can probably
  set the delay_min small.  Check with your system admin to make
  sure this is not a problem.

                    ### Ask before you spider! ###

Example run command:

   ~/swish-e/docsource > ../src/swish-e -S prog -i ./spider.pl

=cut   

#  @servers is a list of hashes -- so you can spider more than one site
#  in one run (or different parts of the same tree)

my @servers = (
    {
        base_url    => 'http://sunsite.berkeley.edu/SWISH-E/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@hank.org',
        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 20,        # Max files to spider
        use_cookies => 0,         # True will keep cookie jar
    },

    {
        base_url    => 'http://www.infopeople.org/',
        same_hosts  => [ qw/infopeople.org/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@hank.org',
        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 20,        # Max files to spider
        use_cookies => 0,         # True will keep cookie jar
    },

);    
    

#---------------------- Public Functions ------------------------------


# This subroutine lets you check a URL before requesting the
# document from the server
# return false to skip the link

sub test_url {
    my ( $url, $URI_object, $server ) = @_;
    return 1;  # Ok to index/spider

    return 0;  # No, don't index or spider;
    
}

# This routine is called when the *first* block of data comes back
# from the server.  If you return false no more content will be read
# from the server.  $response is a HTTP::Response object.


sub test_response {
    my ( $url, $response, $server ) = @_;

    unless ( $response->header('content-type') =~ m!^text/html! ) {
        $server->{no_contents}++;
        return 1;  # ok to spider, but pass the index_no_contents to swish
    }

    # return if $url =~ /4444/;


    return 1;  # ok to index and spider
}
    
#-----------------------------------------------------------------------


    process_server($_) for @servers;


#-----------------------------------------------------------------------

my %visited;

sub process_server {
    my $server = shift;

    my $uri = URI->new( $server->{base_url} );

    $server->{host} = $uri->host;
    $server->{same} = [ $uri->host ];
    push @{$server->{same}}, @{$server->{same_hosts}} if ref $server->{same_hosts};

    $server->{max_time} = $server->{max_time} * 60 + time
        if $server->{max_time};

    $server->{$_} = 0 for qw/urls count duplicate skipped indexed/;
    
    
    
    my $ua = LWP::RobotUA->new( $server->{agent}, $server->{email} );
    next unless $ua;

    $ua->cookie_jar( HTTP::Cookies->new ) if $server->{use_cookies};
    
    $ua->delay( $server->{delay_min} || 0.1 );

    $server->{ua} = $ua;
    eval {
        process_link( $server, $server->{base_url} );
    };
    print @_ if @_;

    print STDERR join "\n",
                 "$0: $server->{base_url} ",
                 "       URLs : $server->{urls}",
                 "   Spidered : $server->{count}",
                 "    Indexed : $server->{indexed}",
                 "   Duplicats: $server->{duplicate}",
                 "   Skipped  : $server->{skipped}",
                 '','';

}

sub process_link {
    my ( $server, $url ) = @_;


    if ( $visited{ $url }++ ) {
        $server->{skipped}++;
        $server->{duplicate}++;
        return;
    }

    die "$0: Max files Reached\n"
        if $server->{max_files} && $server->{urls}++ > $server->{max_files};

    die "$0: Time Limit Exceeded\n"
        if $server->{max_time} && $server->{max_time} < time;

    my $ua = $server->{ua};
    my $uri = URI->new( $url );
    return unless test_url( $url, $uri, $server );

    my $request = HTTP::Request->new('GET', $url );

    my $content = '';

    $server->{no_contents} = 0;

    my $first;
    my $callback = sub {
        unless ( $first++ ) {
            unless ( test_response( $url, $_[1], $server ) ) {
                $server->{skipped}++;
                die "skipped\n";
            }
        }

        $content .= $_[0];

    };

    $server->{count}++;
    my $response = $ua->simple_request( $request, $callback, 4096 );


    if ( $response->is_success && $content ) {

        output_content( $server, \$content, $response );

        my $links = extract_links( $server, \$content, $response );

        # Now spider
        process_link( $server, $_ ) for @$links; 


    } elsif ( $response->is_redirect && $response->header('location') ) {
        process_link( $server,  $response->header('location') );
    }

    

}
    
sub extract_links {
    my ( $server, $content, $response ) = @_;

    return [] unless $response->header('content-type') &&
                     $response->header('content-type') =~ m[^text/html];

    my @links;


    my $base = $response->base;

    my $p = HTML::LinkExtor->new;
    $p->parse( $$content );

    for ( $p->links ) {
        my ( $tag, %attr ) = @$_;
        next unless $tag eq 'a';
        next unless $attr{href};

        my $u = URI->new_abs( $attr{href},$base );
        next unless $u->scheme =~ /^http/;

        next unless grep { $u->host eq $_ } @{$server->{same}};
        $u->host( $server->{host} );
        push @links, $u;
    }

    return \@links;
}

sub output_content {
    my ( $server, $content, $response ) = @_;

    $server->{indexed}++;


    my $headers = join "\n",
        'Path-Name: ' .  $response->request->uri,
        'Content-Length: ' . length $$content,
        '';

    $headers .= 'Last-Mtime: ' . $response->last_modified . "\n"
        if $response->last_modified;


    $headers .= "No-Contents: 1\n" if $server->{no_contents};
    print "$headers\n$$content";
}
        


