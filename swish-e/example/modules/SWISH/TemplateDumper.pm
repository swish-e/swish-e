#===============================================================================
# This just dumps out the results object.
# Need to use a query string to do anything interesting
#
#    $Id$
#
#===============================================================================
package SWISH::TemplateDumper;
use strict;

use Data::Dumper;

sub show_template {
    my ( $class, $template_params, $results ) = @_;

$Data::Dumper::Quotekeys = 0;
    print "Content-Type: text/plain\n\n",
          Dumper $template_params,
          Dumper $results;
}

1;


