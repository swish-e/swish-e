#!/usr/local/bin/perl -w
package SwishSearch;
use strict;

use lib qw( modules );  ### This must be adjusted!


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


use CGI ();  # might not be needed if using Apache::Request


    # Run the script -- entry point if running as a CGI script
    
    handler() unless $ENV{MOD_PERL};



#==================================================================================

# This is written this way so the script can be used as a CGI script or a mod_perl
# module without any code changes.

# All this does is pass a hash of configuration settings to the process_request() function.
# You may either set the hash as shown below, or perhaps read it from a file.
# If running under mod_perl you might want to use a PerlSetVar to specify which or what
# config setting to use (or where to load the config settings from disk).  That way this
# same mod_perl "module" could be used for searching different indexes (perhaps all with
# different templates for a unique look).  Remember to specify absolute paths under mod_perl.

# Configuration is a perl hash.  Note that the settings are separated by a comma.

# Notes:  need to carefully consider what needs to be HTML escaped
#         also, no filtering is done on the query.  May wish to remove '=' for metanames

sub handler {
    my $r = shift;


    # Taint issues
    $ENV{PATH} = '/usr/bin';   # For taint checking

    
    ##### Configuration Parameters #########


    #---- Here's a minimal configuration example for use with a default index.
    # You will probably want to use more options than these.

    my %small_config = (
        swish_binary    => '/usr/local/bin/swish-e',
        swish_index     => '/usr/local/share/swish/index.swish-e',
        title_property  => 'swishtitle',  # Not required, but recommended
    );        

    # And then you would run the script like this:
    # process_request( \%small_config );
        


    #---- This lists all the options, with many commented out ---
    # By default, this config is used -- see the process_request() call below.
    
    # You should adjust for your site, and how your swish index was created.
    # Please don't post this entire section on the swish-e list if looking for help,
    # rather send a small example, without all the comments.

    # Items beginning with an "x" or "#" are commented out
    
    my %CONFIG = (
        title           => 'Search our site',  # Title of your choice.
        swish_binary    => './swish-e',        # Location of swish-e binary


        # The location of your index file.  Typically, this would not be in
        # your web tree.
        # If you have more than one index to search then specify an array
        # reference.  e.g. swish_index =>[ qw/ index1 index2 index3 /],
        
        swish_index     => 'index.swish-e',  # Location of your index file
                                                
                                               # See "select_indexes" below for how to
                                               # select more than one index.

        page_size       => 15,                 # Number of results per page  - default 15


        # Property name to use as the main link text to the indexed document.
        # Typically, this will be 'swishtitle' if have indexed html documents,
        # But you can specify any PropertyName defined in your document.
        # By default, swish will return the pathname for documents that do not
        # have a title.

        title_property => 'swishtitle',



        # prepend this path to the filename (swishdocpath) returned by swish.  This is used to
        # make the href link back to the original document.  Comment out to disable.

        prepend_path    => 'http://localhost/mydocs',


        # Swish has a configuration directive "StoreDescription" that will save part or
        # all of a document's contents in the index file.  This can then be displayed
        # along with results.  If you are indexing a lot of files this can use a lot of disk
        # space, so test carefully before indexing your entire site.
        # Building swish with zlib can greatly reduce the space used by StoreDescription
        #
        # This settings tells this script to display this description.
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

        sorts           => [qw/swishrank swishlastmodified swishtitle swishdocpath/],


        # Secondary_sort is used to sort within a sort
        # You may enter a property name followed by a direction (asc|desc)

        secondary_sort  => [qw/swishlastmodified desc/],





        # You can limit by MetaNames here.  Names listed here will be displayed in
        # a line of radio buttons.
        # The default is to not allow any metaname selection.
        # To use this feature you must define MetaNames while indexing.

        # The special "swishdefault" says to search any text that was not indexed
        # as a metaname (e.g. the body of a HTML document).

        # To see how this might work, add to your config file:
        #   MetaNames swishtitle swishdocpath
        # and try:
        metanames       => [qw/swishdefault swishtitle swishdocpath/],
        
        

        # Another example: if you indexed an email archive
        # that defined the metanames subject name email (as in the swish-e discussion archive)
        # you might use:
        #metanames       => [qw/body subject name email/],

        # Note that you can do a real "all" search if you use nested metanames in your source documents.
        # Nesting metanames is most common with XML documents.
        

        # These are used to map MetaNames and PropertyNames to user-friendly names
        # on the form.

        name_labels => {
            swishdefault        => 'Title & Body',
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


        timeout         => 10,    # limit time used by swish when fetching results - DoS protection.



        # These settings will use some crude highlighting code to highlight search terms in the
        # property specified above as the description_prop (normally, 'swishdescription').


        max_chars       => 500,   # If "highlight" is not defined, then just truncate the description to this many *chars*.
                                  # If you want to go by *words*, enable highlighting,
                                  # and then comment-out show_words.  It will be a little slower.


        # This structure defines term highlighting, and what type of highlighting to use
        highlight       => {

            # Pick highlighting module -- you must make sure the module can be found

            # Ok speed, but doesn't handle phrases.
            #Deals with stemming, but not stopwords
            #package         => 'DefaultHighlight', 

            # Somewhat slow, but deals with phases, stopwords, and stemming.
            # Takes into consideration WordCharacters, IgnoreFirstChars and IgnoreLastChars.
            package         => 'PhraseHighlight',

            # Fast: phrases without regard to wordcharacter settings
            # doesn't do context display, so must match in first X words,
            # doesn't handle stemming or stopwords.
            #package         => 'SimpleHighlight',  

            show_words      => 10,    # Number of swish words words to show around highlighted word
            max_words       => 100,   # If no words are found to highlighted then show this many words
            occurrences     => 6,     # Limit number of occurrences of highlighted words
            #highlight_on   => '<b>', # HTML highlighting codes
            #highlight_off  => '</b>',
            highlight_on    => '<font style="background:#FFFF99">',
            highlight_off   => '</font>',
            meta_to_prop_map => {   # this maps search metatags to display properties
                swishdefault    => [ qw/swishtitle swishdescription/ ],
                swishtitle      => [ qw/swishtitle/ ],
                swishdocpath    => [ qw/swishdocpath/ ],
            },
        },



        # If you specify more than one index file (as an array reference) you
        # can set this allow selection of which indexes to search.
        # The default is to search all indexes specified if this is not used.
        # When used, the first index is the default index.

        # You need to specify your indexes as an array reference: 
        #swish_index     => [ qw/ index.swish-e index.other index2.other index3.other index4.other / ], 

        Xselect_indexes  => {
            #method  => 'radio_group',  # pico radio_group, popup_menu, or checkbox_group
            method  => 'checkbox_group',
            #method => 'popup_menu',
            columns => 3,
            labels  => [ 'Main Index', 'Other Index', qw/ two three four/ ],  # Must match up one-to-one
            description => 'Select Site: ',
        },


        # Similar to select_indexes, this adds a metaname search
        # based on a metaname.  You can use any metaname, and this will
        # add an "AND" search to limit results to a subset of your records.
        # i.e. it adds something like  'site=(foo or bar or baz)' if foo, bar, and baz were selected.

        # Swish-e's ExtractPath would work well with this.  For example, the apache docs:
        # ExtractPath site regex !^/usr/local/apache/htdocs/manual/([^/]+)/.+$!$1!
        # ExtractPathDefault site other
        

        Xselect_by_meta  => {
            #method      => 'radio_group',  # pico radio_group, popup_menu, or checkbox_group
            method      => 'checkbox_group',
            #method      => 'popup_menu',
            columns     => 3,
            metaname    => 'site',     # Can't be a metaname used elsewhere!
            values      => [qw/misc mod vhosts other/],
            labels  => {
                misc    => 'General Apache docs',
                mod     => 'Apache Modules',
                vhosts  => 'Virutal hosts',
            },
            description => 'Limit search to these areas: ',
        },
                                              



        # The 'template' setting defines what generates the output
        # The default is "TemplateDefault" which is reasonably ugly.
        # Note that some of the above options may not be available
        # for templating, as it's up to you do layout the form
        # and results in your template.
        

        xtemplate => {
            package     => 'TemplateDefault',
        },

        xtemplate => {
            package     => 'TemplateDumper',
        },

        xtemplate => {
            package         => 'TemplateToolkit',
            file            => 'search.tt',
            options         => {
                INCLUDE_PATH    => '/data/_g/lii/swish-e/example',
                #PRE_PROCESS     => 'config',
            },
        },

        xtemplate => {
            package         => 'TemplateHTMLTemplate',
            options         => {
                filename            => 'swish.tmpl',
                die_on_bad_params   => 0,
                loop_context_vars   => 1,
                cache               => 1,
            },
        },


        # This defines the package object for reading CGI parameters
        # Defaults to CGI.  Might be useful with mod_perl.
        # request_package     => 'CGI',
        # request_package     => 'Apache::Request',





        # Limit to date ranges



        # This adds in the date_range limiting options
        # You will need the DateRanges.pm module from the author to use that feature

        # Noramlly, you will want to limit by the last modified date, so specify
        # "swishlastmodified" as the property_name.  If indexing a mail archive, and, for
        # example, you store the date (a unix timestamp) as "date" then specify
        # "date" as the property_name.

        Xdate_ranges     => {
            property_name   => 'swishlastmodified',      # property name to limit by

            # what you specify here depends on the DateRanges.pm module.
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




    # Now run the request

    process_request( \%CONFIG );

    return Apache::Constants::OK() if $ENV{MOD_PERL};
}    

#============================================================================
#
#   This is the main entry point, where a config hash is passed in.
#
#============================================================================

sub process_request {
    my $conf = shift;  # configuration parameters

    # create search object
    my $search = SwishQuery->new(
        config    => $conf,
        request   => ($conf->{request_package} ? $conf->{request_package}->new : CGI->new),
    );


    # run the query
    my $results = $search->run_query;  # currently, results is the just the $search object

    my $template = $conf->{template} || { package => 'TemplateDefault' };

    my $package = $template->{package};

    my $file = "$package.pm";
    $file =~ s[::][/]g;

    require $file;
    $package->show_template( $template, $results );
}





#==================================================================================================
package SwishQuery;
#==================================================================================================

use Carp;

#--------------------------------------------------------------------------------
# new() doesn't do much, just create the object
#--------------------------------------------------------------------------------
sub new {
    my $class = shift;
    my %options = @_;

    my $conf = $options{config};

    croak "Failed to set the swish index files in config setting 'swish_index'" unless $conf->{swish_index};
    croak "Failed to specify 'swish_binary' in configuration" unless $conf->{swish_binary};

    # initialize the request search hash
    my $sh = {
       prog         => $conf->{swish_binary},
       config       => $conf,
       q            => $options{request},
       hits         => 0,
       MOD_PERL     => $ENV{MOD_PERL},
    };

    return bless $sh, $class;
}


sub hits { shift->{hits} }

sub config {
    my ($self, $setting, $value ) = @_;

    croak "Failed to pass 'config' a setting" unless $setting;

    my $cur = $self->{config}{$setting} if exists $self->{config}{$setting};

    $self->{config}{$setting} = $value if $value;

    return $cur;
}

sub header {
    my $self = shift;
    return unless ref $self->{_headers} eq 'HASH';
    
    return $self->{_headers}{$_[0]} || '';
}


# return a ref to an array
sub results {
    my $self = shift;
    return $self->{_results} || undef;
}

sub navigation {
    my $self = shift;
    return unless ref $self->{navigation} eq 'HASH';
    
    return exists $self->{navigation}{$_[0]} ? $self->{navigation}{$_[0]} : '';
}

sub CGI { $_[0]->{q} };
    



sub swish_command {

    my $self = shift;

    unless ( @_ ) {
        return $self->{swish_command} ? @{$self->{swish_command}} : undef;
    }

    push @{$self->{swish_command}}, @_;
}


sub errstr {
    my ($self, $value ) = @_;


    $self->{_errstr} = $value if $value;

    return $self->{_errstr} || '';
}




    

#============================================
# This returns "$self" just in case we want to seperate out into two objects later


sub run_query {

    my $self = shift;

    my $q = $self->{q};
    my $conf = $self->{config};



    # Sets the query string, and any -L limits.
    return $self unless $self->build_query;


   
    # Set the starting position (which is offset by one)

    my $start = $q->param('start') || 0;
    $start = 0 unless $start =~ /^\d+$/ && $start >= 0;

    $self->swish_command( '-b', $start+1 );

    

    # Set the max hits

    my $page_size = $self->config('page_size') || 15;
    $self->swish_command( '-m', $page_size );


    return $self unless $self->set_index_file;



    # Set the sort option, if any
    return $self unless $self->set_sort_order;



    # Trap the call - not portable.

    my $timeout = $self->config('timeout');

    if ( $timeout ) {
        eval {
            local $SIG{ALRM} = sub { die "Timed out\n" };
            alarm ( $self->config('timeout') || 5 );
            $self->run_swish;
            alarm 0;
        };

        if ( $@ ) {
            $self->errstr( $@ );
            return $self;
        }
    } else {
        $self->run_swish;
    }



    my $hits = $self->hits;
    return $self unless $hits;



    # Build href for repeated search via GET (forward, backward links)


    my @query_string =
         map { "$_=" . $q->escape( $q->param($_) ) }
            grep { $q->param($_) }  qw/query metaname sort reverse/;


    for my $p ( qw/si sbm/ ) {
        my @settings = $q->param($p);
        next unless @settings;
        push @query_string,  "$p=" . $q->escape( $_ ) for @settings;
    }

    


    if ( $conf->{date_ranges} ) {
        my $dr = DateRanges::GetDateRangeArgs( $q );
        push @query_string, $dr, if $dr;
    }


    $self->{query_href} = $q->script_name . '?' . join '&amp;', @query_string;



    # Return the template fields

    $self->{my_url} = $q->script_name;

    $self->{hits} = $hits;
        
    $self->{navigation}  = {
            showing     => $hits,
            from        => $start + 1,
            to          => $start + $hits,
            hits        => $self->header('number of hits') ||  0,
            run_time    => $self->header('run time') ||  'unknown',
            search_time => $self->header('search time') ||  'unknown',
    };


    $self->set_page ( $page_size );

    return $self;
    
}        


#============================================================
# Build a query string from swish
# Just builds the -w string
#------------------------------------------------------------

sub build_query {
    my $self = shift;

    my $q = $self->{q};
    

    # set up the query string to pass to swish.
    my $query = $q->param('query') || '';

    for ( $query ) {  # trim the query string
        s/\s+$//;
        s/^\s+//;
    }

    $self->{query_simple} = $query;    # without metaname
    $q->param('query', $query );  # clean up the query, if needed.
    

    # Read in the date limits, if any.  This can create a new query
    return unless $self->get_date_limits( \$query );


    unless ( $query ) {
        $self->errstr('Please enter a query string') if $q->param('submit');
        return;
    }
    if ( length( $query ) > 100 ) {
        $self->errstr('Please enter a shorter query');
        return;
    }



    # Adjust the query string for metaname search
    # *Everything* is a metaname search
    # Might also like to allow searching more than one metaname at the same time

    my $metaname = $q->param('metaname') || 'swishdefault';


    # make sure it's a valid metaname

    my $conf = $self->{config};
    my @metas = ('swishdefault');
    push @metas, @{ $self->config('metanames')} if $self->config('metanames');
    unless ( grep { $metaname eq $_ } @metas  ) {
        $self->errstr('Bad MetaName provided');
        return;
    }

    # prepend metaname to query
    $query = $metaname . "=($query)";

    # save the metaname so we know what field to highlight
    $self->{metaname} = $metaname;


    ## Look for a "limit" metaname -- perhaps used with ExtractPath
    # Here we don't worry about user supplied data

    my $limits = $self->config('select_by_meta');
    my @limits = $q->param('sbm');  # Select By Metaname


    # Note that this could be messed up by ending the query in a NOT or OR
    # Should look into doing:
    # $query = "( $query ) AND " . $limits->{metaname} . '=(' . join( ' OR ', @limits ) . ')';
    if ( @limits && ref $limits eq 'HASH' && $limits->{metaname} ) {
        $query .= ' and ' . $limits->{metaname} . '=(' . join( ' or ', @limits ) . ')';
    }


    $self->swish_command('-w', $query );

    return 1;
}

#========================================================================
#  Get the index files from the form, or from simple the config settings
#------------------------------------------------------------------------

sub set_index_file {
    my $self = shift;

    my $q = $self->CGI;
    
    # Set the index file
    
    if ( $self->config('select_indexes') && ref $self->config('swish_index') eq 'ARRAY'  ) {

        my @choices = $q->param('si');
        if ( !@choices ) {
            $self->errstr('Please select a source to search');
            return;
        }

        my @indexes = @{$self->config('swish_index')};


        my @selected_indexes = grep {/^\d+$/ && $_ >= 0 && $_ < @indexes } @choices;

        if ( !@selected_indexes ) {
            $self->errstr('Invalid source selected');
            return $self;
        }
        $self->swish_command( '-f', @indexes[ @selected_indexes ] );


    } else {
        my $indexes = $self->config('swish_index');        
        $self->swish_command( '-f', ref $indexes ? @$indexes : $indexes );
    }

    return 1;
}

#================================================================================
#   Parse out the date limits from the form or from GET request
#
#---------------------------------------------------------------------------------

sub get_date_limits {

    my ( $self, $query_ref ) = @_;

    my $conf = $self->{config};

    # Are date ranges enabled?
    return 1 unless $conf->{date_ranges};


    eval { require DateRanges };
    if ( $@ ) {
        $self->errstr( $@ );
        delete $conf->{date_ranges};
        return;
    }
    
    my $q = $self->{q};

    my %limits;

    unless ( DateRanges::DateRangeParse( $q, \%limits ) ) {
        $self->errstr( $limits{DateRanges_error} || 'Bad date range selection' );
        return;
    }

    # Store the values for later
    
    $self->{DateRanges_time_low} = $limits{DateRanges_time_low};
    $self->{DateRanges_time_high} = $limits{DateRanges_time_high};


    # Allow searchs just be date if not "All dates" search
    # $$$ should place some limits here, and provide a switch to disable
    if ( !$$query_ref && $limits{DateRanges_time_high} ) {
        $$query_ref = 'not skaisikdeekk';
        $self->{_search_all}++;  # flag
    }


    my $limit_prop = $conf->{date_ranges}{property_name} || 'swishlastmodified';


    if ( $limits{DateRanges_time_low} && $limits{DateRanges_time_high} ) {
        $self->swish_command( '-L', $limit_prop, $limits{DateRanges_time_low}, $limits{DateRanges_time_high} );
    }

    return 1;
}



#================================================================
#  Set the sort order
#  Just builds the -s string
#----------------------------------------------------------------

sub set_sort_order {
    my $self = shift;

    my $q = $self->{q};

    my $sorts_array = $self->config('sorts');
    return 1 unless $sorts_array;


    my $conf = $self->{config};


    # Now set sort option - if a valid option submitted (or you could let swish-e return the error).
    my %sorts = map { $_, 1 } @$sorts_array;

    if ( $q->param('sort') && $sorts{ $q->param('sort') } ) {

        my $direction = $q->param('sort') eq 'swishrank'
            ? $q->param('reverse') ? 'asc' : 'desc'
            : $q->param('reverse') ? 'desc' : 'asc';
                
        $self->swish_command( '-s', $q->param('sort'), $direction );

        if ( $conf->{secondary_sort} && $q->param('sort') ne $conf->{secondary_sort}[0] ) {
                $self->swish_command(ref $conf->{secondary_sort} ? @{ $conf->{secondary_sort} } : $conf->{secondary_sort} );
        }

    } else {
        $self->errstr( 'Invalid Sort Option Selected' );
        return;
    }

    return 1;
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

    my ( $self, $Page_Size ) = @_;

    my $q = $self->{q};

    my $navigation = $self->{navigation};
    
        
    my $start = $navigation->{from} - 1;   # Current starting record
    
        
    my $prev = $start - $Page_Size;
    $prev = 0 if $prev < 0;

    if ( $prev < $start ) {
        $navigation->{prev} = $prev;
        $navigation->{prev_count} = $start - $prev;
    }

    
    my $last = $navigation->{hits} - 1;

    
    my $next = $start + $Page_Size;
    $next = $last if $next > $last;
    my $cur_end   = $start + $self->{hits} - 1;
    if ( $next > $cur_end ) {
        $navigation->{next} = $next;
        $navigation->{next_count} = $next + $Page_Size > $last
                                ? $last - $next + 1
                                : $Page_Size;
    }


    # Calculate pages  ( is this -1 correct here? )
    
    my $pages = int (($navigation->{hits} -1) / $Page_Size);
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

    
        $navigation->{pages} =
            join ' ', map {
                my $page_start = $_ * $Page_Size;
                my $page = $_ + 1;
                $page_start == $start
                ? $page
                : qq[<a href="$self->{query_href}&amp;start=$page_start">$page</a>];
                        } @pages;
    }

}

#==================================================
# Format and return the date range options in HTML
#
#--------------------------------------------------
sub get_date_ranges {

    my $self = shift;

    my $q = $self->{q};
    my $conf = $self->{config};

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

use Symbol;

sub run_swish {


    my $self = shift;

    my $results = $self->{results};
    my $conf    = $self->{config};
    my $q       = $self->{q};



    
    my @properties;
    my %seen;

    # Gather up the properties specified
    
    for ( qw/ title_property description_prop display_props / ) {
        push @properties, ref $conf->{$_} ? @{$conf->{$_}} : $conf->{$_}
            if $conf->{$_} && !$seen{$_}++;
    }

    # Add in the default props 
    for ( qw/swishrank swishdocpath/ ) {
        push @properties, $_ unless $seen{$_};
    }

    
    # add in the default prop - a number must be first (this might be a duplicate in -x, oh well)
    @properties = ( 'swishreccount', @properties );

    $self->swish_command( -x => join( '\t', map { "<$_>" } @properties ) . '\n' );
    $self->swish_command( -H => 9 );


    # Run swish 
    my $fh = gensym;
    my $pid = open( $fh, '-|' );

    die "Failed to fork: $!\n" unless defined $pid;

    if ( !$pid ) {  # in child
        exec $self->{prog},  $self->swish_command or die "Failed to exec '$self->{prog}' Error:$!";
    }

    $self->{COMMAND} = join ' ', $self->{prog},  $self->swish_command;


    # read in from child


    my @results;
    
    my $trim_prop = $self->config('description_prop');

    my $highlight = $self->config('highlight');
    my $highlight_object;

    # Loop through values returned from swish.

    my %stops_removed;
    
    while (<$fh>) {

        chomp;

        # This will not work correctly with multiple indexes when different values are used.
        if ( /^# ([^:]+):\s+(.+)$/ ) {
            my $h = lc $1;
            my $value = $2;
            $self->{_headers}{$h} = $value;

            push @{$self->{_headers}{'removed stopwords'}}, $value if $h eq 'removed stopword' && !$stops_removed{$value}++;

            next;
        }


        # return errors as text
        $self->errstr($1), return if /^err:\s*(.+)/;


        # Found a result
        if ( /^\d/ ) {

            my %h;
            @h{@properties} = split /\t/;
            push @results, \%h;

            # There's a chance that the docpath could be modified by highlighting
            # when used in a "display_props".
            $h{saved_swishdocpath} = $h{swishdocpath};

            my $docpath = $h{swishdocpath};
            $docpath =~ s/\s/%20/g;  # Replace spaces
            $h{swishdocpath_href} = ( $self->config('prepend_path') || '' ) . $docpath;

            



            # Now do any formatting
            if ( $highlight ) {
                if ( !$highlight_object ) {
                    my $package = $highlight->{package} || 'DefaultHighlight';

                    eval { require "$package.pm" };
                    if ( $@ ) {
                        $self->errstr( $@ );
                        $highlight = '';
                        next;
                    } else {
                        $highlight_object = $package->new( $self, $self->{metaname} );
                    }
                }

                # Highlight any fields, as needed
                $highlight_object->highlight( \%h  );

                next;
            }




            # Trim down the description if no highlight, or if highlighting some other property
            # Not very nice.  The highlighting code would limit by words

            if ( $trim_prop && $h{$trim_prop} ) {
                my $max = $conf->{max_chars} || 500;

                if ( length $h{$trim_prop} > $max ) {
                    $h{$trim_prop} = substr( $h{$trim_prop}, 0, $max) . ' <b>...</b>';
                }
            }
    
        }

        # Might check for "\n." for end of results.

        
    }

    $self->{hits} = @results;
    $self->{_results} = \@results if @results;
        
}


#=====================================================================================
# This method parses out the query from the "Parsed words" returned by swish
# for use in highlighting routines
# This returns a hash ref:
#  $query->{text}  # evertying is currently at level "text"
#                {$metaname}  # the meta name
#                           [ array of phrases ]
#                    each phrase is made up of an array of words





use constant DEBUG_QUERY_PARSED => 0;

sub extract_query_match {
    my $self = shift;

    my $query = $self->header('parsed words');  # grab query parsed by swish

    

    my %query_match;  # kewords broken down by layer and field.
    $self->{query_match} = \%query_match;


    # Loop through the query

    while ( $query =~ /([a-z]+)\s+=\s+(.+?)(?=\s+[a-z]+\s+=|$)/g ) {

        my ( $field, $words ) = ( $1, $2 );


        my $inquotes;
        my $buffer;
        my %single_words;

        my $layer = 'text';  # This might be used in the future to highlight tags when matching a href.

        # Expand group searches -- not currently used
        my @fields = ( $field );


        for my $word ( split /\s+/, $words ) {


            # XXX This list of swish operators could change "and or not" and is dependent on stopwords.
            # remove control words and parens
            next if !$inquotes && $word =~ /^(and|or|not|\(|\))$/; 

            $buffer = [] unless $inquotes;  # is there a better way to allocate memory like this?

            if ( $word eq '"' ) {
                unless ( $inquotes ) {
                    $inquotes++;
                    next;
                } else {
                    $inquotes = 0;
                }

            } else {
        
                push @$buffer, $word;
            }

        
            next if $inquotes;


            # Only record single words once (this will probably break something)
            # Reason: to reduce the number of matches must check
            next if @$buffer == 1 && $single_words{ $buffer->[0] }++;

        
            push @{$query_match{$layer}{$_}}, $buffer foreach @fields;


        }
    }



    # Now, sort in desending order of phrase lenght


    foreach my $layer ( keys %query_match ) {
        print STDERR "         LAYER: $layer\n" if DEBUG_QUERY_PARSED;


        foreach my $tag ( keys %{$query_match{$layer}} ) {

            @{$query_match{$layer}{$tag}} = sort { @$b <=> @$a } @{$query_match{$layer}{$tag}};


            if ( DEBUG_QUERY_PARSED ) {
                print STDERR "            TAG: '$tag'\n";
                print STDERR "                 : '@$_'\n" foreach @{$query_match{$layer}{$tag}};
            }
        }
    }


    return \%query_match;
}


1;


__END__

=head1 NAME

swish.cgi -- Example Perl script for searching with the SWISH-E search engine.

=head1 DESCRIPTION

C<swish.cgi> is an example CGI script for searching with the SWISH-E search engine version 2.2 and above.
It returns results a page at a time, with matching words from the source document highlighted, showing a
few words of content on either side of the highlighted word.

The script is modular in design.  Both the highlighting code and output generation is handled by modules, which
are included in the F<example/modules> directory.  This allows for easy customization of the output without
changing the main CGI script.  A module exists to generate standard HTML output.  There's also modules and
template examples to use with the popular templating systems HTML::Template and Template-Toolkit.  This allows
you to tightly integrate this script with the look of your site.

This scipt can also run basically unmodified as a mod_perl handler, providing much better performance than
running as a CGI script.

Due to the forking nature of this program and its use of signals,
this script probably will not run under Windows without some modifications.
There's plan to change this soon.


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

=item Copy the modules directory

This is optional, but the script needs to find additional modules.  Unlike CPAN modules that need to
be uncompressed, built, and installed, all you need to do is make sure the modules are some place where
the web server can read them.  You may decide to leave them where you uncompressed the swish-e distribution,
or you may wish to move them to your perl library.

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

=item 2 Set the perl library path

The script must find the modules that the script is distributed with.  These modules handle
the highlighting of the search terms, and the output generation.  Again, where you place the
modules is up to you, and the only requirement is that the web server can access those files.

You tell perl the location of the modules with the "use lib" directive.  The default for this script is:

    use lib qw( modules );

This says to look for the modules in the F<modules> directory of the current directory.

For example, say you want to leave the modules where you unpacked the swish-e distribution.  If
you unpacked in your home directory of F</home/yourname/swish-e> then you must add this to the
script:

    use lib qw( /home/yourname/swish-e/example/modules );

    

=item 3 Set the configuration parameters

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

If running under mod_perl you must specify absolute paths.

There's more than one way to do it, of course.
One option is to place the index in the same directory as the <swish.cgi> script, but
then be sure to use your web server's configuration to prohibit access to the index directly.

Another common option is to maintain a separate directory of the swish index files.  This decision is
up to you.

See the next section for more advanced C<swish.cgi> configurations.


=item 4 Create your index

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
        swish_index     => 'index.swish-e',
        metanames       => [qw/swishdefault swishdocpath swishtitle/],
        display_props   => [qw/swishlastmodified swishdocsize swishdocpath/],
        title_property  => 'swishtitle',  # Not required, but recommended

        name_labels => {
            swishdefault        => 'Body & Title',
            swishtitle          => 'Title',
            swishrank           => 'Rank',
            swishlastmodified   => 'Last Modified Date',
            swishdocpath        => 'Document Path',
            swishdocsize        => 'Document Size',
        },

    );

The above configuration defines metanames to use on the form.
Searches can be limited to these metanames.

"display_props" tells the script to display the property "swishlastmodified" (the last modified
date of the file), the document size, and path with the search results.

The parameter "name_labels" is a hash (reference)
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
        swish_index     => 'index.swish-e',
        metanames       => [qw/swishdefault swishdocpath swishtitle/],
        display_props   => [qw/swishlastmodified swishdocsize swishdocpath/],
        title_property  => 'swishtitle',  # Not required, but recommended
        description_prop=> 'swishdescription',

        name_labels => {
            swishdefault        => 'Body & Title',
            swishtitle          => 'Title',
            swishrank           => 'Rank',
            swishlastmodified   => 'Last Modified Date',
            swishdocpath        => 'Document Path',
            swishdocsize        => 'Document Size',
        },
        highlight       => {
            package         => 'PhraseHighlight',
            meta_to_prop_map => {   # this maps search metatags to display properties
                swishdefault    => [ qw/swishtitle swishdescription/ ],
                swishtitle      => [ qw/swishtitle/ ],
                swishdocpath    => [ qw/swishdocpath/ ],
            },
       }

    );


Other C<swish.cgi> configuration settings are available, and are listed at the top of the F<swish.cgi>
script.  


=back

You should now be ready to run your search engine.  Point your browser to:

    http://www.myserver.name/cgi-bin/swish.cgi

=head1 MOD_PERL

This script can be run under mod_perl (see http://perl.apache.org).
This will improve the response time of the script compared to running under CGI.

Configuration is simple.  In your httpd.conf or your startup.pl file you need to
load the script.  For example, in httpd.conf you can use a perl section:

    <perl>
        use lib '/usr/local/apache/cgi-bin';
        use lib '/home/yourname/swish-e/example/modules';
        require "swish.cgi";
    </perl>

Again, note that the paths used will depend on where you installed the script and the modules.
When running under mod_perl the swish.cgi script becomes a perl module, and therefore the script
does not need to be installed in the cgi-bin directory.  (But, you can actually use the same script as
both a CGI script and a mod_perl module at the same time, read from the same location.)

The above loads the script into mod_perl.  Then to configure the script to run add this to your httpd.conf
configuration file:

    <location /search>
        allow from all
        SetHandler perl-script
        PerlHandler SwishSearch
    </location>

Unlike CGI, mod_perl does not change the current directory to the location of the perl module, so
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

    > ./swish.cgi | head
    Content-Type: text/html; charset=ISO-8859-1

    <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
    <html>
        <head>
           <title>
              Search our site
           </title>
        </head>
        <body>

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
(But, don't use the http input method -- use the -S prog method shown below.)

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
script. See http://swish-e.org.

Before posting please review:

    http://swish-e.org/2.2/docs/INSTALL.html#When_posting_please_provide_the_

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


