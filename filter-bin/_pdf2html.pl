#! /usr/bin/perl -w
use strict;

# -- Filter PDF to simple HTML for swish
# --
# -- 2000-05  rasc
#
=pod

This filter requires two programs "pdfinfo" and "pdftotext".
These programs are part of the xpdf package found at
http://www.foolabs.com/xpdf/xpdf.html.

These programs must be found in the PATH when indexing is run, or 
explicitly set the path in this program:

  $ENV{PATH} = '/path/to/programs'

"pdfinfo" extracts the document info from a pdf file, if any exist,
and creates metanames for swish to index.  See man pdfinfo(1) for
information what keywords are available.

An HTML title is created from the "title" and "subject" pdf info data.
Adjust as needed below.

How the extracted keyword info is indexed in Swish-e is controlled by
the following Swish-e configuration settings: MetaNames, PropertyNames,
UndefinedMetaTags.

Passing the -raw option to pdftotext may improve indexing time by
reducing the size of the converted output.

=cut


my $file = shift || die "Usage: $0 <filename>\n";

#
# -- read pdf meta information
#

my %metadata;

open F, "pdfinfo $file |" || die "$0: Failed to open $file $!";

while (<F>) {
    if ( /^\s*([^:]+):\s+(.+)$/ ) {
        my ( $metaname, $value ) = ( lc( $1 ), escapeHTML( $2 ) );
        $metaname =~ tr/ /_/;
        $metadata{$metaname} = $value;
    }
}
close F or die "$0: Failed close on pipe to pdfinfo for $file: $?";


# Set the default title from the title and subject info

my @title = grep { $_ } @metadata{ qw/title subject/ };
delete $metadata{$_} for qw/title subject/;


my $title = join ' // ', ( @title ? @title : 'Unknown title' );

my $metadata = 
    join "\n",
        map { qq[<meta name="$_" content="$metadata{$_}">] }
                   sort keys %metadata;


print <<EOF;
<html>
<head>
    <title>
        $title
    </title>
    $metadata
</head>
<body>
EOF

# Might be faster to use sysread and read in larger blocks

open F, "pdftotext $file - |" or die "$0: failed to run pdftotext: $!";
print escapeHTML($_) while ( <F> );
close F or die "$0: Failed close on pipe to pdftotext for $file: $?";

print "</body></html>\n";


# How are URLs printed with pdftotext?
sub escapeHTML {

   my $str = shift;

   for ( $str ) {
       s/&/&amp;/go;
       s/</&lt;/go;
       s/>/&gt;/go;
       s/"/&quot;/go;
       tr/\014/ /; # ^L
    }
   return $str;
}

