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
# $Id$

# (Can't use __END__ due to Apache::Registry)

=head1 NAME

swish.cgi -- Example Perl script for searching with the SWISH-E search engine.

=head1 DESCRIPTION

This is an example CGI script for searching with the SWISH-E search engine.  It really doesn't do
much, but shows you how to use SWISH-E to search an index, displaying a few results at a time, and
to sort your results by different Properties.

This program uses a number of modules to make work easy: the standard CGI module to handle form data,
the SWISH (and SWISH::Fork) module to run swish, HTML::Template and HTML::FillInForm to keep the
perl code separated from the presentation code.  Sys::Signal is required by the SWISH::Fork module.

Time::HiRes is not needed for this module, but is somewhat interesting for looking at the time required to fork
perl and exec swish-e.  Running a script in a persistent environment (such as under mod_perl) will improve
the response time seen by your web clients and reduce load on your server.

Time::HiRes may not install on all systems, and if yours
falls into this category then you will need to comment out a few lines in the module -- it should be obvious by
looking at the code.

Many people shy away from installing extra modules
for fear of bloating their code or making their CGI scripts run slowly.  This is misguided.

There are a number of other modules that should be considered when designing your CGI scripts, in general.
POE is interesting and is well suited for this type of application.
Template::Toolkit is also highly recommended, and CGI::Application may make your scripts easier to
design and maintain.  If you must return all results with each swish query, you may wish to look at File::Cache
to cache your search results to disk.
Check them out.


To run this code you must:

=over 4

=item *

Install modules used in this program

=item *

Create a SWISH-E index file

=item *

Adjust the parameters in this program to point to your index file and to define
any properties defined in your index that you wish to return with your search results.

=item *

Adjust the swish.tmpl file to print your properties and customize to your look.

=back

Please see http://sunsite.berkeley.edu/SWISH-E for more information about SWIHS-E and to receive help.

=head1 INSTALLATION

You will need to install the required modules.  If you have CPAN.pm setup on your computer then
installation will be straight forward and not require much effort.  If you don't know what this is
then it may be just as easy to install the modules manually.  If installing modules sound difficult
then you just need a few tips to get started.

Assuming you don't have CPAN.pm setup on your machine, you will need to download and install the
modules manually.  Any decent perl installation will have the LWP bundle installed.  This bundle will include a
program called C<lwp-download>.  See http://search.cpan.org to locate the modules.

The download and installation cycle for modules goes something like this:

   % lwp-download http://www.cpan.org/authors/id/H/HA/HANK/SWISH-X.XX.tar.gz
   % gzip -dc SWISH-X.XX.tar.gz | tar xof -
   % cd SWISH-X.XX
   % perl Makefile.PL
   or
   % perl Makefile.PL PREFIX=$HOME/perl_lib
   % make
   % make test
   (perhaps su root at this point)
   % make install
   % cd ..

Use the PREFIX if you do not have root access or you want to install the modules
in a local library.  You will need to add a C<use lib> statement to the program
if you use a PREFIX to install the modules in a non-standard location.

You will need to repeat the above for all the required modules.  If you install a module yet trying to run
the program says that the module cannot be found in @INC, then carefully check the directories specified
in your C<use lib> statement.

Next, copy the swish.cgi script to your cgi-bin directory where .cgi scripts are automatically
executed as CGI scripts.

Finally, adjust the global vars in the script to point to the location of the swish-e binary,
your swish-e index, and the location of the swish.tmpl HTML::Template file.

Don't forget to check the web server's error log for details if you have any problems.

   

=head1 MOD_PERL

This script may be run under CGI or Apache::Registry, although it would be trivial to
convert it to a normal mod_perl response handler.

If running under mod_perl then you may wish to cache the template.  See the HTML::Template FAQ for
more information.

To set this script up as an Apache::Registry script:

    <files swish.cgi>
        SetHandler perl-script
        PerlHandler Apache::Registry
    </files>

In general, it's a good idea to pre-load modules by using C<PerlModule> statements, or
by C<use>ing the module in a startup.pl script.



=head1 DISCLAIMER

Please use at your own risk, of course.

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

# if you install modules locally you may need a "use lib" statement here
# use lib 'path/to/local/perl/library';

use SWISH;
use CGI;
use HTML::Template;
use HTML::FillInForm;
use Time::HiRes qw(gettimeofday tv_interval);

#------------ Configuration ----------------------

    use vars qw/$swish_binary $swish_index $tmpl_path @properties $page_size/;

    

    $swish_binary = '/usr/local/bin/swish-e';

    # these are normally outside of webspace

    $swish_index  = '/path/to/example.index';
    $tmpl_path    = '/path/to/swish-e/example/swish.tmpl';


    # here list the properties that are defined in your index,
    # and that you want returned with your search results
    # make an empty list "()" if not used.

    @properties   = qw/last_name first_name category/;

    $page_size    = 20;  # results per page

#--------------------------------------------------


# This is all you get...
{
    my $q = CGI->new;
    show_template( $tmpl_path , run_query( $q ), $q );
}

#---------------------------------------------------


#========================================================
# run_query returns a hash for use in the HTML::Template object
#   Returns an empty hash if "query" form field not selected.

sub run_query {

    my $q = shift;

    my $query = $q->param('query') || '';

    for ( $query ) {
        s/\s+$//;
        s/^\s+//;
    }

    unless ( $query ) {
        return $q->param('submit')
            ? { MESSAGE => 'Please enter a query string' }
            : {};
    }

    my $t0 = [gettimeofday];
    
   
    # Set the starting position

    my $start = $q->param('start') || 0;
    $start = 0 unless $start =~ /^\d+$/ && $start >= 0;


    # This arrray stores the results
    my @results;

    # Create a search object

    my $sh = SWISH->connect('Fork',
       prog     => $swish_binary,
       version  => 2.2,  # see perldoc SWISH
       indexes  => $swish_index,
       startnum => $start + 1,  
       maxhits  => $page_size,
       properties => \@properties,
       timeout  => 10,  # kill script if query takes more than ten secs

       # Here's a way to make *every* field available to the template
       # But then you must know the field names in the template
       #results  => sub { push @results, { map { $_, $_[1]->$_() } $_[1]->field_names } },

       # Here's another, more general way
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
                  } @properties ] if @properties;

            push @results, \%result;
        },
       
    );


    # Check for connect errors
    return { MESSAGE => $SWISH::errstr || "Sorry, failed to process your query" } unless $sh;



    # $SWISH::Fork::DEBUG++;  # generates debugging info to STDERR


    # Now set sort option - if a valid option submitted
    
    my %props = map { $_, 1 } @properties;
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


    # Return the template fields

    my $result = {
        FILES       => \@results,
        QUERY       => $q->escapeHTML( $query ),
        QUERY_HREF  => $q->escape( $query ),
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
    $params->{SORTS} = [ map { { SORT_BY => $_ } } @properties ] if @properties;

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
    
        
    my $prev = $start - $page_size;
    $prev = 0 if $prev < 0;

    if ( $prev < $start ) {
        $results->{PREV} = $prev;
        $results->{PREV_COUNT} = $start - $prev;
    }

    
    my $last = $results->{HITS} - 1;

    
    my $next = $start + $page_size;
    $next = $last if $next > $last;
    my $cur_end   = $start + scalar @{$results->{FILES}} - 1;
    if ( $next > $cur_end ) {
        $results->{NEXT} = $next;
        $results->{NEXT_COUNT} = $next + $page_size > $last
                                ? $last - $next + 1
                                : $page_size;
    }


    # Calculate pages  ( is this -1 correct here? )
    
    my $pages = int (($results->{HITS} -1) / $page_size);
    if ( $pages ) {

        my @pages = 0..$pages;

        my $max_pages = 10;

        if ( @pages > $max_pages ) {
            my $current_page = int ( $start / $page_size - $max_pages/2) ;
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
                my $page_start = $_ * $page_size;
                my $url = $q->script_name;
                my $page = $_ + 1;
                $page_start == $start
                ? $page
                : qq[<a href="$url?query=$results->{QUERY_HREF};start=$page_start">$page</a>];
                        } @pages;
    }

}


