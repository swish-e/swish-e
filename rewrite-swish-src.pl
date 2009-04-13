#!/usr/bin/perl -w 
#
# converts swish-e source code to all 64-bit integers.
# --> in progress.
# Copyright 2007-2009 Josh Rabinowitz


use strict;
use Getopt::Long; 
use File::Basename;
use File::Slurp qw(read_file write_file);
use List::MoreUtils qw(any);

my $prog = basename($0);
my $verbose;
my $debug;
my $refresh;

# Usage() : returns usage information
sub Usage {
    "$prog [--verbose] [--debug] [--refresh]\n" . 
    "  Tries to convert source code to 64-bit friendly.\n" .
    "  Expects to be run the base SVN directory of a swish-e checkout.\n" .
    "  --refresh removes and repulls files matching src/*.[ch] before rewrite\n";
}

# call main()
main();

# main()
sub main {
    GetOptions(
        "verbose!" => \$verbose,
        "debug!" => \$debug,
        "refresh!" => \$refresh,
    ) or die Usage();
     # #include <stdint.h> has the uint... typedefs used below
     # note that 
     # 1) stdint.h needs to be included in mem.h and swish.h
     # 2) wait &status () still needs status to be a true 'int' in http
     # 3) printf() formats need to be changed to match (many!), and 
     # 4) probably several other field-size dependent codes.
     #
     # see build warnings for more.
     #
     # this (with above caveats) words on 64bit CentosOS5, 
     # but not 32bit CentosOS5, on which swish-e -h says:
     #  err: Missing switch character at 'HOSTNAME=p4.hostname.com'.
     # Use -h for options.
     #
	 
     # insert #include "swish.h" as needed.
     
     my @files = glob( "src/*.c src/*.h");
	 #my @files = glob( "src/*.c src/*.h src/*/*.c src/*/*.h src/*/*/*.h src/*/*/*.c");
     if ($refresh) {
         system( "rm -f @files" );
         system( "svn up" ); 
     }
     @files = glob( "src/*.c src/*.h");

     # alter swish.h to contain SWINT_T and SWUINT_T typedefs.
     # lines with no_rw don't have replacements done by @regexes below
     insert_at_line_unless_has_regex( 
         "src/swish.h", 325, 'typedef.*SWINT_T', 
             #qq{typedef long long SWINT_T; // no rw64 \ntypedef unsigned long long SWUINT_T; // no rw64 \n},
             qq{#define SWINT_T long long /* no rw64 */ \n#define SWUINT_T unsigned long long /* no rw64 */ \n},
     );

     # include swish.h for SWINT_T and SWUINT_T as needed.
     insert_at_line_unless_has_regex( 
         "src/stemmer.h", 35, 'swish\.h', qq{#include "swish.h"\n},
     );
	 print "$prog: DONE PATCHING\n";

     my @regexes = (
	 #   # replace everthing that looks anything like a 'long' or an 'int'
	 #    # specifically leave alone anything about chars or floats/doubles
	    q(s/ \bunsigned\s+long\s+long\s+int\b     /SWUINT_T/gx),
	    q(s/ \blong\s+long\s+unsigned\s+int\b     /SWUINT_T/gx),
	    q{s/ \bunsigned\s+long\s+int\b /SWUINT_T/gx},
	    q{s/ \bunsigned\s+long\b       /SWUINT_T/gx},
	    q(s/ \bunsigned\s+int\b        /SWUINT_T/gx),
	    q(s/ \blong\s+long\s+int\b     /SWINT_T/gx),
	    q(s/ \blong\s+long\b           /SWINT_T/gx),
	    q(s/ \blong\s+int\b            /SWINT_T/gx),
	    q(s/ \blong\b                  /SWINT_T/gx),
	    q(s/ \bint\b                   /SWINT_T/gx),
     );
     my @exceptions  = (
         # don't replace on lines matching this fileregex and lineregex
         { f=>'\.c$',          r=>'int\s+main' },	# never replace main's return val
         { f=>'src/http\.c$',  r=>'int\s+status' },	# status must be int for wait()
         { f=>"",              r=>'^\s+extern.*printf' },	# leave return codes of exern printfs alone.
     );
     for my $file (@files) {
		 #print "$file\n";
         _apply_regexes( $file, \@regexes, \@exceptions );
     }
}

#================================================================
# _apply_regexes( $file, @search_and_replace_regexes )
# backs up $file to $file.bak, and
# applies supplied regexes to the lines of a file,
sub _apply_regexes {
     my ($file, $regexes, $exceptions) = @_;
     # changes a file by applying the supplied regexes to each line
     my $tmpfile = "$file.tmp";
     open(my $rfh, "<", $file)    || die "$0: Can't open $file: $!";
     open(my $wfh, ">", $tmpfile) || die "$0: Can't open $tmpfile: $!";  # clobber old $file.tmp
     print "$prog: Applying regexes to file $file\n";
     if ($debug) {
         print join("\n", map { "regex: $_" } @$regexes) . "\n";
     }
     LINE: while(<$rfh>) {
         chomp();
         if (m{//.*no rw64} || m{/\*.*no rw64}) {   # if it has a comment with 'no rw64'
             print "$prog: Preserving line '$_' from $file\n";
             print $wfh "$_\n";
             next LINE;
         }
		 for my $e (@$exceptions) {
			 if ($file =~ m/$e->{f}/ && $_ =~ m/$e->{r}/) { # if the file and the regex match
				 print "$prog: Preserving line '$_' from $file\n";
				 print $wfh "$_\n";
				 next LINE;
			 }
		 }
         for my $r (@$regexes) {
             # $r should operate on $_ !
             eval $r;   # operates on $_, which is the current line
             die "$0: Error in regex: $r: $@" if $@;
         }
         print $wfh "$_\n";
     }
     close($rfh) || die "$0: Can't open $file: $!";
     close($wfh) || die "$0: Can't close $tmpfile: $!";
     rename( $file, "$file.bak" );
     rename( $tmpfile, $file ) || die "$0: Can't rename $tmpfile to $file: $!";
}


sub insert_at_line_unless_has_regex {
    my ($filename, $linenum, $regex, @addlines) = @_;
    die "$prog: file does not exist: $filename\n" unless -e $filename;
    die "$prog: file is empty: $filename\n" unless -s $filename;
    my @lines = read_file( $filename );
    if( any { m/$regex/ } @lines ) { 
        print "$prog: not adding line(s) to $filename, contains $regex\n" if $verbose;
        return 0;     # has regex, leave alone
    }
    print "$prog: inserting lines into $filename\n";
    splice( @lines, $linenum-1, 0, @addlines );     # otherwise insert lines...
    write_file( $filename, @lines );                # and rewrite file
}

=pod

=head1 NAME

rewrite-swish-src.pl -- tries to rewrite swish-e source to be portable

=head1 SYNOPSIS

The synopsis, showing one or more typical command-line usages.

=head1 DESCRIPTION

tries to rewrite swish-e source to be portable between 32bit and 64bit architectures.

=head1 OPTIONS

Overall view of the options.

=over 4

=item --verbose/--noverbose

Turns on/off verbose mode. (off by default)

=item --verbose/--noverbose

Turns on/off verbose mode. (off by default)

=item DOCS TO COME

TO COME - SEE ./rewrite-swish-src.pl --help

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

