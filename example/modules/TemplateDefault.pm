#=====================================================================
# These routines format the HTML output.
#    $Id$
#=====================================================================
package TemplateDefault;
use strict;

use CGI;

sub show_template {
    my ( $class, $template_params, $results ) = @_;


    my $q = $results->CGI;

    my $output =  $q->header . page_header( $results );

    # Show form at top always
    $output .= show_form( $results );


    if ( $results->results ) {
        $output .=  results_header( $results );
        $output .=  show_result( $results, $_ ) for @{ $results->results };
    }

    # Form after results (or at top if no results)
    #$output .= show_form( $results );

    $output .= footer( $results );

    print $output;

}

#=====================================================================
# This generates the header

sub page_header {
    my $results = shift;
    my $title = $results->config('title') || 'Search our site with Swish-e';
    my $message = $results->errstr;

    $message = $message
        ? qq[<br><font color=red>$message</font>]
        : '' ;


    my $html_title = $results->results
        ? ( $results->navigation('hits')
            . ' Results for ['
            . CGI::escapeHTML( $results->{query_simple} )
            . ']'
           )

        : ( $results->errstr || $title );

    return <<EOF;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
    <head>
       <title>
          $html_title
       </title>
    </head>
    <body>
        <h2>
        <a href="http://swish-e.org">
        <img border="0" alt="Swish-e home page" src="http://swish-e.org/Images/swish-e.gif"></a> $title $message
        
        </h2>
EOF
}

#=====================================================================
# This generates the form
#
#   Pass:
#       $results hash

sub show_form {

    my $results = shift;
    my $q = $results->{q};


    my $query = $q->param('query') || '';

    $query = CGI::escapeHTML( $query );  # May contain quotes


    # Here's some form components
    
    my $meta_select_list    = get_meta_name_limits( $results );
    my $sorts               = get_sort_select_list( $results );
    my $select_index        = get_index_select_list( $results );
    my $limit_select        = get_limit_select( $results );
    
    my $date_ranges_select  = $results->get_date_ranges;


    my $form = $q->script_name;
    
    return <<EOF;
    <form method="post" action="$form" enctype="application/x-www-form-urlencoded" class="form">
        <input / maxlength="200" value="$query" size="32" type="text" name="query">
        <input / value="Search!" type="submit" name="submit"><br>

        $meta_select_list
        $sorts
        $select_index
        $limit_select
        $date_ranges_select
    </form>
EOF
}


#=====================================================================
# This routine creates the results header display
# and navigation bar
#
#
#

sub results_header {

    my $results = shift;
    my $config = $results->{config};
    my $q = $results->{q};



    my $swr = $results->header('removed stopwords');
    my $stopwords = '';


    if ( $swr && ref $swr eq 'ARRAY' ) {
        $stopwords = @$swr > 1
        ? join( ', ', map { "<b>$_</b>" } @$swr ) . ' are very common words and were not included in your search'
        : join( ', ', map { "<b>$_</b>" } @$swr ) . ' is a very common word and was not included in your search';
    }

    my $limits = '';

    #  Ok, this is ugly.


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

    my $query_href = $results->{query_href};
    my $query_simple = CGI::escapeHTML( $results->{query_simple} );
    my $pages       = $results->navigation('pages');

    my $prev        = $results->navigation('prev');
    my $prev_count  = $results->navigation('prev_count');
    my $next        = $results->navigation('next');
    my $next_count  = $results->navigation('next_count');
    
    my $hits        = $results->navigation('hits');
    my $from        = $results->navigation('from');
    my $to          = $results->navigation('to');
    
    my $run_time    = $results->navigation('run_time');
    my $search_time = $results->navigation('search_time');

    
    


    my $links = '';

    $links .= '<font size="-1" face="Geneva, Arial, Helvetica, San-Serif">&nbsp;Page:</font>' . $pages
        if $pages;

    $links .= qq[ <a href="$query_href&amp;start=$prev">Previous $prev_count</a>]
        if $prev_count;

    $links .= qq[ <a href="$query_href&amp;start=$next">Next $next_count</a>]
        if $next_count;


    # Save for the bottom of the screen.
    $results->{LINKS} = $links;

    $links = qq[<tr><td colspan=2 bgcolor="#EEEEEE">$links</td></tr>] if $links;

    $query_simple = $query_simple
        ? "&nbsp;Results for <b>$query_simple</b>"
        : '';
        
    

    return <<EOF;

    <table cellpadding=0 cellspacing=0 border=0 width="100%">
        <tr>
            <td height=20 bgcolor="#FF9999">
                <font size="-1" face="Geneva, Arial, Helvetica, San-Serif">
                $query_simple
                &nbsp; $from to $to of $hits results.
                </font>
            </td>
            <td align=right bgcolor="#FF9999">
                <font size="-2" face="Geneva, Arial, Helvetica, San-Serif">
                Run time: $run_time |
                Search time: $search_time &nbsp; &nbsp;
                </font>
            </td>
        </tr>

        $links
        $limits
        $stopwords

    </table>

EOF

}

#=====================================================================
# This routine formats a single result
#
#
sub show_result {
    my ($results, $this_result ) = @_;

    my $conf = $results->{conf};

    my $DocTitle = $results->config('title_property') || 'swishtitle';


    my $title = $this_result->{$DocTitle} || $this_result->{swishdocpath} || '?';

    my $name_labels = $results->config('name_labels');


                

    # The the properties to display
    
    my $props = '';

    my $display_props = $results->config('display_props');
    if ( $display_props ) {

        
        $props = join "\n",
            '<br><table cellpadding=0 cellspacing=0>',
            map ( {
                '<tr><td><small>'
                . ( $name_labels->{$_} || $_ )
                . ':</small></td><td><small> '
                . '<b>'
                . $this_result->{$_}
                . '</b>'
                . '</small></td></tr>'
                 }  @$display_props
            ),
            '</table>';
    }


    my $description_prop = $results->config('description_prop');
    
    my $description = '';
    if ( $description_prop ) {
        $description = $this_result->{ $description_prop } || '';
    }


    return <<EOF;
    <dl>
        <dt>$this_result->{swishreccount} <a href="$this_result->{swishdocpath_href}">$title</a> <small>-- rank: <b>$this_result->{swishrank}</b></small></dt>
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
               ? '<br><small>Response brought to you by <a href="http://perl.apache.org"><em>mod_perl</em></a></small>'
               : '';

    return <<EOF;

    <hr>
    <small>Powered by <em>Swish-e</em> <a href="http://swish-e.org">swish-e.org</a></small>
    $mod_perl

    <p>
        <a href="http://validator.w3.org/check/referer"><img border="0"
            src="http://www.w3.org/Icons/valid-html401"
            alt="Valid HTML 4.01!" height="31" width="88"></a>
    </p>
    </body>
</html>
EOF
}

#==================================================================
#  Form setup for sorts and metas
#
#  This could be methods of $results object
#  (and then available for Template-Toolkit)
#  But that's too much HTML in the object, perhaps.
#
#
#==================================================================

sub get_meta_name_limits {
    my ( $results ) = @_;

    my $metanames = $results->config('metanames');
    return '' unless $metanames;

    
    my $name_labels = $results->config('name_labels');
    my $q = $results->CGI;


    return join "\n",
        'Limit search to:',
        $q->radio_group(
            -name   =>'metaname',
            -values => $metanames,
            -default=>$metanames->[0],
            -labels =>$name_labels
        ),
        '<br>';
}

sub get_sort_select_list {
    my ( $results ) = @_;

    my $sort_metas = $results->config('sorts');
    return '' unless $sort_metas;

    
    my $name_labels = $results->config('name_labels');
    my $q = $results->CGI;



    return join "\n",
        'Sort by:',
        $q->popup_menu(
            -name   =>'sort',
            -values => $sort_metas,
            -default=>$sort_metas->[0],
            -labels =>$name_labels
        ),
        $q->checkbox(
            -name   => 'reverse',
            -label  => 'Reverse Sort'
        );
}



sub get_index_select_list {
    my ( $results ) = @_;
    my $q = $results->CGI;


    my $indexes = $results->config('swish_index');
    return '' unless ref $indexes eq 'ARRAY';

    my $select_config = $results->config('select_indexes');
    return '' unless $select_config && ref $select_config eq 'HASH';


    # Should return a warning, as this might be a likely mistake
    # This jumps through hoops so that real index file name is not exposed
    
    return '' unless exists $select_config->{labels}
              && ref $select_config->{labels} eq 'ARRAY'
              && @$indexes == @{$select_config->{labels}};


    my @labels = @{$select_config->{labels}};
    my %map;

    for ( 0..$#labels ) {
        $map{$_} = $labels[$_];
    }

    my $method = $select_config->{method} || 'checkbox_group';
    my @cols = $select_config->{columns} ? ('-columns', $select_config->{columns}) : ();

    return join "\n",
        '<br>',
        ( $select_config->{description} || 'Select: '),
        $q->$method(
        -name   => 'si',
        -values => [0..$#labels],
        -default=> 0,
        -labels => \%map,
        @cols );
}


sub get_limit_select {
    my ( $results ) = @_;
    my $q = $results->CGI;


    my $limit = $results->config('select_by_meta');
    return '' unless ref $limit eq 'HASH';

    my $method = $limit->{method} || 'checkbox_group';

    my @options = (
        -name   => 'sbm',
        -values => $limit->{values},
        -labels => $limit->{labels} || {},
    );

    push @options, ( -columns=> $limit->{columns} ) if $limit->{columns};
    

    return join "\n",
        '<br>',
        ( $limit->{description} || 'Select: '),
        $q->$method( @options );
}
1;

