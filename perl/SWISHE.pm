package SWISHE;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);

# Probably shouldn't export everything 
@EXPORT = qw(
    SwishOpen
    SwishSearch
    SwishClose
    SwishNext
    SwishSeek
    SwishError
    SwishHeaderParameter
    SwishStopWords
    SwishWords
    SwishStem
    SwishErrorString
    SwishHeaders
    SetLimitParameter
);

$VERSION = '0.02';

bootstrap SWISHE $VERSION;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__

=head1 NAME

SWISH-PERL - Perl Interface to the SWISH-E Library

=head1 SYNOPSIS

    use SWISHE;

    my $indexfilename1 = '/path/to/index1.swish';
    my $indexfilename2 = '/path/to/index2.swish';

    # To search for several indexes just put them together
    $indexfiles = "$indexfilename1 $indexfilename2";

    my $handle = SwishOpen( $indexfiles )
        or die "Failed to open '$indexfiles'";

    # Get a few headers from the index files
    my @headers = qw/WordCharacters BeginCharacters EndCharacters/;
    for ( @headers ) {
        my @h = SwishHeaderParameter( $handle, $_ );
        print "$_ for index 0 is $h[0]\n",
              "$_ for index 1 is $h[1]\n\n";
    }


    # Now search
    @standard = ('Rank', 'File Name', 'Title', 'Document Size');
    @props = qw/prop1 prop2 prop3/;
    
    $props = join ' ', @props;
    $sort  = 'prop1 asc prop2 desc';
    $query = 'meta1=metatest1';

    my $num_results = SwishSearch($handle, $query, 1, $props, $sort);

    unless ( $num_results ) {
        print "No Results\n";

        my $error = SwishError( $handle );
        print "Error number: $error\n" if $error;

        return;  # or next.
    }

    my %results; # place to store the return values by name
    
    while(  @results{ @standard, @props } = SwishNext( $handle )) {
        print "\n";
        printf("%20s -> '%s'\n", $_, $results{$_}) for @standard, @props;
    }

    # No more queries on these indexes
    SwishClose( $handle );


=head1 ABSTRACT

SWISHE version 2.1.x creates an archive library of the internal SWISHE C functions.
This perl module provides access to those functions by embedding the SWISHE search code in
your application.  The benefits are faster searches (no need to fork/execute an external program)
and avoids commonly used unsafe system calls.

This module provides direct access to the SWISHE C library functions.  For a higher level, object
oriented interface to SWISH visit http://search.cpan.org/search?mode=module&query=SWISH


=head1 INSTALLATION

Before you can build the perl module you must build and install SWISH-E.  Please read the
B<INSTALL> documentation included in the SWISHE distribution package.

    perldoc INSTALL

After building the SWISHE executable and successfully running make test, you will need to install
the SWISHE archive library.  This is done while in the top-level directory of the SWISHE distribution.

    % su root
    % make install-lib
    % exit

This will install the archive library (F<libswish-e.a>) into /usr/local/lib by default.

Next, build the perl module.

    % cd perl
    % perl Makefile.PL
    % make
    % make test
    % su root
    % make install
    % exit

If you do not have root access you can instead use

    % perl Makefile.PL PREFIX=/path/to/my/local/perl/library

And then in your perl script:

    use lib '/path/to/my/local/perl/library';


To test it you can run the test.pl script.  Type "./test.pl" at your command prompt.
This perl script uses the index file built by the "make test" command used during the build
of SWISHE as described in the B<INSTALL> document.

B<NOTE>Currently swish will exit the running program on some fatal errors.  In general
this should not happen, but it is something to think about if running under mod_perl
as an error will kill off the web server process.  Apache, for example, should recover
by starting up a new child process.  But, it's not a very graceful way to handle errors.

=head1 FUNCTION DESCRIPTIONS

The following describes the perl interface to the SWISHE C library.

=over 4

=item B<$handle = SwishOpen( $IndexFiles );>

Open one or more index files and returns a handle.

    Examples:

        $handle = SwishOpen( 'index_file.idx' );

        # open two indexes
        $handle = SwishOpen( 'index1.idx index2.idx' );

Returns undefined on an error, but the only errors are typically fatal, so
will most likely exit the running program.


=item B<SwishClose( $handle );>

Closes the handle returned by SwishOpen.  
Closes all the opened files and frees the used memory.

=item B<$num_results = SwishSearch($handle, $search, $structure, $properties, $sortspec);>

Returns the number of hits, zero for no results, or a negative number.
If zero SwishError( $handle ) will return the error code which can be passed
to SwishErrorString() to fetch an error string.

The values passed are:

=over 2

=item *

$handle is the handle returned by SwishOpen

=item *

$search is the search string.

Examples:
    my $query = 'title="this a is phrase"';
    my $query = '(title="this phrase") or (description=(these words))';

=item *

$structure is an integer value only applicable for an html search.  It defines
where in an html search to look.
It can be IN_FILE, IN_TITLE, IN_HEAD, IN_BODY, IN_COMMENTS, IN_HEADER or IN_EMPHASIZED or
or'ed combinations of them (e.g.: IN_HEAD | IN_BODY).
Use IN_FILE (a value of 1) if your documents are not html.
The numerical values for these constants are in src/swish.h

You can define these in your code with:

    # Set bits
    use constant IN_FILE  => 1;
    use constant IN_TITLE => 2;
    use constant IN_HEAD  => 4;

Not many people use the structure feature.    

=item *

$properties is a string with the properties to be returned separated by spaces.  Properties must
be defined during indexing.  See B<README-SWISHE> for more information.

Example:

    my $properties = 'subject description';

You may also use the swish internal properties:

    my $properties = 'subject description swishrank swishlastmodified';


=item *

$sortspec is the sort spec if different from relevance.

Examples:
    my $sortspec = ''  # sort by relevance

    # sort first in ascending order by title,
    # then by other fields in descending order
    my $sortspec = 'title asc category desc category desc';

=back

=item B<SwishNext( $handle )>

($rank, $filename, $title, $size, @properties) = SwishNext( $handle );

This function returns the next hit of a search. Must be executed after SwishSearch to read the results.

=over 2

=item *

$rank - An integer from 1 to 1000 indicating the relevance of the result

=item *

$filename - The source filename

=item *

$title - The title as indexed (as found in the HTML E<lt>TITLEE<gt> section)

=item *

$size - The length of the source document

=item *

@properties - The list of properties returned for this result.

=back

See the SYNOPSIS above for an example.


=item B<$rc=SwishSeek($handle, $num);>

Repositions the pointer in the result list to the element pointed by num.
It is useful when you want to read only the results starting at $num (e.g. for showing
results one page at a time).

=item B<$error_number=SwishError($handle);>

Returns the last error if any (always a negative value).
If there is not an error it will return 0.

=item B<$error_string=SwishErrorString( $error_number );>

Returns the error string for the number supplied.

    print 'Error: ', SwishErrorString( SwishError($handle) ), "\n";

=item B<@ParameterArray=SwishHeaderParameter($handle,$HeaderParameterName);>

This function is useful to access the header data of the index files
Returns the contents of the requested header parameter of all index files
opened by SwishOpen in an array.

Example:

    @wordchars = SwishHeaderParameter( $handle, 'WordCharacters' );
    print "WordCharacters for index 0 = $wordchars[0]\n";
    print "WordCharacters for index 1 = $wordchars[1]\n";


Valid values for HeaderParameterName are:

    WordCharacters
    BeginCharacters
    EndCharacters
    IgnoreFirstChar
    IgnoreLastChar
    Indexed on
    Description
    IndexPointer
    IndexAdmin
    Stemming
    Soundex

Note that this list may be incomplete.  Check the source code or the swish-e
discussion list for more info.

=item B<@stopwords = SwishStopWords( $handle, $indexfilename );>

Returns an array containing all the stopwords stored in the index file pointed by $indexfilename
where $indexfilename must match one of the names used in SwishOpen.

Example:
    @stopwords = SwishStopWords( $handle, $indexfilename );
    print 'Stopwords: ',
          join(', ', @stopwords),
          "\n";

=item B<@keywords = SwishWords( $handle, $indexfilename, $c);>

Returns an array containing all the keywords stored in the index file pointed by
$indexfilename ($indexfilename must match one of the names used in SwishOpen)
and starting with the character $c.

Example:
    my $letter = 't';
    @keywords = SwishWords( $handle, $indexfilename, $letter);

    print "List of keywords that start with the letter '$letter':\n",
          join("\n", @keywords),
          "\n";

=item B<$stemword=SwishStem( $word );>

Returns the stemmed word preserving the original one.

Example:
    my $stemword = SwishStem( 'parking' );
    print $stem_word;     # prints park

=back

=head1 SUPPORT

Questions about this module and SWISHE should be posted to the SWISHE mailing list.
See http://swish-e.org


=head1 AUTHOR

Jose Ruiz -- jmruiz@boe.es  (Documentation by Bill Moseley)


=head1 SEE ALSO

http://swish-e.org

SWISH, SWISH::Library at your local CPAN site.


=head1 Document Info

$Id$




