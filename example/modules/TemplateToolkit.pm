#=======================================================================
#  Module for using Template-Toolkit for generating output
#    $Id$
#
#=======================================================================
package TemplateToolkit;
use strict;

use Template;
use vars '$Template';


sub show_template {
    my ( $class, $template_params, $results ) = @_;

    my $cgi = $results->CGI;


    #/* Cached if running under mod_perl */
    $Template ||= Template->new( $template_params->{options} );


    die $Template->error() unless $Template;


    print $cgi->header;

    my $subclass = TemplateToolkit::Helpers->new( $results );

    my $vars = {
        search  => $subclass,
        CGI     => $results->CGI,
    };

    $Template->process( $template_params->{file}, $vars )
	|| die "Template process failed for page '$template_params->{file}' ", $Template->error(), "\n";
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
package TemplateToolkit::Helpers;
use strict;

sub new {
    my ( $class, $results ) = @_;


    @TemplateToolkit::Helpers::ISA = ref $results;  # that doesn't look right.

    return bless $results, $class;
}

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

sub stopwords_removed {
    my $results = shift;
    
    my $swr = $results->header('removed stopwords');
    my $stopwords = '';


    if ( $swr && ref $swr eq 'ARRAY' ) {
        $stopwords = @$swr > 1
        ? join( ', ', map { "<b>$_</b>" } @$swr ) . ' are very common words and were not included in your search'
        : join( ', ', map { "<b>$_</b>" } @$swr ) . ' is a very common word and was not included in your search';
    }

    return $stopwords;
}


   
1;

