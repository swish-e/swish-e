package SWISHE::Doc;

use FindBin qw($Bin);

use vars qw/$VERSION/;
my $ver;

BEGIN {

    if ( open FH, "$Bin/../../src/swish.h" ) {
        while (<FH>) {
            if ( /#define SWISH_VERSION "(.+)"/ ) {
                $VERSION = $1;
                last;
            }
        }
    }

$VERSION ||= "Failed to parse Version from '$Bin/../../src/swish.h'";
}

1;
