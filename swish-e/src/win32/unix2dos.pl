#!/usr/local/bin/perl

# Convert from unix 2 dos.
# add ^M after the ^J

local($infile,$outfile);

if ($#ARGV != 0) {
  print "Usage: $0 [file] \n";
  exit;
}

($infile = $ARGV[0]) if (defined $ARGV[0]);
($outfile = $ARGV[0].".temp") if (defined $ARGV[0].".temp");

open(INFILE, "<$infile") || die "Can't open input file $infile";
open(OUTFILE, ">$outfile") || die "Can't open input file $outfile";

while ($data=<INFILE>) {
  $data =~ s/\n/\r\n/;
  print OUTFILE $data;
}

close(INFILE);
close(OUTFILE);

rename $outfile,$infile;

