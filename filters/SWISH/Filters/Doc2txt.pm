package SWISH::Filters::Doc2txt;
use vars qw/ %FilterInfo $VERSION /;


$VERSION = '0.01';

%FilterInfo = (
    type     => 2,  # normal filter
    priority => 50, # normal priority 1-100
);

sub filter {
    my $filter = shift;

    # Do we care about this document?
    return unless $filter->content_type =~ m!application/msword!;

    # We need a file name to pass to the catdoc program
    my $file = $filter->fetch_filename;
    

    # Grab output from running program
    my $content = $filter->run_program( 'catdoc', $file );

    return unless $content;

    # update the document's content type
    $filter->set_content_type( 'text/plain' );

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

=head1 AUTHOR

Bill Moseley

=head1 SEE ALSO

L<SWISH::Filter>


