#!/usr/bin/perl -w
use strict;
use Getopt::Long;
use Data::Dumper;
use Pod::Usage;
use File::Basename;

# This is a script for generating daily builds/release builds
# needs cleaning up and less dependency on paths listed here.

die "no home environment variable" unless $ENV{HOME};

#Top level directory
my $base = '/data/_a/webdata/SWISH-E';

# Fix broken sunstie
$ENV{LDFLAGS} = '-R/usr/local/lib';

#--------------- config ---------------------------------------------

my %config = (
	topdir    => "$base/builds",  # must not be relative directory
	doclink   => "$base/current/docs",   # symlink to point to HTML docs
	tardir    => "$base/Download",        # where to store tarball
	cvsco     => 'cvs -z3 -d:ext:whmoseley@cvs.swishe.sourceforge.net:/cvsroot/swishe',
	keep_days => 15,  
);

#--------------------------------------------------------------------


$config{timestamp} = 1;  # default to tarball with timestamp

GetOptions( \%config, 
    qw/
       topdir=s doclink=s tardir=s keep_days=i cvsco=s
       dump verbose timestamp! ask
       fetchtarurl=s noremove useexistingbuild useexistinginstall
       help man
    /
);

# Set dependent variables
set_dependent_vars( \%config );

if ( $config{dump} ) {
    print STDERR Dumper \%config;
    exit;
}

my @functions = (
    [ \&cvs_checkout, 'Check out from CVS' ],  # can be replaced by a LWP fetch with --fetchtarurl=<url>
    [ \&configure,    'Run configure' ],
    [ \&make_install, 'Run make install' ],
    [ \&make_docs,    "Run make docs and point $config{doclink} to HTML docs" ],
    [ \&make_dist,    "Run make dist and move tarball to $config{tardir}"  ],
#    [ \&move_logs,    "Move build logs to $config{tardir}" ],
    [ \&remove_build_dir, "Remove Build dir $config{builddir}" ],
);



pod2usage(0) if $config{help};
pod2usage( -verbose => 2) if $config{man};


# replace cvs co with a LWP fetch?
if ( $config{fetchtarurl} ) {
    $functions[0] = [ \&fetch_tar_url, 'Fetch tarball' ],
}



die "doclink not symbolic link: $config{doclink}:$!" unless -l $config{doclink}; 


unlink $config{buildlog}, $config{errorlog};

umask 022;  # we want people (web servers) to read files created
	
# Move to the toplevel directory
unless ( chdir $config{topdir} ) {
        warn "Failed to chdir to $config{topdir}: $!";
	# Try to create the directory
	mkdir $config{topdir},0777 || die "Failed to mkdir topdir $config{topdir}: $!";
	warn "Created top level directory $config{topdir}\n";
	chdir $config{topdir} || die "Failed to chdir to $config{topdir}";
}

# Validate the tarball directory
unless ( -d $config{tardir} ) {               
	mkdir $config{tardir},0777 || die "Failed to mkdir tardir $config{tardir}: $!";
	warn "Created tarball directory $config{tardir}\n";
}



if ( $config{ask} ) {
   for my $step ( @functions ) {
       my $answer;
       my $done = 0;
       do {
            print "$step->[1]? ";
            $answer = <STDIN>;
            $done =  $answer =~ /^(y|n)/i;
            print "\nPlease answer y or n\n" unless $done;
       } until $done;

       $step->[2] = 1 if $answer =~ /^n/i;
    }
}

for my $step ( @functions ) {
    next if $step->[2];  # skip from above
    log_message( $step->[1] );
    $step->[0]->(\%config) || exit_with_error( \%config );
}
exit 0;





sub cvs_checkout {
    my $c = shift;

    if ( -e $c->{builddir} ) {
        if ( $c->{useexistingbuild} ) {
            log_message("No cvs co: Using existing build directory $c->{builddir}");
            return 1;
        }
        warn "Build directory $c->{builddir} already exists.  Use -useexitingbuild to ignnore.\n";
        return;
    }

    my $command = "$c->{cvsco} co -d $c->{builddir} swish-e";
    run_command( $command );
}

sub configure {
    my $c = shift;
    my $options = $c->{config_options} || '';
    
    chdir $c->{builddir} || die "Failed to chdir $c->{builddir}: $!";
    log_message( "Changed to build directory: $c->{builddir}" );

    # Make sure install dir doesn't exist
    if ( -e $c->{installdir} ) {
        if ( $c->{useexistinginstall} ) {
            warn "Warning: Using existing install directory $c->{installdir}\n";
        } else {
            warn "Install directory already $c->{installdir} already exists.  Use -useexistinginstall to ignore\n";
            return;
        }
    }

    my $timestamp = $c->{timestamp} ? '--enable-daystamp'  : '';    
    my $command = "./configure $timestamp $options --prefix=$c->{installdir}";
    run_command( $command ) || return;
    
    chdir ".." || die "Failed to chdir to parent directory";
    return 1;
}


sub make_install {
    my $c = shift;
    
    chdir $c->{builddir} || die "Failed to chdir $c->{builddir}: $!";
    log_message( "Changed to build directory: $c->{builddir}" );

    
    run_command( "make install" ) || return;

    unless ( -d $c->{installdir} ) {
        warn "Did not find install directory $c->{installdir} after make install";
        return;
    }

    # check binary and fetch version
    my $binary = "$c->{installdir}/bin/swish-e";
    die "Failed to find swish-e binary $binary\n" unless -x $binary;
    open( SWISH, "$binary -V |" ) || die "Failed to open pipe from $binary: $!";
    my $version = <SWISH>;
    close( SWISH ) || warn "Failed close of $binary: $!";
    chomp $version;
    if ( $version =~ /^SWISH-E\s+(.+)$/ ) {
        $c->{version} = $1;
    }
    
    
    chdir ".." || die "Failed to chdir to parent directory";
    return 1;
}

sub make_dist {
    my $c = shift;

    chdir $c->{builddir} || die "Failed to chdir $c->{builddir}: $!";
    log_message( "Changed to build directory: $c->{builddir}" );

    run_command( "make dist" ) || return;

    # Do we know the version?
    if ( $c->{version} && -e "swish-e-$c->{version}.tar.gz") {
        run_command( "mv swish-e-$c->{version}.tar.gz $c->{tardir}" );
        run_command( "rm -f $c->{tardir}/latest.tar.gz" );
        run_command( "ln -s $c->{tardir}/swish-e-$c->{version}.tar.gz $c->{tardir}/latest.tar.gz");
    } else {
        log_message( "Found version [$c->{version}] but didn't find [swish-e-$c->{version}.tar.gz]" )
            if $c->{version};
        run_command( "mv swish-e*.tar.gz $c->{tardir}") || return;
    }

    # purge old ones
    #purge_old_tarballs( $c );

    chdir ".." || die "Failed to chdir to parent directory";
    return 1;
}

sub purge_old_tarballs {
    my $c = shift;
    opendir( DIR, $c->{tardir} ) || die "Failed to open tardir: $!";

    my $days = $c->{keep_days} || 10;
    my $old = time - ( 60 * 60 * 24 * $days );

    my @deletes;
    while ( my $file = readdir( DIR ) ) {
        next unless $file =~ /^swish-e.*\.tar\.gz$/;
        next if (stat "$c->{tardir}/$file")[9] > $old;
        push @deletes, "$c->{tardir}/$file";
    }
    
 
    # only a warning
    if ( unlink( @deletes) != @deletes ) {
        print STDERR "Failed to unlink old files: @deletes : $!";
    }
    
    return 1;

}

sub move_logs {
    my $c = shift;

    run_command( "mv  $c->{errorlog} $c->{tardir}") || return;
    run_command( "mv  $c->{buildlog} $c->{tardir}") || return;

}
                                                

sub make_docs {
    my $c = shift;
    my $options = $c->{configure_options} || '';
    
    chdir "$c->{builddir}/html" || die "Failed to chdir $c->{builddir}/html: $!";
    log_message( "Changed to build directory: $c->{builddir}/html" );
    
    run_command( "make searchdoc" ) || return;
    chdir ".." || die "Failed to chdir to parent directory";
    
    unlink( $c->{doclink} ) || die "Failed to unlink $c->{doclink}: $!";
    
 
    run_command("ln -s $c->{htmldir} $c->{doclink}" ) || return;

    chdir ".." || die "Failed to chdir to parent directory";
    
    
    # Now remove any previous install directory
    if ( ! $c->{noremove} && open FH, $c->{lastinstall} ) {
    	my $last = <FH>;
        close FH;

    	chomp $last;

    	unless ( $last =~ m[$config{topdir}/install_] ) {
    	    warn "Last install directory has bad format: $last\n";

        } elsif ( $last eq $c->{installdir} ) {
            warn "Not removing last install directory because same as current\n";
            log_message( "Not removing last install directory because same as current" );

    	} elsif ( ! -d $last ) {
	    warn "Last install dir $last is not a directory\n";
        } else {
            run_command( "rm -rf $last" );
        }
    }
    open (FH, ">$c->{lastinstall}") or die "Failed to open $c->{lastinstall}: $!";
    print FH $c->{installdir},"\n";
    close FH;
    return 1;
}

sub remove_build_dir { 
    my $c = shift;
    return 1 if $c->{noremove};
    run_command( "rm -rf $c->{builddir}" ) if -d $c->{builddir};
}
                



sub run_command {
    my $command = shift;
    log_message( $command );

    return !system( "$command 2>>$config{errorlog} >>$config{buildlog}" );
}

sub log_message {
    my $command = shift;
    open( LOG, ">>$config{buildlog}") || die "failed to open log file: $!";
    print LOG "\n", scalar localtime,"\n $command\n\n";
    close LOG;
}


        
sub exit_with_error {
    my $c = shift;
    print STDERR `cat $c->{errorlog}` if -e $c->{errorlog};
    die "aborted $0\n";
}
	 

sub set_dependent_vars {
    my $config = shift;

    #  This must be absolute
    $config->{buildlog} = "$config->{topdir}/build.log";
    $config->{errorlog} = "$config->{topdir}/error.log";


    my ($d,$m,$y) = (localtime)[3,4,5];
    my $today = sprintf('%04d-%02d-%02d', $y+1900, $m+1, $d );

    my $build_name = "swish-e-$today";

    # Builddir must be relative to current directory
    $config->{builddir} = "build_$build_name";
    $config->{installdir} = "$config{topdir}/install_$build_name";

    $config->{htmldir} = "$config->{installdir}/share/doc/swish-e/html";

    # For tracking last installation directory
    $config->{lastinstall} = "$config->{topdir}/lastinstall";
	
}

sub fetch_tar_url {
    my $config = shift;

    require LWP::Simple;
    require URI;
    
    log_message( "Fetching $config->{fetchtarurl}");

    my $u = URI->new( $config->{fetchtarurl} ) || die "$config->{fetchtarurl} doesn't look like a URL: $!";

    my ( $name, $path, $suffix ) = fileparse( $u->path, qr/\.tar.gz/ );

    die "Failed to parse $config->{fetchtarurl} [$path $name $suffix]\n"
       unless $path && $name && $suffix;

    my $tar = $name . $suffix;

    my $status = LWP::Simple::getstore( $config->{fetchtarurl}, $tar ) || die "Failed to fetch $config->{fetchtarurl}";
    die "fetching $config->{fetchtarurl} returned a status of $status" unless $status == 200;

    die "Can't find fetched file $tar" unless -e $tar;


    run_command( "gzip -dc $tar | tar xvf -" ) || return;

    # see if the tar name is indeed the install directory
    unless ( -d $name ) {
        # ok, try and get from the tar
        open( TAR, "gzip -dc $tar | tar tf -|") or die "Failed to pipe from tar:$!";
        $name = '';
        while ( <TAR> ) {
            chomp;
            if ( m!^(.+?)/! && -d $1 ) {
                $name = $1;
                last;
            }
        }
        close TAR;
    }
        

    die "Failed to find source directory after unpacking $tar" unless $name;

    # Set the build dir
    $config->{builddir} = $name;

    return 1;

}

__END__

=head1 NAME 

swish-daily.pl - Builds a daily snapshot

=head1 SYNOPSIS

./swish-daily.pl [options]

Options:

    -help               brief help 
    -man                longer help
    -dump               dump configuration and exit.  Good for seeing defaults
    -ask                ask what tasks to run
    -topdir=<path>      path to top-level directory (build happens below)
    -doclink=<path>     symlink to replace with a symlink to HTML docs
    -tardir=<path>      where to place tarball when finished
    -keep_day=<int>     number of days to keep old tarballs
    -cvsco=<string>     command to checkout from cvs
    -[no]timestamp      if set will create tarball with a timestamp
    -fetchtarurl=<url>  specify a URL to a tarball to fetch (instead of using cvs)
    -noremove           doesn't remove last install directory or build directory
    -useexistingbuild   don't abort if the build directory exists, and use it.  (Avoids cvs co)
    -useexistininstall  don't abort if the install directory exists, and use it.

=head1 OPTIONS

Only a few options are discussed here

=over 8

=item B<-dump>

This dumps the configuration and exits.  Use to see what paths are going to be used.

=item B<-ask>

Allows you to select a subset of operations.  Obviously, some operations depend on others.

=item B<-topdir>

Toplevel dir for operation.  The build directory is a subdirectory of this directory.

=item B<-doclink>

This is a link that points to the HTML documentation.  It has to exist as a symlink
or else an error will be reported.  After the HTML docs are build (and indexed) that symlink
will be replaced with a new symlink pointing to the just-build HTML docs.

=item B<-tardir>

This is a directory where the finished tarball will be placed.  Old tar files (older than -keep_days old)
will be removed.

=item B<-cvsco>

String for checking out from cvs.  Run -dump to see an example.

=item B<-[no]timestamp>

This is set by default and causes the tarball to be build with a day stamp.  Use -notimestamp
to build without a timestamp.  (Sets --enable-daystamp in configure).

=item B<-fetchtarurl>

Specifies a URL where to fetch the tarball.  This will replace the use of cvs for fetching the source.

=item B<-noremove>

Normally the build directory and the last installation directory (from a previous run)
are removed when done.  This prevents this.

=item B<-useexistingbuild>

Normally the script will abort if an existing build directory exists.
This options allows the script to continue.  Mostly useful to skip the
cvs checkout step when debugging this script.

=item B<-useexistinginstall>

The script will stop before running configure if the install directory already exists.
This is a general sanity check and to prevent overwriting an existing installation 
directory.  -useexistinginstall will say it's ok to use the existing install directory.


=back

=head1 DESCRIPTION

This script is used to generate a daily build of swish-e from cvs (or tarball). 

It buids swish and installs to a local directory (using a date stamp).  

The HTML docs are indexed and a symlink points to the new HTML docs.
At this point the old install location is removed.

The build directory is then removed leaving only the current install directory.

A top-level directory is used (-topdir) as the working directory.  This directory
will be created if it doesn't already exist.  The program then changes directory to 
this directory.

Then swish-e is checked out from cvs into a subdirectory and make install is run.
A prefix of ${topdir}/install_swish-e-<date> is used as the installation directory.

After make install the HTML docs are indexed and a symlink pointing to the HTML
docs is make (this keeps the web link /dev/docs pointing to current HTML docs).

Next a tarball is created and moved.  Old tar files are removed.

build and error logs are also copied to the tarball directory.

Finally, the old installation directory is removed.

Might be nice to have a way to have more than one install directory in the same day.


=cut
