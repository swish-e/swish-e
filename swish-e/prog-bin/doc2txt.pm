package doc2txt;
use strict;

=pod

=head1 NAME

pdf2xml - swish-e sample module to convert MS Word docs to text

=head1 SYNOPSIS

    use doc2txt;
    my $doc_record_ref = doc2txt( $doc_file_name );

    # or by passing content in a scalar reference
    my $doc_text_ref = doc2txt( \$doc_content );


    

=head1 DESCRIPTION

Sample module for use with other swish-e 'prog' document source programs.

Pass either a file name, or a scalar reference.

The differece is when you pass a reference to a scalar
only the content is returned.  When you pass a file name
an entire record is returned ready to be fed to swish -- this
includes the headers required by swish for indexing.


=head1 REQUIREMENTS

Uses the catdoc program.  http://www.fe.msk.ru/~vitus/catdoc/

You may need to adjust the parameters used to call catdoc.

You will also need the module File::Temp available from CPAN if passing content
to this module (instead of a file name).  I'm not thrilled about how that
currently works...


=head1 AUTHOR

Bill Moseley

=cut

use Symbol;


use vars qw(
    @ISA
    @EXPORT
    $VERSION
);

# $Id$
$VERSION = sprintf '%d.%02d', q$Revision$ =~ /: (\d+)\.(\d+)/;

require Exporter;
@ISA    = qw(Exporter);
@EXPORT = qw(doc2txt);

my @InfoTags = qw/Title Subject Author CreationDate Creator Producer ModDate Keywords/;

my $catdoc = 'catdoc -a';  # how cat doc is called. Rainer uses catdoc -s8859-1 -d8859-1


sub doc2txt {
    my $file_or_content = shift;


    my $file = ref $file_or_content
    ? create_temp_file( $file_or_content )
    : $file_or_content;


    my $content = `$catdoc $file`;

    return \$content if ref $file_or_content;

    # otherwise build the headers

    my $mtime  = (stat $file )[9];

    my $size = length $content;

    my $ret = <<EOF;
Content-Length: $size
Last-Mtime: $mtime
Path-Name: $file

EOF

    $ret .= $content;

    return \$ret;
    

}


# This is the portable way to do this, I suppose.
# Otherwise, just create a file in the local directory.

sub create_temp_file {
    my $scalar_ref = shift;

    require "File/Temp.pm";

    my ( $fh, $file_name ) = File::Temp::tempfile( UNLINK => 1 );

    print $fh $$scalar_ref or die $!;


    close $fh or die "Failed to close '$file_name' $!";

    return $file_name;
}
    

