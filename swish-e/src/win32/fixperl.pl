use File::Find;
use File::Basename;
use Win32::TieRegistry 0.20 ( KEY_READ );

# Gloabal registry access options
my $registry_options = {
    Delimiter => '/',
    Access    => KEY_READ(),
};

# First, let's get location of ActivePerl
my $active_key = "LMachine/Software/ActiveState/ActivePerl";
my $perl_binary = get_perl_binary($active_key);



# Our Registry keys that we need
my $install_key = "LMachine/Software/SWISH-E Team/SWISH-E";
my @config_options = qw/installdir mylibexecdir libexecdir perlmoduledir pkgdatadir swishbinary /;

# Fetch data from registry
my $config = get_registry_data( $install_key, @config_options );

die "Failed to read the registry [$install_key].  Cannot continue\n"
    unless ref $config;

for ( @config_options ) {
   die "Failed to read registry [$install_key/$_]\n"
       unless $config->{$_};
}

# Add "perlbinary" into the config hash
$config->{perlbinary} = $perl_binary;
push @config_options, 'perlbinary';

# Now look for .in files to update at install time
find( {wanted => \&wanted }, $config->{installdir} );

sub wanted{ 
    return if -d;
    return if !-r;
    return unless /\.in$/;
    

	my $filename = $_;
    $basename = basename($filename, qw{.in});
    # open files
    open ( INF, "<$filename" ) or die "Failed to open [$filename] for reading:$!";
    open ( OUTF, ">$basename") or die "Failed to open [$basename] for output: $!";

    my $count;
    while ( <INF> ) {
        for my $setting ( @config_options ) {
            $count += s/qw\( \@\@$setting\@\@ \)/'$config->{$setting}'/g;
            $count += s/\@\@$setting\@\@/$config->{$setting}/g;
        }
        print OUTF;
    }
    close INF;

    printf("%20s --> %20s (%3d changes )\n", $filename, $basename, $count);



    # normal people will see this.  let's not scare them.
    unlink $filename || warn "Failed to unlink '$filename':$!\n";
}


# This fetches data from a registry entry based on a CurrentVersion lookup.

sub get_registry_data {
    my ( $top_level_key, @params ) = @_;

    my %data;

    my $key = Win32::TieRegistry->new( $top_level_key, $registry_options );
    unless ( $key ) {
        warn "Can't access registry key [$top_level_key]: $^E\n";
        return;
    }

    my $cur_version = $key->GetValue("CurrentVersion");
    unless ( $cur_version ) {
        warn "Failed to get current version from registry [$top_level_key]\n";
        return;
    }

    $data{CurrentVersion} = $cur_version;

    my $cur_key = $key->Open($cur_version);
    unless ( $cur_key ) {
        warn "Failed to find registry entry [$key\\$cur_version]\n";
        return;
    }

    # Load registry entries
    $data{$_} = $cur_key->GetValue($_) for @params;

    return \%data;
}

sub get_perl_binary {
    my $key = shift;

    # Get "(default)" key for install directory
    my $reg = get_registry_data( $key, "" );

    return unless ref $reg && $reg->{""};

    $perl_build = $reg->{CurrentVersion};

    my $perl_binary = $reg->{""} . ( $reg->{""} =~ /\\$/ ? 'bin\perl.exe' : '\bin\perl.exe');

    if ( -x $perl_binary ) {
        warn "Found Perl at: $perl_binary (build $perl_build)\n";
        return $perl_binary;
    }

    warn "Failed to find perl binary [$perl_binary]\n";
    return;
}

