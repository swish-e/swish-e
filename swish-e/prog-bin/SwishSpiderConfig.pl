use vars '@servers';

=pod

=head1 NAME

SwishSpiderConfig.pl - Sample swish-e spider configuration

=head1 DESCRIPTION

This is a sample configuation file for the spider.pl program provided
with the swish-e distribution.

Please see C<perldoc spider.pl> for more information.

=cut

#--------------------- Global Config ----------------------------

#  @servers is a list of hashes -- so you can spider more than one site
#  in one run (or different parts of the same tree)
#  The main program expects to use @SwishSpiderConfig::servers.

@servers = (
    {
        base_url    => 'http://sunsite.berkeley.edu/SWISH-E/',
        same_hosts  => [ qw/www.sunsite.berkeley.edu/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@domain.invalid',
        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 20,        # Max files to spider
        debug       => 1,         # Display URLs as indexing to STDERR
        skip        => 0,         # Flag to disable spidering this host.
        ignore_robots_file => 0,  # Don't set that to one, unless you are sure.

        use_cookies => 0,         # True will keep cookie jar
                                  # Some sites require cookies
                                  # Requires HTTP::Cookies

        thin_dots   => 1,         # If true will strip /../ from URLs                                  

        use_md5     => 1,         # If true, this will use the Digest::MD5
                                  # module to create checksums on content
                                  # This will very likely catch files
                                  # with differet URLs that are the same
                                  # content.  Will trap / and /index.html,
                                  # for example.


        # Here are hooks to callback routines to validate urls and responses
        # Probably a good idea to use them so you don't try to index
        # Binary data.  Look at content-type headers!
        
        test_url        => \&test_url,
        test_response   => \&test_response,
        filter_content  => \&filter_content,        
        
    },

    # You can define more than one server to spider.
    # Each is just a hash reference.

    {
        base_url    => 'http://www.infopeople.org/',
        same_hosts  => [ qw/infopeople.org/ ],
        agent       => 'swish-e spider http://sunsite.berkeley.edu/SWISH-E/',
        email       => 'swish@domain.invalid',
        delay_min   => .0001,     # Delay in minutes between requests
        max_time    => 10,        # Max time to spider in minutes
        max_files   => 10,        # Max files to spider
        thin_dots   => 1,         # If true will strip /../ from URLs                                  
        use_md5     => 1,         # If true, this will use the Digest::MD5
    },

);    
    

#---------------------- Public Functions ------------------------------
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
    my ( $URI_object, $server ) = @_;
    return 1;  # Ok to index/spider

    return 0;  # No, don't index or spider;
    
}

# This routine is called when the *first* block of data comes back
# from the server.  If you return false no more content will be read
# from the server.  $response is a HTTP::Response object.


sub test_response {
    my ( $response, $server ) = @_;

    # note that the URI object is in response->request->uri
    

    unless ( $response->header('content-type') =~ m!^text/html! ) {
        $server->{no_contents}++;
        return 1;  # ok to spider, but pass the index_no_contents to swish
    }


    return 1;  # ok to index and spider
}

sub filter_content {
    my ( $response, $server, $content_ref ) = @_;

    # modify $content_ref

}

1;
