#!/usr/local/bin/perl -w


####################################################################################
#
#    If this text is displayed on your browser then your web server
#    is not configured to run .cgi programs.  Contact your web server administrator.
#
#    To display documentation for this program type "perldoc swish.cgi"
#
#    swish.cgi $Revision$ Copyright (C) 2001 Bill Moseley swishscript@hank.org
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
package SwishSearch;
use strict;
use lib '.';
use CGI;
use Symbol;

use vars qw/$NotAWord/;


    # Run the script    
    handler() unless $ENV{MOD_PERL};



#==================================================================================

# This is written this way so the script can be used as a CGI script or a mod_perl
# module without any code changes.

sub handler {
    my $r = shift;


    # Taint issues
    $ENV{PATH} = '/usr/bin';   # For taint checking

    
    ##### Configuration Parameters #########
    
    my %CONFIG = (
        title           => 'Search our site',           # Title of your choice.
        swish_binary    => './swish-e',                 # Location of swish-e binary
        swish_index     => './index.swish-e',           # Location of your index file
        page_size       => 15,                          # Number of results per page


        # Property name to use as the main link text to the indexed document.
        # Typically, this will be 'swishtitle' if have indexed html documents,
        # But you can specify any PropertyName defined in your document.
        # By default, swish will return the pathname for documents that do not
        # have a title.  Default is 'swishtitle' if not used.

        title_property => 'swishtitle',



        # prepend this path to the filename returned by swish.  This is used to
        # make the href link back to the original document.  Comment out to disable.

        #prepend_path    => 'http://localhost/mydocs',



        # Swish has a configuration directive "StoreDescription" that will save part or
        # all of a document's contents in the index file.  This can then be displayed
        # along with results.  If you are indexing a lot of files this can use a lot of disk
        # space, so test carefully before indexing your entire site.
        # Building swish with zlib can greatly reduce the space used by StoreDescription
        #
        # This settings tell this script to display this description.
        # Normally, this should be 'swishdescription', but you can specify another property name.
        # There is no default.
       
        description_prop    => 'swishdescription',



        # Property names listed here will be displayed in a table below each result
        # You may wish to modify this list if you are using document properties (PropertyNames)
        # in your swish-e index configuration
        # There is no default.
        
        display_props   => [qw/swishlastmodified swishdocsize swishdocpath/],



        # Results can be be sorted by any of the properties listed here
        # They will be displayed in a drop-down list
        # Again, you may modify this list if you are using document properties of your own creation
        # Swish uses the rank as the default sort

        sorts           => [qw/swishrank swishlastmodified swishtitle/],



        # Secondary_sort is used to sort within a sort
        # You may enter a property name followed by a direction (asc|desc)

        secondary_sort  => [qw/swishlastmodified desc/],




        # You can limit by MetaNames here.  Names listed here will be displayed in
        # a line of radio buttons.
        # The special case of "ALL" means do not do a metaname search.
        # By default this is commented out.

        # For example, if you indexed an email archive
        # that defined the metanames subject name email (as in the swish-e discussion archive)
        # you might use:
        #metanames       => [qw/ALL subject name email/],

        # To see how this might work, add "MetaNames swishtitle swishdocpath" to your config file
        # and try:
        #metanames       => [qw/ALL swishtitle swishdocpath/],

        # "ALL" is probably a bad description, as it's really selecting where to search, and "ALL"
        # doesn't mean it searches all, just the "swishdefault" text, which is any text that's
        # not assigned a metaname.  Therefore, this is probably more correct:
        #metanames       => [qw/swishdefault swishtitle swishdocpath/],

        # Note that you can do a real "all" search if you use nested metanames in your source documents.
        

        # These are used to map MetaNames and PropertyNames to user-friendly names.
        name_labels => {
            swishdefault        => 'Default content',
            swishtitle          => 'Title',
            swishrank           => 'Rank',
            swishlastmodified   => 'Last Modified Date',
            swishdocpath        => 'Document Path',
            swishdocsize        => 'Document Size',
            subject             => 'Message Subject',
            name                => "Poster's Name",
            email               => "Poster's Email",
            sent                => 'Message Date',
            ALL                 => 'Message text',
        },



        # These settings will use some crude highlighting code to highlight search terms in the
        # property specified above as the description_prop (normally, 'swishdescription').


        max_chars       => 500,   # If highlight is off, then just truncate the description to this many *chars*.
                                  # If you want to go by *words*, enable highlighting,
                                  # and then comment-out show_words.  It will be a little slower.


        highlight_words => 1,     # enable highlighting

        show_words      => 12,    # Number of swish words+non-swish words to show around highlighted word
        max_words       => 100,   # If no words are found to highlighted then show this many words
        occurrences     => 6,     # Limit number of occurrences of highlighted words
        #highlight_on   => '<b>', # HTML highlighting codes
        #highlight_off  => '</b>',
        highlight_on    => '<font style="background:#FFFF99">',
        highlight_off   => '</font>',



        # This adds in the date_range limiting options
        # You will need the DateRanges.pm module from the author to use that feature

        date_ranges     => {
            property_name   => 'sent',      # property name to limit by

            time_periods    => [
                'All',
                'Today',
                'Yesterday',
                #'Yesterday onward',
                'This Week',
                'Last Week',
                'Last 90 Days',
                'This Month',
                'Last Month',
                #'Past',
                #'Future',
                #'Next 30 Days',
            ],

            line_break      => 0,
            default         => 'All',
            date_range      => 1,
        },

    );


    # disable "date_ranges" in the above example -- need additional module for that feature
    delete $CONFIG{date_ranges};



    # Used to get all documents, yet limit by a date range.
    $NotAWord = 'not skaisikdeekk';



    # Now run the request

    process_request( \%CONFIG );

    return Apache::Constants::OK() if $ENV{MOD_PERL};
}    

#============================================================================
sub process_request {
    my $conf = shift;  # configuration parameters

    # Set some defaults
    $conf->{name_labels} = {} unless $conf->{name_labels};
    $conf->{page_size} ||= 15;
    $conf->{title_property} ||= 'swishtitle';
    $conf->{title} ||= 'Swish-e Search Form';

    my $q = CGI->new;


    my $results = run_query( $q, $conf );

    print $q->header,
          header( $results );

    unless ( $results->{FILES} ) {
        print footer( $results );
        exit;
    }


    print results_header( $results );
    print show_result( $results, $_ ) for @{$results->{FILES}};
    print "<P>$results->{LINKS}<P>";

    print footer( $results );
}

#=====================================================================
# These routines format the HTML output.
#=====================================================================


#=====================================================================
# This generates the header which includes the form
#
#   Pass:
#       $results hash

sub header {

    my $results = shift;
    my $conf = $results->{conf};
    my $q = $results->{q};


    my $query = $q->param('query') || '';
    $query = '' if $query eq $NotAWord;

    $query = CGI->escapeHTML( $query );  # May contain quotes

    my $title = $conf->{title};

    my $message = $results->{MESSAGE}
        ? qq[<br><font color=red>$results->{MESSAGE}</font>]
        : '' ;


    # This is for sticky forms (probably should use CGI.pm for this now)
    my (%checked, %selected );
    my ( $checked, $limits, $sorts ) = setup_form( $results, \%checked, \%selected );



    my $form = $q->script_name;

    my $date_ranges_select = get_date_ranges( $q, $results );

                     
    
    return <<EOF;
<html>
    <head>
       <title>
          Search Page
       </title>
    </head>
    <body>
        <h2>
        <img src="http://swish-e.org/Images/swish-e.gif"> $title $message
        
        </h2>

            
        <form method="post" action="$form" enctype="application/x-www-form-urlencoded" class="form">
            <input / maxlength="200" value="$query" size="32" type="text" name="query">
            <input / value="Search!" type="submit" name="submit"><br>
    
            $limits
            $sorts
            $date_ranges_select
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
    my $config = $results->{config};
    my $q = $results->{q};

    my $limits = '';

    if ( $results->{DateRanges_time_low} && $results->{DateRanges_time_high} ) {
        my $low = scalar localtime $results->{DateRanges_time_low};
        my $high = scalar localtime $results->{DateRanges_time_high};
        $limits = <<EOF;
        <tr>
            <td colspan=2>
                <font size="-2" face="Geneva, Arial, Helvetica, San-Serif">
                &nbsp;Results limited to dates $low to $high
                </font>
            </td>
        </tr>
EOF
    }

    
    


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
    $user_query = 'All' if $results->{QUERY_SIMPLE} eq $NotAWord;

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
        $limits

    </table>
    

    <p>

EOF

}

#=====================================================================
# This routine formats a single result
#
#
sub show_result {
    my ($results, $result ) = @_;

    my $conf = $results->{conf};

    my $DocTitle = $conf->{title_property} || 'swishtitle';

    my $title = $result->{$DocTitle} || $result->{swishdocpath};

                

    my $props = '';
    if ( $conf->{display_props} ) {
        my $length = 0;
        for ( @{$conf->{display_props}} ) {
            my $label = $conf->{name_labels}{$_} || $_;
            $length = length($label) if length($label) > $length;
        }

        
        $props = join "\n",
            '<br><table cellpadding=0 cellspacing=0>',
            map ( {
                '<tr><td><small>'
                . ( $conf->{name_labels}{$_} || $_ )
                . ':</small></td><td><small> '
                . '<b>'
                . $result->{$_}
                . '</b>'
                . '</small></td></tr>'
                 }  @{$conf->{display_props}}
            ),
            '</table>';
    }
            

    my $PathPrePend = $conf->{prepend_path} || '';
    
    my $description = '';
    if ( $conf->{description_prop} ) {
        $description = $result->{$conf->{description_prop}} || '';
    }

    return <<EOF;
    <dl>
        <dt>$result->{swishreccount} <a href="$PathPrePend$result->{save_swishdocpath}">$title</a> <small>-- rank: <b>$result->{swishrank}</b></small></dt>
        <dd>$description

        $props
        </dd>
    </dl>

EOF

}

#=====================================================================
# This is displayed on the bottom of every page
#
#


sub footer {

    my $mod_perl = $ENV{MOD_PERL}
               ? '<br><small>Response brought to you by <em>MOD_PERL</em> <a href="http://perl.apache.org">perl.apache.org</a></small>'
               : '';

    return <<EOF;

    <hr>
    <small>Powered by <em>Swish-e</em> <a href="http://swish-e.org">swish-e.org</a></small>
    $mod_perl
  </body>
</html>
EOF
}

#==================================================================
#  Form setup for sorts and metas
#
#  Should probably use CGI's sticky forms instead
#
#==================================================================

sub setup_form {
    my ( $results, $checked, $selected ) = @_;

    my $conf = $results->{conf};
    my $q = $results->{q};
    

    if ( $conf->{metanames} ) {
        for ( @{$conf->{metanames}} ) {
            $checked->{$_} = $q->param('metaname') && $q->param('metaname') eq $_
                ? 'checked'
                : '';
        }
    }
    

    if ( $conf->{sorts} ) {
        for ( @{$conf->{sorts}} ) {
            $selected->{$_} = $q->param('sort') && $q->param('sort') eq $_
                ? 'selected'
                : '';
        }
    }
    my $check = $q->param('reverse') ? 'checked' : '';



    # Set the limit by values
    
    my $limits = '';
    if ( $conf->{metanames} ) {

        $checked->{$conf->{metanames}[0]} = 'checked' unless join '', values %$checked;
            
        $limits = join "\n",
            'Limit search to:',
            map( {
                qq[<input value="$_" type="radio" $checked->{$_} name="metaname">]
                . ( $conf->{name_labels}{$_} || $_ )
                } @{$conf->{metanames}}
            ),
            '<br>';
    }

    my $sorts = '';
    if ( $conf->{sorts} ) {
        $sorts = join "\n",
            'Sort by:',
            '<select name="sort">',
            map( {
                qq[<option $selected->{$_} value="$_">]
                . ( $conf->{name_labels}{$_} || $_ )
                . '</option>'
                } @{$conf->{sorts}}
            ),
            '</select>',
            qq[<input value="1" type="checkbox" $check name="reverse">Reverse Sort];
    }

    return ( $check, $limits, $sorts );
}

#==================================================
# Format and return the date range options
#
#--------------------------------------------------
sub get_date_ranges {
    my ( $q, $results ) = @_;

    my $conf = $results->{conf};

    return '' unless $conf->{date_ranges};

    # pass parametes, and a hash to store the returned values.

    my %fields;
    
    DateRanges::DateRangeForm( $q, $conf->{date_ranges}, \%fields );


    # Set the layout:
    
    my $string = '<br>Limit to: '
                 . ( $fields{buttons} ? "$fields{buttons}<br>" : '' )
                 . ( $fields{date_range_button} || '' )
                 . ( $fields{date_range_low}
                     ? " $fields{date_range_low} through $fields{date_range_high}"
                     : '' );

    return $string;
}

#================================================================================
#   Parse out the date limits from the form or from GET request
#
#---------------------------------------------------------------------------------

sub get_date_limits {
    my $results = shift;
    my $q = $results->{q};

    # Are date ranges enabled?
    return 1 unless $results->{conf}{date_ranges};

    eval { require DateRanges };
    if ( $@ ) {
        $results->{MESSAGE} = 'Missing module "DateRanges"';
        delete $results->{conf}{date_ranges};
        return 0;
    }
    
    my %limits;

    unless ( DateRanges::DateRangeParse( $q, \%limits ) ) {
        $results->{MESSAGE} = $limits{DateRanges_error} || 'Bad date range selection';
        return 0;
    }

    $results->{DateRanges_time_low} = $limits{DateRanges_time_low};
    $results->{DateRanges_time_high} = $limits{DateRanges_time_high};


    # Allow searchs just be date if not "All dates" search
    ${$results->{query}} = $NotAWord if !${$results->{query}} && $limits{DateRanges_time_high};


    my $conf = $results->{conf};
    my $sh   = $results->{sh};
    my $limit_prop = $conf->{date_ranges}{property_name};


    # $$$ need to fix for generalized interface! 
    if ( $limits{DateRanges_time_low} && $limits{DateRanges_time_high} ) {
        push @{$sh->{commands}}, ( '-L', $limit_prop, $limits{DateRanges_time_low}, $limits{DateRanges_time_high} );
    }

    return 1;
}


#================================================================
#  Set the sort order
#----------------------------------------------------------------

sub set_sort_order {
    my $results = shift;

    my $q = $results->{q};
    my $sh = $results->{sh};
    my $conf = $results->{conf};

    return 1 unless $conf->{sorts};


    # Now set sort option - if a valid option submitted (or you could let swish-e return the error).
    my %sorts = map { $_, 1 } @{$conf->{sorts}};

    if ( $q->param('sort') && $sorts{ $q->param('sort') } ) {

            my $direction = $q->param('sort') eq 'swishrank'
                ? $q->param('reverse') ? 'asc' : 'desc'
                : $q->param('reverse') ? 'desc' : 'asc';
                    
            # $sh->{sortorder} = ['-s', $q->param('sort'), $direction ];
            push @{$sh->{commands}}, ( '-s', $q->param('sort'), $direction );

            if ( $conf->{secondary_sort} && $q->param('sort') ne $conf->{secondary_sort}[0] ) {
                    # push @{$sh->{sortorder}}, @SecondarySort;
                    push @{$sh->{commands}}, ref $conf->{secondary_sort} ? @{ $conf->{secondary_sort} } : $conf->{secondary_sort};
            }

    } else {
        $results->{MESSAGE} = 'Invalid Sort Option Selected';
        return;
    }

    return 1;
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

    my ( $q, $conf ) = @_;


    # set up the query string to pass to swish.
    my $query = $q->param('query') || '';

    for ( $query ) {  # trim the query string
        s/\s+$//;
        s/^\s+//;
    }


    # initialize the request search hash
    my $sh = {
       prog         => $conf->{swish_binary},
       maxhits      => $conf->{page_size},
       q            => $q,
       commands     => [],      # commands passed to swish
       query        => \$query,
    };


    # Save the index file
    push @{$sh->{commands}}, '-f', ref $conf->{swish_index} ? @{ $conf->{swish_index} } : $conf->{swish_index};


    # initialize the results hash (should be called "self")
    my %results = (
        conf    => $conf,
        q       => $q,
        query   => \$query,
        sh      => $sh,     # request hash
    );


    # Read in the date limits, if any.
    return \%results unless get_date_limits( \%results );


    unless ( $query ) {
        $results{MESSAGE} = 'Please enter a query string' if $q->param('submit');
        return \%results;
    }


    if ( length( $query ) > 100 ) {
        $results{MESSAGE} = 'Please enter a shorter query';
        return \%results;
    }

    my $query_simple = $query;    # without metaname
    $q->param('query', $query );  # clean up the query, if needed.


    # Adjust the query string for metaname search

    my $metaname = $q->param('metaname') || '';

    if ( $metaname && $metaname ne 'ALL' ) {

        unless ( grep { $metaname eq $_ } @{$conf->{metanames}} ) {
            $results{MESSAGE} = 'Bad MetaName provided';
            return \%results;
        }

        # prepend metaname to search, if required.
        $query = $metaname . "=($query)";
        $sh->{metaname} = $metaname;
    }
    

   
    # Set the starting position

    my $start = $q->param('start') || 0;
    $start = 0 unless $start =~ /^\d+$/ && $start >= 0;
    $sh->{startnum} = $start + 1;


    # Set the sort option, if any
    return \%results unless set_sort_order( \%results );


    my $ret;


    # Trap the call - not portable.
    
    eval {
        local $SIG{ALRM} = sub { die "Timed out\n" };
        alarm 10;
        $ret = run_swish( \%results );
    };



    # error conditions
    
    if ( $@ ) {
        $results{MESSAGE} = $@;
        return \%results;
    }

    unless ( ref $ret ) {
        $results{MESSAGE} = $ret;
        return \%results;
    }
    
    if ( ! @{$ret->{FILES}} ) {
        $results{MESSAGE} = 'Failed to find results';
        return \%results;
    }





    # Build href for repeated search via GET

    my $href = $q->script_name . '?' .
        join( '&amp;',
            map { "$_=" . $q->escape( $q->param($_) ) }
                grep { $q->param($_) }  qw/query metaname sort reverse/
        );

            

    if ( $conf->{date_ranges} ) {
        my $dr = DateRanges::GetDateRangeArgs( $q );
        $href .= "&amp;" . $dr if $dr;
    }


    my $hits = @{$ret->{FILES}};


    # Return the template fields

    my $results = {
        %results,       # results array as setup above (including {FILES}
        
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

    my $Page_Size = $results->{conf}{page_size};
    
        
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


    # Now, wait a minute.  Look at this more, as I'd hope that making a
    # qr// go out of scope would release the compiled pattern.

    return $ENV{MOD_PERL}
    ? (
        qr/([^$wc]+)/,                     # regexp for splitting into swish-words
        qr/^$ignoref([$wc]+?)$ignorel$/i,  # regexp for extracting out the words to compare
        qr/^(?:$match_string)$/,           # regexp for comparing extracted words to query
                                           # Must force lower case before testing
    )
    
    : (
        qr/([^$wc]+)/o,                     # regexp for splitting into swish-words
        qr/^$ignoref([$wc]+?)$ignorel$/oi,  # regexp for extracting out the words to compare
        qr/^(?:$match_string)$/o,           # regexp for comparing extracted words to query
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

    my $results = shift;
    my $sh = $results->{sh};
    my $conf = $results->{conf};
    my $q = $results->{q};


    
    my @properties;
    my %seen;

    # Gather up the properties specified
    
    for ( qw/ title_property description_prop display_props / ) {
        push @properties, ref $conf->{$_} ? @{$conf->{$_}} : $conf->{$_}
            if $conf->{$_} && !$seen{$_}++;
    }

    my %display_props = map { $_, 1 } @properties;

    # add in the default props - a number must be first
    @properties = (qw/swishreccount swishrank swishdocpath/, @properties );
        


    my $fh = gensym;
    my $pid = open( $fh, '-|' );

    die "Failed to fork: $!\n" unless defined $pid;

    if ( !$pid ) {  # in child

        my @cmd = (
            $sh->{prog},
            -w => ${$sh->{query}},
            @{$sh->{commands}},
            -b => $sh->{startnum},
            -m => $sh->{maxhits},
            -H => 9,
            -x => join( '\t', map { "<$_>" } @properties ) . '\n',
        );


        exec @cmd or die "Failed to exec '$sh->{prog}' Error:$!";
    }


    # read in from child

    my %ret;

    my @results;

    
    my @regexps;  # regular expressions used for highlighting
    my $regexp_set;
    my $stemmer_function;


    my $highlight = $conf->{highlight_words} || 0;
    my $highlight_prop = $conf->{description_prop} || 'swishdescription';
    my $trim_prop = $highlight_prop;

    my $meta = $sh->{metaname} || '';

    # If a meta search, and it is also a display prop, then highlight that.
    if ( $meta && $meta ne 'ALL' ) {
        if ( $display_props{$meta} ) {
            $highlight_prop = $meta;
        } else {
            $highlight = 0;
        }
    }
            
    delete $display_props{swishdescription};  # This is to prevent running escapeHTML on this property -- so big, and storedescrition is stripped

    $trim_prop = 0 if $highlight && $trim_prop eq $highlight_prop;


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

            # This can't be escapeHTML'd
            $h{save_swishdocpath} = $h{swishdocpath};


            # Escape display properties (except swishdescription)
            $h{$_} = $q->escapeHTML( $h{$_} )  for keys %display_props;


            # Trim down the description if no highlight, or if highlighting some other property
            # Not very nice.  The highlighting code would limit by words

            if ( $trim_prop && $h{$trim_prop} ) {
                my $max = $conf->{max_chars} || 500;

                if ( length $h{$trim_prop} > $max ) {
                    $h{$trim_prop} = substr( $h{$trim_prop}, 0, $max) . ' <b>...</b>';
                }
            }

            


            if ( $highlight && $highlight_prop && exists $h{$highlight_prop} ) {



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


                # Now do the actual highlighting
                
                $h{$highlight_prop} = 
                    highlight( $results, \$h{$highlight_prop}, $stemmer_function, @regexps );

            }
        }

        # Might check for "\n." for end of results.

        
    }
    $results->{header} = $ret{header};

    $results->{FILES} = \@results;
    return $results;
        
}

#==========================================================================
# This routine highlights words in source text
# Source text must be plain text.
#
# This is a very basic highlighting routine that fails for phrase searches.
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
    my ( $results, $text_ref, $stemmer_function, $wc_regexp, $extract_regexp, $match_regexp ) = @_;


    my $last = 0;

    my $Show_Words = $results->{conf}{show_words} || 10;
    my $Occurrences = $results->{conf}{occurrences} || 5;
    my $Max_Words = $results->{conf}{max_words} || 100;
    my $On = $results->{conf}{highlight_on} || '<b>';
    my $Off = $results->{conf}{highlight_off} || '</b>';



    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.
    my @words = split /$wc_regexp/, $$text_ref;

    return 'No Content saved: Check StoreDescription setting' unless @words;

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

                $words[$pos] = "$begin$On$word$Off$end";


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
        for my $i ( 0 .. $Max_Words ) {
            last if $i >= $#words;
            push @output, $words[$i];
        }
    }
        
        

    push @output, $dotdotdot if !$printing;

    return join '', @output;


}

1;


__END__

=head1 NAME

swish.cgi -- Example Perl script for searching with the SWISH-E search engine.

=head1 DESCRIPTION

C<swish.cgi> is an example CGI script for searching with the SWISH-E search engine version 2.2 and above.
It returns results a page at a time, with matching words from the source document highlighted, showing a
few words of content on either side of the highlighted word.

This example does not require installation of additional Perl modules (from CPAN), unless you wish
to use stemming with your index.  In this case you will need the SWISH::Stemmer module, which is
included with the SWISH-E distribution (and also available on CPAN).  Instructions for installing
the module are included below.

A more advanced example script is also provided called C<swish2.cgi>.  That script uses a number of
perl modules for templating (separation of content from program logic)
and abstracting the interface with swish-e.

Due to the forking nature of this program and its use of signals,
this script probably will not run under Windows without some modifications.

This script can be run under mod_perl.  See below for details.

=head1 INSTALLATION

Installing a CGI application is dependent on your specific web server's configuration.
For this discussion we will assume you are using Apache in a typical configuration.  For example,
a common location for the DocumentRoot is C</usr/local/apache/htdocs>.  If you are installing this
on your shell account, your DocumentRoot might be C<~yourname/public_html>.

For the sake of this example we will assume the following:

    /usr/local/apache/htdocs        - Document root
    /usr/local/apache/cgi-bin       - CGI directory

=head2 Move the files to their locations

=over 4

=item Copy the swish.cgi file to your CGI directory

Most web servers have a directory where CGI programs are kept.  If this is the case on your
server copy the C<swish.cgi> perl script into that directory.  You will need to provide read
and execute permisssions to the file.  Exactly what permissions are needed again depends on
your specific configuration.  For example, under Unix:

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

To make things somewhat simple, the configuration parameters are included at the top of the program.
The parameters are all part of a C<hash> structure, and the comments at the top of the program should
get you going.  You will need to specify the location of the swish-e binary, your index file or files,
and a title.

For exampe:

    %CONFIG = (
        title           => 'Search the Swish-e list',   # Title of your choice.
        swish_binary    => './swish-e',                 # Location of swish-e binary
        swish_index     => '../index.swish-e',          # Location of your index file
    );

Or if searching more than one index:
    
    %CONFIG = (
        title           => 'Search the Swish-e list',
        swish_binary    => './swish-e',
        swish_index     => ['../index.swish-e', '../index2'],
    );


The examples above place the swish index file(s)
in the directory above the C<swish.cgi> CGI script.  If using the example paths above
of C</usr/local/apache/cgi-bin> for the CGI bin directory, that means that the index file
is in C</usr/local/apache>.  That places the index out of web space (e.g. cannot be accessed
via the web server), yet relative to where the C<swish.cgi> script is located.

There's more than one way to do it, of course.
One option is to place it in the same directory as the <swish.cgi> script, but
then be sure to use your web server's configuration to prohibit access to the index directly.

Another common option is to maintain a separate directory of the swish index files.  This decision is
up to you.

See the next section for more advanced C<swish.cgi> configurations.


=item 3 Create your index

You must index your web site before you can begin to use the C<swish.cgi> script.
Create a configuration file called C<swish.conf> in the directory where you will store
the index file.

This next example uses the file system to index your web documents.
In general, you will probably wish to I<spider> your web site if your web pages do not
map exactly to your file system, and to only index files available from links on you web
site.

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

    # Allow limiting search to titles and URLs.
    MetaNames swishdocpath swishtitle

    # Optionally use stemming for "fuzzy" searches
    #UseStemming yes

Now to index you simply run:

    swish-e -c swish.conf

The default index file C<index.swish-e> will be placed in the current directory.

Note that the above swish-e configuration defines two MetaNames "swishdocpath" and "swishtitle".
This allows searching just the document path or the title instead of the document's content.

Here's an expanded C<swish.cgi> configuration to make use of the above settings used while indexing:

    %CONFIG = (
        title           => 'Search the Apache documentation',
        swish_binary    => './swish-e',
        swish_index     => ['../index.swish-e', '../index2'],
        metanames       => [qw/swishdefault swishdocpath swishtitle/],
        display_props   => [qw/swishlastmodified swishdocsize swishdocpath/],

        name_labels => {
            swishdefault        => 'Default content',
            swishtitle          => 'Title',
            swishrank           => 'Rank',
            swishlastmodified   => 'Last Modified Date',
            swishdocpath        => 'Document Path',
            swishdocsize        => 'Document Size',
            ALL                 => 'Message text',
        },

    );

The above configuration defines metanames to use on the form (including the special "ALL" name which
tells the script to do a non-metaname search).  Searches can be limited to these metanames.

"display_props" tells the script to display the property "swishlastmodified" (the last modified
date of the file) with the search results.

The parameter "name_labels" is a hash
that is used to give friendly names to the metanames.

Swish-e can store part of all of the contents of the documents as they are indexed, and this
"document description" can be returned with search results.

    # Store the text of the documents within the swish index file
    StoreDescription HTML <body> 100000

Adding the above to your C<swish.conf> file tells swish-e to store up to 100,000 characters from the body of each document within the
swish-e index.  To display this information in search results, highlighting search terms,
use the follow configuration in C<swish.cgi>:

    %CONFIG = (
        title           => 'Search the Apache documentation',
        swish_binary    => './swish-e',
        swish_index     => ['../index.swish-e', '../index2'],
        metanames       => [qw/swishdefault swishdocpath swishtitle/],
        display_props   => [qw/swishlastmodified swishdocsize swishdocpath/],

        name_labels     => {
            swishdocpath        => 'URL path',
            swishtitle          => 'Document title',
            swishdefault        => '
            ALL                 => 'Document text',
            swishlastmodified   => 'Document last modified',
        },

        description_prop=> 'swishdescription',
        highlight_words => 1,
    );

Other C<swish.cgi> configuration settings are available.  Please see the example configuration
at the top of the CGI script.


=back

You should now be ready to run your search engine.  Point your browser to:

    http://www.myserver.name/cgi-bin/swish.cgi

=head1 MOD_PERL

This script can be run under MOD_PERL.  This will improve the response time of the
script compared to running under CGI.

Configuration is simple.  In your httpd.conf or your startup.pl file you need to
load the script.  For example, in httpd.conf you can use a perl section:

    <perl>
        use lib '/usr/local/apache/cgi-bin';
        require "swish.cgi";
    </perl>

This loads the script into mod_perl.  Then to configure the script to run:

    <location /search>
        allow from all
        SetHandler perl-script
        PerlHandler SwishSearch
    </location>

Unlike CGI, mod_perl does not change dir to the location of the perl module, so
your settings for the swish binary and the path to your index files must be absolute
paths (or relative to the server root).

Please post to the swish-e discussion list if you have any questions about running this
script under mod_perl.


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

Please use this CGI script at your own risk.

This script has been tested and used without problem, but you should still be aware that
any code running on your server represents a risk.  If you have any concerns please carefully
review the code.

See http://www.w3.org/Security/Faq/www-security-faq.html

=head1 SUPPORT

The SWISH-E discussion list is the place to ask for any help regarding SWISH-E or this example
script. See http://swish-e.org

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

