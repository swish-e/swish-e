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
use lib qw/modules/;
use CGI;




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
        swish_binary    => './swish-e',
        swish_index     => 'index.swish-e',
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
        
        swish_index     => './index.swish-e',  # Location of your index file
                                                
                                               # See "select_indexes" below for how to
                                               # select more than one index.

        page_size       => 15,                 # Number of results per page  - default 15


        # Property name to use as the main link text to the indexed document.
        # Typically, this will be 'swishtitle' if have indexed html documents,
        # But you can specify any PropertyName defined in your document.
        # By default, swish will return the pathname for documents that do not
        # have a title.

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
            swishtitle          => 'Title only',
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
        

        template => {
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

    require "$package.pm";
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
       q            => $options{request}
    };

    return bless $sh, $class;
}



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


sub results {
    my $self = shift;
    return ref $self->{_results} eq 'ARRAY' ? @{$self->{_results}} : ();
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

    my $hits;

    my $timeout = $self->config('timeout');

    if ( $timeout ) {
        eval {
            local $SIG{ALRM} = sub { die "Timed out\n" };
            alarm ( $self->config('timeout') || 5 );
            $hits = $self->run_swish;
            alarm 0;
        };

        if ( $@ ) {
            $self->errstr( $@ );
            return $self;
        }
    } else {
        $hits = $self->run_swish;
    }


    ## These can be done in run_swish!
    
    # Returned an error string
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
    $self->{MOD_PERL} = $ENV{MOD_PERL};
        
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

    my %display_props = map { $_, 1 } @properties;

    # add in the default props - a number must be first
    @properties = (qw/swishreccount swishrank swishdocpath/, @properties );

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
    $self->{_results} = \@results;

    
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
            $h{saved_swishdocpath} = $h{swishdocpath} if exists $h{swishdocpath};



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

    return scalar @results;
        
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


