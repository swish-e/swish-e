use File::Find;
use File::Basename;
use Win32::Registry;

# grab the install dir from registry
my ($regkey, $type, $instdir);
$::HKEY_LOCAL_MACHINE->Open("SOFTWARE\\SWISH-E Team\\SWISH-E", $regkey)
    or die "Can't open SWISH-E registry key: $^E";
$regkey->QueryValueEx("", $type, $instdir) or die "No SWISH-E install directory! $^E \n";
print "SWISH-E installed to $instdir\n";

# recurse like the wind
find( {wanted => \&wanted }, $instdir );

# process found files
sub wanted{ 
    # banish errors to the Land of Wind and Ghosts
    return if -d;
    return if !-r;

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

