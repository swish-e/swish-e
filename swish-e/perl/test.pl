#!/usr/local/bin/perl -w

    use strict;

=pod
    Test script for the SWISHE library

    please see perldoc README-PERL for more information
    $Id$
=cut

    # Import symbols from the SWISHE.pm module
    use SWISHE;


    # In this test we will use the same index twice
    # The results will seem odd since normally you would use
    # two different index files, but it demonstrates
    # how to process two index files.
    
    my $indexfilename1 = '../tests/test.index';
    my $indexfilename2 = $indexfilename1;

    die "Index file '$indexfilename1' not found!  Did you run make test from the top level directory?\n"
        unless -e $indexfilename1;

    my $indexfiles = $indexfilename1;        


    # To search for several indexes just put them together
    # Uncomment to test/demonstrate the use of two index files
    
    #my $indexfiles = "$indexfilename1 $indexfilename2";


    # First, open the index files. 
    # This reads in headers and prepares the index for searching
    # You can run more than one query once the index is opened.

    
    my $handle = SwishOpen( $indexfiles )
        or die "Failed to SwishOpen '$indexfiles'";


    # Get a few headers from the index files for display

    my @headers = ( qw/WordCharacters BeginCharacters EndCharacters IndexedOn FileCount/ );
  
    for my $header ( @headers ) {
        print_header("Header '$header'");

        my @h =  SwishHeaderParameter( $handle, $header );

        print "$header for index $_ is $h[$_]\n" for 0..$#h;
    }


    # Now, let's run a few queries...

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
            query   => '"three little pigs"',
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
        {
            title   => 'Advanced search',
            query   => 'test or meta1=m* or meta2=m* or meta3=m*',
            props   => 'meta1 meta2 meta3',
            sort    => '',
            context => 1,   # Search the entire file
        },
        {
            title   => 'Limit to title',
            query   => 'test or meta1=m* or meta2=m* or meta3=m*',
            props   => 'meta1 meta2 meta3 swishrank swishdocpath swishlastmodified',
            sort    => '',
            context => 1,   # Search the entire file
            limit   => [ 'swishtitle', '<=', 'If you are seeing this, the test' ],
        },
        {
            title   => 'Limit to title - second test with same query',
            query   => 'test or meta1=m* or meta2=m* or meta3=m*',
            props   => 'meta1 meta2 meta3 swishrank swishdocpath swishlastmodified',
            sort    => '',
            context => 1,   # Search the entire file
            limit   => [ 'swishtitle', '<=', 'If you are seeing this, the test' ],
        },
    );

    # Need an array in perl to deliver the above hash contents to swish in
    # the correct order
    my @settings = qw/query context props sort/;



    print_header("*** Now searching ****");
    print "Note that some META names have embedded newlines.\n";


    # Use an array for a hash slice when reading results.  See SwishNext below.
    my @labels = qw/
        rank
        file_name
        title
        content_length
    /;


    for my $search ( @searches ) {
        print_header( "$search->{title} - Query: '$search->{query}'" );


        # Since we *might* use SetLimitParameter, make sure it's reset first
        ClearLimitParameter( $handle );
        
        if ( $search->{limit} ) {
            print "limiting to @{$search->{limit}}\n\n";
            SetLimitParameter( $handle, ,@{$search->{limit}});
        }

        # Here's the actual query
        
        my $num_results = SwishSearch( $handle, @{$search}{ @settings } );

        print "# Number of results = $num_results\n\n";

        if ( $num_results <= 0 ) {
            print ($num_results ? SwishErrorString( $num_results ) : 'No Results');

            my $error = SwishError( $handle );
            print "\nError number: $error\n" if $error;

            next;
        }

        my %result;
        my @properties = split /\s+/, $search->{props};
        my %props;

        while ( ( @result{ @labels }, @props{@properties} ) = SwishNext( $handle )) {

            for ( @labels ) {
                printf("  %20s -> '%s'\n", $_ ,$result{$_});
            }
            for ( @properties ) {
                printf("  %20s:(%-20s) -> '%s'\n", 'Property', $_, $props{$_} || '<blank>' );
            }
            print "-----------\n";
        }
    }

    print_header('Other Functions');


    # Now, demonstrate the use of SwishStem to find the stem of a word.

    my @stemwords = qw/parking libaries library librarians money monies running runs is/;
    print "\nStemming:\n";
    print "    '$_' => '" . ( SwishStem( $_ ) || 'returned undefined: Word not stemmed for some reason' ) . "'\n" for @stemwords;
    print "\n";


    
    # Grab the stop words from the header

    my @stopwords = SwishStopWords( $handle, $indexfilename1 );
    print 'Stopwords: ',
          ( @stopwords ? join(', ', @stopwords) : '** None **' ),
          "\n\n";


    # Let's see what words in the index begin with the letter "t".

    my $letter = 't';
    my @keywords = SwishWords( $handle, $indexfilename1, $letter);

    print "List of keywords that start with the letter '$letter':\n",
          join("\n", @keywords),
          "\n\n";



    # Free the memory.

    SwishClose( $handle );

    # If swish was built with memory debugging this will dump extra info
    SWISHE::MemSummary();


sub print_header {
    print "\n", '-' x length( $_[0] ),"\n",
          $_[0],
          "\n", '-' x length( $_[0] ),"\n";
}


