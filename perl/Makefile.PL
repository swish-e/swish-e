#!/usr/bin/perl -w

use strict;
use ExtUtils::MakeMaker;
use Config;         # for path separator
use File::Spec;     # for catpath

# $Id$

my $tests = 't/*.t';
my $SWISH_BINARY = 'swish-e';
my $help = <<EOF;

Makefile.PL options:

    SWISHBIN        = full path to swish-e binary
    SWISHSKIPTEST   = don't abort if binary cannot be found and don't run tests
    SWISHHELP       = print this message

Can be either environment variables or passed on command line like:

    perl Makefile.PL SWISHBIN=/usr/local/bin/swish-e

EOF

my %valid = map { $_ , 1 } qw/ SWISHBIN SWISHSKIPTEST SWISHHELP /;


$SIG{__DIE__} = sub {
    print STDERR "\n** ERROR: ", @_;
    print STDERR $help;
    exit 1;
};



my %config;
while( $_ = shift ) {
    if ( $_ eq 'SWISHHELP' ) {
        $ENV{SWISHHELP} = 'y';
        last;
    }

    my ( $param, $value ) = split /=/;
    if ( $param =~ /^SWISH/ ) {
        die "Invalid option '$param'\n" unless $valid{$param};
        $ENV{$param} = $value || '';
    } else {
        $config{$param} = $value || '';
    }
}


if ( exists $ENV{SWISHHELP} ) {
    print $help;
    exit 0;
}

my $swish_binary = find_binary();

if ( $swish_binary ) {
    print "\nUsing swish-e binary '$swish_binary'\n";

    test_version( $swish_binary, '2.4.3' );
    create_index( $swish_binary );

} else {
    die "Failed to find swish-e binary '$SWISH_BINARY'"
        unless $ENV{SWISHSKIPTEST};
}

$tests = 't/dummy.t' if $ENV{SWISHSKIPTEST};

WriteMakefile(
    %config,
    'NAME'          => 'SWISH::API',
    'VERSION_FROM'  => 'API.pm',
    'LIBS'          => ['-lswish-e'],
    'NORECURS'      => 1,          # keep it from recursing into subdirectories
    'DIR'           => [],
    'XSPROTOARG'    => '-noprototypes',
    'PREREQ_PM'     => {
        'File::Spec'    => '0.8',
    },
    'clean'         => {
        'FILES'         => join( ' ', qw(
            t/index.swish-e
            t/index.swish-e.prop
        )),
    },
    'test'          => {
        'TESTS'         => $tests,
    },
);


sub find_binary {

    my $binary = $ENV{SWISHBIN} || find_program_in_path( $SWISH_BINARY );

    # Could ask here, but might as well just let them set SWISHBIN
    return unless $binary;

    die "Program '$binary' does not exist: $!" unless -e $binary;
    die "Program '$binary' cannot be executed: $!" unless -e $binary;
    die "'$binary' is a directory" if -d $binary;

    return $binary;
}


sub find_program_in_path {
    my $name = shift;

     for my $dir ( split /$Config{path_sep}/, $ENV{PATH} ) {
        my $path = File::Spec->catfile( $dir, $name );

        for my $extension ( '', '.exe' ) {
            my $file = $path . $extension;
            return $file  if -x $file && !-d _;
        }
    }
    return;
}


sub get_version {

      my $e = shift;
      return unless $e && -x $e && $e =~ m!^/!;
      my $version = `$e -V`;
      $version =~ tr/[0-9].//cd;

      die "Failed to get Swish-e version from '$e -V'"
        unless $version;

      return $version;
}

sub test_version {
    my ( $swish, $test_ver ) = @_;

    my $version = get_version($swish);
    print "Your SWISH-E version: $version\n";

    die "Version is not newer than '$test_ver'" unless version_newer( $test_ver, $version);
}

# Note that .01 == .1 -- is that correct?

sub version_newer {
    my ( $test_ver, $check_ver ) = @_;

    my @test_ver = split /\./, $test_ver;
    my @check_ver = split /\./, $check_ver;

    die "mismatched version formats '$test_ver' and '$check_ver'"
        unless @test_ver == @check_ver;

    while ( (my $test = shift @test_ver) && (my $check = shift @check_ver ) ) {
        next if $test == $check;
        return $check > $test;
    }

    return 1;  # same version
}


sub create_index {
    my ($swish, $version) = @_;

    my $index = 't/index.swish-e';
    my $conf = 't/test.conf';


    unlink $index if -e $index;

    my @command = ( $swish,  '-c', $conf, '-f', $index, '-v','0' );


    print "Creating index...'@command'\n\n";

    system(@command);

    die "Failed to create index file '$index'" unless -r $index;
}
