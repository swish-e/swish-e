#!/usr/bin/perl -w
#
#    build swish-e with zlib and libxml2 support.
#    tries to be intelligent about OS and compiler
#    options.
#
################################################################################
# History
#
#   0.01
#    initial public release . Wed Sep 15 12:07:55 CDT 2004 . karman@cray.com
#
#   0.02
#    --srcdir works; IRIX fixes; --quiet fixes
#
#   0.03
#    chdir to $Bin to check for cvs build
#    include $installdir in -I when checking for preinstalled libxml2
#    linux fixes
#    added --disable-shared to libxml2 build
#    --force actually works
#    added --static to make --disable-shared optional
#
#   0.04
#    fixed swishdir/installdir bug
#    thanks to chyla@knihovnabbb.cz
#
#   0.05     Tue Nov 16 07:08:56 CST 2004
#    rewrote get_src() with a little more reality in mind
#    fixed swish option to refer to swish-e, swish, or swishe
#        but look for file called 'swish-e' or 'latest'
#    added --progress option to print a few messages under --quiet
#    added --make option for when you can't symlink make -> gmake
#    libxml2-2.6.16  --- still need to get 'latest' support
#
#   0.06    Wed Nov 17 08:18:05 CST 2004
#    arrgg. need to read my email more thoroughly; libxml 2.6.16 fails
#        because of unannounced api change. reverting to .13
#    --errlog option; errors now print to stderr instead of to log by default
#    SWISHBIN replaced by SWISHBINDIR for making SWISH::API
#    --perllib to indicate specific install dir, 
#        otherwise defaults to perl Config
#    usage fixes
#
#   0.07    Wed Dec  8 15:25:57 CST 2004
#    added xpdf to build (mostly for CrayDoc)
#    fixed get src logic (again)
#    standardized swish,swish-e,swishe debacle
#    remember CWD from where we start so relative paths are resolved from opts
#    zlib update
#    changed libxml2 back to .16 to support 2.4.3 fix
#    added swish_api_dir to determine where exactly the swish::api mod is installed
#    use File::Basename
#    assuming >=2.4.3 is now latest.
#    all 3rdparty stuff now has 'latest' URL (thanks to developers!)
#    changed default ld_opts to same as Linux, et al.
#    fixed SWISH::API LIBS and LDFLAGS opt to work on Solaris
#    unset LD_LIBRARY_PATH env var to accurately test linking
#
#   0.08    Tue Mar 29 14:18:56 CST 2005
#    perldoc fixes
#    -R optional in ::API build
#    xpdf build fixes -- only include pdftotext and pdfinfo
#    docs.cray.com URL fix for xpdf
#    -s option for zlib configure (default is dynamic/shared libs)
#    --disable-docs for new swish-e build
#    added --increm feature
#    URL fix for new swish-e.org site
#    added /usr/local to default LD_RUN_PATH and -I/path for all include paths
#
#   0.09    Tue Apr 26 09:09:10 CDT 2005
#    added libxml2 to swish-e -I path
#    fixed xpdf src url
#    rewrote perllib logic
#    -W test for all install dirs
#    thanks to chyla@knihovnabbb.cz for feedback on these fixes/features
#    
#
#   0.10    Thu Feb 23 10:44:26 CST 2006
#    changed Xpdf URL to peknet patched copy of 3.01pl2
#
#   0.11    Fri Jul  7 11:15:41 CDT 2006
#    changed CVS url per sf.net updates
#    updated src URLs
################################################################################

$| = 1;

use strict;
require 5.006;    # might work with < perl 5.6.0 but untested
use sigtrap qw(die normal-signals error-signals);
use Getopt::Long;
use Config qw( %Config );
use Cwd;
use File::Basename;
use File::Path qw( mkpath );
use File::Spec;
use FindBin qw( $Bin );

my $Version     = '0.11';

my %URLs    = (

'swish-e'     => 'http://swish-e.org/distribution/latest.tar.gz',
'zlib'        => 'http://www.zlib.net/zlib-1.2.3.tar.gz',
# madler@alumni.caltech.edu assures me that zlib will support a
# http://www.zlib.net/zlib-current.tar.gz link but no such link as of
# 7/12/2006

'libxml2'    => 'ftp://xmlsoft.org/libxml2/LATEST_LIBXML2',
# peknet hosts a pre-patched version of XPDF
'xpdf'       => 'http://peknet.com/src/xpdf-3.01pl2.tar.gz',

);


my $swish_cvs     = ':pserver:anonymous@swishe.cvs.sourceforge.net:/cvsroot/swishe';
my $cvs_cmd     = "cvs -q -d$swish_cvs co swish-e";

my $defdir     = '/usr/local/swish-e/latest';
my $deftmp     = $ENV{TMPDIR} || $ENV{TMP} || $ENV{CRAYDOC_TMP} || '/tmp';

my $opts     = {};

my $usage_gutter = 25;
 
my $allopts =
{
        
    'quiet'        => "run non-interactively",
    'cvs'        => "use latest CVS version of Swish-e",
    'swish-e=s'    => "use <X> as Swish-e source -- either URL, tar.gz or directory",
    'libxml2=s'    => "use <X> as libxml2 source -- either URL, tar.gz or directory",
    'zlib=s'    => "use <X> as zlib source -- either URL, tar.gz or directory",
    'prevzlib=s'    => "use already-installed zlib in directory <X>",
    'prevxml2=s'    => "use already-installed libxml2 in directory <X>",
    'installdir=s'    => "install in <X> [$defdir]",
    'verbose'    => "don't send compiler output to log, send it to stdout",
    'srcdir=s'    => "look in <X> for all src tar.gz files [./]",
    'tmpdir=s'    => "use <X> to unpack tar files, compile and test [$deftmp]",
    'help'        => "print usage statement",
    'opts'        => "print options and description",
    'force'        => "install zlib and libxml2 no matter what",
    'debug'        => "lots of on-screen verbage",
    'sudo'      => "run 'make' commands with sudo\n"
                .' 'x$usage_gutter.
                "!! must be interactive if sudo expects a password !!",
    'static'    => "build with --disable-shared flag",
    'progress'    => "with --quiet, will print a few important progress messages to stdout",
    'make=s'    => "use <X> as make command (useful if your GNU make is named gmake)",
    'errlog=s'    => 'write build errors to <X> file instead of stderr',
    'perllib=s'    => "install SWISH::API in <X>\n".
                ' 'x$usage_gutter.
                "[ $Config{sitearch} ]",
    'xpdf=s'    => 'use <X> as xpdf source -- either URL, tar.gz or directory',
    'noxpdf'    => "don't install xpdf",
    'increm'    => "build swish-e with experimental incremental feature",
    
};

my $helpful =<<EOF;
Please send email to the author with this message.
Include all screen output and a description of your problem.
Thanks.

EOF

usage() unless GetOptions($opts,keys %$allopts);
usage() if $opts->{help};

if ($opts->{opts})
{
# print all options and descriptions
    printopts();
    exit;
}

################################################################################

use vars qw( 
        $os $arch $cc $installdir $nogcc $min_gcc $tmpdir
        $outlog $errlog $output $ld_opts $Cout $Cin
        $zlib_test $libxml2_test $ld_test $gcc_test
        $swishdir $fetcher $libxml2dir $zlibdir $MinLibxml2 $cmdout
        $Make $startdir $swish_api_dir $perllib
        
        );

$zlibdir    = $opts->{prevzlib} || '';
$libxml2dir    = $opts->{prevxml2} || '';


# some defaults
$installdir = $opts->{installdir} || $defdir;
$installdir =~ s,/+$,,;
$perllib = $opts->{perllib} || "$Config{sitearch}/../"; # must go up one level
$startdir     = Cwd::cwd();
$MinLibxml2    = '2.4.3';
# we ought to be able to retrieve this from swish src somehow
# but we don't have source till after we've tested for this -- chick and egg

$nogcc      = "I refuse to compile without gcc\nCheck out http://gcc.gnu.org\n";
$min_gcc    = '2.95';
$tmpdir     = $opts->{tmpdir} || $deftmp;
$outlog     = $tmpdir . '/buildswishe.log';
$errlog     = $opts->{errlog} || '';
$output     = $opts->{verbose} ? '' : " 1>>$outlog";
$output     .= " 2>>$errlog " if $errlog;

$cmdout     = $opts->{quiet} ? ' 1>/dev/null 2>/dev/null ' : '';

$ld_opts    = '';    # define these based on OS platform

$Make       = $opts->{make} || 'make';

# better C tests would be nice. these seem to work.
$Cout       = "$tmpdir/test";
$Cin        = "$tmpdir/$$.c";
    
$zlib_test =<<EOF;
#include <stdio.h>
#include "zlib.h"

int main ( void )
{
    printf("%s\\n", ZLIB_VERSION);
    return 0;

}

EOF

$ld_test = <<EOF;
#include <stdio.h>
int main( void )
{
   printf("success!\\n");
   return 0;
}


EOF

# because xml-config doesn't always exist...
$libxml2_test =<<EOF;
#include <stdio.h>
#include "libxml/xmlversion.h"

int main ( void )
{
    printf("%s\\n", LIBXML_DOTTED_VERSION);
    return 0;

}

EOF


# gcc might be symlinked from something else, or named cc or some
# other weirdness, so test it to be sure
$gcc_test =<<EOF;
#include <stdio.h>
int main ( void )
{
    #ifndef __GNUC__
           printf("not gcc!\\n");
           return 1;
           
    #else
        printf("yep, gcc\\n");
        return 0;
    #endif
  
}

EOF


##########################################################
## run
checkenv();
makedirs();
sanity();

my %routines = (

    libxml2         => \&libxml2,
    zlib            => \&zlib,
    'swish-e'       => \&swishe,
    'swish::api'    => \&swish_api,
    xpdf            => \&xpdf,
    
    );
    
my @toinstall = qw( zlib libxml2 swish-e swish::api );    # order matters
    
push(@toinstall,'xpdf') unless $opts->{noxpdf};
    
for ( @toinstall )
{

    my $prevout;
    if ( $opts->{progress} ) {
    
        $prevout = select STDOUT;

    }
    print '=' x 60 . "\n";
    print "building $_ ...\n";
    print '~' x 60 . "\n";
    
    if ( $opts->{progress} ) {
    
        select $prevout;
        
    }
    
    my $f = $routines{$_};
    &$f;
    
}

test();
cleanup();
exit;

END
{ 
  unless ($opts->{debug}) {
   unlink( "$tmpdir/$$.c", "$tmpdir/test") if $tmpdir;
  }
}

##########################################################

sub printopts
{    
    print "\n$0 options\n", '-' x 30 , "\n";
    print "defaults print in [ ] \n";
    for (sort keys %$allopts) {
        
        (my $o = $_) =~ s,[:=]s,=<X>,g;
        
        my $space = ' ' x ( $usage_gutter - length($o) );
        
        print "  --$o $space $allopts->{$_}\n" ;
    }
    print "\n\n";
    exit;
}


sub usage
{

    system("perldoc $0");
    printopts();
    exit;

}

sub checkenv
{

    if ( $opts->{quiet} ) {
    
        open(QUIET, ">/dev/null") or die "can't write to /dev/null: $!\n";
        select(QUIET);    # should send all default print()s to oblivion
        
    }
    
    $os     = $Config{osname} || $ENV{OSTYPE} || `uname`;
    $arch     = $Config{archname};
    $cc     = $Config{cc};    # for SWISH::API should use same compiler as perl
                # was compiled with, and that SHOULD be gcc.

    if ($opts->{progress}) {
    print STDOUT "os... $os\n";
    print STDOUT "arch... $arch\n";
    print STDOUT "cc... $cc\n";
    print STDOUT "opt: $_ -> $opts->{$_}\n" for sort keys %$opts;
    }
    
    unlink $outlog, $errlog;
    # so we start afresh
    # but don't warn if we fail to unlink (e.g., perms or something)
    
    ld_opts();
    set_fetcher();
}

sub makedirs
{

    my @d = (
        $installdir,
        $tmpdir,
        "$installdir/lib",
        "$installdir/include",
        $perllib
        );
    mkpath( \@d, 1, 0755 );
    for (@d) {
    
        if ( $opts->{progress} ) {
        print STDOUT "checking dir $_ ... ";
        }
        unless ( -W $_ )
        {
            warn "can't write to $_: $!\n";
            nice_exit();
        }
        if ( $opts->{progress} ) {
        print STDOUT "\n";
        }
    }
    
}

sub sanity
{

    test_make();
    
    print "is $cc a GNU C compiler... ";
    test_compiler();
    
# basic ld test with libc
    if ( test_c_prog( $ld_test, '-lc' ) ) {
    
        die "can't link against libc with $cc\n";
    
    }
    
    unless ( $opts->{quiet} ) {
    
        print "Continue on our merry way? [y] ";
        nice_exit() unless confirm();
        
    }
    
}


sub test_c_prog
{
    my $prog = shift;
    
# if last value in @_ is a SCALAR ref, use it as a test against output of prog

    my $test = pop @_ if ref $_[-1] eq 'SCALAR';
    
 # write
    open(TMP, "> $tmpdir/$$.c") || die "can't write to $tmpdir\n";
    print TMP $prog;
    close(TMP);
    
 # compile
    my $extra     = join ' ', @_;
    $extra         .= ' -w' if $os =~ /irix/i;
    my $cmd     = "$cc $extra -o $Cout $Cin";
    
    print "\n\ncompile cmd: $cmd\n" if $opts->{debug};
    
    return 1 if system("$cmd $cmdout");    # don't bother if we can't even compile    
    
    if ($test) {
    
        chomp( my $o = `$Cout` );
        
        if ($o lt $$test) {
            
            print $o . " -- failed!\n";
            
            return 1;    # 1 indicated failure like system() does
            
        }
        
    }
    
 # run it
 
     warn "run: $Cout $cmdout\n" if $opts->{debug};
 
    return system("$Cout $cmdout");

}

# fix from chyla@knihovnabbb.cz
# Tue Apr 26 09:15:16 CDT 2005
sub test_for_prior_zlib
{
     return undef if $opts->{force};

     print "testing for already-installed zlib... ";

     my $err = test_c_prog( $zlib_test, '-lz', $ld_opts );

     return get_ld_path('libz', '', \$zlibdir) unless $err;

     return 0;    # 0 indicates failure
}


sub test_for_prior_libxml2
{

    return undef if $opts->{force};

    print "testing for already-installed libxml2... ";
    
# first include -lxml2 and see if it returns a path

    my $path = get_ld_path('libxml2', '', \$libxml2dir)
      unless test_c_prog(
                $ld_test,
                $ld_opts,
                '-lxml2' );

# then check version with xmlversion.h

    return 0 unless $path;
    
    print "minimum libxml2 version required is $MinLibxml2 and you have... ";
    
    # include $installdir in -I path, since we may be a repeat user
    my $err = test_c_prog(
                $libxml2_test,
                '-I'. File::Spec->catfile( $installdir, 'include', 'libxml2' ),
                '-I'. File::Spec->catfile( $path, 'include', 'libxml2' ),
                $ld_opts,
                '-lxml2',
                \$MinLibxml2
                );
      
      
    return get_ld_path('libxml2', '', \$libxml2dir) unless $err;
    
    return 0;    # 0 indicates failure
    
}


sub cleanup
{

    print STDOUT "Swish-e was installed in $installdir\n";
    print STDOUT "SWISH::API was installed in $swish_api_dir\n";

}

sub test
{

    test_api();
    
}


sub set_fetcher
{
# we'd prefer to use some external command like wget
# but will resort to the LWP stuff if need be

    $fetcher = which('wget');
    $fetcher ||= which('curl');
    $fetcher ||= perl_fetcher();
    print "no program available to fetch remote sw source\n" unless $fetcher;

}


sub perl_fetcher
{
    print "trying LWP::Simple\n";
    eval { require LWP::Simple };
    $@ ? return 0 : return \&LWP::Simple::getstore;

}

sub which
{

    chomp(my $w = `which $_[0] 2>&1`);
    $w =~ s,\n,,g;
    $w !~ m/not found|^[^\/]/i
    ? $w
    : 0;
    
}


sub test_compiler
{
# verify that we are using a current version of gcc
# other compilers MIGHT work, but we KNOW gcc does
    if ( test_c_prog( $gcc_test ) ) {
        warn     "Warning: this perl was not compiled with gcc!\n".
            "Instead: $cc\n".
            "This may cause compile of SWISH::API to fail.\n".
            "We'll try anyway, but be warned.\n";
            
# if $cc is not gcc, test for it in our path anyway.
        my $gcc = which('gcc');
        if ( $gcc ) {
           print     "Found a gcc at $gcc\n";
           if ($opts->{interactive}) {
            print    "Should I use that? [yes] ";
            if ( confirm() ) {
                $cc = $gcc;
            } else {
                print $nogcc;
                nice_exit();
            }
           } else {
               $cc = $gcc;
           }
           
        } else {
            #die $nogcc;
            return 0;
        }
    }

# by now, we should be using gcc
# check version
    print "testing gcc version... ";
    my @v = `$cc -v 2>&1`;
    my @vers = grep { /version \d/ } @v;
    if (! @vers) {
        warn     "Warning: trouble getting version from $cc\n".
            "Minimum version required is $min_gcc\n".
            "We'll try anyway.\n";
    } else {
        V: for (@vers) {
            next if ! m/version \d/;
            my ($v) = ($_ =~ m/version ([\d\.]+)/);
            $v =~ s,^(\d\.\d+)\.?(\d*?),$1$2,g;
            if ($v < $min_gcc) {
                die "Minimum version of gcc is $min_gcc\n";
            } else {
                print "version $v\n";
            }
        }
    }

}



sub ld_opts
{
# the libtool message, here for reference
#------------------------------------------
#If you ever happen to want to link against installed libraries
#in a given directory, LIBDIR, you must either use libtool, and
#specify the full pathname of the library, or use the `-LLIBDIR'
#flag during linking and do at least one of the following:
#   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
#     during execution
#   - add LIBDIR to the `LD_RUN_PATH' environment variable
#     during linking
#   - use the `-Wl,--rpath -Wl,LIBDIR' linker flag
# 
#See any operating system documentation about shared libraries for
#more information, such as the ld(1) and ld.so(8) manual pages.
#------------------------------------------

# we opt for the LD_RUN_PATH/-LLIBDIR combination because:
# 1. we can't rely on LD_LIBRARY_PATH for future execution
# 2. --rpath doesn't seem to work for all gcc's equally well

# set LD_RUN_PATH, etc to include our new lib/ path
# this whole library path deal is a nefarious and odious beast, largely
# because default libraries go in one path, while later-installed libraries
# might go anywhere at all. and while some OSes use LD_RUN_PATH, some do not.
# we try and cover all the bases here.

# both set env var LD_RUN_PATH and pass it at configure time

    my @path = ( File::Spec->catfile($installdir, 'lib'),
                 $installdir, $libxml2dir, $zlibdir, 
                 '/usr/local', '/usr/local/lib' );
    
    my %uniq = ();
    $uniq{$_}++ for @path;
    
    $ENV{LD_RUN_PATH} ||= '.';
    
    $ENV{LD_RUN_PATH} = join(':', grep { /./ } keys %uniq, $ENV{LD_RUN_PATH});
    
    
# unset LD_LIBRARY_PATH for this script, so that tests accurately reflect what we've linked
    $ENV{LD_LIBRARY_PATH} = '';
    
    
# Darwin (Mac OS X) requirements

    if ($os =~ /darwin/i) {
    
        $ENV{DYLD_LIBRARY_PATH} = "$installdir/lib";
        $ld_opts =     "-Wl,-L" .
                join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
        
        
        
# irix
    } elsif ($os =~ /irix/i) {
    
        $ld_opts =     "-Wl,-L" . 
                join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
        
    # special irix req
        
        $ENV{LD_LIBRARYN32_PATH} = $ENV{LD_RUN_PATH};
        
        
# solaris 
    } elsif ($os =~ /sun|sol/i) {
        $ld_opts = "-Wl,-rpath -Wl,$ENV{LD_RUN_PATH} -L$ENV{LD_RUN_PATH}";
        
        #$ld_opts =     "-Wl,-L" . 
        #        join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
        
        
# linux
    } elsif ($os =~ /linux/i) {
    
        $ld_opts =     "-Wl,-L" . 
                join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
            
    
# default        
    } else {
    
        #$ld_opts =     '-Wl,-rpath ' .
        #        "-Wl,$ENV{LD_RUN_PATH} -L$ENV{LD_RUN_PATH}";
                
#        $ld_opts =     '-Wl,-rpath';
#        for ( split(/:/,$ENV{LD_RUN_PATH}) ) {
#            $ld_opts .= " -Wl,$_ -L$_ ";
#        }
    
        $ld_opts =     "-Wl,-L" . 
            join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );

        
    }
    
    
# include the -I include paths too

    $ld_opts .= " -I$_/include" for ( grep { s,/lib$,, } split(/:/, $ENV{LD_RUN_PATH} ) );
    
    
    if ($opts->{debug}) {
        print "LD_RUN_PATH = $ENV{LD_RUN_PATH}\n";
        print "ld opts = $ld_opts\n";
    }
}


sub get_ld_path
{
    my $lib = shift || '';
    my $file = shift || $Cout;
    my $dir = shift;
    
    my $cmd = 'ldd';
    $cmd = 'otool -L' if $os =~ m/darwin/i;
    
    my @s = `$cmd $file`;
    
    
   if ($opts->{debug}) {
    warn '-' x 40 . "\n";
    warn "ld test for $lib\n";
    
    warn "libraries that ld linked to in $file:\n";
    warn "$_" for @s;
    
    warn "looking for '/lib/$lib' in array\n";

   }
   
   # \d* is for irix which often uses lib32, etc.
   
    my @where = grep { m!/lib[\d]*/$lib! } @s;
    my $path = shift @where;    # should only be one path
    chomp($path);
    
    warn "found path: $path\n" if $opts->{debug};
    
    $path =~ s,^[^/]*,,g;
    
    warn "no space: $path\n" if $opts->{debug};
    
    my @c = split(m!/!, $path);
    
    my @g;
    for (@c) {
        next if ! $_;
        last if $_ =~ m/^lib\d*$/;
        warn "adding $_ to \@g\n" if $opts->{debug};
        push @g, $_;

    }
    
    $path = '/' . join('/',@g);
    
    print "libpath is $path\n";
    
    $$dir = $path;
    
    return $path;
    
}

sub confirm
{
# check STDIN for a 'yes' or ''
    return 1 if $opts->{quiet};
    
    my $i = <>;
    chomp($i);
    if (! $i or $i =~ m/^y/) {
        return 1;
    } else {
        return 0;
    }
}

sub get_src
{

# returns dir of unpacked src

    my $sw = shift;

    my $src;    # will ultimately be a directory path to sw src
    
    # check tmpdir first since that's where we download via http
    
  DIR: for my $Dir ( $tmpdir, $startdir, $Bin ) {
   
    chdir( $Dir );
        
# several scenarios
# 1. srcdir indicates dir to look in for tar.gz files
# 2. srcdir indicates base dir, plus --sw=file.tar.gz for specific file
# 3. --sw=file.tar.gz with no srcdir base
# 4. no --srcdir and no --sw [ this is default ]
    
    if ( $opts->{srcdir} and ! $opts->{$sw} ) {
    
        my $dir = $opts->{srcdir};
        
        $dir = File::Spec->catfile( $startdir, $dir ) if $dir !~ m!^/!;
        
        print "looking in $dir for $sw|latest.*tar.gz\n";
        
        my $found = 0;
        opendir(DIR, $dir) || die "can't open dir $dir: $!\n";
        while(my $f = readdir(DIR)) {
        
            print "file: $f\n" if $opts->{debug};
        
            if ($f =~ m/($sw|latest).*tar\.gz/) {
            
                $dir .= "/$f";
                $found++;
                last;
            }
            
        }
        closedir(DIR);
        
        $dir = File::Spec->canonpath( $dir );
        
        if ($found) {
            print "Found $dir\n";
            print "Shall I use that? [y] ";
            nice_exit() unless confirm();
            $src = $dir;
            last DIR;
        }
    
    } elsif ( $opts->{srcdir} and $opts->{$sw} ) {
    
    # --sw could be a file or a dir; we trust the user
    # we might also have specified a file IN srcdir
    # so try and allow for both
    
        my $full = File::Spec->catfile( $opts->{srcdir}, $opts->{$sw} );
        unless ( -e $full ) {
        
            $src = $opts->{$sw}; # use exactly what user said
            
        } else {
        
            $src = $full;    # combine --srcdir and --sw
            
        }
    
    
    } elsif ( $opts->{$sw} ) {
    
        $src = $opts->{$sw};
    
    } else {
    
    # assume our hardcoded URL value
    
        $src = $URLs{$sw};
    
    }
    
    next DIR unless $src;
    
    my ($bare,$path) = fileparse( $src );
    
    if ($opts->{debug}) {
    
        print "sw was $sw\n";
        print "src looks like $src\n";
        print "bare looks like $bare\n";
        print "path looks like $path\n";
        
    }
    
    
# now check on readability of what we've deduced

    if ( -r "$Dir/$bare"  and $src =~ m!^http://! ) {
    # use local cached version if it's here
    
        print "Found local copy of $bare in $Dir.\n";
        print "Should I use that instead of downloading? [y] ";
        
        $src = "$Dir/$bare" if confirm();

    }

    if ( -r $src ) {
    
        # nothing to do
    
    } elsif ( ! -r $src and $src !~ m/^http:/ ) {
    
    # try $Bin directory
        
        if ( -r $bare ) {
        
            print "$src is not readable\n";
            print "however, I found $bare.\n";
            print "shall I use that instead ? [y] ";
            
            $src = confirm()
            ? "$Bin/$bare"
            : nice_exit();
            
        } else {
        
            print "couldn't read $src or $Bin/$bare\n";
            nice_exit();
            
        }
        
    } elsif ( $src =~ m/http:/ ) {
    
    # fetch it
    
        chdir( $tmpdir );
    
        print "fetching $src -> $tmpdir/$bare\n";
        
        if (ref $fetcher eq 'CODE') {
        
            &$fetcher( $src, $bare );
            
        } else {
        
            my $cmd = $fetcher;
            
            if ($fetcher =~ m/curl$/) {
        
                $cmd .= " -O ";
        
            }
            
            print "$cmd $src\n";
            
            nice_exit() if system("$cmd $src");
            
        }
        
        $src = "$tmpdir/$bare";
    
    
    } else {
    
        warn "can't determine how to get src file/dir.\n";
        warn "src = $src\n";
        nice_exit();
        
    }
    
    # if we found something, don't look again
    last DIR if $src && -r $src;
    
  }
    
# at this point, $src should be either a dir or a tar.gz file
    
    return -d $src ? File::Spec->canonpath( $src ) : decompress( $src );

}

sub nice_exit
{
    warn "$_\n" for @_;
    warn '=' x 40 . "\n";
    warn "err num: $?\n";
    warn "last err: $!\n";
    warn "check $errlog for more errors\n";
    warn '=' x 40 . "\n";
    warn "sorry it didn't work out\n";
    exit 1;
    
}

sub test_make
{
    print "testing $Make ...  ";
    my @o = `$Make -v`;
    unless ( grep { /GNU\b.*\ 3\.(7|8)/ } @o)
    {
        warn "we've only tested with GNU make version 3.79 or higher\n";
        warn "you've got " . shift @o;
    } else {
        print $o[0];
    }
}


sub configure
{

    #make_clean();    # do we really need to make clean each time?
                    # if source changes, make will figure that out.
                    
    #print "I'm in ";
    #system("pwd");
    my $arg = join(' ',@_) || '';
    my $conf = "./configure --prefix=$installdir $arg $output";
    print "configuring with:\n$conf\n";
    nice_exit() if system($conf);

}

sub make
{
    my $c = shift || $Make;
    my $opts = join ' ', @_;
    print "running $c... ";
    nice_exit() if system("$c $opts $output");

}

sub make_clean
{

    my $c = shift || "$Make distclean";
    print "running $c ... \n";
    if (system("$c $output")) {
        system("rm -f Makefile config.cache");
    }
    
}

sub make_test
{
    my $c = shift || "$Make test";
    print "running $c ...\n";
    if ( system("$c $output") ) {
        warn "\aWARNING: $c failed!!\n";
        warn "make install will run anyway but results are unpredictable.\n";
    }
    
    1;    # always return true from make test so make install will work
        # we assume user will read the err message if make test failed
    
}


sub make_install
{
    my $arg = shift || '';
    my $c = $opts->{sudo}
    ? "sudo $Make install"
    : "$Make install";
    print "running $c $arg\n";
    nice_exit() if system("$c $arg $output");
}

sub decompress
{

# not as simple as you might think. various tar versions
# print output in different ways (stdout/stderr, e.g.), and tar.gz file name
# doesn't always match output dir (as in latest.tar.gz -> swish-e.XX )

    my $f = shift || warn "need file to decompress!\n";
    print "decompressing $f...\n";
    
# unpack in the tmpdir
    chdir($tmpdir);
    
    unless ( -f $f and -r $f ) {
    
        warn "can't read $f: $!\n";
        nice_exit();
        
    }
    
    #print "I'm in ";
    #system('pwd');
    my $c = "gunzip -c $f | tar xvf -";
    #print "trying: $c\n";
    my @out = `$c 2>&1`;
    # directory name should be first value
    my $d = shift @out;
    chomp($d);
    $d =~ s,^(?:x\ )(\w+.*),$1,gs;
    $d =~ s,([\w\.\-\_]+).*,$1,gs;

    print "src directory looks like '$d'\n";
    return "$tmpdir/$d";
    
}

sub zlib
{
    
   print "you indicated $zlibdir\nwe'll trust you.\n" if $zlibdir;
    
   return if ( $zlibdir || test_for_prior_zlib() );    

   print "looks like we'll install zlib.\n" unless $opts->{quiet};

   my $dir = get_src( 'zlib' );
   chdir($dir) || die "can't chdir to $dir: $!\n";
   $opts->{static} ? configure( ) : configure( '-s' );
   make();
   make_test();
# these seem to be ignored at ./configure time??
   my @arg = (
        "includedir=$installdir/include",
           "libdir=$installdir/lib",
        "mandir=$installdir/man"
        );
   make_install( join ' ', @arg );
   $zlibdir = $installdir;


}

sub libxml2
{

    print "you indicated $libxml2dir\nwe'll trust you.\n" if $libxml2dir;

    return if ( $libxml2dir || test_for_prior_libxml2() );
    
    print "looks like we'll install libxml2.\n" unless $opts->{quiet};
    
    my $dir = get_src( 'libxml2' );
    chdir($dir) || die "can't chdir to $dir: $!\n";
    
    my @arg = ( "LDFLAGS='$ld_opts' CFLAGS='-w'" );    # turn off warnings for libxml2
    push(@arg, " --disable-shared ") if $opts->{static};
    configure( @arg );
    
    make( );
    make_test( );
    make_install( );
    
    $libxml2dir = $installdir;
    
}

sub xpdf
{
# we only want the pdftotext component
# and there are reports of xpdf failing to compile
# on some Linux flavors because of the lack of X/Motif
# so just compile the pdftotext pieces

# Thanks to Peter Hayes at Cray for this discovery
    
    my $dir = get_src( 'xpdf' );
    chdir($dir) || die "can't chdir to $dir: $!\n";
    my @arg = ( "LDFLAGS='$ld_opts'" );
    push(@arg, " --disable-shared ") if $opts->{static};

    configure( @arg );
    chdir( 'goo' ) or nice_exit( "can't chdir to 'goo': $!\n" );
    make();
    chdir( '../fofi' ) or nice_exit( "can't chdir to 'fofi': $!\n" );
    make();
    chdir( '../xpdf' ) or nice_exit( "can't chdir to 'xpdf': $!\n" );
    make( 0, 'pdftotext' );
    make( 0, 'pdfinfo' );
    my $copier = $opts->{sudo} ? "sudo cp" : "cp";
    system("$copier pdftotext $installdir/bin")
        and nice_exit( "can't cp pdftotext -> $installdir/bin: $!\n" );
        
    system("$copier pdfinfo $installdir/bin")
        and nice_exit( "can't cp pdfinfo -> $installdir/bin: $!\n" );
        
        
    print "\n";
        
    #make_test();    # not supported
    #make_install();
    
    
}


sub swishe
{

# --swishe trumps all
# else if we've specified --cvs, download latest.
# else check if script was run from a src dir.
# if all else fails, use the get_src() function to locate it.
    
    chdir( $Bin );    # back to where we started
    print "we're in " . `pwd` if $opts->{debug};

    if ($opts->{cvs}) {
# get latest cvs source and set dir

       chdir($tmpdir) or die "can't chdir $tmpdir: $!\n";
    
      # if a previous checkout exists, update it instead of co
       if ( -d 'swish-e' ) {
       
           chdir('swish-e') or die "can't chdir swish-e: $!\n";
        $cvs_cmd =~ s/co swish-e/update -dP/;
       }
        
       system($cvs_cmd);
       $swishdir = "$tmpdir/swish-e";
        
    } elsif ( -x 'configure' and -d 'src' and -d 'man' and -d 'perl' and -d 'pod' ) {
    
# if we are building from source dir (i.e., this script is in the source distrib)
# adjust accordingly
    
# a better way?
        $swishdir = $Bin;
        
    } else {

        $swishdir = get_src( 'swish-e' );
        
    }

    chdir($swishdir) || die "can't chdir to $swishdir: $!\n";
    
    if ($opts->{debug}) {
    
        print "building in " . `pwd`;
        
    }
    
    $zlibdir ||= $installdir;
    $libxml2dir ||= $installdir;
    
    my @arg = (
            "--with-zlib=$zlibdir",
            "--with-libxml2=$libxml2dir",
            "--disable-docs",
            "LDFLAGS='$ld_opts'",
            "CPPFLAGS='-I$zlibdir/include -I$libxml2dir/include -I$libxml2dir/include/libxml2'",
            );
            
    push(@arg, " --disable-shared ") if $opts->{static};
    push(@arg, " --enable-incremental ") if $opts->{increm};

    configure(@arg);
    make();
    make_test();
    make_install();
    return get_ld_path('',"$installdir/bin/swish-e");

}

sub swish_api
{
    chdir("$swishdir/perl");
    
# solaris needs the -R
# linux, et al, do not
    my $libs = $os =~ /sun|sol/i
        ? "-L$installdir/lib -R$installdir/lib -lswish-e -lz"
        : "-L$installdir/lib -lswish-e -lz";
    
    my @a = (    
                "LIBS='$libs'",
                #"LDFLAGS='-L$installdir/lib'",
                #LDFLAGS seems to be ignored by MakeMaker 
                # and/or not supported under some versions.

                "CCFLAGS='-I$installdir/include'",
                'LIB='.$perllib,
                'PREFIX='.$perllib
                );
                
    my $arg = join ' ', @a;
                
    $ENV{SWISHBINDIR} = "$installdir/bin";
    
    my $cmd = "$^X Makefile.PL $arg";
    
    # 2.4.3 and later support non-interactive perllib
    if ( $swishdir =~ m/2\.[45]\.[3-9]/ )
    {
        $cmd .= " $output";
    }
    
    
    print "env SWISHBINDIR set to '$installdir/bin'\n";
    print "configuring with:\n";
    print "$cmd\n";
                                                                
    nice_exit() if system( $cmd );
    make();
    make_test();
    make_install();
    
}


sub test_api
{

# load SWISH::API
    print '=' x 60 . "\n";
    print "testing SWISH::API now that we've installed it.\n";
    print '~' x 60 . "\n";
    
    my $vers = sprintf("%vd", $^V);

    delete $ENV{PERL5LIB};    # just in case we have it somewhere else...
    my $inc = join ' ',
            "-I" . File::Spec->catfile( $perllib, $vers, $arch ).
            "-I" . File::Spec->catfile( $perllib, 'site_perl', $vers, $arch ),
            "-I" . File::Spec->catfile( $perllib, $arch );
        
    my $cmd = "$^X $inc -MSWISH::API -e '\$c = new SWISH::API(\"foo\")'";
        
    print "cmd: $cmd\n" if $opts->{debug};
        
    nice_exit() if system( $cmd );
    
    $swish_api_dir = `$^X $inc -MSWISH::API -e 'print \$INC{"SWISH/API.pm"}'`;
    
    print "looks good.\n";
    
    1;
}



1;
__END__

=pod

=head1 NAME

buildswishe.pl -- build latest version of Swish-e


=head1 DESCRIPTION

Perl script that optionally downloads latest versions of zlib, libxml2
and Swish-e, compiles, tests and installs.

buildswishe.pl aims to be smart
about compiler options and LD_RUN_PATH settings for various flavors
of Unix and Linux.


=head1 SYNOPSIS

  # run interactively
  perl buildswishe.pl
      
  # run with all defaults (non-interactive)
  perl buildswishe.pl --quiet
  
  # to see this doc and options (at end)
  perl buildswishe.pl --help
      
  # to see options
  perl buildswishe.pl --opts


=head1 REQUIREMENTS

 Perl 5.6.0 or later
 GNU gcc (tested with vers 2.95 or later)
 wget, curl or LWP::Simple (if fetching src)
 tar
 gzip
 GNU make (tested with 3.79 and later)
 ld
 ldd (or otool for Darwin)
 
=head1 TODO

A good test suite for Swish-e.

=head1 AUTHOR

Peter Karman - karman@cray.com

I welcome bug reports, hints, suggestions, and good beer.

Thanks to the Swish-e community and developers. A real swell product.

This script intended to be used with CrayDoc 4 (http://www.cray.com/craydoc/)
but should work fine for Swish-e users and developers.


=head1 LICENSE

 ###############################################################################
 #
 #    CrayDoc 4
 #    Copyright (C) 2004-2005 Cray Inc swpubs@cray.com
 #
 #    This program is free software; you can redistribute it and/or modify
 #    it under the terms of the GNU General Public License as published by
 #    the Free Software Foundation; either version 2 of the License, or
 #    (at your option) any later version.
 #
 #    This program is distributed in the hope that it will be useful,
 #    but WITHOUT ANY WARRANTY; without even the implied warranty of
 #    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #    GNU General Public License for more details.
 #
 #    You should have received a copy of the GNU General Public License
 #    along with this program; if not, write to the Free Software
 #    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 #
 ###############################################################################

=head1 SEE ALSO

 LWP::Simple
 http://www.swish-e.org/
 http://gcc.gnu.org/
 http://www.cray.com/craydoc/

=cut
