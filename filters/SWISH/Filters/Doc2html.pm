package SWISH::Filters::Doc2html;
use vars qw/ @ISA $VERSION $prog /;

$prog = 'wvWare';

$VERSION = '0.01';
@ISA = ('SWISH::Filter');
sub new {
    my ( $pack, %params ) = @_;

    my $self = bless {
        name => $params{name} || $pack,
    }, $pack;


    # check for helpers
    my $path = $self->find_binary( $prog );
    unless ( $path ) {
        $self->mywarn("Can not use Filter $pack -- need to install $prog");
        return;
    }
    $self->{$prog} = $path;

    return $self;

}

sub name { $_->{name} || 'unknown' };

sub priority{ 40 };  # This will be used over the catdoc filter which is 50

sub filter {
    my ( $self, $filter) = @_;

    # Do we care about this document?
    return unless $filter->content_type =~ m!application/(x-)?msword!;

    # We need a file name to pass to the wvware program
    my $file = $filter->fetch_filename;

    # Grab output from running program
    my $content = $filter->run_program( $prog, "-1", $file );
    return unless $content;

    # update the document's content type
    $filter->set_content_type( 'text/html' );

    # return the document
    return \$content;
}
1;

__END__

=head1 NAME

SWISH::Filters::Doc2html - Perl extension for filtering MSWord documents
with Swish-e

=head1 DESCRIPTION

This is a plug-in module that uses the "wvware" program to convert MS Word
documents to HTML for indexing by Swish-e.  "wvware" can be downloaded from:

    http://wvware.sourceforge.net/

The program "wvware" must be installed and in your PATH before running Swish-e.
This has been tested only under Win32- binary package from
http://gnuwin32.sourceforge.net/packages/wv.htm

Tested with Debian Linux and wvWare wvWare 0.7.3.


=head1 SEE ALSO

L<SWISH::Filter>
