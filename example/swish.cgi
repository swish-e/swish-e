#!/usr/local/bin/perl -w
package SwishSearch;
use strict;

use lib qw( modules );  ### This may need to be adjusted!
                        ### It should point to the location of the
                        ### associated script modules directory


$SwishSearch::DEBUG_BASIC       = 1;  # Show command used to run swish
$SwishSearch::DEBUG_COMMAND     = 2;  # Show command used to run swish
$SwishSearch::DEBUG_HEADERS     = 4;  # Swish output headers
$SwishSearch::DEBUG_OUTPUT      = 8;  # Swish output besides headers
$SwishSearch::DEBUG_SUMMARY     = 16;  # Summary of results parsed
$SwishSearch::DEBUG_DUMP_DATA   = 32;  # dump data that is sent to templating modules




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
#    Ok, piped opens.
#
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

# This is written this way so the script can be used as a CGI script or a mod_perl
# module without any code changes.

# use CGI ();  # might not be needed if using Apache::Request

#=================================================================================
#   CGI entry point
#
#=================================================================================



# Run the script -- entry point if running as a CGI script

    unless ( $ENV{MOD_PERL} ) {
        my $config = default_config();

        # Merge with disk config file.
        $config = merge_read_config( $config );
        process_request( $config );
    }


#=================================================================================
#   mod_perl entry point
#
#   As an example, you might use a PerlSetVar to point to paths to different
#   config files, and then cache the different configurations by path.
#
#=================================================================================

my %cached_configs;

sub handler {
    my $r = shift;

    if ( my $config_path = $r->dir_config( 'Swish_Conf_File' ) ) {

        # Already cached?
        if ( $cached_configs{ $config_path } ) {
            process_request( $cached_configs{ $config_path } );
            return Apache::Constants::OK();
        }

        # Else, load config
        my $config = default_config();
        $config->{config_file} = $config_path;

        # Merge with disk config file.
        $cached_configs{ $config_path } = merge_read_config( $config );

        process_request( $cached_configs{ $config_path } );
        return Apache::Constants::OK();
    }


    # Otherwise, use hard-coded config
    process_request( default_config() );

    return Apache::Constants::OK();

}



#==================================================================================
#   This sets the default configuration parameters
#
#   Any configuration read from disk is merged with these settings.
#
#   Only a few settings are actually required.  Some reasonable defaults are used
#   for most.  If fact, you can probably create a complete config as:
#
#    return = {
#        swish_binary    => '/usr/local/bin/swish-e',
#        swish_index     => '/usr/local/share/swish/index.swish-e',
#        title_property  => 'swishtitle',  # Not required, but recommended
#    };
#
#   But, that doesn't really show all the options.
#
#   You can modify the options below, or you can use a config file.  The config file
#   is .swishcgi.conf by default (read from the current directory) that must return
#   a hash reference.  For example, to create a config file that changes the default
#   title and index file name, plus uses Template::Toolkit to generate output
#   create a config file as:
#
#       # Example config file -- returns a hash reference
#       {
#           title           => 'Search Our Site',
#           swish_index     => 'index.web',
#
#           template => {
#            package         => 'TemplateToolkit',
#            file            => 'search.tt',
#            options         => {
#                INCLUDE_PATH    => '/home/user/swish-e/example',
#            },
#       };
#
#
#-----------------------------------------------------------------------------------

sub default_config {


    
    ##### Configuration Parameters #########

    #---- This lists all the options, with many commented out ---
    # By default, this config is used -- see the process_request() call below.
    
    # You should adjust for your site, and how your swish index was created.

    ##>>
    ##>>  Please don't post this entire section on the swish-e list if looking for help!
    ##>>
    ##>>  Send a small example, without all the comments.

    #======================================================================
    # NOTE: Items beginning with an "x" or "#" are commented out
    #       the "x" form simply renames (hides) that setting.  It's used
    #       to make it easy to disable a mult-line configuation setting.
    #
    #   If you do not understand a setting then best to leave the default.
    #
    #   Please follow the documentation (perldoc swish.cgi) and set up
    #   a test using the defaults before making changes.  It's much easier
    #   to modify a working example than to try to get a modified example to work ;)
    #
    #   Again, this is a Perl hash structure.  Commas are important.
    #======================================================================
    
    return {
        title           => 'Search our site',  # Title of your choice.  Displays on the search page
        swish_binary    => './swish-e',        # Location of swish-e binary


        # By default, this script tries to read a config file.  You should probably
        # comment this out if not used save a disk stat
        config_file     => '.swishcgi.conf',    # Default config file


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
        # In other words, this is used for the text of the links of the search results.
        #  <a href="prepend_path/swishdocpath">title_property</a>

        title_property => 'swishtitle',



        # prepend this path to the filename (swishdocpath) returned by swish.  This is used to
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
            #method  => 'radio_group',  # pick radio_group, popup_menu, or checkbox_group
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
            #method      => 'radio_group',  # pick: radio_group, popup_menu, or checkbox_group
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
                INCLUDE_PATH    => '/home/user/swish-e/example',
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



        # The "on_intranet" setting is just a flag that can be used to say you do
        # not have an external internet connection.  It's here because the default
        # page generation includes links to images on swish-e.or and on www.w3.org.
        # If this is set to one then those images will not be shown.
        # (This only effects the default ouput module TemplateDefault)

        on_intranet => 0,



        # Here you can hard-code debugging options.  The will help you find
        # where you made your mistake ;)
        # Using all at once will generate a lot of messages to STDERR
        # Please see the documentation before using these.
        # Typically, you will set these from the command line instead of in the configuration.
        
        # debug_options => 'basic, command, headers, output, summary, dump',



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

        date_ranges     => {
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

    };

}

#============================================================================
#   Read config settings from disk, and merge
#   Note, all errors are ignored since by default this script looks for a
#   config file.
#
#============================================================================
sub merge_read_config {
    my $config = shift;

    set_debug($config);  # get from config or from %ENV

    return $config unless $config->{config_file};

    my $return = do $config->{config_file};

    return $config unless ref $return eq 'HASH';

    if ( $config->{debug} || $return->{debug} ) {
        require Data::Dumper;
        print STDERR "\n---------- Read config parameters from '$config->{config_file}' ------\n",
              Data::Dumper::Dumper($return),
              "-------------------------\n";
    }

    set_debug( $return );
        

    # Merge settings
    return { %$config, %$return };
}


#---------------------------------------------------------------------------------------------------
sub set_debug {
    my $conf = shift;

    unless ( $ENV{SWISH_DEBUG} ||$conf->{debug_options} ) {
        $conf->{debug} = 0;
        return;
    }
    
    my %debug = (
        basic       => [$SwishSearch::DEBUG_BASIC,   'Basic debugging'],
        command     => [$SwishSearch::DEBUG_COMMAND, 'Show command used to run swish'],
        headers     => [$SwishSearch::DEBUG_HEADERS, 'Show headers returned from swish'],
        output      => [$SwishSearch::DEBUG_OUTPUT,  'Show output from swish'],
        summary     => [$SwishSearch::DEBUG_SUMMARY, 'Show summary of results'],
        dump        => [$SwishSearch::DEBUG_DUMP_DATA, 'Show all data available to templates'],
    );


    $conf->{debug} = 1;

    for ( split /\s*,\s*/, $ENV{SWISH_DEBUG} ) {
        if ( exists $debug{ lc $_ } ) {
            $conf->{debug} |= $debug{ lc $_ }->[0];
            next;
        }

        print STDERR "Unknown debug option '$_'.  Must be one of:\n",
             join( "\n", map { sprintf('  %10s: %10s', $_, $debug{$_}->[1]) } sort { $debug{$a}->[0] <=> $debug{$b}->[0] }keys %debug),
             "\n\n";
        exit;
    }

    print STDERR "Debug level set to: $conf->{debug}\n";
}
        

#============================================================================
#
#   This is the main entry point, where a config hash is passed in.
#
#============================================================================

sub process_request {
    my $conf = shift;  # configuration parameters

    # Use CGI.pm by default
    my $request_package = $conf->{request_package} || 'CGI';
    $request_package =~ s[::][/]g;
    require "$request_package.pm";

    my $request_object = $conf->{request_package} ? $conf->{request_package}->new : CGI->new;

    if ( $conf->{debug} ) {
        print STDERR 'Enter a query [all]: ';
        my $query = <STDIN>;
        $query =~ tr/\r//d;
        chomp $query;
        unless ( $query ) {
            print STDERR "Using 'not asdfghjklzxcv' to match all records\n";
            $query = 'not asdfghjklzxcv';
        }

        $request_object->param('query', $query );

        print STDERR 'Enter max results to display [1]: ';
        my $max = <STDIN>;
        chomp $max;
        $max = 1 unless $max && $max =~/^\d+$/;

        $conf->{page_size} = $max;
    }
        


    # create search object
    my $search = SwishQuery->new(
        config    => $conf,
        request   => $request_object,
    );


    # run the query
    my $results = $search->run_query;  # currently, results is the just the $search object

    if ( $conf->{debug} ) {
        if ( $conf->{debug} & $SwishSearch::DEBUG_DUMP_DATA ) {
            require Data::Dumper;
            print STDERR "\n------------- Results structure passed to template ------------\n",
                  Data::Dumper::Dumper( $results ),
                  "--------------------------\n";
        } elsif ( $conf->{debug} & $SwishSearch::DEBUG_SUMMARY ) {
            print STDERR "\n------------- Results Summary ------------\n";
            if ( $results->{hits} ) {
                require Data::Dumper;
                print STDERR "Showing $results->{navigation}{showing} of $results->{navigation}{hits}\n",
                    Data::Dumper::Dumper( $results->{_results} );
            } else {
                print STDERR "** NO RESULTS **\n";
            }

            print STDERR "--------------------------\n";
        } else {
            print STDERR ( ($results->{hits} ? "Found $results->{hits} results\n" : "Failed to find any results\n" . $results->errstr . "\n" ),"\n" );
        }
    }
    
    

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
# Or use this instead -- PLEASE see perldoc CGI::Carp for details
# use CGI::Carp qw(fatalsToBrowser warningsToBrowser);

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



    my $timeout = $self->config('timeout') || 0;

    eval {
        local $SIG{ALRM} = sub { die "Timed out\n" };
        alarm $timeout if $timeout && $^O !~ /Win32/i;
        $self->run_swish;
        alarm 0  unless $^O =~ /Win32/i;
        waitpid $self->{pid}, 0 if $self->{pid};  # for IPC::Open2
    };

    if ( $@ ) {
        print STDERR "\n*** $$ Failed to run swish: '$@' ***\n" if $conf->{debug};
        $self->errstr( $@ );
        return $self;
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
        print STDERR "\n------ Can't use DateRanges feature ------------\n",
                     "\nScript will run, but you can't use the date range feature\n", 
                     $@,
                     "\n--------------\n" if $conf->{debug};
            
        delete $conf->{date_ranges};
        return 1;
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

    my $sortby =  $q->param('sort') || 'swishrank';

    if ( $sortby && $sorts{ $sortby } ) {

        my $direction = $sortby eq 'swishrank'
            ? $q->param('reverse') ? 'asc' : 'desc'
            : $q->param('reverse') ? 'desc' : 'asc';
                
        $self->swish_command( '-s', $sortby, $direction );

        if ( $conf->{secondary_sort} && $sortby ne $conf->{secondary_sort}[0] ) {
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

    my $fh = $^O =~ /Win32/i
             ? windows_fork( $conf, $self )
             : real_fork( $conf, $self );


    $self->{COMMAND} = join ' ', $self->{prog},  $self->swish_command;


    # read in from child


    my @results;
    
    my $trim_prop = $self->config('description_prop');

    my $highlight = $self->config('highlight');
    my $highlight_object;

    # Loop through values returned from swish.

    my %stops_removed;

    my $unknown_output = '';


    while (<$fh>) {

        chomp;
        tr/\r//d;

        # This will not work correctly with multiple indexes when different values are used.
        if ( /^# ([^:]+):\s+(.+)$/ ) {

            print STDERR "$_\n" if $conf->{debug} & $SwishSearch::DEBUG_HEADERS;

            my $h = lc $1;
            my $value = $2;
            $self->{_headers}{$h} = $value;

            push @{$self->{_headers}{'removed stopwords'}}, $value if $h eq 'removed stopword' && !$stops_removed{$value}++;

            next;
        } elsif ( $conf->{debug} & $SwishSearch::DEBUG_OUTPUT ) {
            print STDERR "$_\n";
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

            next;
    
        } elsif ( /^\.$/ ) {
            last;

        } else {
            next if /^#/;
        }

        $unknown_output .= "'$_'\n";



        
    }

    die "Swish returned unknown output: $unknown_output\n" if $unknown_output;

    $self->{hits} = @results;
    $self->{_results} = \@results if @results;
        
}

#==================================================================
# Run swish-e by forking
#

use Symbol;

sub real_fork {
    my ( $conf, $self ) = @_;


    # Run swish 
    my $fh = gensym;
    my $pid = open( $fh, '-|' );

    die "Failed to fork: $!\n" unless defined $pid;

     

    if ( !$pid ) {  # in child
        if ( $conf->{debug} & $SwishSearch::DEBUG_COMMAND ) {
            print STDERR "---- Running swish with the following command and parameters ----\n";
            print STDERR join( "  \\\n", map { /[^\/.\-\w\d]/ ? qq['$_'] : $_ }  $self->{prog}, $self->swish_command );
            print STDERR "\n-----------------------------------------------\n";
        }


        unless ( exec $self->{prog},  $self->swish_command ) {
            warn "Child process Failed to exec '$self->{prog}' Error: $!";
            exit;
        }
    }

    return $fh;
}


#=====================================================================================
#   Windows work around
#   from perldoc perlfok -- na, that doesn't work.  Try IPC::Open2
#
sub windows_fork {
    my ( $conf, $self ) = @_;

    if ( $conf->{debug} & $SwishSearch::DEBUG_COMMAND ) {
        print STDERR "---- Running swish with the following command and parameters ----\n";
        print STDERR join( ' ', map { /[^.\-\w\d]/ ? qq["$_"] : $_ } map { s/"/\\"/g; $_ }  $self->{prog}, $self->swish_command );
        print STDERR "\n-----------------------------------------------\n";
    }
    

    require IPC::Open2;
    my ( $rdrfh, $wtrfh );

    # Ok, I'll say it.  Windows sucks.
    my @command = map { s/"/\\"/g; $_ }  $self->{prog}, $self->swish_command;
    my $pid = IPC::Open2::open2($rdrfh, $wtrfh, @command );


    $self->{pid} = $pid;

    return $rdrfh;
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

C<swish.cgi> is a CGI script for searching with the SWISH-E search engine version 2.1-dev and above.
It returns results a page at a time, with matching words from the source document highlighted, showing a
few words of content on either side of the highlighted word.

The script is highly configurable; you can search multiple (or selectable) indexes, limit searches to
part of the index, allow sorting by a number of different properties, limit results to a date range, and so on.

The standard configuration (i.e. not using a config file) should work with most swish index files.
Customization of the parameters will be
needed if you are indexing special meta data and want to search and/or display the meta data.  The
configuration can be modified by editing this script directly, or by using a configuration file (.swishcgi.conf
by default).

You are stongly encourraged to get the default configuration working before making changes.  Most problems
using this script are the result of configuration modifcations.

The script is modular in design.  Both the highlighting code and output generation is handled by modules, which
are included in the F<example/modules> directory.  This allows for easy customization of the output without
changing the main CGI script.  A module exists to generate standard HTML output.  There's also modules and
template examples to use with the popular Perl templating systems HTML::Template and Template-Toolkit.  This allows
you to tightly integrate this script with the look of an existing template-driven web site.

This scipt can also run basically unmodified as a mod_perl handler, providing much better performance than
running as a CGI script.

Please read the rest of the documentation.  There's a C<DEBUGGING> section, and a C<FAQ> section.

This script should work on Windows, but security may be an issue.

=head1 REQUIREMENTS

You should be running a reasonably current version of Perl.  5.00503 or above is recommended (anything older
will not be supported).

If you wish to use the date range feature you will need to install the Date::Calc module.  This is available
from http://search.cpan.org.


=head1 INSTALLATION

Here's an example installation session.  Please get a simple installation working before modifying the
configuration file.  Most problems reported for using this script have been due to inproper configuration.

The script's default settings are setup for initial testing.  By default the settings expect to find
most files and the swish-e binary in the same directory as the script.

For I<security> reasons, once you have tested the script you will want to change settings to limit access
to some of these files by the web server
(either by moving them out of web space, or using access control such as F<.htaccess>).
An example of using F<.htaccess> on Apache is given below.

It's expected that you have already unpacked the swish-e distribution
and built the swish-e binary (if using a source distribution).

Below is a (unix) session where we create a directory, move required files into this directory, adjust
permissions, index some documents, and symlink into the web server.

=over 4

=item 1 Move required files into their own directory.

This assumes that swish-e was unpacked and build in the ~/swish-e directory.    

    ~ >mkdir swishdir
    ~ >cd swishdir
    ~/swishdir >cp ~/swish-e/example/swish.cgi .
    ~/swishdir >cp -rp ~/swish-e/example/modules .
    ~/swishdir >cp ~/swish-e/src/swish-e .
    ~/swishdir >chmod 755 swish.cgi
    ~/swishdir >chmod 644 modules/*


=item 2 Create an index

This step you will create a simple configuration file.  In this example the Apache documentation
is indexed.  Last we run a simple query to test swish.

    ~/swishdir >cat swish.conf            
    IndexDir /usr/local/apache/htdocs
    IndexOnly .html .htm
    DefaultContents HTML
    StoreDescription HTML <body> 200000
    MetaNames swishdocpath swishtitle

    ~/swishdir >./swish-e -c swish.conf   
    Indexing Data Source: "File-System"
    Indexing "/usr/local/apache/htdocs"
    Removing very common words...
    no words removed.
    Writing main index...
    Sorting words ...
    Sorting 7005 words alphabetically
    Writing header ...
    Writing index entries ...
      Writing word text: Complete
      Writing word hash: Complete
      Writing word data: Complete
    7005 unique words indexed.
    5 properties sorted.                                              
    124 files indexed.  1485844 total bytes.  171704 total words.
    Elapsed time: 00:00:02 CPU time: 00:00:02
    Indexing done!

Now, verify that the index can be searched:

    ~/swishdir >./swish-e -w install -m 1
    # SWISH format: 2.1-dev-25
    # Search words: install
    # Number of hits: 14
    # Search time: 0.001 seconds
    # Run time: 0.040 seconds
    1000 /usr/local/apache/htdocs/manual/dso.html "Apache 1.3 Dynamic Shared Object (DSO) support" 17341
    .

Let's see what files we have in our directory now:

    ~/swishdir >ls -1 -F
    index.swish-e
    index.swish-e.prop
    modules/
    swish-e*
    swish.cgi*
    swish.conf

=item 3 Test the CGI script

This is a simple step, but often overlooked.  You should test from the command line instead of jumpping
ahead and testing with the web server.  See the C<DEBUGGING> section below for more information.

    ~/swishdir >./swish.cgi | head
    Content-Type: text/html; charset=ISO-8859-1

    <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
    <html>
        <head>
           <title>
              Search our site
           </title>
        </head>
        <body>

The above shows that the script can be run directly, and generates a correct HTTP header and HTML.

If you run the above and see something like this:

    ~/swishdir >./swish.cgi
    bash: ./swish.cgi: No such file or directory

then you probably need to edit the script to point to the correct location of your perl program.
Here's one way to find out where perl is located (again, on unix):

    ~/swishdir >which perl
    /usr/local/bin/perl

    ~/swishdir >/usr/local/bin/perl -v   
    This is perl, v5.6.0 built for i586-linux
    ...

Good! We are using a reasonably current version of perl.  You should be running
at least perl 5.005 (5.00503 really).  You will may have problems otherwise.

Now that we know perl is at F</usr/local/bin/perl> we can adjust the "shebang" line
in the perl script (e.g. the first line of the script):

    ~/swishdir >pico swish.cgi
    (edit the #! line)
    ~/swishdir >head -1 swish.cgi
    #!/usr/local/bin/perl -w

=item 4 Test with your web server

How you do this is completely dependent on your web server, and you may need to talk to your web
server admin to get this working.  Often files with the .cgi extension are automatically set up to
run as CGI scripts, but not always.  In other words, this step is really up to you to figure out!

First, I create a symlink in Apache's document root to point to my test directory "swishdir".  This will work
because I know my Apache server is configured to follow symbolic links.

    ~/swishdir >su -c 'ln -s /home/bill/swishdir /usr/local/apache/htdocs/swishdir'
    Password: *********

If your account is on an ISP and your web directory is F<~/public_html> the you might just move the entire
directory:

    mv ~/swishdir ~/public_html

Now, let's make a real HTTP request.  I happen to have Apache setup on a local port:

    ~/swishdir >GET http://localhost:8000/swishdir/swish.cgi | head -3
    #!/usr/local/bin/perl -w
    package SwishSearch;
    use strict;

Oh, darn. It looks like Apache is not running the script and instead returning it as a
static page.  I need to tell Apache that swish.cgi is a CGI script.

In my case F<.htaccess> comes to the rescue:

    ~/swishdir >cat .htaccess 

    # Deny everything by default
    Deny From All

    # But allow just CGI script
    <files swish.cgi>
        Options ExecCGI
        Allow From All
        SetHandler cgi-script
    </files>

Let's try the request one more time:    

    ~/swishdir >GET http://localhost:8000/swishdir/swish.cgi | head
    <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
    <html>
        <head>
           <title>
              Search our site
           </title>
        </head>
        <body>
            <h2>
            <a href="http://swish-e.org">

That looks better!  Now use your web browser to test.

Make sure you look at your web server's error log file while testing the script.

BTW - "GET" is a program included with Perl's LWP library.  If you do no have this you might
try something like:

    wget -O - http://localhost:8000/swishdir/swish.cgi | head

and if nothing else, you can always telnet to the web server and make a basic request.

    ~/swishtest > telnet localhost 8000
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    GET /swishtest/swish.cgi http/1.0

    HTTP/1.1 200 OK
    Date: Wed, 13 Feb 2002 20:14:31 GMT
    Server: Apache/1.3.20 (Unix) mod_perl/1.25_01
    Connection: close
    Content-Type: text/html; charset=ISO-8859-1

    <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
    <html>
        <head>
           <title>
              Search our site
           </title>
        </head>
        <body>

This may seem like a lot of work compared to using a browser, but browsers
are a poor tool for basic CGI debugging.
        

=back

If you have problems check the C<DEBUGGING> section below.

=head1 CONFIGURATION

If you want to change the location of the swish-e binary or the index index file, use multiple indexes, add additional metanames and properties,
change the default highlighting behavior, etc., you will need to adjust the script's configuration settings.

Please get a test setup working with the default parameters before making changes to any configuration settings.
Better to debug one thing at a time...

In general, you will need to adjust the script's settings to match the index file you are searching.  For example,
if you are indexing a hypermail list archive you may want to make the script
use metanames/properties of Subject, Author, and, Email address.  Or you may wish to provide a way to limit
searches to parts of your index file (e.g. parts of your directory tree).

To make things somewhat "simple", the configuration parameters are included near the top of the swish.cgi program.
That is the only place that the individual parameters are defined and explained, so you will need to open up
the swish.cgi script in an editor to view the options.  Further questions about individual settings should
be refered to the swish-e discussion list.

The parameters are all part of a perl C<hash> structure, and the comments at the top of the program should
get you going.  The perl hash structure may seem a bit confusing, but it makes it easy to create nested and complex
parameters. 

You have two options for changing the configuration settings from their default values:
you may edit the script directly, or you may use a configuration file.  In either case, the configuration
settings are a basic perl hash reference.

Using a configuration file is described below, but contains the same hash structure.

There are many configuation settings, and some of them are commented out either by using
a "#" symbol, or by simply renaming the configuration directive (e.g. by adding an "x" to the parameter
name).

A very basic configuration setup might look like:

    return {
        title           => 'Search the Swish-e list',   # Title of your choice.
        swish_binary    => './swish-e',                 # Location of swish-e binary
        swish_index     => 'index.swish-e',             # Location of your index file
    };

Or if searching more than one index:
    
    return {
        title           => 'Search the Swish-e list',
        swish_binary    => './swish-e',
        swish_index     => ['index.swish-e', 'index2'],
    };

Both of these examples return a reference to a perl hash ( C<return {...}> ).  In the second example,
the multiple index files are set as an array reference.

Note that in the example above the swish-e binary file is relative to the current directory.
If running under mod_perl you will typically need to use absolute paths.

B<Using A Configuration File>

As mentioned above, you can either edit the F<swish.cgi> script directly and modify the configuration settings, or
use an external configuration file.  The settings in the configuration file are merged with (override)
the settings defined in the script.

The advantage of using a configuration script is that you are not editing the swish.cgi script directly, and
downloading a new version won't mean re-editing the cgi script.  Also, if running under mod_perl you can use the same
script loaed into Apache to manage many different search pages.

By default, the script will attempt to read from the file F<.swishcgi.conf>.
For example, you might only wish to change the title used
in the script.  Simply create a file called F<.swishcgi.conf> in the same directory as the CGI script:

    > cat .swishcgi.conf
    # Example swish.cgi configuration script.
    return {
       title => 'Search Our Mailing List Archive',
    };

The settings you use will depend on the index you create with swish.  Here's a basic configuation:

   return {
        title           => 'Search the Apache documentation',
        swish_binary    => './swish-e',
        swish_index     => 'index.swish-e',
        metanames       => [qw/swishdefault swishdocpath swishtitle/],
        display_props   => [qw/swishtitle swishlastmodified swishdocsize swishdocpath/],
        title_property  => 'swishdocpath',
        prepend_path    => 'http://myhost/apachedocs', 

        name_labels => {
            swishdefault        => 'Search All',
            swishtitle          => 'Title',
            swishrank           => 'Rank',
            swishlastmodified   => 'Last Modified Date',
            swishdocpath        => 'Document Path',
            swishdocsize        => 'Document Size',
        },

    };

The above configuration defines metanames to use on the form.
Searches can be limited to these metanames.

"display_props" tells the script to display the property "swishlastmodified" (the last modified
date of the file), the document size, and path with the search results.

The parameter "name_labels" is a hash (reference)
that is used to give friendly names to the metanames.

Here's another example.  Say you want to search either (or both) the Apache 1.3 documentation or the
Apache 2.0 documentation:

    return {
       title       => 'Search the Apache Documentation',
       date_ranges => 0,
       swish_index => [ qw/ index.apache index.apache2 / ],
       select_indexes  => {
            method  => 'checkbox_group',
            labels  => [ '1.3.23 docs', '2.0 docs' ],  # Must match up one-to-one to swish_index
            description => 'Select: ',              
        },

    };

Now you can select either or both sets of documentation while searching.    


Please refer to the default configuration settings near the top of the script for details on
the available settings.

=head1 DEBUGGING

Most problems with using this script have been a result of improper configuation.  Please
get the script working with default settings before adjusting the configuration settings.

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

If you don't have access to the web server's error_log file, you can modify the script to report
errors to the browser screen.  Open the script and search for "CGI::Carp".  (Author's suggestion is
to debug from the command line -- adding the browser and web server into the equation only complicates
debugging.)

The script does offer some basic debugging options that allow debugging from the command line.
The debugging options are enabled by setting
an environment variable "SWISH_DEBUG".  How that is set depends on your operating system and the
shell you are using.  These examples are using the "bash" shell syntax.

Note:  You can also use the "debug_options" configuation setting, but the recommended method
is to set the environment variable.

You can list the available debugging options like this:

    >SWISH_DEBUG=help ./swish.cgi >outfile
    Unknown debug option 'help'.  Must be one of:
           basic: Basic debugging
         command: Show command used to run swish
         headers: Show headers returned from swish
          output: Show output from swish
         summary: Show summary of results
            dump: Show all data available to templates

As you work yourself down the list you will get more detail output.  You can combine
options like:

    >SWISH_DEBUG=command,headers,summary ./swish.cgi >outfile

You will be asked for an input query and the max number of results to return.  You can use the defaults
in most cases.  It's a good idea to redirect output to a file.  Any error messages are sent to stderr, so
those will still be displayed (unless you redirect stderr, too).

Here are some examples:

    ~/swishtest >SWISH_DEBUG=basic ./swish.cgi >outfile
    Debug level set to: 1
    Enter a query [all]: 
    Using 'not asdfghjklzxcv' to match all records
    Enter max results to display [1]: 

    ------ Can't use DateRanges feature ------------

    Script will run, but you can't use the date range feature
    Can't locate Date/Calc.pm in @INC (@INC contains: modules /usr/local/lib/perl5/5.6.0/i586-linux /usr/local/lib/perl5/5.6.0 /usr/local/lib/perl5/site_perl/5.6.0/i586-linux /usr/local/lib/perl5/site_perl/5.6.0 /usr/local/lib/perl5/site_perl/5.005/i586-linux /usr/local/lib/perl5/site_perl/5.005 /usr/local/lib/perl5/site_perl .) at modules/DateRanges.pm line 107, <STDIN> line 2.
    BEGIN failed--compilation aborted at modules/DateRanges.pm line 107, <STDIN> line 2.
    Compilation failed in require at ./swish.cgi line 971, <STDIN> line 2.

    --------------
    Can't exec "./swish-e": No such file or directory at ./swish.cgi line 1245, <STDIN> line 2.
    Child process Failed to exec './swish-e' Error: No such file or directory at ./swish.cgi line 1246, <STDIN> line 2.
    Failed to find any results

The above told me about two problems.  First, it's telling me that the Date::Calc module is not installed.
The Date::Calc module is needed to use the date limiting feature of the script.

The second problem is a bit more serious.  It's saying that the script can't find the swish-e binary file.
I simply forgot to copy it.

    ~/swishtest >cp ~/swish-e/src/swish-e .
    ~/swishtest >cat .swishcgi.conf
        return {
           title       => 'Search the Apache Documentation',
           date_ranges => 0,
        };

Now, let's try again:

    ~/swishtest >SWISH_DEBUG=basic ./swish.cgi >outfile
    Debug level set to: 1

    ---------- Read config parameters from '.swishcgi.conf' ------
    $VAR1 = {
              'date_ranges' => 0,
              'title' => 'Search the Apache Documentation'
            };
    -------------------------
    Enter a query [all]: 
    Using 'not asdfghjklzxcv' to match all records
    Enter max results to display [1]: 
    Found 1 results

    Can't locate TemplateDefault.pm in @INC (@INC contains: modules /usr/local/lib/perl5/5.6.0/i586-linux /usr/local/lib/perl5/5.6.0 /usr/local/lib/perl5/site_perl/5.6.0/i586-linux /usr/local/lib/perl5/site_perl/5.6.0 /usr/local/lib/perl5/site_perl/5.005/i586-linux /usr/local/lib/perl5/site_perl/5.005 /usr/local/lib/perl5/site_perl .) at ./swish.cgi line 608.

Bother.  I fixed the first two problems, but now there's this new error.  Oh, I somehow forgot to
copy the modules directory.  The obvious way to fix that is to copy the directory.  But, there may
be times where you want to put the module directory in another location.  So, let's modify the
F<.swishcgi.conf> file and add a "use lib" setting:

    ~/swishtest >cat .swishcgi.conf
    use lib '/home/bill/swish-e/example/modules';

    return {
       title       => 'Search the Apache Documentation',
       date_ranges => 0,
    };

    ~/swishtest >SWISH_DEBUG=basic ./swish.cgi >outfile
    Debug level set to: 1

    ---------- Read config parameters from '.swishcgi.conf' ------
    $VAR1 = {
              'date_ranges' => 0,
              'title' => 'Search the Apache Documentation'
            };
    -------------------------
    Enter a query [all]: 
    Using 'not asdfghjklzxcv' to match all records
    Enter max results to display [1]: 
    Found 1 results

Now were talking.

Here's a common problem.  Everything checks out, but when you run the script you see the message:

    Swish returned unknown output

Ok, let's find out what output it is returning:

    ~/swishtest >SWISH_DEBUG=headers,output ./swish.cgi >outfile
    Debug level set to: 13

    ---------- Read config parameters from '.swishcgi.conf' ------
    $VAR1 = {
              'swish_binary' => '/usr/local/bin/swish-e',
              'date_ranges' => 0,
              'title' => 'Search the Apache Documentation'
            };
    -------------------------
    Enter a query [all]: 
    Using 'not asdfghjklzxcv' to match all records
    Enter max results to display [1]: 
      usage: swish [-i dir file ... ] [-S system] [-c file] [-f file] [-l] [-v (num)]
      ...
    version: 2.0
       docs: http://sunsite.berkeley.edu/SWISH-E/

    *** 9872 Failed to run swish: 'Swish returned unknown output' ***
    Failed to find any results

Oh, looks like /usr/local/bin/swish-e is version 2.0 of swish.  We need 2.1-dev and above!

=head1 FAQ

Here's some common questions and answers.

=head2 How do I change the way the output looks?

The script uses a module to generate ouput.  By default it uses the TemplateDefault.pm module.
The module used can be selected in the configuration file.

If you want to make simple changes you can edit the TemplatDefault.pm module directly.  If you want to
copy a module, you must also change the "package" statement at the top of the module.  For example:

    cp TempateDefault.pm MyTemplateDefault.pm

Then at the top of the module adjust the "package" line to:

    package MyTemplateDefault;

If you are designing a new site, or if your site is already created with one of the perl templating systems,
then you can use one of the modules that uses templates to generate the output.

The template modules are passed a hash with the results from the search, plus other data use to create the
output page.  You can see this hash by using the debugging option "dump" or by using the TemplateDumper.pm
module:

    ~/swishtest >cat .swishcgi.conf 
        return {
           title       => 'Search the Apache Documentation',
           template => {
                package     => 'TemplateDumper',
            },
        };

And run a query.  For example:

    http://localhost:8000/swishtest/swish.cgi?query=install

Three are three highlighting modules included.  Each is a trade-off of speed vs. accuracy:

    DefaultHighlight.pm - reasonably fast, but does not highlight phrases
    PhraseHighlight.pm  - reasonably slow, but is reasonably accurate
    SimpleHighlight.pm  - fast, some phrases, but least accurate

Eh, the default is actually "PhraseHighlight.pm".  Oh well.

Optimizations to these modules are welcome!

=head2 My ISP doesn't provide access to the web server logs

There are a number of options.  One way it to use the CGI::Carp module.  Search in the
swish.cgi script for:

    use Carp;
    # Or use this instead -- PLEASE see perldoc CGI::Carp for details
    # use CGI::Carp qw(fatalsToBrowser warningsToBrowser);

And change it to look like:    

    #use Carp;
    # Or use this instead -- PLEASE see perldoc CGI::Carp for details
    use CGI::Carp qw(fatalsToBrowser warningsToBrowser);

This should be only for debugging purposes, as if used in production you may end up sending
quite ugly and confusing messages to your browsers.

=head2 Why does the output show (NULL)?

The most common reason is that you did not use StoreDescription in your config file while indexing.

    StoreDescription HTML <body> 200000

That tells swish to store the first 200,000 characters of text extracted from the body of each document parsed
by the HTML parser.  The text is stored as property "swishdescription".  Running:

    ~/swishtest > ./swish-e -T index_metanames

will display the properties defined in your index file.    

This can happen with other properties, too.
For example, this will happen when you are asking for a property to display that is not defined in swish.

    ~/swishtest > ./swish-e -w install -m 1 -p foo   
    # SWISH format: 2.1-dev-25
    # Search words: install
    err: Unknown Display property name "foo"
    .

    ~/swishtest > ./swish-e -w install -m 1 -x 'Property foo=<foo>\n'
    # SWISH format: 2.1-dev-25
    # Search words: install
    # Number of hits: 14
    # Search time: 0.000 seconds
    # Run time: 0.038 seconds
    Property foo=(NULL)
    .

To check that a property exists in your index you can run:

    ~/swishtest > ./swish-e -w not dkdk -T index_metanames | grep foo
            foo : id=10 type=70  META_PROP:STRING(case:ignore) *presorted*

Ok, in this case we see that "foo" is really defined as a property.  Now let's make sure F<swish.cgi>
is asking for "foo" (sorry for the long lines):

    ~/swishtest > SWISH_DEBUG=command ./swish.cgi > /dev/null
    Debug level set to: 3
    Enter a query [all]: 
    Using 'not asdfghjklzxcv' to match all records
    Enter max results to display [1]: 
    ---- Running swish with the following command and parameters ----
    ./swish-e  \
    -w  \
    'swishdefault=(not asdfghjklzxcv)'  \
    -b  \
    1  \
    -m  \
    1  \
    -f  \
    index.swish-e  \
    -s  \
    swishrank  \
    desc  \
    swishlastmodified  \
    desc  \
    -x  \
    '<swishreccount>\t<swishtitle>\t<swishdescription>\t<swishlastmodified>\t<swishdocsize>\t<swishdocpath>\t<fos>\t<swishrank>\t<swishdocpath>\n'  \
    -H  \
    9

If you look carefully you will see that the -x parameter has "fos" instead of "foo", so there's our problem.


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

Take a look at the C<handler()> routine in this script for ideas how to use PerlSetVar commands
in httpd.conf to control the script.

Please post to the swish-e discussion list if you have any questions about running this
script under mod_perl.

    
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

Security on Windows questionable.

=head1 SUPPORT

The SWISH-E discussion list is the place to ask for any help regarding SWISH-E or this example
script. See http://swish-e.org.

Before posting please review:

    http://swish-e.org/2.2/docs/INSTALL.html#When_posting_please_provide_the_

Please do not contact the author or any of the swish-e developers directly.

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


