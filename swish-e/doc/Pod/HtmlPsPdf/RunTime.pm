package Pod::HtmlPsPdf::RunTime;

use strict;

%Pod::HtmlPsPdf::RunTime::options = ();

# check whether we have a Storable avalable
eval {require Storable;};
my $Storable_exists = $@ ? 0 : 1;

########################
sub set_runtime_options{
  %Pod::HtmlPsPdf::RunTime::options = %{+shift};
} # end of sub set_runtime_options

# returns 1 if true, 0 -- otherwise
#######################
sub has_storable_module{

  # ps2html is bundled, so we can create PS
  return $Storable_exists;

} # end of sub has_storable_module

# returns 1 if true, 0 -- otherwise
#################
sub can_create_ps{

  # ps2html is bundled, so we can always create PS
  return 1;

  # if you unbundle it make sure you write here a code similar to
  # can_create_pdf()

} # end of sub can_create_ps

# returns 1 if true, 0 -- otherwise
#################
sub can_create_pdf{

    # check whether ps2pdf exists
  my $ps2pdf_exists = `which ps2pdf` ? 1 : 0;

  print(qq{It seems that you do not have ps2pdf installed! You have
	   to install it if you want to generate the PDF file
	  }),
	    return 0 
	      unless $ps2pdf_exists;
  return 1;

}  # end of sub can_create_pdf


1;
__END__
