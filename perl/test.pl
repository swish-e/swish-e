#!/usr/local/bin/perl -w

    use strict;

=pod
    Test script for the SWISHE library
    please see perldoc README-PERL for more information
    $Id$
=cut

    # Import symbols
    use SWISHE;


    # In this test we will use the same index twice
    
    my $indexfilename1 = '../tests/test.index';
    my $indexfilename2 = $indexfilename1;

    die "Index file '$indexfilename1' not found!  Did you run make test?\n"
        unless -e $indexfilename1;

    my $indexfiles = $indexfilename1;        


    # To search for several indexes just put them together
    #my $indexfiles = "$indexfilename1 $indexfilename2";


    # Open the index files
    
    my $handle = SwishOpen( $indexfiles )
        or die "Failed to open '$indexfiles'";


    # Get a few headers from the index files

    my @headers = qw/WordCharacters BeginCharacters EndCharacters/;
    push @headers, 'Indexed on';
  
    for ( @headers ) {
        print_header("Header '$_'");

        my @h =  SwishHeaderParameter( $handle, $_ );
        print "$_ for index 0 is $h[0]\n";
    }


    # Define a few searches

    my @searches = (
        {
            title   => 'Normal search',
            query   => 'test',
            props   => '',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'MetaTag search 1',
            query   => 'meta1=metatest1',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'MetaTag search 2',
            query   => 'meta2=metatest2',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'XML Search',
            query   => 'meta3=metatest3',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'Phrase Search',
            query   => 'meta3="three little pigs"',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'Advanced search',
            query   => 'test or meta1=m* or meta2=m* or meta3=m*',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
    );

    my @settings = qw/query context props sort/;

    print_header("*** Now searching ****");
    print "Note that some META names have embedded newlines.\n";


    for my $search ( @searches ) {
        print_header( "$search->{title} - Query: '$search->{query}'" );
              
        my $num_results = SwishSearch( $handle, @{$search}{ @settings } );

        print "# Number of results = $num_results\n\n";

        unless ( $num_results ) {
            print "No Results\n";

            my $error = SwishError( $handle );
            print "Error number: $error\n" if $error;

            next;
        }

        while( my($rank,$index,$file,$title,$summary,$start,$size,@props) = SwishNext( $handle )) {
            print join( ' ',
                  $rank,
                  $index,
                  $file,
                  qq["$title"],
                  qq["$summary"],
                  $start,
                  $size,
                  map{ qq["$_"] } @props,
                  ),"\n";
        }
    }

    print_header('Other Functions');



    my @stemwords = qw/parking libaries library librarians money monies running runs is/;
    print "\nStemming:\n";
    print "    '$_' => '" . SwishStem( $_ ) . "'\n" for @stemwords;
    print "\n";

    my @stopwords = SwishStopWords( $handle, $indexfilename1 );
    print 'Stopwords: ',
          ( @stopwords ? join(', ', @stopwords) : '** None **' ),
          "\n\n";


    my $letter = 't';
    my @keywords = SwishWords( $handle, $indexfilename1, $letter);

    print "List of keywords that start with the letter '$letter':\n",
          join("\n", @keywords),
          "\n\n";



    SwishClose( $handle );

sub print_header {
    print "\n", '-' x length( $_[0] ),"\n",
          $_[0],
          "\n", '-' x length( $_[0] ),"\n";
}


