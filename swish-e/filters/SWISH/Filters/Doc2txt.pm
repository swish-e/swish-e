package SWISH::Filters::Doc2txt;
use strict;
use vars qw/ $VERSION /;

$VERSION = '0.02';


sub new {
    my ( $class ) = @_;

    my $self = bless {
        mimetypes   => [ qr!application/(x-)?msword! ], # list of types this filter handles
        priority    => 50,  # Make this a higher number (lower priority than the wvware filter
    }, $class;


    # check for helpers
    return $self->set_programs( 'catdoc' );

}


sub filter {
    my ( $self, $doc ) = @_;

    my $content = $self->run_catdoc( $doc->fetch_filename ) || return;

    # update the document's content type
    $doc->set_content_type( 'text/plain' );

    # return the document
    return \$content;
}
1;

__END__

=head1 NAME

SWISH::Filters::Doc2txt - Perl extension for filtering MSWord documents with Swish-e

=head1 DESCRIPTION

This is a plug-in module that uses the "catdoc" program to convert MS Word documents
to text for indexing by Swish-e.  "catdoc" can be downloaded from:

    http://www.ice.ru/~vitus/catdoc/ver-0.9.html

The program "catdoc" must be installed and your PATH before running Swish-e.

=head1 BUGS

This filter does not specify input or output character encodings.  This will change in the
future to all use of the user_data to set the encoding.

A minor optimization during spidering (i.e. when docs are in memory instead of on disk)
would be to use open2() call to let catdoc read from stdin instead of from a file.

=head1 AUTHOR

Bill Moseley

=head1 SEE ALSO

L<SWISH::Filter>


