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

    my $cgi = $results->cgi;


    #/* Cached if running under mod_perl */
    $Template ||= Template->new( $template_params->{options} );

    die $Template->error() unless $Template;


    print $cgi->header;


    $Template->process( $template_params->{file}, $results )
	|| die "Template process failed: ", $Template->error(), "\n";
}	
   
1;

