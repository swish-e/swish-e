use ExtUtils::MakeMaker;

# $Id$

set_swish_version();

WriteMakefile(
    'NAME'          => 'SWISH::API',
    'VERSION_FROM'  => 'API.pm',
    'LIBS'          => ['-lswish-e'],
    'NORECURS'      => 1,          # keep it from recursing into subdirectories
    'DIR'           => [],
    'XSPROTOARG'    => '-noprototypes',
);

sub get_version
{

      my $e = shift;
      return 0 if ! $e;
      my $version = `$e -V`;
      $version =~ tr/[0-9].//cd;
      return $version;
}

sub set_swish_version {
    $|=1;

   my $swish = $ENV{SWISHBIN} || '';
   my $version = get_version($swish) || '';

    print "\n\nFor testing we will need the path to your SWISH-E binary.\n",
          qq[** Enter "skip" (without quotes) to skip testing - you will not be able to run "make test"\n\n];
          
    print "\nThe SWISHBIN environment variable may also be used for non-interactive builds.\n\n";

    for ( split /:/, $ENV{PATH} ) {
        if ( -x "$_/swish-e" && !-d _ ) {
            $swish = "$_/swish-e";
            last;
        }
    }

    while ( ! $version ) {
        my $enter = $swish;

        print "Enter the location of the swish-e executable[$enter]: ";

        $enter = <STDIN>;
        chomp $enter;

        if ( $enter =~/skip/i ) {
            print "\nNo swish-e binary selected -- not creating test index\n",
                  "You will not be able to run make test\n\n";
            return;
        }

        $enter = $enter || $swish;

        $enter =~ s/^(~|\$HOME)/($ENV{HOME}||'')/e;
        
        if ( $enter && !-x $enter ) {
            print "File '$enter' missing or not exectuable\n";

        } else {
            $version = get_version($swish);
       
            
            if ( $version ) {
                print "Your SWISH-E version: $version\n";
                $swish = $enter;
                last;
            } else {
                print "Failed to parse version from '$enter'\n";
            }
        }
            
    }

    create_index( $swish, $version );
}    

sub create_index {
    my ($swish, $version) = @_;

    my $index = 't/index.swish-e';
    my $conf = 't/test.conf';
    

    unlink $index if -e $index;

    my @command = ( $swish,  '-c', $conf, '-f', $index, '-v','0' );
        

    print "Creating index...'@command'\n\n";
    
    system(@command);

    die "Failed to create index file" unless -r $index;
}
