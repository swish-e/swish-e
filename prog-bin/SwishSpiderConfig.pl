=pod

=head1 NAME

SwishSpiderConfig.pl - Sample swish-e spider configuration

=head1 DESCRIPTION

This is a sample configuation file for the spider.pl program provided
with the swish-e distribution.

It contains settings for spidering three servers (two are the same server).
All are disabled (skip => 1) to prevent every new swish user from spidering these sites.

These are just examples.  Please spider your own web site.

**Also, please don't use this exact file as your configuration file.**

Trim your file down to just the content you need, especially
if posting your config to the Swish-e list requesting for help.  Remove these comments
and remove everything below that you are not using.

The first example is relativly simple.  It just spiders any URL that
ends in C<.html>.

The second example is a bit more advanced and shows how to filter content.

First, this the spider doesn't request image files (files that end in .gif or .jpeg)
then only indexes files with of C<text/html text/plain application/pdf application/msword> content
type.

C<application/pdf> and C<application/msword> are then run through filters to extract
out their content.  The example filter subroutines are included below, as well.

This config is set to only spider 100 URLs, or index 20 files, which ever comes first.

The third example shows more options (which are listed in C<perldoc spider.pl>), and how you might use
subroutine calls for checking URLs, content, and filtering instead of inlined subroutines shown in
the first two examples.


Please see C<perldoc spider.pl> for more information.

=cut

#--------------------- Global Config ----------------------------

#  @servers is a list of hashes -- so you can spider more than one site
#  in one run (or different parts of the same tree)
#  The main program expects to use this array (@SwishSpiderConfig::servers).

  ### Please do not spider these examples -- spider your own servers, with permission ####

@servers = (

    #=============================================================================
    # This is a simple example, that includes a few limits
    # Only files ending in .html will be spidered (probably a bit too restrictive)
    {
        skip        => 1,  # skip spidering this server
        
        base_url    => 'http://www.infopeople.org/index.html',
        same_hosts  => [ qw/infopeople.org/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@domain.invalid',

        # limit to only .html files
        test_url    => sub { $_[0]->path =~ /\.html?$/ },

        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 100,       # Max Unique URLs to spider
        max_indexed => 20,        # Max number of files to send to swish for indexing
        keep_alive  => 1,         # enable keep alives requests
    },


    #=============================================================================
    # This is a more advanced example that uses more features,
    # such as ignoring some file extensions, and only indexing
    # some content-types, plus filters PDF and MS Word docs.
    # The call-back subroutines are explained a bit more below.
    {
        skip        => 1,  # skip spidering this server
        debug       => DEBUG_URL,  # print some debugging info to STDERR                                  

        base_url        => 'http://www.infopeople.org/',
        email           => 'swish@domain.invalid',
        delay_min       => .0001,
        link_tags       => [qw/ a frame /],
        max_files       => 50,         
        max_indexed     => 20,        # Max number of files to send to swish for indexing

        max_size        => 1_000_000,  # limit to 1MB file size
        max_depth       => 10,         # spider only ten levels deep
        keep_alives     => 1,

        test_url        => sub { $_[0]->path !~ /\.(?:gif|jpeg)$/ },

        test_response   => sub {
            my $content_type = $_[2]->content_type;
            my $ok = grep { $_ eq $content_type } qw{ text/html text/plain application/pdf application/msword };

            # This might be used if you only wanted to index PDF files, yet spider still spider.
            #$_[1]->{no_index} = $content_type ne 'application/pdf';

            return 1 if $ok;
            print STDERR "$_[0] wrong content type ( $content_type )\n";
            return;
        },

        filter_content  => [ \&pdf, \&doc ],
    },


    #=============================================================================
    # This example just shows more settings.  See perldoc spider.pl for info
    
    {
        skip        => 1,         # Flag to disable spidering this host.

        base_url    => 'http://sunsite.berkeley.edu/SWISH-E/index.html',
        same_hosts  => [ qw/www.sunsite.berkeley.edu/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@domain.invalid',
        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 20,        # Max files to spider
        ignore_robots_file => 0,  # Don't set that to one, unless you are sure.

        use_cookies => 0,         # True will keep cookie jar
                                  # Some sites require cookies
                                  # Requires HTTP::Cookies

        use_md5     => 1,         # If true, this will use the Digest::MD5
                                  # module to create checksums on content
                                  # This will very likely catch files
                                  # with differet URLs that are the same
                                  # content.  Will trap / and /index.html,
                                  # for example.

        debug       => DEBUG_URL | DEBUG_HEADERS,  # print some debugging info to STDERR                                  


        # Here are hooks to callback routines to validate urls and responses
        # Probably a good idea to use them so you don't try to index
        # Binary data.  Look at content-type headers!
        
        test_url        => \&test_url,
        test_response   => \&test_response,
        filter_content  => \&filter_content,        
        
    },



);    
    

#---------------------- Public Functions ------------------------------
#  Here are some examples of callback functions
#
#
#  Use these to adjust skip/ignore based on filename/content-type
#  Or to filter content (pdf -> text, for example)
#
#  Remember to include the code references in the config, above.
#
#----------------------------------------------------------------------


# This subroutine lets you check a URL before requesting the
# document from the server
# return false to skip the link

sub test_url {
    my ( $uri, $server ) = @_;
    # return 1;  # Ok to index/spider
    # return 0;  # No, don't index or spider;

    # ignore any .gif files
    return $uri->path =~ /\.html?$/;
    
}

# This routine is called when the *first* block of data comes back
# from the server.  If you return false no more content will be read
# from the server.  $response is a HTTP::Response object.


sub test_response {
    my ( $uri, $server, $response ) = @_;

    $server->{no_contents}++ unless $response->content_type =~ m[^text/html];
    return 1;  # ok to index and spider
}

# This routine can be used to filter content

sub filter_content {
   my ( $uri, $server, $response, $content_ref ) = @_;

    # modify $content_ref
    $$content_ref = modify_content( $content_ref );
    return 1;  # make sure you return true!

}

# Maybe do something here ;)
sub modify_content {
    my $content_ref = shift;

    
    return $$content_ref;
}

    

# Here's some real examples

# This converts PDF files into HTML.  The second parameter of
# pdf2html tells which pfd info filed to set as <title>

use pdf2html;  # included example pdf converter module
sub pdf {
   my ( $uri, $server, $response, $content_ref ) = @_;

   return 1 unless $response->content_type eq 'application/pdf';

   # for logging counts
   $server->{counts}{'PDF transformed'}++;

   $$content_ref = ${pdf2html( $content_ref, 'title' )};
   $$content_ref =~ tr/ / /s;
   return 1;
}

use doc2txt;  # included example pdf converter module
sub doc {
   my ( $uri, $server, $response, $content_ref ) = @_;

   return 1 unless $response->content_type eq 'application/msword';

   # for logging counts
   $server->{counts}{'DOC transformed'}++;

   $$content_ref = ${doc2txt( $content_ref )};
   $$content_ref =~ tr/ / /s;
   return 1;
}

# Must return true...

1;
