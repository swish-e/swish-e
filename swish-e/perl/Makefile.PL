use ExtUtils::MakeMaker;
$Verbose = 1;

print <<EOF;

==========================================================
If you are not building swish-e with libxml2 support
(and thus do not have libxml2 installed on your machine)
you will see a warning.  This is harmless.

If you built swish-e with libxml2 then this module will
want to find libxml2, even though it's not used in the
perl module.  If it's not found automatically, then you may
need to mess with CCFLAGS and LDFROM flags.

For example:

   perl Makefile.PL \
   CCFLAGS=-I$HOME/t \
   LDFROM="-L$HOME/libxml -R$HOME/libxml"

   
Perhaps a better solution is to build swish-e without
libxml2 support (so libxml2 is not needed), build the perl
module, the build perl with libxml2 support.  That would
probably reduce the size of the perl module.
==========================================================

Press Return:


EOF
<STDIN>;

WriteMakefile(
    NAME            => 'SWISHE',
    LIBS            => ['-L../src -lswish-e -lm -lxml2', '-lswish-e -lm -lxml2'],
    NORECURS        => 1,          # keep it from recursing into subdirectories
    VERSION_FROM    => 'SWISHE.pm',  # finds $VERSION
    DIR             => [],
);

