#!/usr/local/bin/perl -w
use strict;

####################################################################################
#
#    If this text is displayed on your browser then your web server
#    is not configured to run .cgi programs.
#
#    To display documentation for this program type "perldoc swish.cgi"
#
#    swish.cgi $Revision$ Copyright (C) 2001 Bill Moseley search@hank.org
#    Example CGI program for searching with SWISH-E
#
#    This example program will only run under an OS that supports fork().
#
#    Documentation below, or try "perldoc swish.cgi"
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version
#    2 of the License, or (at your option) any later version.
#    
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    The above lines must remain at the top of this program
#
#    $Id$
#
####################################################################################


    ##### Configuration Parameters #########


    my $Title = 'Search Our Website';               # Title of your choice.
    my $Swish_Binary = '/usr/local/bin/swish-e';    # Location of swish-e binary
    my $Swish_Index   = '../index.swish-e';         # Location of your index file

    my $Page_Size = 15;                             # Number of results per page

    my $Highlight_On = 1;                           # Set to zero to turn off highlighting (and see how slow highlighting is)   

    my $Show_Words = 12;                            # Number of swish words+non-swish words to show around highlighted word
                                                    # Set to zero to not highlight and only show $Min_Words
    my $Occurrences = 6;                            # Limit number of occurrences  of highlighted words
    my $Min_Words = 100;                            # If no words are found to highlighted then show this many words
    




    # These should be left alone.

    my @PropertyNames = ();
    my @Sorts = qw/swishrank swishtitle swishdocpath swishdocsize swishlastmodified/;
    my @MetaNames = qw/swishdocpath swishtitle ALL/;

    

    use CGI;
    use Symbol;

    {    

        my $q = CGI->new;

        my $results = run_query( $q );

        $results->{title} = $Title || 'Search our Website';
    

        print $q->header,
              header( $q, $results );


        unless ( $results->{FILES} ) {
            print footer();
            exit;
        }


        print results_header( $results );
        print show_result( $_ ) for @{$results->{FILES}};
        print "<P>$results->{LINKS}<P>";
    
        print footer();
    }


#=====================================================================
# These routines format the HTML output.
#=====================================================================


#=====================================================================
# This generates the header which includes the form
#
#   Pass:
#       $q      - a CGI object
#       $params - program settings        
#

sub header {

    my $q      = shift;
    my $params = shift;

    my $query = CGI->escapeHTML( $q->param('query') || '' );
    my $title = $params->{title} || 'Swish-e Search Form';

    my $message = $params->{MESSAGE}
        ? qq[<br><font color=red>$params->{MESSAGE}</font>]
        : '' ;

    my %checked;
    for ( qw/swishdocpath swishtitle ALL/ ) {
        $checked{$_} = $q->param('metaname') && $q->param('metaname') eq $_
            ? 'checked'
            : '';
    }
    $checked{ALL} = 'checked' unless $q->param('metaname');            

    my %selected;
    for ( qw/swishrank swishtitle swishdocpath swishdocsize swishlastmodified/ ) {
        $selected{$_} = $q->param('sort') && $q->param('sort') eq $_
            ? 'selected'
            : '';
    }

    my $checked = $q->param('reverse') ? 'checked' : '';

    
    return <<EOF;
<html>
    <head>
       <title>
          Search Page
       </title>
    </head>
    <body>
        <h2>
        <img src="/images/swish.gif"> $title $message
        
        </h2>

            
        <form method="post" action="/cgi-bin/swish.cgi" enctype="application/x-www-form-urlencoded" class="form">
            <input / maxlength="200" value="$query" size="32" type="text" name="query">
            <input / value="Search!" type="submit" name="submit"><br>

    

                Limit search to:
                    <input value="swishdocpath" type="radio" $checked{swishdocpath} name="metaname">URL
                    <input value="swishtitle" type="radio" $checked{swishtitle} name="metaname">Title
                    <input value="ALL" type="radio" $checked{ALL} name="metaname">Do not Limit
                <br>
        



        
                Sort by:
                <select name="sort">
                        <option $selected{swishrank} value="swishrank">Rank</option>
                        <option $selected{swishtitle} value="swishtitle">Title</option>
                        <option $selected{swishdocpath} value="swishdocpath">URL</option>
                        <option $selected{swishdocsize} value="swishdocsize">Size</option>
                        <option $selected{swishlastmodified} value="swishlastmodified">Modified Date</option>
                </select>
                <input value="1" type="checkbox" $checked name="reverse">Reverse Sort
                                    

        </form>


    <p>
EOF
}

#=====================================================================
# This routine creates the results header display
#
#
#
#

sub results_header {

    my $results = shift;


    my $links = '';

    $links .= '<font size="-1" face="Geneva, Arial, Helvetica, San-Serif">&nbsp;Page:</font>' . $results->{PAGES}
        if $results->{PAGES};

    $links .= qq[ <a href="$results->{QUERY_HREF}&amp;start=$results->{PREV}">Previous $results->{PREV_COUNT}</a>]
        if $results->{PREV_COUNT};

    $links .= qq[ <a href="$results->{QUERY_HREF}&amp;start=$results->{NEXT}">Next $results->{NEXT_COUNT}</a>]
        if $results->{NEXT_COUNT};

    $results->{LINKS} = $links;

    $links = qq[<tr><td colspan=2 bgcolor="#EEEEEE">$links</td></tr>] if $links;
    
    my $user_query = CGI->escapeHTML( $results->{QUERY_SIMPLE} );

    return <<EOF;

    <table cellpadding=0 cellspacing=0 border=0 width="100%">
        <tr>
            <td height=20 bgcolor="#FF9999">
                <font size="-1" face="Geneva, Arial, Helvetica, San-Serif">
                &nbsp;Results for <b>$user_query</b>
                &nbsp; $results->{FROM} to $results->{TO} of $results->{HITS} results.
                </font>
            </td>
            <td align=right bgcolor="#FF9999">
                <font size="-2" face="Geneva, Arial, Helvetica, San-Serif">
                Run time: $results->{RUN_TIME} |
                Search time: $results->{SEARCH_TIME} &nbsp; &nbsp;
                </font>
            </td>
        </tr>

        $links

    </table>
    

    <p>

EOF

}

#=====================================================================
# This routine formats a single result
#
#
sub show_result {
    my $result = shift;

    return <<EOF;
    <dl>
        <dt>$result->{swishreccount} <a href="$result->{swishdocpath}">$result->{swishtitle}</a> <small>-- rank: <b>$result->{swishrank}</b></small></dt>
        <dd>$result->{swishdescription}<br>


        <small>
            <a href="$result->{swishdocpath}">$result->{swishdocpath}</a>
            $result->{swishlastmodified}
            $result->{swishdocsize} bytes.
        </small>
        </dd>
    </dl>

EOF

}

#=====================================================================
# This is displayed on the bottom of every page
#
#

sub footer {
    return <<EOF;

    <hr>
    
  </body>
</html>
EOF
}

#============================================
# This function parses the CGI parameters,
# and runs the query if a query was entered
#
#   Pass:
#       $q - a CGI object
#
#   Returns:
#       a reference to a hash with an error message or results
#


sub run_query {

    my $q = shift;

    # set up the query string to pass to swish.
    my $query = $q->param('query') || '';

    for ( $query ) {  # trim the query string
        s/\s+$//;
        s/^\s+//;
    }

    unless ( $query ) {
        return $q->param('submit')
            ? { MESSAGE => 'Please enter a query string' }
            : {};
    }

    if ( length( $query ) > 100 ) {
        return { MESSAGE => 'Please enter a shorter query' };
    }

    my $query_simple = $query;

    $q->param('query', $query );  # clean up the query, if needed.

    my $metaname = $q->param('metaname') || '';

    if ( $metaname && $metaname ne 'ALL' ) {

        return { MESSAGE => 'Bad MetaName provided' }
            unless grep { $metaname eq $_ } @MetaNames;

        # prepend metaname to search, if required.
        $query = $q->param('metaname') . "=($query)";
    }
    

   
    # Set the starting position

    my $start = $q->param('start') || 0;
    $start = 0 unless $start =~ /^\d+$/ && $start >= 0;



    # Create a search record

    my $sh = {
       prog     => $Swish_Binary,
       indexes  => $Swish_Index,
       query    => $query,
       startnum => $start + 1,  
       maxhits  => $Page_Size,
       properties => \@PropertyNames,
    };


    # Now set sort option - if a valid option submitted (or you could let swish-e return the error).
    
    my %sorts = map { $_, 1 } @Sorts;

    if ( $q->param('sort') && $sorts{ $q->param('sort') } ) {

            my $direction = $q->param('sort') eq 'swishrank'
                ? $q->param('reverse') ? 'asc' : 'desc'
                : $q->param('reverse') ? 'desc' : 'asc';
                        
            $sh->{sortorder} = [ $q->param('sort'), $direction ];
    } else {
        return { MESSAGE => 'Invalid Sort Option Selected' };
    }


    my $ret;


    # Trap the call
    
    eval {
        local $SIG{ALRM} = sub { die "Timed out\n" };
        alarm 10;
        $ret = run_swish( $sh );
    };
    
    return { MESSAGE => $@ } if $@;

    return { MESSAGE => $ret } unless ref $ret;


    # Build href for repeated search

    my $href = $q->script_name . '?' .
        join( '&amp;',
            map { "$_=" . $q->escape( $q->param($_) ) }
                grep { $q->param($_) }  qw/query metaname sort reverse/
        );


    my $hits = @{$ret->{FILES}};


    # Return the template fields

    my $results = {
        %$ret,          # items return from running swish
        
        QUERY       => $q->escapeHTML( $query ),
        QUERY_HREF  => $href,           # for running this query again
        QUERY_SIMPLE=> $query_simple,   # the query w/o any metanames - same as QUERY if metanames are not used
        MY_URL      => $q->script_name,
        SHOWING     => $hits,
        HITS        => $ret->{header}{'number of hits'} ||  0,
        RUN_TIME    => $ret->{header}{'run time'} ||  'unknown',
        SEARCH_TIME => $ret->{header}{'search time'} ||  'unknown',
        FROM        => $start + 1,
        TO          => $start + $hits,
        MOD_PERL    => $ENV{MOD_PERL},
    };

    set_page( $results, $q );



    return $results;

    
}        

#========================================================
# Sets prev and next page links.
# Feel free to clean this code up!
#
#   Pass:
#       $resutls - reference to a hash (for access to the headers returned by swish)
#       $q       - CGI object
#
#   Returns:
#       Sets entries in the $results hash
#
    
sub set_page {
    my ( $results, $q ) = @_;
    
        
    my $start = $results->{FROM} - 1;   # Current starting record
    
        
    my $prev = $start - $Page_Size;
    $prev = 0 if $prev < 0;

    if ( $prev < $start ) {
        $results->{PREV} = $prev;
        $results->{PREV_COUNT} = $start - $prev;
    }

    
    my $last = $results->{HITS} - 1;

    
    my $next = $start + $Page_Size;
    $next = $last if $next > $last;
    my $cur_end   = $start + scalar @{$results->{FILES}} - 1;
    if ( $next > $cur_end ) {
        $results->{NEXT} = $next;
        $results->{NEXT_COUNT} = $next + $Page_Size > $last
                                ? $last - $next + 1
                                : $Page_Size;
    }


    # Calculate pages  ( is this -1 correct here? )
    
    my $pages = int (($results->{HITS} -1) / $Page_Size);
    if ( $pages ) {

        my @pages = 0..$pages;

        my $max_pages = 10;

        if ( @pages > $max_pages ) {
            my $current_page = int ( $start / $Page_Size - $max_pages/2) ;
            $current_page = 0 if $current_page < 0;
            if ( $current_page + $max_pages - 1 > $pages ) {
                $current_page = $pages - $max_pages;
            }
            
            @pages = $current_page..$current_page + $max_pages - 1;
            unshift @pages, 0 if $current_page;
            push @pages, $pages unless $current_page + $max_pages - 1 == $pages;
        }

    
        $results->{PAGES} =
            join ' ', map {
                my $page_start = $_ * $Page_Size;
                my $page = $_ + 1;
                $page_start == $start
                ? $page
                : qq[<a href="$results->{QUERY_HREF}&amp;start=$page_start">$page</a>];
                        } @pages;
    }

}

#============================================
# Returns compiled regular expressions for matching
#
#   Pass:
#       Reference to headers hash
#
#   Returns an array (or undef)
#       $wordchar_regexp    = used for splitting the text
#       $extract_regexp     = used to extract a word to match against
#       $query_regexp       = used for matching words
#

sub set_match_regexp {
    my $header = shift;

    my ($query, $wc, $ignoref, $ignorel ) =
        @{$header}{'parsed words',qw/wordcharacters ignorefirstchar ignorelastchar/};

    return unless $wc && $query;  #  Shouldn't happen

    $wc = quotemeta $wc;


    my $match_string =
        join '|',
           map { substr( $_, -1, 1 ) eq '*'
                    ? quotemeta( substr( $_, 0, -1) ) . "[$wc]*?"
                    : quotemeta
               }
                grep { ! /^(and|or|not|["()=])$/oi }
                    split /\s+/, $query;


    return unless $match_string;

    for ( $ignoref, $ignorel ) {
        if ( $_ ) {
            $_ = quotemeta;
            $_ = "([$_]*)";
        } else {
            $_ = '()';
        }
    }


    $wc .= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';  # Warning: dependent on tolower used while indexing


    return (
        qr/([^$wc]+)/o,                     # regexp for splitting into swish-words
        qr/^$ignoref([$wc]+?)$ignorel$/io,  # regexp for extracting out the words to compare
        qr/^$match_string$/o,               # regexp for comparing extracted words to query
                                            # Must force lower case before testing
    );
}    

    
#============================================
# Run swish-e and gathers headers and results
# Currently requires fork() to run.
#
#   Pass:
#       $sh - an array with search parameters
#
#   Returns:
#       a reference to a hash that contains the headers and results
#       or possibly a scalar with an error message.
#

sub run_swish {

    my $sh = shift;

    my @properties = qw(
        swishreccount
        swishtitle
        swishrank
        swishdocpath
        swishdocsize
        swishlastmodified
        swishdescription
        swishdbfile
    );
        

    my $fh = gensym;
    my $pid;

    my %ret;

    my @results;

    if ( $pid = open( $fh, '-|' ) ) {

    my @regexps;  # regular expressions used for highlighting
    my $regexp_set;
    my $stemmer_function;

        while (<$fh>) {


            chomp;

            # This will not work correctly with multiple indexes
            if ( /^# ([^:]+):\s+(.+)$/ ) {
                $ret{header}{ lc($1) } = $2;
                next;
            }


            # return errors as text
            return $1 if /^err:\s*(.+)/;


            # Found a result
            if ( /^\d/ ) {

                my %h;
                @h{@properties} = split /\t/;
                push @results, \%h;

                if ( ! $Highlight_On ) {
                    $h{swishdescription} = substr( ($h{swishdescription} || ''), 0, 1000 );
                    next;
                }


                # This is to prepare for highlighting - only do first time
                unless ( $regexp_set++ ) {
                    @regexps = set_match_regexp( $ret{header} );


                    if ( $ret{header}{'stemming applied'} =~ /^(?:1|yes)$/i ) {
                        eval { require SWISH::Stemmer };
                        if ( $@ ) {
                            $ret{MESSAGE} = 'Stemmed index needs Stemmer.pm to highlight';
                        } else {
                            $stemmer_function = \&SWISH::Stemmer::SwishStem;
                        }
                    }
                }


                $h{swishdescription} = highlight( \$h{swishdescription}, $stemmer_function, @regexps )
                    if $h{swishdescription};
                
            }

            # Might check for "\n." for end of results.

            
        }

        $ret{FILES} = \@results;


        return \%ret;
        
    } else {

        return "Failed to fork '$sh->{query}': $!" if !defined $pid;

        my $output_format = join( '\t', map { "<$_>" } @properties ) . '\n';


        exec $sh->{prog},
            -w => $sh->{query},
            -f => $sh->{indexes},
            -b => $sh->{startnum},
            -m => $sh->{maxhits},
            -s => @{$sh->{sortorder}},
            -H => 9,
            -x => $output_format;

        die "Failed to exec '$sh->{prog}' Error:$!";
    }
}

#==========================================================================
# This routine highlights words in source text
# Source text must be plain text.
#
# This is a very basic highlighting routine that fails for phrase searches, and
# Does not know how to deal with metaname searches.
#
# The text returned contains highlighted words, plus a few words on either side to give
# context of the results.
#
#   Pass:
#       the text to highlight, a transformation function (normally a stemmer function)
#       and pre-compiled regular expressions
#
#   Returns:
#       the highlighted text
#

sub highlight {
    my ( $text_ref, $stemmer_function, $wc_regexp, $extract_regexp, $match_regexp ) = @_;


    my $last = 0;


    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.
    my @words = split /$wc_regexp/, $$text_ref;


    my @flags;
    $flags[$#words] = 0;  # Extend array.

    my $occurrences = $Occurrences ;


    my $pos = $words[0] eq '' ? 2 : 0;  # Start depends on if first word was wordcharacters or not

    while ( $Show_Words && $pos <= $#words ) {

        if ( $words[$pos] =~ /$extract_regexp/ ) {

            my ( $begin, $word, $end ) = ( $1, $2, $3 );

            my $test = $stemmer_function
                       ? $stemmer_function->($word)
                       : lc $word;

            $test ||= lc $word;                       

            # Not check if word matches
            if ( $test =~ /$match_regexp/ ) {

                $words[$pos] = "$begin<b>$word</b>$end";


                my $start = $pos - $Show_Words + 1;
                my $end   = $pos + $Show_Words - 1;
                if ( $start < 0 ) {
                    $end = $end - $start;
                    $start = 0;
                }
                
                $end = $#words if $end > $#words;

                $flags[$_]++ for $start .. $end;


                # All done, and mark where to stop looking
                if ( $occurrences-- <= 0 ) {
                    $last = $end;
                    last;
                }
            }
        }

       $pos += 2;  # Skip to next wordchar word
    }


    my $dotdotdot = ' <b>...</b>';


    my @output;

    my $printing;
    my $first = 1;
    my $some_printed;

    if ( $Show_Words ) {
        for my $i ( 0 ..$#words ) {

            if ( $last && $i >= $last && $i < $#words ) {
                push @output, $dotdotdot;
                last;
            }

            if ( $flags[$i] ) {

                push @output, $dotdotdot if !$printing++ && !$first;
                push @output, $words[$i];
                $some_printed++;

            } else {
                $printing = 0;
            }

            $first = 0;
        
        }
    }

    if ( !$some_printed ) {
        for my $i ( 0 .. $Min_Words ) {
            last if $i >= $#words;
            push @output, $words[$i];
        }
    }
        
        

    push @output, $dotdotdot if !$printing;

    return join '', @output;


}


__END__

=head1 NAME

swish.cgi -- Example Perl script for searching with the SWISH-E search engine.

=head1 DESCRIPTION

This is an example CGI script for searching with the SWISH-E search engine version 2.2 and above.
It returns results a page at a time, with matching words from the source document highlighted, showing a
few words of content on either side of the highlighted word.

This example does not require installation of additional Perl modules (from CPAN), unless you wish
to use stemming with your index.  In this case you will need the SWISH::Stemmer module, which is
included with the SWISH-E distribution (and also available on CPAN).  Instructions for installing
the module are included below.

A more advanced example script is also provided called C<swish2.cgi>.  That script uses a number of
perl modules for templating (separation of content from program logic)
and abstracting the interface with swish-e.

Due to the forking nature of this program, this will probably not run under Windows without some
modification.

=head1 INSTALLATION

Installing a CGI application is dependent on your specific web server's configuration.
For this discussion we will assume you are using Apache, and in a typical configuration.  For example,
a common location for the DocumentRoot is C</usr/local/apache/htdocs>.  If you are installing this
on your shell account, your DocumentRoot might be C<~yourname/public_html>.  So, for the sake of this example,
we will assume the following:

    /usr/local/apache/htdocs        - Document root
    /usr/local/apache/htdocs/images - images directory
    /usr/local/apache/cgi-bin       - CGI directory

=head2 Move the files to their locations

=over 4

=item 1 Copy the swish.cgi file to your CGI directory

Most web servers have a directory where CGI programs are kept.  If this is the case on your
server copy the C<swish.cgi> perl script into that directory.  You will need to provide read
and execute permisssions to the file.  Exactly what permissions are needed again depends on
your specific configuration.  But in general, you should be able to use the command:

    chmod 0755 swish.cgi

This gives the file owner (that's you) write access, and everyone read and execute access.    

Note that you are not required to use a cgi-bin directory with Apache.  You may place the
CGI script in any directory accessible via the web server and
enable it as a CGI script with something like the following
(place either in httpd.conf or in .htaccess):

    <Files swish.cgi>
        Allow from all
        SetHandler cgi-script
        Options +ExecCGI
    </Files>        

Using this method you don't even need to use the C<.cgi> extension.  For example, rename
the script to "search" and then use that in the C<Files> directive.

=item 2 Copy the swish.gif file to your images directory.

The C<swish.cgi> script expects the C<swish.gif> file to be located in the web path
C</images/swish.gif>.  If this is not the case on your server you will need to adjust the
script.

=back

=head1 CONFIGURATION

=head2 Configure the swish.cgi program

Use a text editor and open the C<swish.cgi> program.

=over 4

=item 1 Check the C<shebang> line

The first line of the program must point to the location of your perl program.  Typical
examples are:

    #!/usr/local/bin/perl -w
    #!/usr/bin/perl -w
    #!/opt/perl/bin/perl -w

=item 2 Set the configuration parameters

To make things simple, the configuration parameters are included at the top of the program.
Look for the following code:

    ##### Configuration Parameters #########

    my $Title = 'Search Our Website';               # Title of your choice.
    my $Swish_Binary = '/usr/local/bin/swish-e';    # Location of swish-e binary
    my $Swish_Index   = '../index.swish-e';         # Location of your index file
    my $Page_Size = 20;                             # Number of results per page
    my $Show_Words = 12;                            # Number of swish words+non-swish words to show around highlighted word
    my $Occurrences = 6;                            # Limit number of occurrences of highlighted words
    my $Min_Words = 100;                            # If no words are found to highlighted then show this many words

The comments should be self explanatory.  The example above places the swish index file
in the directory above the C<swish.cgi> CGI script.  If using the example paths above
of C</usr/local/apache/cgi-bin> for the CGI bin directory, that means that the index file
is in C</usr/local/apache>.  That places the index out of web space (e.g. cannot be accessed
via the web server), yet relative to where the C<swish.cgi> script is located.

There's more than one way to do it, of course.  Some people like to keep the index "tied" to
the search script.  One option is to place it in the same directory as the <swish.cgi> script, but
then be sure to use your web server's configuration to prohibit access to the index directly.

Another common option is to maintain a separate directory of the swish index files.  This decision is
up to you.

=item 3 Create your index

You must index your web site before you can begin to use the C<swish.cgi> script.
Create a configuration file called C<swish.conf> in the directory where you will store
the index file.

This example uses the file system to index your web documents.
In general, you will probably wish to I<spider> your web site if your web pages do not
map exactly to your file system, and to only index files available from links on you web
site.

The file system is the fastest way to index.  For example, when indexing the Apache documentation
it took two seconds to index, yet over a minute to spider using the "http" input method.
See B<Spidering> below for more information.

Example C<swish.conf> file:

    # Define what to index
    IndexDir /usr/local/apache/htdocs
    IndexOnly .html .htm

    # Tell swish how to parse .html and .html documents
    IndexContents HTML .html .htm
    # And just in case we have files without an extension
    DefaultContents HTML

    # Replace the path name with a URL
    ReplaceRules replace /usr/local/apache/htdocs/ http://www.myserver.name/

    # Store the text of the documents within the swish index file
    StoreDescription HTML <body> 200000

    # Allow limiting search to titles and URLs.
    MetaNames swishdocpath swishtitle

    # Optionally use stemming for "fuzzy" searches
    #UseStemming yes

Now to index you simply run:

    swish-e -c swish.conf

The default index file C<index.swish-e> will be placed in the current directory.

=back

Now you should be ready to run your search engine.  Point your browser to:

    http://www.myserver.name/cgi-bin/swish.cgi

=head1 DEBUGGING

The key to debugging CGI scripts is to run them from the command line, not with a browser.

First, make sure the program compiles correctly:

    > perl -c swish.cgi
    swish.cgi syntax OK

Next, simply try running the program:

    > ./swish.cgi
    Content-Type: text/html; charset=ISO-8859-1

    <html>
        <head>
           <title>
              Search Page
           </title>
        </head>
        <body>
            <h2>
            <img src="/images/swish.gif"> Search Our Website
     ...

Now, you know that the program compiles and will run from the command line.
Next, try accessing the script from a web browser.

If you see the contents of the CGI script instead of its output then your web server is
not configured to run the script.  You will need to look at settings like ScriptAlias, SetHandler,
and Options.

If an error is reported (such as Internal Server Error or Forbidden)
you need to locate your web server's error_log file
and carefully read what the problem is.  Contact your web administrator for help.
    
    
=head1 Spidering

There are two ways to spider with swish-e.  One uses the "http" input method that uses code that's
part of swish.  The other way is to use the new "prog" method along with a perl helper program called
C<spider.pl>.

Here's an example of a configuration file for spidering with the "http" input method.
You can see that the configuration is not much different than the file system input method.

    # Define what to index
    IndexDir http://www.myserver.name/index.html
    IndexOnly .html .htm

    IndexContents HTML .html .htm
    DefaultContents HTML
    StoreDescription HTML <body> 200000
    MetaNames swishdocpath swishtitle

    # Define http method specific settings -- see swish-e documentation
    SpiderDirectory ../swish-e/src/
    Delay 0

You index with the command:

    swish-e -S http -c spider.conf

Note that this does take longer.  For example, spidering the Apache documentation on
a local web server with this method took over a minute, where indexing with the
file system took less than two seconds.  Using the "prog" method can speed this up.

Here's an example configuration file for using the "prog" input method:

    # Define the location of the spider helper program
    IndexDir ../swish-e/prog-bin/spider.pl

    # Tell the spider what to index.
    SwishProgParameters default http://www.myserver.name/index.html

    IndexContents HTML .html .htm
    DefaultContents HTML
    StoreDescription HTML <body> 200000
    MetaNames swishdocpath swishtitle

Then to index you use the command:

    swish-e -c prog.conf -S prog -v 0

Spidering with this method took nine seconds.    




=head1 Stemmed Indexes

Many people enable a feature of swish called word stemming to provide "fuzzy" search
options to their users.
The stemming code does not actually find the "stem" of word, rather removes and/or replaces
common endings on words.
Stemming is far from perfect, and many words do not stem as you might expect.  But, it can
be a helpful tool for searching your site.  You may wish to create both a stemmed and non-stemmed index, and
provide a checkbox for selecting the index file.

To enable a stemmed index you simply add to your configuration file:

    UseStemming yes

If you want to use a stemmed index with this program and continue to highlight search terms you will need
to install a perl module that will stem words.  This section explains how to do this.

The perl module is included with the swish-e distribution.  It can be found in the examples directory (where
you found this file) and called something like:

    SWISH-Stemmer-0.05.tar.gz

The module should also be available on CPAN (http://search.cpan.org/).    

Here's an example session for installing the module.  (There will be quite a bit of output
when running make.)


    % gzip -dc SWISH-Stemmer-0.05.tar.gz |tar xof -
    % cd SWISH-Stemmer-0.05
    % perl Makefile.PL
    or
    % perl Makefile.PL PREFIX=$HOME/perl_lib
    % make
    % make test

    (perhaps su root at this point if you did not use a PREFIX)
    % make install
    % cd ..

Use the B<PREFIX> if you do not have root access or you want to install the modules
in a local library.  If you do use a PREFIX setting, add a C<use lib> statement to the top of this
swish.cgi program.

For example:

    use lib qw(
        /home/bmoseley/perl_lib/lib/site_perl/5.6.0
        /home/bmoseley/perl_lib/lib/site_perl/5.6.0/i386-linux/
    );

Once the stemmer module is installed, and you are using a stemmed index, the C<swish.cgi> script will automatically
detect this and use the stemmer module.

=head1 DISCLAIMER

Please use at your own risk, of course.

This script has been tested and used without problem, but you should still be aware that
any code running on your server represents a risk.  If you have any concerns please carefully
review the code.

=head1 SUPPORT

The SWISH-E discussion list is the place to ask for any help regarding SWISH-E or this example
script. See http://sunsite.berkeley.edu/SWISH-E/

Please do not contact the author directly.

=head1 LICENSE

swish.cgi $Revision$ Copyright (C) 2001 Bill Moseley search@hank.org
Example CGI program for searching with SWISH-E


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version
2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


=head1 AUTHOR

Bill Moseley -- search@hank.org

=cut

