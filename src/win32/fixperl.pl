use File::Find;
use File::Basename;

# we only want one directory
my $instdir = shift;

# recurse like the wind
find( {wanted => \&wanted }, $instdir );


sub wanted{ 
    # banish errors to the Land of Wind and Ghosts
    return if -d;                                                                   return if !-r;

    # we only want .in files
    if ( /\.in$/ ) {
        $basename = basename($_, qw{.in});

        # open files
        open ( INF, $_) or return;
        @line = <INF>;
        open ( OUTF, ">$basename") or return;

		# libexecdir is always $instdir\\lib\\swish-e on Win32
        foreach (@line) {
            $_ =~ s/\@\@perlmoduledir\@\@/$instdir\\lib\\swish-e/g;
            print OUTF "$_";
        }

        # normal people will see this.  let's not scare them.
        print "$File::Find::dir\\$basename\n";
    } 
}

