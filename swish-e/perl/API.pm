package SWISH::API;

# $Id$

require DynaLoader;

use vars qw/  @ISA $VERSION /;
@ISA = 'DynaLoader';

$VERSION = '0.01';

bootstrap SWISH::API $VERSION;

# Preloaded methods go here.

1;
__END__

=head1 NAME

SWISH::API - Perl interface to the Swish-e C Library

=head1 SYNOPSIS

    use SWISH::API;

    my $swish = SWISH::API->new( 'index.swish-e' );

    $swish->AbortLastError
        if $swish->Error;


    # A short-cut way to search
    
    my $results = $swish->Query( "foo OR bar" );


    # Or more typically
    my $search = $swish->New_Search_Object;

    # then in a loop 
    my $results = $search->Execute( $query );

    # always check for errors (but aborting is not always necessary)

    $swish->AbortLastError
        if $swish->Error;


    # Display a list of results


    my $hits = $results->Hits;
    if ( !$hits ) {
        print "No Results\n";
        return;  /* for example *.
    }

    print "Found ", $results->Hits, " hits\n";


    # Seek to a given page - should check for errors
    $results->SeekResult( ($page-1) * $page_size );

    while ( my $result = $results->NextResult ) {
        printf("Path: %s\n  Rank: %lu\n  Size: %lu\n  Title: %s\n  Index: %s\n  Modified: %s\n  Record #: %lu\n  File   #: %lu\n\n",
            $result->Property( "swishdocpath" ),
            $result->Property( "swishrank" ),
            $result->Property( "swishdocsize" ),
            $result->Property( "swishtitle" ),
            $result->Property( "swishdbfile" ),
            $result->Property( "swishlastmodified" ),
            $result->Property( "swishreccount" ),
            $result->Property( "swishfilenum" )
        );
    }


=head1 DESCRIPTION

This module provides a Perl interface to the Swish-e search engine.
This module allows embedding the swish-e search code into your application
avoiding the need to fork to run the swish-e binary and to keep an index file
open when running multiple queries.  This results in increased search performance.

=head1 DEPENDENCIES

You must have installed Swish-e version 2.4 before building this module.
Download from:

    http://swish-e.org

=head1 OVERVIEW

This module includes a number of classes.

Searching consists of connecting to a swish-e index (or indexes), and then running queries
against the open index.  Connecting to the index creates a swish object blessed into
the SWISH::API class.

A SWISH::API::Search object is created from the SWISH::API object.
The SWISH::API::Search object can have associated parameters (e.g. result sort order).

The SWISH::API::Search object is used to query the associated index file or files.
A query on a search object returns a results object of the class SWISH::API::Results.
Then individual results of the SWISH::API::Result class can be fetched by calling a
method of the results object.

Finally, a result's properties can be accessed by calling methods on the result object.


=head1 METHODS

=head2 SWISH::API - Swish Handle Object

To begin using Swish you must first create a Swish Handle object.  This
object makes the connection to one or more index files and is used to
create objects used for searching the associated index files.

=over 4

=item $swish = SWISH::API->new( $index_files );

This method returns a swish handle object blessed into the SWISH::API class.
$index_files is a space separated list of index files to open.
This always returns an object, even on errors.
Caller must check for errors (see below).

=item @indexes = $swish->IndexNames;

Returns a list of index names associated with the swish handle.
These were the indexes specified as a parameter on the SWISH::API->new call.
This can be used in calls below that require specifying the index file name.

=item @header_names = $swish->HeaderNames;

Returns a list of possible header names.  These can be used to lookup
header values.  See C<SwishHeaderValue> method below.

=item @values = $swish->HeaderValue( $index_file, $header_name );

A swish-e index has data associated with it stored in the index header.  This method
provides access to that data.    

Returns the header value for the header and index file specified.  Most headers
are a single item, but some headers (e.g. "Stopwords") return a list.

The list of possible header names can be obtained from the SwishHeaderNames method.

=back

=head3 Error Handling

All errors are stored in and accessed via the SWISH::API object (the Swish Handle).
That is, even an error that occurs when calling a method on a result
(SWISH::API::Result) object will store the error in the parent SWISH:API object.

Check for errors after every method call.  Some errors are critical errors and will require
destruction of the SWISH::API object.  Critical errors will typically only happen when
attaching to the database and are errors such as an invalid index file name, permissions
errors, or passing invalid objects to calls.

Typically, if you receive an error when attaching to an index file or files you should assume
that the error is critical and let the swish object fall out of scope (and destroyed).  Otherwise,
if an error is detected you should check if it is a critical error.
If the error is not critical you may
continue using the objects that have been created (for example, an invalid meta name will
generate a non-critical error, so you may continue searching using the same search object).

Error state is cleared upon a new query.

Again, all error methods need to be called on the parent swish object

=over 4

=item $swish->Error

Returns true if an error occurred on the last operation.  On errors the value returned
is the internal Swish-e error number (which is less than zero).

=item $swish->CriticalError

Returns true if the last error was a critical error

=item $swish->AbortLastError

Aborts the running program and prints an error message to STDERR.

=item $str = $swish->ErrorString

Returns the string description of the current error (based on the value
returned by $swish->Error).  This is a generic error string.

=item $msg = $swish->LastErrorMsg

Returns a string with specific information about the last error, if any.
For example, if a query of:

    badmeta=foo

and "badmeta" is an invalid metaname $swish->ErrorString
might return "Unknown metaname", but $swish->LastErrorMsg might return "badmeta".


=back

=head3 Generating Search and Result Objects

=over 4

=item $search = $swish->New_Search_Object( $query );

This creates a new search object blessed into the SWISH::API::Search class.  The optional
$query parameter is a query string to store in the search object.

See the section on C<SWISH::API::Search> for methods available on the returned object.

The advantage of this method is that a search object can be used for multiple queries:

    $search = $swish->New_Search_Objet;
    while ( $query = next_query() ) {
        $results = $search->Execute( $query );
        ...
    }

=item $results = $swish->Query( $query );

This is a short-cut which avoids the step of creating a separate search object.
It returns a results object blessed into the SWISH::API::Results class described below.

This method basically is the equivalent of

    $results = $swish->New_Search_Object->Execute( $query );


=back

=head2 SWISH::API::Search - Search Objects

A search object holds the parameters used to generate a list of results.  These methods
are used to adjust these parameters and to create the list of results for the current
set of search parameters.

=over 4

=item $search->SetQuery( $query );

This will set (or replace) the query string associated with a search object.
This method is typically not used as the query can be set when executing the
actual query or when creating a search object.

=item $search->SetStructure( $structure_bits );

This method may change in the future.

A "structure" is a bit-mapped flag used to limit search results to specific parts
of an HTML document, such as the title or in H tags.  The possible bits are:

    IN_FILE         = 1      This is the default
    IN_TITLE        = 2      In <title> tag
    IN_HEAD         = 4      In <head> tag
    IN_BODY         = 8      In <body>
    IN_COMMENTS     = 16     In html comments
    IN_HEADER       = 32     In <h*> 
    IN_EMPHASIZED   = 64     In <em>, <b>, <strong>, <i>
    IN_META         = 128    In a meta tag (e.g. not swishdefault)

So if you wish to limit your searches to words in heading tags (e.g. E<lt>H1E<gt>)
or in the E<lt>titleE<gt> tag use:

    $search->SetStructure( IN_HEAD | IN_TITLE );
    

=item $search->PhraseDelimiter( $char );

Sets the character used as the phrase delimiter in searches.  The default
is double-quotes (").

=item $search->SetSearchLimit( $property, $low, $high );

Sets a range from $low to $high inclusive that the give $property must be in
to be selected as a result.  Call multiple times to set more than one limit
on different properties.
Limits are ANDed, that is, a result must be within the range of all limits
specified to be included in a list of results.

For example to limit searches to documents modified in the last 48 hours:

    my $start = time - 48 * 60 * 60;
    $search->SetSearchLimit( 'swishlastmodified', $start, $time );

An error will be set if the property 
has already been specified or if $high > $low.

Other errors may not be reported until running the query, such as
the property name is invalid or if $low or $high are not
numeric and the property specified is a numeric property.

Once a query is run
you cannot change the limit settings without calling the ResetSearchLimit
method first.

=item $search->ResetSearchLimit;

Clears the limit parameters for the given object.  This must be called if
the limit parameters need to be changed.

=item $search->SetSort( $sort_string );

Sets the sort order of search results.  The string is a space separated
list of valid document properties.  Each property may contain a qualifier
that sets the direction of the sort.

For example, to sort the results by path name in ascending order and by rank in
descending order:

    $search->SetSort( 'swishdocpath asc swishrank desc' );

The "asc" and "desc" qualifiers are optional, and if omitted ascending is assumed.

Currently, errors (e.g invalid property name) are not detected on this call, but rather when
executing a query.  This may change in the future.

=back

=head2 SWISH::API::Results - Generating and accessing results

Searching generates a results object blessed into the SWISH::API::Results class.

=over 4

=item $results = $search->Execute( $query );

Executes a query based on the parameters in the search object.
$query is an optional query string to use for the search ($query replaces
the set query string in the search object).

A typical use would be to create a search object once and then call this method
for each query using the same search object changing only the passed in $query.

The caller should check for errors after making this all.


=back

=head2 Results Methods

A query creates a results object that contains information about the query
(e.g. number of hits) and access to the individual results.

=over 4

=item $hits = $results->Hits;

Returns the number of results for the query.  If zero and no errors were reported
after calling $search->Execute then the query returned zero results.

=item @parsed_words = $results->ParsedWords( $index_name );

Returns an array of tokenized words and operators with stopwords removed.
This is the array of tokens used by swish for the query.

$index_name must match one of the index files specified on the creation of
the swish object (via the SWISH::API->new call).

The parsed words are useful for highlighting search terms in associated documents.

=item @removed_stopwords = $results->RemovedStopwords( $index_name) ;

Returns an array of stopwords removed from a query, if any, for the index
specified.

$index_name must match one of the index files specified on the creation of
the swish object (via the SWISH::API->new call).

=item $results->SeekResult( $position );

Seeks to the position specified in the result list.  Zero is the first position
and $results->Hits-1 is the last position.  Seeking past the end of results
sets a non-critical error condition.

Useful for seeking to a specific "page" of results.

=item $result = $results->NextResult;

Fetches the next result from the list of results.  Returns undef if no
more results are available.  $result is an object blessed into the
SWISH::API::Result class.

=back

=head2 SWISH::API::Result - Result Methods

The follow methods provide access to data related to an individual result.

=over 4

=item $prop = $result->property( $prop_name );

Fetches the property specified for the current result.

=item $value = $result->ResultIndexValue( $header_name );

Returns the header value specified.  This is similar to
$swish->HeaderValue(), but the index file is not specified
(it is determined by the result).

=back

=head2 Utility Methods

=over 4

=item $stemmed_word = $swish->Stem( $word );

Returns the stemmed version of the passed in word.

=item @parsed_words = $swish->SwishWords( $string, $index_file );

* Not implemented *

Splits up the input string into tokens of swish words and operators.


=head1 NOTES

Perl's garbage collection makes it easy to write code for searching with Swish-e,
but care must be taken not to keep objects around too long which can use up memory.

Here's an example of a potential problem.  Say you have a very large number
of documents indexed and you want to find the first hit for a number of 
popular keywords (error checking omitted in this bad example):

    sub first_hit {
      my $query = shift;
      my $handle = SWISH::API->new( 'index.swish-e');
      my $results = $handle->Query( $query );
      my $first_hit = $results->NextResult;
      return $first_hit;
    }

    my @first_hit_list;
    for ( @keywords )
        push @first_hit_list, $first_hit($_);
    }

The first_hit() subroutine is returning a SWISH::Result object.  That makes
it easy to access properties:

   # print file names
   for my $result ( @first_hit_list ) {
      print $result->Property('swishdocpath'),"\n";
   }

But as long as a SWISH::API::Result object is around, so is the entire list
of results generated by the $handle->Query() call, and the index file is
still open (because a SWISH::API::Result depends on a SWISH::API::Results object, which
depends on a SWISH::API object).

In this case it would be better to return from first_hit() just the
properties you need:

      ...
      my $first_hit = $results->NextResult;
      return $first_hit->Property('swishdocpath');
   }

Then when first_hit() sub ends the result list will be freed, and the 
index file closed, thanks to Perl's reference count tracking.

Note: the other problem with the above code is that the same index file is
opened for each call to the function.  Don't do that, instead open the
index file once.


=head1 COPYRIGHT

This library is free software; you can redistribute it
and/or modify it under the same terms as Perl itself.


=head1 AUTHOR

Bill Moseley moseley@hank.org. 2002/2003

=head1 SUPPORT

Please contact the Swish-e discussion email list for support with this module
or with Swish-e.  Please do not contact the developers directly.


=cut

