#!/usr/bin/perl -w
#
#	build swish-e with zlib and libxml2 support.
#	tries to be intelligent about OS and compiler
#	options.
#
#	Wed Sep 15 12:07:55 CDT 2004 -- karman@cray.com
#
#	0.01	-- initial public release
#	0.02	-- --srcdir works; IRIX fixes; --quiet fixes
#
##########################################################
$| = 1;

use strict;
require 5.006;	# might work with < perl 5.6.0 but untested
use sigtrap qw(die normal-signals error-signals);
use Getopt::Long;
use Config qw( %Config );
use File::Path qw( mkpath );
use FindBin qw($Bin);

my $Version 	= '0.02';

# there must be a better way to dynamically retrieve the latest version
# of a sw package, other than hardcoding urls. help?

my %URLs	= (

'swishe' 	=> 'http://swish-e.org/Download/latest.tar.gz',
'zlib'		=> 'http://www.zlib.net/zlib-1.2.1.tar.gz',
'libxml2'	=> 'http://xmlsoft.org/sources/libxml2-2.6.10.tar.gz',

);

my $swish_cvs 	= ':pserver:anonymous@cvs.sourceforge.net:/cvsroot/swishe';
my $cvs_cmd 	= "cvs -q -d$swish_cvs co swish-e";

my $defdir 	= '/usr/local/swish-e/latest';
my $deftmp 	= $ENV{TMPDIR} || $ENV{TMP} || '/tmp';

my $opts 	= {};
 
my $allopts = {
        
	'quiet'		=> "run non-interactively",
	'cvs'		=> "use latest CVS version of SWISH-E",
	'swish=s'	=> "use <X> as swish-e source -- either URL, tar.gz or directory",
	'libxml2=s'	=> "use <X> as libxml2 source -- either URL, tar.gz or directory",
	'zlib=s'	=> "use <X> as zlib source -- either URL, tar.gz or directory",
	'prevzlib=s'	=> "use already-installed zlib in directory <X>",
	'prevxml2=s'	=> "use already-installed libxml2 in directory <X>",
	'installdir=s'	=> "install in <X> [$defdir]",
	'verbose'	=> "don't send compiler output to log, send it to stdout",
	'srcdir=s'	=> "look in <X> for all src tar.gz files [./]",
	'tmpdir=s'	=> "use <X> to compile and test [$deftmp]",
	'help'		=> "print usage statement",
	'opts'		=> "print options and description",
	'force'		=> "install zlib and libxml2 no matter what",
	'debug'		=> "lots of on-screen verbage"
	
};

my $helpful =<<EOF;
Please send email to the author with this message.
Include all screen output and a description of your problem.
Thanks.

EOF

if (! GetOptions($opts,keys %$allopts) or $opts->{help}) {
	usage();
}

if ($opts->{opts}) {
# print all options and descriptions
	printopts();
	exit;
}

use vars qw( $os $arch $cc $installdir $nogcc $min_gcc $tmpdir
		$outlog $errlog $output $ld_opts $Cout $Cin
		$zlib_test $libxml2_test $ld_test $gcc_test
		$swishdir $fetcher $libxml2dir $zlibdir $MinLibxml2 $cmdout
		
		);

$zlibdir	= $opts->{prevzlib} || '';
$libxml2dir	= $opts->{prevxml2} || '';


# some defaults
$installdir = $opts->{installdir} || $defdir;

$MinLibxml2	= '2.4.3';	# this is same as in swish-2.4.2 -- sync it better than this.
$nogcc 		= "I refuse to compile without gcc\nCheck out http://gcc.gnu.org\n";
$min_gcc 	= '2.95';
$tmpdir 	= $opts->{tmpdir} || $deftmp;
$outlog 	= $tmpdir . '/buildswishe.log';
$errlog		= $tmpdir . '/buildswishe.err.log';
$output 	= $opts->{verbose} ? '' : " 1>>$outlog 2>>$errlog ";

$cmdout		= $opts->{quiet} ? ' 1>/dev/null 2>/dev/null ' : '';

$ld_opts 	= '';	# define these based on OS platform


# better C tests would be nice. these seem to work.
$Cout 		= "$tmpdir/test";
$Cin		= "$tmpdir/$$.c";
	
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

	libxml2		=> \&libxml2,
	zlib		=> \&zlib,
	swish		=> \&swishe,
	'swish::api'	=> \&swish_api
	
	);
	
for ( qw( zlib libxml2 swish swish::api ) ) {

	print '=' x 40 . "\n";
	print "building $_ ...\n";
	print '=' x 40 . "\n";
	my $f = $routines{$_};
	&$f;
	
}

test();
cleanup();
exit;

END
{ 
  unless ($opts->{debug}) {
   system("rm -f $tmpdir/$$.c $tmpdir/test") if $tmpdir;
  }
}

##########################################################

sub printopts
{	
	print "\n$0 options\n", '-' x 30 , "\n";
	my $buf = 20;
	for (sort keys %$allopts) {
		
		(my $o = $_) =~ s,[:=]s,=<X>,g;
		
		my $space = ' ' x ( $buf - length($o) );
		
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

	if ($opts->{quiet}) {
	
		open(QUIET, ">/dev/null") or die "can't write to /dev/null: $!\n";
		select(QUIET);	# should send all default print()s to oblivion
		
	}
	
	$os 	= $Config{osname} || $ENV{OSTYPE} || `uname`;
	$arch 	= $Config{archname};
	$cc 	= $Config{cc};	# for SWISH::API should use same compiler as perl
				# was compiled with, and that SHOULD be gcc.

    if ($opts->{debug}) {
	print "os... $os\n";
	print "arch... $arch\n";
	print "cc... $cc\n";
    }
    
	system("rm -f $outlog $errlog");	# so we start afresh
	
	ld_opts();
	set_fetcher();
}

sub makedirs
{

	my @d = ($installdir,
		 $tmpdir,
		"$installdir/lib",
		"$installdir/include"
		);
	mkpath( \@d, 1, 0755 );
	
}

sub sanity
{

	print "is $cc a GNU C compiler... ";
	test_compiler();
	
# basic ld test with libc
	if ( test_c_prog( $ld_test, '-lc' ) ) {
	
		die "can't link against libc with $cc\n";
	
	}
	
	if ( ! $opts->{quiet} ) {
	
		print "Continue on our merry way? [y] ";
		nice_exit() if ! confirm();
		
	}
	
}

sub prompt_to_install
{

	my $what = shift;
	$opts->{force}
	? print "you used --force to install $what\n"
	: print "$what is not installed\n";
	print "would you like to install it? [y] ";
	return confirm();
	
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
	my $extra 	= join ' ', @_;
	$extra 		.= ' -w' if $os =~ /irix/i;
	my $cmd 	= "$cc $extra -o $Cout $Cin";
	
	#print "compile cmd: $cmd\n";
	
	return 1 if system("$cmd $cmdout");	# don't bother if we can't even compile	
	
	if ($test) {
	
		chomp( my $o = `$Cout` );
		
		if ($o lt $$test) {
			
			print $o . "\n";
			
			return 1;	# 1 indicated failure like system() does
			
		}
		
	}
	
 # run it
 
 	warn "run: $Cout $cmdout\n" if $opts->{debug};
 
	return system("$Cout $cmdout");

}


sub test_for_prior_zlib
{

	print "testing for already-installed zlib... "; 
	return get_ld_path('libz', '', \$zlibdir) if ! test_c_prog( $zlib_test, '-lz' );
	
}

sub test_for_prior_libxml2
{

	print "testing for already-installed libxml2... ";
	
# first include -lxml2 and see if it returns a path
	
	my $path = get_ld_path('libxml2', '', \$libxml2dir)
	  if ! test_c_prog( $ld_test, $ld_opts, '-lxml2' );

# then check version with xmlversion.h

	return 0 if ! $path;
	
	print "minimum libxml2 version required is $MinLibxml2 and you have... ";
	return get_ld_path('libxml2', '', \$libxml2dir)
	  if ! test_c_prog( $libxml2_test, "-I$path/include/libxml2", '-lxml2', \$MinLibxml2 );
	  
	
}


sub cleanup
{

	print "swish-e was installed in $installdir\n";

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
	warn "no program available to fetch remote sw source\n" if ! $fetcher;

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
		warn 	"Warning: this perl was not compiled with gcc!\n".
			"Instead: $cc\n".
			"This may cause compile of SWISH::API to fail.\n".
			"We'll try anyway, but be warned.\n";
			
# if $cc is not gcc, test for it in our path anyway.
		my $gcc = which('gcc');
		if ( $gcc ) {
		   print 	"Found a gcc at $gcc\n";
		   if ($opts->{interactive}) {
			print	"Should I use that? [yes] ";
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
			die $nogcc;
		}
	}

# by now, we should be using gcc
# check version
	print "testing gcc version... ";
	my @v = `$cc -v 2>&1`;
	my @vers = grep { /version \d/ } @v;
	if (! @vers) {
		warn 	"Warning: trouble getting version from $cc\n".
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

	my @path = ( "$installdir/lib", $installdir, $libxml2dir, $zlibdir );
	
	$ENV{LD_RUN_PATH} ||= '.';
	
	$ENV{LD_RUN_PATH} = join(':', grep { /./ } @path, $ENV{LD_RUN_PATH});
	
	
# Darwin (Mac OS X) requirements

	if ($os =~ /darwin/i) {
	
		$ENV{DYLD_LIBRARY_PATH} = "$installdir/lib";
		$ld_opts = "-Wl,-L" . join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
		
		
		
# irix
	} elsif ($os =~ /irix/i) {
	
		$ld_opts = 	"-Wl,-L" . 
				join(' -Wl,-L', split(/:/,$ENV{LD_RUN_PATH}) );
		
		$ENV{LD_LIBRARYN32_PATH} = $ENV{LD_RUN_PATH};
		
		
# solaris 
	} elsif ($os =~ /sun|sol/i) {
		$ld_opts = "-Wl,-rpath -Wl,$ENV{LD_RUN_PATH} -L$ENV{LD_RUN_PATH}";
		
		
		
# linux
	} elsif ($os =~ /linux/i) {
	
		$ld_opts = "-Wl,-rpath -Wl,$ENV{LD_RUN_PATH} -L$ENV{LD_RUN_PATH}";
			
	
# default		
	} else {
	
		#$ld_opts = 	'-Wl,-rpath ' .
		#		"-Wl,$ENV{LD_RUN_PATH} -L$ENV{LD_RUN_PATH}";
				
		$ld_opts = 	'-Wl,-rpath';
		for ( split(/:/,$ENV{LD_RUN_PATH}) ) {
			$ld_opts .= " -Wl,$_ -L$_ ";
		}
	
		
	}
	
	print "LD_RUN_PATH = $ENV{LD_RUN_PATH}\n";
	print "ld opts = $ld_opts\n";
	
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
	my $path = shift @where;	# should only be one path
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
	
	warn "PATH is $path\n" if $opts->{debug};
	
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

# check for src sw in following order:
#	$opts
#	$PWD
#	tar.gz
#	%URLs

	chdir( $Bin );
	
	my $src = $opts->{$sw} || $opts->{srcdir} || $URLs{$sw};
	
	my ($bare) = ( $src =~ m/^.*\/(.*)/ );
	
	if ( -r $bare and $src =~ m/^http:/ ) {
	
	# use previously downloaded version
	
		$src = "$Bin/$bare";
	
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
		print "fetching $sw -> $src\n";
		
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
		
		$src = "$Bin/$bare";	# default is to download to same dir where we run script
	
	
	} elsif ( -d $src ) {
	# find a tar.gz or subdir in $src and reset
	
		my $dir = $src;
		
		print "looking in $dir for $sw|latest.*tar.gz\n";
		
		my $found = 0;
		opendir(DIR, $dir) || die "can't open dir $dir: $!\n";
		while(my $f = readdir(DIR)) {
		
			if ($f =~ m/($sw|latest).*tar\.gz/) {
			
				$dir .= "/$f";
				$found++;
			}
			
		}
		closedir(DIR);
		
		if ($found) {
			print "Found $dir\n";
			print "Shall I use that? [y] ";
			nice_exit() if ! confirm();
			$src = $dir;
		}
	
	}
	
# at this point, $src should be either a dir or a tar.gz file

	my $dir = -d $src
	? $src
	: decompress( $src );
	
	return $dir;

}

sub nice_exit
{
	warn '=' x 40 . "\n";
	warn "err num: $?\n";
	warn "last err: $!\n";
	warn "check $errlog for more errors\n";
	warn '=' x 40 . "\n";
	die "sorry it didn't work out\n";
	
}

sub test_make
{

	my @o = `make -v`;
	if (! grep { /GNU Make version 3\.(7|8)/ } @o) {
		warn "we've only tested with GNU make version 3.79 or higher\n";
		warn "you've got " . shift @o;
	}
}


sub configure
{

	make_clean();
	#print "I'm in ";
	#system("pwd");
	my $arg = join(' ',@_) || '';
	my $conf = "./configure --prefix=$installdir $arg $output";
	print "configuring with:\n$conf\n";
	nice_exit() if system($conf);

}

sub make
{

	print "running make... ";
	nice_exit() if system("make $output");

}

sub make_clean
{

	print "running make clean ... \n";
	if (system("make distclean $output")) {
		system("rm -f Makefile config.cache");
	}
	
}

sub make_test
{
	print "running make test ...\n";
	if (system("make test $output")) {
		warn "make test failed\n";
		if (! $opts->{verbose}) {
			warn "running again so you can see output\n";
			system("make test");
		}
	}
	
}


sub make_install
{
	my $arg = shift || '';
	print "running make install $arg\n";
	nice_exit() if system("make install $arg $output");
	
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
   configure();
   make();
   make_test();
# these seem to be ignored at ./configure time??
   my @arg = (	"includedir=$installdir/include",
   		"libdir=$installdir/lib",
		"mandir=$installdir/man"
		);
   make_install( join ' ', @arg );


}

sub libxml2
{

	print "you indicated $libxml2dir\nwe'll trust you.\n" if $libxml2dir;

	return if ( $libxml2dir || test_for_prior_libxml2() );
	
	print "looks like we'll install libxml2.\n" unless $opts->{quiet};
	
	my $dir = get_src( 'libxml2' );
	chdir($dir) || die "can't chdir to $dir: $!\n";
	configure("LDFLAGS='$ld_opts'");
	make();
	make_test();
	make_install();
	
}


sub swishe
{

	if ($opts->{cvs}) {
# get latest cvs source and set dir

	   chdir($tmpdir) or die "can't chdir $tmpdir: $!\n";
	
	  # if a previous checkout exists, update it instead of co
	   if ( -d 'swish-e' ) {
	   
	   	chdir('swish-e') or die "can't chdir swish-e: $!\n";
		$cvs_cmd =~ s/co swish-e/update -dP/;
	   }
		
	   system($cvs_cmd);
	   $opts->{swishe} = "$tmpdir/swish-e";
		
	} elsif ( -x 'configure' and -d 'src' ) {
	
# if we are building from source dir (i.e., this script is in the source distrib)
# adjust accordingly
	
# a better way?
		$opts->{swishe} = $Bin;
		
	}

	my $dir = get_src( 'swishe' );
	$swishdir = $dir;
	chdir($dir) || die "can't chdir to $dir: $!\n";
	
	if ($opts->{debug}) {
	
		print "building in ";
		system('pwd');
		
	}
	
	$zlibdir ||= $swishdir;
	$libxml2dir ||= $swishdir;
	
	my @arg = (	"--with-zlib=$zlibdir",
			"--with-libxml2=$libxml2dir",
			"--disable-shared",
			"LDFLAGS='$ld_opts'",
			"CPPFLAGS='-I$zlibdir/include -I$libxml2dir/include'",
			);

	configure(@arg);
	make();
	make_test();
	make_install();
	return get_ld_path('',"$installdir/bin/swish-e");

}

sub swish_api
{

	chdir("$swishdir/perl");
	my $arg = join ' ', (	"PREFIX=$installdir",
				"LIB=$installdir/lib",
				"LIBS='-L$installdir/lib -lswish-e -lz'",
				"LDFLAGS='-L$installdir'",
				"CCFLAGS='-I$installdir/include'"
				);
				
	$ENV{SWISHBIN} = "$installdir/bin/swish-e";
				
	print "configuring with:\n";
	print "$^X Makefile.PL $arg\n";
				
	nice_exit() if system("$^X Makefile.PL $arg");
	make();
	make_test();
	make_install();


}


sub test_api
{

# load SWISH::API
	delete $ENV{PERL5LIB};	# just in case we have it somewhere else...
	my $inc = "-I$installdir/lib -I$installdir/lib/$arch";
	nice_exit() if system("$^X $inc -MSWISH::API -e '\$c = new SWISH::API(\"foo\")'");
}





1;
__END__

=pod

=head1 NAME

buildswishe.pl -- build latest version of SWISH-E


=head1 DESCRIPTION

Perl script that optionally downloads latest versions of zlib, libxml2
and SWISH-E, compiles, tests and installs.

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

A better way to get latest version of zlib and libxml2. Right now
the version numbers are hardcoded, or you must know them and pass as options.
Is there a way to query the websites for this?

A good test suite for SWISH-E.

=head1 AUTHOR

Peter Karman - karman@cray.com

I welcome bug reports, hints, suggestions, and good beer.

Thanks to the SWISH-E community and developers. A real swell product.

This script intended to be used with CrayDoc 4 (http://www.cray.com/craydoc/)
but should work fine for swish-e users and developers.


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


