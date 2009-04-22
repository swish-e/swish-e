#!/usr/bin/perl -w 
#
## do-rewrite : automates development of converting swish-e to 64 bit safe.
#
#  allows you to add globs #  of files to be altered (see @globs below) 
#  and test the results of builds thereafter.
#  
#  My tactic is to start with swish.[ch] and work down in the code,
#  fixing warnings as I go.

use strict;
use Getopt::Long; 
use File::Basename;

my $prog = basename($0);
my $verbose;
my $dry;
my $backup;
my $refresh;

# Usage() : returns usage information
sub Usage {
    "$prog [--verbose] [--dry] [--backup] [--refresh]\n";
}

# call main()
main();

# main()
sub main {
    GetOptions(
        "verbose!" => \$verbose,
        "refresh!" => \$refresh,
        "dry!" => \$dry,
        "backup!" => \$backup,
    ) or die Usage();

    # just replacing these four globs works. but obiously doesn't make portable swish-e
    my @globs = qw ( src/swish.[ch] src/swish2.[ch] src/rank.[ch] src/db_read.[ch] );

    my $rewrite = "./rewrite-swish-src.pl";
    # --refresh means get the code anew. So we get rewrite... to refetch the code but not alter it for 64bit
    if ($refresh) {
        mysystem( "$rewrite -refresh -no-alter" );
    }
    
    if ($backup) {
        #mysystem( "cd ..; tar -zcf swish-e-portable.`date +'%Y%m%d-%H%M%S'`.tar.gz swish-e2.4" );
        mysystem( "cd ..; cp -rp swish-e2.4 swish-e-portable.`date +'%Y%m%d-%H%M%S'`" );
    }
    for (@globs) {
        mysystem( "$rewrite '$_'" );
    }
}

sub mysystem {
    my ($cmd) = @_;
    print "$prog: Running $cmd\n" if $verbose;
    unless( $dry ) {
        system( $cmd ) && die "$prog: Failed: $cmd: $!\n";
    }


}

=pod

=head1 NAME

new_perl_script -- does something awesome

=head1 SYNOPSIS

The synopsis, showing one or more typical command-line usages.

=head1 DESCRIPTION

What the script does.

=head1 OPTIONS

Overall view of the options.

=over 4

=item --verbose/--noverbose

Turns on/off verbose mode. (off by default)

=back

=head1 TO DO

If you want such a section.

=head1 BUGS

None

=head1 COPYRIGHT

Copyright (c) 2009 Josh Rabinowitz, All Rights Reserved.

=head1 AUTHORS

Josh Rabinowitz

=cut    


