#!/usr/local/bin/perl -w
use strict;

#    search.cgi $Revision$ Copyright (C) 2001 Bill Moseley search@hank.org
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
# $Id$


# Global vars used

    use vars qw/
        $Swish_Binary $Swish_Index $Tmpl_Path @PropertyNames
        @MetaNames $Metaname_Default $All_Meta $Page_Size
    /;


#------------ Configuration ----------------------

    # Set these to as needed for your system and your index file

    # You might want to read these in from a file (based on
    # the script name or extra path info), or use PerlSetVars
    # under mod_perl to pass in the parameters.


    # These paths are normally outside of webspace

    # This one is obvious, I hope:

    $Swish_Binary = '/usr/local/bin/swish-e';


    # The index file can also be a reference to an array of index files.

    $Swish_Index  = '/path/to/example.index';


    # The template file is the one supplied with this example CGI script

    $Tmpl_Path    = '/path/to/swish-e/example/swish.tmpl';


    # Here list the properties that are defined in your index,
    # and that you want displayed with your search results
    # Comment out if not used

    @PropertyNames   = qw/last_name first_name city phone/;



    # If you defined MetaNames in your document (to search by field)
    # specify their names here.  These will be used when generating the query.
    # Comment out if not used
    # This adds a radio group on the form for limiting your search if more than one
    # MetaName is listed.
    
    @MetaNames = qw/name description/;


    # The $Metaname_Default does two things.  If you set @MetaNames to more than one
    # value, this will set the default radio button selected when the script first starts.
    # If you set MetaNames to the empty list, but set $MetaName_Default to a value, then
    # this value will be used as the metaname for all queries.

    $Metaname_Default = 'description';  # set the default radio button

    # if $All_Meta is set true, and @MetaNames is not the empty list, then
    # all queries must be a metaname search.
    # if $All_Meta is set false, the an additional radio button will be added
    # to allow searching without a metaname prepended to the query.
    # For HTML docs, it's typically 0, for XML set to 1.

    $All_Meta = 0;


    $Page_Size    = 20;  # results per page

#---------- End of Configuration ----------------------

#
# Sorry about the pod here, but can't use __END__ due to Apache::Registry -- so CGI scripts (perl) must read in
# the pod section -- but doesn't really matter (speed wise) if running CGI, anyway....
#

=head1 NAME

swish.cgi -- Example Perl script for searching with the SWISH-E search engine.

=head1 DESCRIPTION

This is an example CGI script for searching with the SWISH-E search engine version 2.2 and above.
It demonstrates how to use SWISH-E to search an index (or indexes), limit searches to metanames,
displaying a few results at a time (good for your server),
and how to sort your results by different Properties.  In addition, it attempts to demonstrate good
programming techniques such as separation of content from code, and modular code design.

This script is not meant to be a complete solution to your searching needs.  Rather, an example of a
working script that can be easily modified to meet your needs.

This program uses a number of modules to make work easy: the standard CGI module to handle form data,
the SWISH (and SWISH::Fork) module to run swish, HTML::Template and HTML::FillInForm to keep the
perl code separated from the presentation code.

The modules SWISH and SWISH::Fork are used as a high level interface to swish.  Their goal it to provide a
relatively simple and consistent interface to swish.  Running swish from a CGI program is not difficult.  But, running
swish in a secure way is a bit more difficult, and is often overlooked in many example scripts.
Plus, the SWISH module
makes it easy to just pass swish a query and return the results, headers, and errors without having to worry about
the details of running swish.  In addition, the SWISH module's interface is designed to make it easy
to access swish in other ways than simply running an external program (i.e. via the Swish-e C library, or via
a yet-to-be-built, swish-e server) without having to redesign your code.

Time::HiRes is not needed for this program.  But, it is somewhat interesting for looking at the time required to fork
perl, exec swish-e, and read all results, compared to just the time to run the query.
Running a script in a persistent environment (such as under mod_perl) will improve
the response time seen by your web clients and reduce load on your server.

Time::HiRes may not install on all systems, and if yours
falls into this category then you must comment out a few lines in this script -- it should be obvious by
looking at the code.

Sys::Signal is required by the SWISH::Fork module to properly handle timeouts.  A bug in current versions of
perl do not properly reset signal handlers under some situations.

B<A note about installing Perl modules>

Many people shy away from installing extra modules
for fear of bloating their code or making their CGI scripts run slowly.
This is misguided.
Code reuse, especially tested and peer reviewed code, and modular design are good things.
Installing Perl modules is not difficult, and you do not need root access.  Modules will not make your
program too large, or make it run slowly -- these are common misconceptions.

There are a number of other modules that should be considered when designing your CGI scripts, in general.
POE is interesting and is well suited for this type of application.
Template::Toolkit is also highly recommended, and CGI::Application may make your scripts easier to
design and maintain.  If you must return all results with each swish query, you may wish to look at File::Cache
to cache your search results to disk.
Check them all out.

To run this code you must:

=over 4

=item *

Install modules used in this program.

=item *

Create a SWISH-E index file

=item *

Adjust the parameters in this program to point to your index file and list
any properties you wish to sort on and display, and list metanames for limiting
your search to parts of your document.

=item *

Adjust the swish.tmpl file to print your properties and customize to your look.

=back

Please see http://sunsite.berkeley.edu/SWISH-E for more information about SWIHS-E and to receive help.

=head1 INSTALLATION

=over 4

=item 1 Install the required modules.



The required modules can be found at your favorite CPAN site (http://search.cpan.org)

    SWISH
    SWISH::Fork
    HTML::Template
    HTML::FillInForm
    HTML::Parser
    Time::HiRes  (you can get by without this module)

(See below for Windows instructions.)    

If you have CPAN.pm setup on your computer then
installation will be straight forward and not require much effort.  If you don't know what this is
then it may be just as easy to install the modules manually.  If installing modules sounds difficult
then you just need a few tips to get started.

Assuming you don't have CPAN.pm setup on your machine, download and install the
modules manually.  Any decent perl installation will have the LWP bundle installed.  This bundle will include a
program called C<lwp-download>.  The C<wget> program is another option for downloading from CPAN.

See http://search.cpan.org to locate the modules.

The download and installation cycle for modules goes something like this:

   % lwp-download http://www.cpan.org/authors/id/H/HA/HANK/SWISH-X.XX.tar.gz
   % gzip -dc SWISH-X.XX.tar.gz | tar xof -
   % cd SWISH-X.XX
   % perl Makefile.PL
   or
   % perl Makefile.PL PREFIX=$HOME/perl_lib
   % make
   % make test
   (perhaps su root at this point if you did not use a PREFIX)
   % make install
   % cd ..

Use the B<PREFIX> if you do not have root access or you want to install the modules
in a local library.  Add a C<use lib> statement to the program
if you use a PREFIX to install the modules in a non-standard location.

For example:

    use lib qw(
        /home/bmoseley/perl_lib/lib/site_perl/5.6.0
        /home/bmoseley/perl_lib/lib/site_perl/5.6.0/i386-linux/
    );
    
Repeat the above for all the required modules.  If you install a module but trying to run
the program returns an error says that the module cannot be found in @INC, then carefully check the directories specified
in your C<use lib> statement.

=item 2 Set up your web server to run this CGI script

Copy this script (swish.cgi) to your cgi-bin directory where .cgi scripts are automatically
executed as CGI scripts (or create an alias in your web server's configuration setup).

The details of setting up a CGI script depend on the web server you are using.  If you do have problems
contact your web administrator, or carefully check the error messages your web server reports in its error
log.

mod_perl setup is described below.


=item 3 Modify a few global variables in the script

Adjust the global vars in the script to point to the location of the swish-e binary,
your swish-e index, and the location of the swish.tmpl HTML::Template file.  See the parameter
setup at the top of this search.cgi script for complete information.

The use of global vars here just makes it easy as an example script. A better method would be for the
script to read the settings from a configuration file, or passed in from the environment (or PerlSetVar)
set in the web server.

=item 4 Customize the output to your look.

Although the template supplied will generate a reasonable output, you will probably want to customize
it to your look.

The HTML::Template file (swish.tmpl) should be easy to understand (if not then: perldoc HTML::Template),
and should be easy enough to customize to your look.  You may need to fixup the link to the
documents returned by swish (or use swish's ReplaceRules configuration directive during indexing).

=back

Again, don't forget to check the web server's error log for details if you have any problems.  In general, debug
CGI scripts from the command line instead of via the web server.

=head2 Windows Specific Information

Installation of some modules under ActiveState's version of Perl is handled via the Perl Package Manager (C<ppm>).
This makes installing packages very easy under Windows.  Not all modules required for this script are
available from ActiveState's ppm library, but the ones that are not are easily installed manually or via CPAN.pm.

Here's how to install the modules using C<ppm>:

    C:\> ppm install HTML::Parser
    C:\> ppm install HTML::Template
    C:\> ppm install Time::HiRes

That's easy enough.  The other modules can be installed by using the CPAN module, even under Windows, or
by the manual method detailed above.  To make this all work, though, will require the C<nmake> utility
available for free from Microsoft.  It's easy to find; here's one location:

    http://download.microsoft.com/download/vc15/Patch/1.52/W95/EN-US/Nmake15.exe

Once that's installed you simply replace calls to C<make> in the example above with C<nmake>.
You can either use the manual method shown above, or use the CPAN.pm module to automate the entire process.

If you decide to use CPAN.pm, the first time you start it (C<perl -MCPAN -e shell>) it will ask you a number of
questions -- for most you can just accept the defaults.

=head1 MOD_PERL

This script may be run under CGI or Apache::Registry, although it would be trivial to
convert it to a normal mod_perl response handler.

If running under mod_perl then you may wish to cache the template.  See the HTML::Template FAQ for
more information.

To set this script up as an Apache::Registry script use something similar to
the following (perhaps inside a <Directory> block):

    <files swish.cgi>
        SetHandler perl-script
        PerlHandler Apache::Registry
    </files>

In general, it's a good idea to pre-load modules by using C<PerlModule> statements, or
by C<use>ing the module in a startup.pl script.



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

search.cgi $Revision$ Copyright (C) 2001 Bill Moseley search@hank.org
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

# ** Here's the main part of the script, if you were wondering...

# if you install modules locally you may need a "use lib" statement here
# see the pod docs above for more info.

# use lib 'path/to/local/perl/library';


use SWISH;
use CGI;
use HTML::Template;
use HTML::FillInForm;
use Time::HiRes qw(gettimeofday tv_interval);

#--------------------------------------------------

{
    my $q = CGI->new;

    # This sets the default selected radio group button.
    $q->param('metaname', $Metaname_Default ) if $Metaname_Default && !defined $q->param('metaname');
    
    show_template( $Tmpl_Path , run_query( $q ), $q );
}



#========================================================
# run_query returns a reference to a hash for use in the HTML::Template object
#   Returns an empty hash if "query" form field not selected (i.e. first time run)

sub run_query {

    my $q = shift;  # pass in the CGI object


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

    $q->param('query', $query );  # clean up the query, if needed.

    # prepend metaname to search, if required.
    $query = $q->param('metaname') . "=($query)" if $q->param('metaname');
    

    my $t0 = [gettimeofday];  # Time::HiRes - comment out if not needed
    
   
    # Set the starting position

    my $start = $q->param('start') || 0;
    $start = 0 unless $start =~ /^\d+$/ && $start >= 0;


    # This arrray stores the results returned from swish
    my @results;

    # Create a search object

    my $sh = SWISH->connect(
       'Fork',           # this says to use the SWISH::Fork module
       prog     => $Swish_Binary,
       version  => 2.2,  # see perldoc SWISH
       indexes  => $Swish_Index,
       startnum => $start + 1,  
       maxhits  => $Page_Size,
       properties => \@PropertyNames,
       timeout  => 10,  # kill script if query takes more than ten secs

       # Here's a way to make *every* field available to the template
       # But then you must explicitly set the field names in the template
       #results  => sub { push @results, { map { $_, $_[1]->$_() } $_[1]->field_names } },

       # Here's another, more general way that makes the template cleaner.
       results => sub {
            my %result = (
                FILE   => $_[1]->swishdocpath,
                TITLE  => $_[1]->swishtitle,
                TIME   => $_[1]->swishlastmodified,
                RECNUM => $_[1]->swishreccount,
                RANK   => $_[1]->swishrank,
            );

            $result{PROPERTIES} =
                [ map {
                    {
                        PROP_NAME  => $_,
                        PROP_VALUE => $_[1]->$_(),
                    }
                  } @PropertyNames ] if @PropertyNames;

            push @results, \%result;
        },
       
    );


    # Check for connect errors
    return { MESSAGE => $SWISH::errstr || 'Sorry, failed to process your query' } unless $sh;



    # $SWISH::Fork::DEBUG++;  # generates (a lot of) debugging info to STDERR 


    # Now set sort option - if a valid option submitted (or you could let swish-e return the error).
    
    my %props = map { $_, 1 } @PropertyNames;
    $props{swishrank}++;  # also can sort by rank
    if ( $q->param('sort') && $props{ $q->param('sort') } ) {

        my $direction = $q->param('sort') eq 'swishrank'
                        ? $q->param('reverse') ? 'asc' : 'desc'
                        : $q->param('reverse') ? 'desc' : 'asc';
                        
        $sh->sortorder( [$q->param('sort'), $direction ] );

    }
        
    


    # Now run the query - you might want to do some checks on $query here
    my $hits  = $sh->query( $query );

    my $elapsed = sprintf('%.3f',tv_interval($t0));


    # Check for errors
    return {
        MESSAGE => $sh->errstr,
        QUERY   => $q->escapeHTML( $query ),
    } unless $hits;

    # Build href for repeated search

    my $href = $q->script_name . '?' .
        join( '&amp;',
            map { "$_=" . $q->escape( $q->param($_) ) }
                grep { $q->param($_) }  qw/query metaname sort reverse/
        );


    # Return the template fields

    my $result = {
        FILES       => \@results,
        QUERY       => $q->escapeHTML( $query ),
        QUERY_HREF  => $href,               # for running this query again
        QUERY_SIMPLE=> scalar $q->param( $query ), # the query w/o any metanames - same as QUERY if metanames are not used
        TOTAL_TIME  => $elapsed,
        MY_URL      => $q->script_name,
        SHOWING     => $hits,
        HITS        => $sh->get_header('number of hits') ||  0,
        RUN_TIME    => $sh->get_header('run time') ||  'unknown',
        SEARCH_TIME => $sh->get_header('search time') ||  'unknown',
        FROM        => $start + 1,
        TO          => $start + $hits,
    };

    set_page( $result, $q );

    return $result;
}        
        

#========================================================
# show_template -- displays a page, and exits.
#
#   Uses HTML::FillInForm to provide "sticky" forms
#

sub show_template {

    my ( $file, $params, $q ) = @_;
    
    my $template = HTML::Template->new(
        filename            => $file,
        die_on_bad_params   => 0,
        loop_context_vars   => 1,
    );

    $params->{MY_URL} = $q->script_name;

    # Allow for sort selection in a <select>
    $params->{SORTS} = [ map { { SORT_BY => $_ } } @PropertyNames ] if @PropertyNames;

    $params->{METANAMES} = [ map { { METANAME => $_ } } @MetaNames ] if @MetaNames;
    $params->{ALL_META} = $All_Meta;

    $template->param( $params );
    my $page = $template->output;


    my $fif = new HTML::FillInForm;

    print $q->header,
          $fif->fill(
            scalarref => \$page,
            fobject   => $q,
          );
}

#========================================================
# Sets prev and next page links.
# Feel free to clean this code up!
    
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


