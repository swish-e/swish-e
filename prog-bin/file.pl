#!/usr/bin/perl -w
use strict;

# This is a short example that basically does the same
# thing as the default file system access method

# This will index only one directory (the current directory)
# See the more advanced example DirTree.pl in the swish-e prog-bin directory.


my $dir = '.';
opendir D, $dir or die $!;
while ( $_ = readdir D   ) {
    next unless /\.htm$/;
    my ( $size, $mtime )  = (stat "$dir/$_" )[7,9];
    open FH, "$dir/$_" or die "$! on $dir/$_";

    print <<EOF;
Content-Length: $size
Last-Mtime: $mtime
Path-Name: $dir/$_

EOF
    print <FH>;
}



    
