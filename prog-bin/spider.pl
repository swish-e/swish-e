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


use LWP::RobotUA;
use HTML::LinkExtor;

use vars '$VERSION';
$VERSION = sprintf '%d.%02d', q$Revision$ =~ /: (\d+)\.(\d+)/;



    
#-----------------------------------------------------------------------

    use vars '@servers';

    my $config = shift || 'SwishSpiderConfig.pl';
    do $config or die "Failed to read $0 configuration parameters '$config' $!";

    print STDERR "$0: Reading parameters from '$config'\n";

    my $abort;
    local $SIG{HUP} = sub { $abort++ };

    my $depth = 0;
    my %visited;  # global -- I suppose would be smarter to localize it per server.

    process_server($_) for @servers;


#-----------------------------------------------------------------------


sub process_server {
    my $server = shift;

    my $start = time;

    if ( $server->{skip} ) {
        print STDERR "Skipping: $server->{base_url}\n";
        return;
    }

    require "HTTP/Cookies.pm" if $server->{use_cookies};
    require "Digest/MD5.pm" if $server->{use_md5};
            

    my $uri = URI->new( $server->{base_url} );
    $uri->fragment(undef);

    $server->{authority} = $uri->authority;
    $server->{same} = [ $uri->authority ];
    push @{$server->{same}}, @{$server->{same_hosts}} if ref $server->{same_hosts};

    $server->{max_time} = $server->{max_time} * 60 + time
        if $server->{max_time};

    $server->{agent} ||= 'swish-e spider 2.2 http://sunsite.berkeley.edu/SWISH-E/';

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


    $ua->cookie_jar( HTTP::Cookies->new ) if $server->{use_cookies};



    $server->{$_} = 0 for qw/urls count duplicate md5_duplicate skipped indexed spidered/;
    
    eval {
        process_link( $server, $uri );
    };
    print STDERR $@ if $@;

    $start = time - $start;
    $start++ unless $start;
    my $rate = sprintf( '%0.1f' ,$server->{urls}/$start  );


    print STDERR join "\n",
                 "$0: $server->{base_url}",
                 "       URLs : $server->{urls}  ($rate/sec)",
                 "   Spidered : $server->{spidered}",
                 "    Indexed : $server->{indexed}",
                 "  Duplicats : $server->{duplicate}",
                 "    Skipped : $server->{skipped}",
                 "   MD5 Dups : $server->{md5_duplicate}",
                 '','';

}


my $parent;
#----------- Process a url and recurse -----------------------
sub process_link {
    my ( $server, $uri ) = @_;

    die if $abort;


    thin_dots( $uri, $server ) if $server->{thin_dots};


    if ( $visited{ $uri->canonical }++ ) {
        $server->{skipped}++;
        $server->{duplicate}++;
        return;
    }

    die "$0: Max files Reached\n"
        if $server->{max_files} && ++$server->{urls} > $server->{max_files};

    die "$0: Time Limit Exceeded\n"
        if $server->{max_time} && $server->{max_time} < time;

    my $ua = $server->{ua};
    return if $server->{test_url} && ! $server->{test_url}->( $uri, $server );

    my $request = HTTP::Request->new('GET', $uri );

    my $content = '';

    $server->{no_contents} = 0;

    my $first;
    my $callback = sub {
        unless ( $first++ ) {
            if ( $server->{test_response} && ! $server->{test_response}->( $_[1], $server ) ) {
                $server->{skipped}++;
                die "skipped\n";
            }
        }

        $content .= $_[0];

    };

    my $response = $ua->simple_request( $request, $callback, 4096 );


    if ( !$response->is_success && $response->status_line =~ 'robots.txt' ) {
        print STDERR "-- $depth $uri skipped: ", $response->status_line,"\n" if $server->{debug};
        return;
    }



    if ( $server->{debug} && $response->status_line !~ 'robots.txt' ) {
        print STDERR '>> ',
          join( ' ',
                $depth,
                $response->request->uri->canonical,
                $response->code,
                $response->header('content-type'),
                $response->header('content-length'),
           ),"\n";

        if ( $response->code ne 200 && $parent ) {
            print STDERR "   on page: ", $parent,"\n";
        }
           
    }


    if ( $response->is_success && $content ) {

        # make sure content is unique - probably better to chunk into an MD5 object above

        if ( $server->{use_md5} ) {
            my $digest =  Digest::MD5::md5($content);

            if ( $visited{ $digest } ) {

                print STDERR "-- page $uri has same digest as $visited{ $digest }\n"
                    if $server->{debug};
            
                $server->{skipped}++;
                $server->{md5_duplicate}++;
                return;
            }
            $visited{ $digest } = $uri;
        }
        

        $server->{filter_content} && $server->{filter_content}->( $response, $server, \$content );

        output_content( $server, \$content, $response );

        $server->{spidered}++;
        my $links = extract_links( $server, \$content, $response );

        # Now spider
        my $last_page = $parent || '';
        $parent = $uri;
        $depth++;
        process_link( $server, $_ ) for @$links;
        $depth--;
        $parent = $last_page;


    } elsif ( $response->is_redirect && $response->header('location') ) {
        my $u = URI->new_abs( $response->header('location'), $response->base );
        process_link( $server, $u );
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
        $u->fragment( undef );

        next unless $u->scheme =~ /^http/;  # no https at this time.

        unless ( $u->host ) {
            warn "Bad link?? $u in $base\n";
            next;
        }

        next unless grep { $u->authority eq $_ } @{$server->{same}};
        $u->authority( $server->{authority} );  # Force all the same host name

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
        

# This is not very good...

sub thin_dots {
    my ( $uri, $server ) = shift;


    my $p = $uri->path;

    my $c = 0;
    my @new;

    my $trailing_slash = substr( $p, -1, 1 ) eq '/';

    foreach ( split m[/+], $p ) {
        /^\.\.$/ && do {
            pop @new;
            $c--;
            next;
        };

            $new[$c++] = $_;
    }
    $uri->path( join '/', @new );

    $uri->path( $uri->path . '/' ) if $trailing_slash;  # cough, hack, cough.

    if ( $server->{debug} && $p ne $uri->path ) {
        print STDERR "thin_dots: $p -> ", $uri->path, "\n";
    }
}

__END__

=head1 NAME

spider.pl - Example Perl program to spider web servers

=head1 SYNOPSIS

 ~/swish-e/docsource > ../src/swish-e -S prog -i ./spider.pl

=head1 DESCRIPTION

This is a swish-e "prog" document source program for spidering
web servers.  It can be used instead of the C<http> method for
spidering with swish.

You define "servers" to spider, set a few parameters,
create callback routines, and start indexing as the synopsis above shows.
All this setup is currently done in a loaded configuration file.
The associated example is called C<SwishSpiderConfig.pl> and is
included in this C<prog-bin> directory along with this script.

The available configuration parameters are discussed below.

This script will not spider files blocked by robots.txt, unless
explicitly overridden -- something you probably should not do.
This script only spiders one file at a time, so load on the web server is not that great.
Still, discuss spidering with a site's administrator before beginning.

The spider program keeps track of URLs visited, so a file is only indexed
one time.  There are two features that can be enabled (disabled by default)
to try to avoid indexing the same file that can be found by different URLs.

First, the C<thin_dots> feature tries to make the following URLs the same by
removing the dots:

    http://localhost/path/to/some/file.html
    http://localhost/path/to/../to/some/file.html

Second, the Digest::MD5 module can be used to create a "fingerprint" of every page
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

Note: the "prog" document source in swish bypasses many configuration settings.
For example, you cannot use the C<IndexOnly> directive with the "prog" document
source.  This is by design to limit the overhead when using an external program
for providing documents to swish; it's assumed that the program can filter
as well as swish can internally.

So, for spidering, if you do not wish to index images, for example, you will
need to either filter by the URL or by the content-type returned from the web
server.  See L<CALLBACK FUNCTIONS|CALLBACK FUNCTIONS> below for more information.

=head1 REQUIREMENTS

Perl 5 (hopefully 5.00503) or later.

You must have the LWP Bundle on your computer.  Load the LWP::Bundle via the CPAN.pm shell,
or download libwww-perl-x.xx from CPAN (or via ActiveState's ppm utility).
Also required is the the HTML-Parser-x.xx bundle of modules also from CPAN
(and from ActiveState for Windows).

You will also need Digest::MD5 if you wish to use the MD5 feature.


=head1 CONFIGURATION FILE

Configuration is not very fancy.  The spider.pl program simple does a
C<do 'path';> to read in the parameters and create the callback subroutines.
The C<path> is the first parameter passed to the script, which is set
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
use C<SwishSpiderConfig.pl>.

The configuration file sets a variable C<@servers> (in package main).
Each element in C<@servers> is a reference to a hash.  The elements of the has
are described next.  More than one server hash may be defined -- each server
will be spidered in order listed in C<@servers>.

Examples:

    my %serverA = (
        base_url    => 'http://sunsite.berkeley.edu/SWISH-E/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu/ ],
        email       => 'my@email.address',
    );
    @servers = ( \%serverA, \%serverB, );

=head1 CONFIGURATION OPTIONS

This describes the required and optional keys in the server configuration hash.

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

=item delay_min

This optional key sets the delay in minutes to wait between requests.  See the
LWP::RobotUA man page for more information.  The default is 0.1 (6 seconds),
but in general you will probably want it much smaller.  But, check with
the webmaster before using too small a number.

To Do: how to completely disable and spider as fast as possible?

=item max_time

This optional key will set the max minutes to spider.   Spidering
for this host will stop after C<max_time> minutes, and move on to the
next server, if any.  The default is to not limit by time.

=item max_files

This optional key sets the max number of files to spider before aborting.
The default is to not limit by number of files.

=item skip

This optional key can be used to skip the current server.  It's only purpose
is to make it easy to disable a server in a configuration file.

=item debug

Set this to true for a bunch of (boring|interesting) output sent to STDERR.

=item ignore_robots_file

If this is set to true then the robots.txt file will not be checked when spidering
this server.  Don't use this option unless you know what you are doing.

=item use_cookies

If this is set then a "cookie jar" will be maintained while spidering.  Some
(poorly written ;) sites require cookies to be enabled on clients.

This requires the HTTP::Cookies module.

=item thin_dots

If this optional setting is true then the spider will attempt to remove dots from
URLs.  For example,

    http://localhost/path/to/../to/../to/my/file.html

will be indexed and spidered as:

    http://localhost/path/to/my/file.html

This should prevent relative URLs in A href tags from looking like different
pages, when they point to the same document.

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
processing URLs.  These allow you to filter URLs and/or filter the content
before it is passed onto swish for indexing.

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
        return if $uri->path =~ /\.(gif|jpeg|jpg|png)$/;
        return 1;
    }

=item test_response

C<test_response> is similar to C<test_url>, but it is called I<after> the URI
has been requested from the web server.  The subroutine is passed a HTTP::Response
object, and a reference the server parameter hash.  Again, returning false will
cause the spider to ignore this resource.

The spider requests a document in "chunks" or 4096 bytes.  4096 is only a suggestion
of how many bytes to return in each chunk.  The C<test_response> routine is
called when the first chunk is received only.  This allows ignoring (aborting)
reading of a very large file, for example, without having to read the entire file.

This subroutine is useful for filtering documents based on data returned in the
headers of the response.  For example, if you have lots of large images on your
web server, and they cannot be easily identified by the URL (as above), then
you could use this callback subroutine to check the Content-Type: header.

    $server{test_response} = sub {
        my ( $response, $server ) = @_;
        return $response->header('content-type') !~ m[^image/];
    }

Swish has a configuration directive C<NoContents> that will instruct swish to
index only the file name, and not the contents.  This is often used when
indexing binary files such as image files, but can also be used with html
files to index only the document titles.

You can turn this feature on for specific documents by setting a flag in
the server hash passed into the C<test_response> subroutine:

    $server{test_response} = sub {
        my ( $response, $server ) = @_;
        $server->{no_contents} =
            $response->header('content-type') =~ m[^image/];
        return 1;  # ok to spider, but pass the index_no_contents to swish
    }

The entire contents of the resource is still read from the web server, and passed
on to swish, but swish will also be passed a C<No-Contents> header which tells
swish to enable the NoContents feature for this document only.


=item filter_content

This callback subroutine can filter the contents before it gets
passed to swish for indexing.  For example, you could add meta tags to HTML
documents or convert PDF or Word documents into text, HTML, or XML.

For example:

    use pdf2xml;  # included example pdf converter module
    $server{filter_content} = sub {
        my ( $response, $server, $content_ref ) = @_;
        $$content_ref = pdf2xml( $content_ref );
    }

=back

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

