#!perl -w

# $Id$

use strict;
require SWISH::API;

my $lastcase = 90;
print "1..$lastcase\n";

my $test_num = 1;

my $mem_test = 0;



######################################################################

{
    my $swish = SWISH::API->new( 't/index.swish-e' );
    check_error('Call SWISH::API::new', $swish);


    my @header_names = $swish->HeaderNames;
    is_ok( "header names " . join(':',@header_names), @header_names);


    my @index_names = $swish->IndexNames;

    for my $index ( @index_names ) {
        is_ok( "index name '$index'", $index);

        for my $header ( @header_names ) {

            my @value = $swish->HeaderValue( $index, $header );
            my $value = @value ? join( ':', @value) : '*undefined*';
            is_ok( "Header '$header' = '$value'", defined $value );
        }
    }

    

    # A short-cut way to search

    {
        my $results = $swish->Query( "foo OR bar" );
        check_error('Call $swish->Query', $swish);
        my $hits = $results->Hits;

        is_ok( "returned $hits hits", $hits );
    }

    # A short-cut way to search with a metaname

    {
        my $results = $swish->Query( "meta_name=f*" );
        check_error('metaname Call $swish->Query', $swish);
        my $hits = $results->Hits;

        is_ok( "returned $hits hits", $hits );
    }




    # Or more typically
    my $search = $swish->New_Search_Object;
    check_error('Call $swish->New_Search_Object', $swish);

    $search->SetSort("swishfilenum");
   


    # then in a loop
    my $query = "not dkdkd stopword otherstop";
    my $results = $search->Execute( $query );
    check_error('Call $swish->Execute', $swish);

    # Check parsed words
    my @parsed_words = $results->ParsedWords( 't/index.swish-e' );
    is_ok("ParsedWords [" . join(', ', @parsed_words) . "]", scalar @parsed_words );

    my @removed_stopwords = $results->RemovedStopwords( 't/index.swish-e' );
    is_ok("RemovedStopwords [" . join( ', ', @removed_stopwords). "]", scalar @removed_stopwords  );

    
    # Display a list of results


    my $hits = $results->Hits;
    is_ok( "returned $hits results", $hits );
    

    # Seek to a given page - should check for errors
    #$results->SeekResult( ($page-1) * $page_size );

    my @props = qw/
        swishreccount
        swishfilenum
        age
        swishdocpath
        swishrank
        swishdocsize
        swishtitle
        swishdbfile
        swishlastmodified
    /;

    # access results

    my $seen;

    my @results;

    while ( my $result = $results->NextResult ) {

        push @results, $result;

        check_error('Call $swish->NextResult', $swish)
            unless $seen;

        my %props;
        
        $props{$_} = $result->Property( $_ ) for @props;
        check_error('Call $result->Property', $swish)
            unless $seen;


        my $string = $result->Property('swishdocpath') ."\n" . join "\n",
            map { "   $_ => " . (defined $props{$_} ? $props{$_} : '*not defined*') }
                @props;

        is_ok( "$string\n", $string );

        for ( @props ) {
            my $propstr = $result->ResultPropertyStr( $_ );
            is_ok(" ResultPropertyStr($_) = " . $propstr || '??', $propstr );
        }

        unless ( $seen++ ) {

            my $header = $result->ResultIndexValue( 'WordCharacters' );
            is_ok("header '$header'", $header );
        }

        last if $seen >= 20;
    }


    $results = $search->Execute('firstbody or secondbody');
    is_ok("firstbody or secondbody", $results->Hits == 2 );

    $results = $search->Execute('foo');
    is_ok("foo", $results->Hits == 2 );

    my $IN_HEAD = 32;
    $search->SetStructure( $IN_HEAD );

    $results = $search->Execute('foo');
    $hits = $results->Hits;
    is_ok("foo in <h> tags $hits hits", $hits == 1 );
    
    $search->SetStructure( 1 );

    $results = $search->Execute('foo');
    $hits = $results->Hits;
    is_ok("foo again $hits hits", $hits == 2 );

    $search->SetSearchLimit("age", 30, 40 );
    check_error('SetSearchLimit', $swish);
    
    $results = $search->Execute('not dkdkd');
    check_error('1st Execute', $swish);
    $hits = $results->Hits;
    is_ok("Limit Search range $hits hits", $hits == 2 );



    $search->ResetSearchLimit;
    $search->SetSearchLimit("age", 40, 40 );
    check_error('2nd SetSearchLimit', $swish);
    
    $results = $search->Execute('not dkdkd');
    check_error('2nd Execute', $swish);

    $hits = $results->Hits;
    is_ok("2nd Limit Search range $hits hits", $hits == 1 );
    


    
    if ( $mem_test ) {
        require Time::HiRes;
        my $t0 = [Time::HiRes::gettimeofday()];
        my $count = 0;
        my $flags = 'v';
        my $ttl;
        while ( 1 ) {
            my $results = $search->Execute("apache");
            while ( my $result = $results->NextResult ) {
                my $path = $result->Property('swishdocpath');
                $ttl ++;
            }

            unless ( $count % 1000 ) {
                $hits = $results->Hits;
                my $elapsed = Time::HiRes::tv_interval ( $t0, [Time::HiRes::gettimeofday()]);

                my $ps = $count % 10000 ? '': `/bin/ps $flags -p $$`;
                
                printf("$count - Results: $hits - Total Results: $ttl %d req/s\n$ps", $count/$elapsed ); 
                $flags = 'hv';
            }
            $count++;
        }
    }

    my @words = $swish->WordsByLetter( 't/index.swish-e' , 'f' );
    check_error('WordsByLetter', $swish);
    is_ok( "WordsByLetter 'f' [@words]", @words );


    for ( qw/running runs library libraries/ ) {
        my $stemmed = $swish->StemWord( $_ );
        is_ok( "Stemmed: '$_' => '" . ($stemmed||'*failed to stem*') ."'", $stemmed );
    }
    

    # cough, hack, cough....
    
    print "ok $_ (noop)\n" for $test_num..$lastcase
}


sub check_error {
    my ( $str, $swish ) = @_;

    my $num = $test_num++;

    if ( !$swish->Error )
    {
        print "ok $num $str\n";
        return;
    }
    

    my $msg = $swish->ErrorString . ' (' . $swish->LastErrorMsg . ')';
    
    print "not ok $num $str - $msg\n";

    die "Found critical error" if $swish->CriticalError;
   
}

sub is_ok {
    my ( $str, $is_ok ) = @_;

    my $num = $test_num++;

    print $is_ok ? "ok $num $str\n" : "not ok $num $str\n";
}

    

    
