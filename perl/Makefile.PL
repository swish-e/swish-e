#!/usr/local/bin/perl -w
use strict;
use ExtUtils::MakeMaker;

$Verbose = 1;

print "Parsing ../src/acconfig.h...\n";

my $zlib;
my $libxml2;

    open FH, '../src/acconfig.h' or die $!;

    while (<FH>) {
        $zlib++ if /^#define HAVE_ZLIB 1/;
        $libxml2++ if /^#define HAVE_LIBXML2 1/;
    }


if ( $libxml2 ) {
    print <<'EOF';

==========================================================
Note about libxml2.

It seems as if you built swish-e with libxml2.

If libxml2 is not found automatically, then you may
need to mess with CCFLAGS and LDFROM flags.

For example:

   perl Makefile.PL \
   CCFLAGS=-I$HOME/local/include \
   LDFROM="-L$HOME/local/lib -R$HOME/local/lib"

libxml2 is not needed for the perl module -- it's only
used for indexing.  It will increase the size of the
perl module if included and/or require libxml2 to be
loaded even though it's not used.

This is not a problem and can be ignored, but:

You may wish to rebuild swish-e without libxml2
support, and then create the perl module.  Note that
rebuilding swish will overwrite the swish-e binary, so you
may want to copy or install the swish-e binary before
rebuilding without libxml2 support.

Hopefully this will better in the future.

==========================================================

EOF
    print "press return: ";
    <STDIN>;
}

printf( "Swish was %sbuilt with zlib support\n", $zlib ? '' : 'not ' );
printf( "Swish was %sbuilt with libxml2 support\n", $libxml2 ? '' : 'not ' );



my $libs = '-L../src -lswish-e';
$libs .= ' -lz' if $zlib;
$libs .= ' -lxml2' if $libxml2;

WriteMakefile(
    NAME            => 'SWISHE',
    LIBS            => [$libs],
    NORECURS        => 1,          # keep it from recursing into subdirectories
    VERSION_FROM    => 'SWISHE.pm',  # finds $VERSION
    DIR             => [],
);

