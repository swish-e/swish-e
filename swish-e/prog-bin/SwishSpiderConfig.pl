=pod

=head1 NAME

SwishSpiderConfig.pl - Sample swish-e spider configuration

=head1 DESCRIPTION

This is a sample configuration file for the spider.pl program provided
with the swish-e distribution. 

A spider.pl configuration file is not required as spider.pl has reasonable
defaults.  In fact, it's recommended that you only use a spider.pl
configuration file *after* successfully indexing with spider.pl's default
settings.  To use the default settings set in your *swish-e* config file:

   SwishProgParameters default <url to spider>
   IndexDir spider.pl

The "default" parameter instructs spider.pl to use its
default parameters.  To specify a spider configuration file use:

   SwishProgParameters <path to config file>
   IndexDir spider.pl

If no parameters are passed to spider.pl (i.e. SwishProgParameters is not used)
then spider.pl will look for a file called F<SwishSpiderConfig.pl> in the current
directory.

A spider.pl config file is useful when you need to change the default
behavior of the way spider.pl operates.  For example, you may wish to index
just part of your site, or tell the spider that example.com,
www.example.com and web.example.com are all the same site.

The configuration file is actually Perl code.  This makes it possible to do
reasonably complicated things directly within the config file. For example,
parse HTML content into sections and index each section as a separate "document"
allowing searches to be targeted. 

The spider.pl config file must set an array called "@servers".
The "@servers" array holds one or more descriptions of a server
to index.  In other words, you may define multiple configurations to index
different servers (or different parts of the same server) and group then
together in the @servers array.
Each server description is contained in a single Perl hash.

For example, to index two sites define two Perl hashes:

	my %main_site = (
	    base_url   => 'http://example.com',
	    same_hosts => 'www.example.com',
	    email      => 'admin@example.com',
	);


	my %news_site = (
	    base_url   => 'http://news.example.com',
	    email      => 'admin@example.com',
	);

	@servers = ( \%main_site, \%news_site );
        1;

	
The above defines two Perl hashes (%main_site and %news_site) and then places
a *reference* (the backslash before the name of the hash) to each of those
hashes in the @servers array.  The "1;" at the end is required at the end
of the file (Perl must see a true value at the end of the file).

Below are two example configurations, but included in the same @servers
array (as anonymous Perl hashes).  They both have the skip flag set which
disables their use (this is just an example after all).

The first is a simple example of a few parameters, and shows the use of
a "test_url" function to limit what files are fetched from the server (in
this example only .html files are fetched).

The second example is slightly more complex and makes use the the
SWISH::Filter module to filter documents (such as PDF and MS Word).

Note: The examples below are outside "pod" documentation -- if you are reading
this with the "perldoc" command you will not see the examples below.

=cut


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
        
        base_url    => 'http://www.swish-e.org/index.html',
        same_hosts  => [ qw/swish-e.org/ ],
        agent       => 'swish-e spider http://swish-e.org/',
        email       => 'swish@domain.invalid',

        # limit to only .html files
        test_url    => sub { $_[0]->path =~ /\.html?$/ },

        delay_sec   => 2,         # Delay in seconds between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 100,       # Max Unique URLs to spider
        max_indexed => 20,        # Max number of files to send to swish for indexing
        keep_alive  => 1,         # enable keep alives requests
    },

    #=============================================================================
    # This example just shows more settings, and makes use of the SWISH::Filter
    # module for converting documents.  Some sites require cookies, so this
    # config enables spider.pl's use of cookies, and also enables MD5
    # checksums to catch duplicate pages (i.e. if / and /index.html point
    # to the same page).
    # This example also only indexes the "docs" sub-tree of the swish-e
    # site by checking the path of the URLs

    {
        skip        => 0,         # Flag to disable spidering this host.

        base_url    => 'http://swish-e.org/current/docs/',
        same_hosts  => [ qw/www.swish-e.org/ ],
        agent       => 'swish-e spider http://swish-e.org/',
        email       => 'swish@domain.invalid',
        keep_alive  => 1,         # Try to keep the connection open
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 20,        # Max files to spider
	delay_secs  => 2,         # Delay in seconds between requests
        ignore_robots_file => 0,  # Don't set that to one, unless you are sure.

        use_cookies => 1,         # True will keep cookie jar
                                  # Some sites require cookies
                                  # Requires HTTP::Cookies

        use_md5     => 1,         # If true, this will use the Digest::MD5
                                  # module to create checksums on content
                                  # This will very likely catch files
                                  # with differet URLs that are the same
                                  # content.  Will trap / and /index.html,
                                  # for example.

	# This will generate A LOT of debugging information to STDOUT
        debug       => DEBUG_URL | DEBUG_SKIPPED | DEBUG_HEADERS,


        # Here are hooks to callback routines to validate urls and responses
        # Probably a good idea to use them so you don't try to index
        # Binary data.  Look at content-type headers!
        
 #       test_url        => \&test_url,
 #       test_response   => \&test_response,
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
#  Remember to include the code references in the config as above.
#
#----------------------------------------------------------------------


# This subroutine lets you check a URL before requesting the
# document from the server
# return false to skip the link

sub test_url {
    my ( $uri, $server ) = @_;
    # return 1;  # Ok to index/spider
    # return 0;  # No, don't index or spider;

    # ignore any common image files
    return if $uri->path =~ /\.(gif|jpg|jpeg|png)?$/;

    # make sure that the path is limited to the docs path
    return $uri->path =~ m[^/current/docs/];
    
}



# This routine is called when the *first* block of data comes back
# from the server.  If you return false no more content will be read
# from the server.  $response is a HTTP::Response object.
# It's useful for checking the content type of documents.
#
# For example, say we have a lot of audio files linked our our site that we
# do not want to index.  But we also have a lot of image files that we want
# to index the path name only.

sub test_response {
    my ( $uri, $server, $response ) = @_;

    return if $response->content_type =~ m[^audio/];

    # In this example set the "no_contents" flag for 
    $server->{no_contents}++ unless $response->content_type =~ m[^image/];
    return 1;  # ok to index and spider
}



# This is an example of how to use the SWISH::Filter module included
# with the swish-e distribution.
#
# Returns:
#      true if content-type is text/* or if the document was filtered
#      false if document was not filtered
#      aborts if module cannot be loaded or filter object cannot be created.
#

my $filter;  # cache the object.

sub filter_content {
    my ( $uri, $server, $response, $content_ref ) = @_;

    # Uncomment this to enable debugging of SWISH::Filter
    # $ENV{FILTER_DEBUG} = 1;

    my $content_type = $response->content_type;

    # Ignore text/* content type -- no need to filter
    return 1 if !$content_type || $content_type =~ m!^text/!;
    

    # Load the module - returns FALSE if cannot load module.
    unless ( $filter ) {
        eval { require SWISH::Filter };
        if ( $@ ) {
            $server->{abort} = $@;
            return;
        }
        $filter = SWISH::Filter->new;
        unless ( $filter ) {
            $server->{abort} = "Failed to create filter object";
            return;
        }
    }

    # If not filtered return false and doc will be ignored (not indexed)
    
    my $doc = $filter->convert(
        document => $content_ref,
        name     => $response->base,
        content_type => $content_type,
    );
    return unless $doc;
    # return unless $doc->was_filtered # could do this since checking for text/* above
    return if $doc->is_binary;

    $$content_ref = ${$doc->fetch_doc};

    # let's see if we can set the parser.
    $server->{parser_type} = $doc->swish_parser_type || '';

    return 1;
}

## Must return a true value!!

1;

