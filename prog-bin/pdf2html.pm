package pdf2html;
use strict;

=pod

=head1 NAME

pdf2html - swish-e sample module to convert pdf to html

=head1 SYNOPSIS

    use pdf2html;
    my $html_record_ref = pdf2html( $pdf_file_name, 'title' );

    # or by passing content in a scalar reference
    my $html_text_ref = pdf2html( \$pdf_content, 'title' );


    

=head1 DESCRIPTION

Sample module for use with other swish-e 'prog' document source programs.

Pass either a file name, or a scalar reference.

The differece is when you pass a reference to a scalar
only the content is returned.  When you pass a file name
an entire record is returned ready to be fed to swish -- this
includes the headers required by swish for indexing.

The second optional parameter is the extracted PDF info tag to use as the HTML title.



The plan is to find a library that will do this to avoid forking an external
program.

=head1 REQUIREMENTS

Uses the xpdf package that includes the pdftotext conversion program.
This is available from http://www.foolabs.com/xpdf/xpdf.html.

You will also need the module File::Temp (and its dependencies)
available from CPAN if passing content to this module (instead of a file name).


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
@EXPORT = qw(pdf2html);

my @InfoTags = qw/Title Subject Author CreationDate Creator Producer ModDate Keywords/;


sub pdf2html {
    my $file_or_content = shift;
    my $title_tag = shift;


    my $file = ref $file_or_content
    ? create_temp_file( $file_or_content )
    : $file_or_content;

    my $headers = get_pdf_headers( $file, $title_tag ) || '';

    my $content_ref = get_pdf_content_ref( $file );

    my $txt = <<EOF;
<html>    
<head>
$headers
</head>
<body>
<pre>
$$content_ref
</pre>
</body>
</html>
EOF

    if ( ref $file_or_content ) {
        unlink $file;
        return \$txt if ref;
    }

    my $mtime  = (stat $file )[9];

    my $size = length $txt;

    my $ret = <<EOF;
Content-Length: $size
Last-Mtime: $mtime
Path-Name: $file

EOF

$ret .= $txt;

    return \$ret;
    

}

sub get_pdf_headers {

    my $file = shift;
    my $title_tag = shift;

    
    my $sym = gensym;

    open $sym, "pdfinfo $file |" || die "$0: Failed to open $file $!";

    my %metadata;

    while (<$sym>) {
        if ( /^\s*([^:]+):\s+(.+)$/ ) {
            my ( $metaname, $value ) = ( lc( $1 ), escapeXML( $2 ) );
            $metaname =~ tr/ /_/;
            $metadata{$metaname} = $value;
        }
    }
    close $sym or warn "$0: Failed close on pipe to pdfinfo for $file: $?";

    my $metas = join "\n", map { qq[<meta name="$_" content="$metadata{$_}">] } sort keys %metadata;

    if ( $title_tag && exists $metadata{ $title_tag } ) {
        $metas = "<title>$metadata{ $title_tag }</title>\n$metas";
    }

    return $metas;

}

sub get_pdf_content_ref {
    my $file = shift;

    my $sym = gensym;
    open $sym, "pdftotext $file - |" or die "$0: failed to run pdftotext: $!";

    local $/ = undef;
    my $content = escapeXML(<$sym>);

    close $sym or warn "$0: Failed close on pipe to pdftotext for $file: $?";

    return \$content;
}



# How are URLs printed with pdftotext?
sub escapeXML {

   my $str = shift;

   for ( $str ) {
       s/</&lt;/go;
       s/>/&gt;/go;
       tr/\014/ /; # ^L
       # s/&/&amp;/go;
       # s/"/&quot;/go;
    }
   return $str;
}

# This is the portable way to do this, I suppose.
# Otherwise, just create a file in the local directory.

sub create_temp_file {
    my $scalar_ref = shift;

    require "File/Temp.pm";

    my ( $fh, $file_name ) = File::Temp::tempfile();

    print $fh $$scalar_ref or die $!;


    close $fh or die "Failed to close '$file_name' $!";

    return $file_name;
}
    

