package Pod::HtmlPsPdf::Book;

use strict;

use File::Copy;

use Pod::HtmlPsPdf::Config  ();
use Pod::HtmlPsPdf::Common  ();
use Pod::HtmlPsPdf::RunTime ();
use Pod::HtmlPsPdf::Chapter ();

my $config = Pod::HtmlPsPdf::Config->new();

########
sub new{
  my $class = shift;

    # create the build directories if they are needed and missing
  my $rel_root   = $config->get_param('rel_root');
  my $split_root = $config->get_param('split_root');
  my $ps_root    = $config->get_param('ps_root');
  my $dir_mode   = $config->get_param('dir_mode');

  mkdir $rel_root,$dir_mode unless -e $rel_root;
  mkdir $split_root, $dir_mode
    if $Pod::HtmlPsPdf::RunTime::options{split_html} and !-e $split_root ;
  mkdir $ps_root, $dir_mode
    if $Pod::HtmlPsPdf::RunTime::options{generate_ps} and !-e $ps_root ;

  my $rh_main_toc = '';
  my $toc_file = $config->get_param('toc_file');

   # it just saves me time when I change only a few files and don't
    # want to wait for everything to get rebuilt
  $rh_main_toc = (    Pod::HtmlPsPdf::RunTime::has_storable_module()
		  and !$Pod::HtmlPsPdf::RunTime::options{makefile_mode}
		  and -e $toc_file)
    ? Storable::retrieve($toc_file)
    : {};

# when we will finish to parse the pod files, %valid_anchors will
# include all the valid anchors
# $valid_anchors{target_page#anchor} = "title";

# when we will finish to parse the pod files, %links_to_check will
# include all the crossreference links
# $links_to_check{srcpage}[3] = "target_page#anchor";

  my $self = bless {
		    rh_valid_anchors  => {},
		    rh_links_to_check => {},
		    rh_main_toc       => $rh_main_toc, # Main Table of Contents for index.html
		   }, ref($class)||$class;


  return $self;

} # end of sub new


# 
##############
sub get_param{
  my $self = shift;

  return () unless @_;
  return unless defined wantarray;	
  my @values = map {defined $self->{$_} ? $self->{$_} : ''} @_;

  return wantarray ? @values : $values[0];

} # end of sub get_param


####################
sub create_html_version{
  my $self = shift;

  print "+++ Processing the pod files \n";

  my $src_root      = $config->get_param('src_root');
  my $last_modified = $config->get_param('last_modified');
  my $verbose       = $Pod::HtmlPsPdf::RunTime::options{verbose};

  my $ra_ordered_srcs  = $config->get_param('pod_files');

    # strip the .pod extension
  my @basenames = map { s/\.pod$//; $_} @{$ra_ordered_srcs};

    # Process each pod
  for (my $i=0; $i<@basenames; $i++) {  # tom wrote that?

    my $file     = "$basenames[$i].pod";
    my $src_file = "$src_root/$file";

      # to rebuild or not to rebuild

    # This is messed up -- as it always looks at the main html output
    # for modified instead of what it's really doing.  So it fails to
    # create the postscript/pdf html files if not forced. - moseley

    # For now, I'll -f pdf building, but not write as I'm not sure what's
    # requred for the pdf/ps stuff.

    my $generate_ps = $Pod::HtmlPsPdf::RunTime::options{generate_ps};

    my $not_modified =
        -e $last_modified and -M $src_file > -M $last_modified
	    ? 1
	    : 0;

    printf("--- %-22s: skipping (not modified)\n",$file), next
      if !$Pod::HtmlPsPdf::RunTime::options{rebuild_all}
         && $not_modified && !$generate_ps;

	  
    my $rebuild_reason = '';
    $rebuild_reason = "modified" unless $not_modified;
    $rebuild_reason .= " / forced"
      if $Pod::HtmlPsPdf::RunTime::options{rebuild_all};
    printf "+++ %-22s: processing ($rebuild_reason)\n",$file;

      # prev/next linking
    my $prev_page = ($i and defined $basenames[$i-1])
      ? "$basenames[$i-1].html" : '';
    my $next_page = (       defined $basenames[$i+1])
      ? "$basenames[$i+1].html" : '';
    my $curr_page = "$basenames[$i].html";
    my $curr_page_index = $i+1;

    my $doc_root = "./";
    $curr_page =~ s|^\./||; # strip the leading './'
    $doc_root .= "../" x ($curr_page =~ tr|/|/|);
    $doc_root =~ s|/$||; # remove the last '/'

    # adjust the links for prev/next for sources files relative to the
    # current doc
    if ($prev_page) {
        $prev_page =~ s|^\./||; # strip the leading './'
        $prev_page = "$doc_root/$prev_page";
    }
    if ($next_page) {
        $next_page =~ s|^\./||; # strip the leading './'
        $next_page = "$doc_root/$next_page";
    }

      # convert pod to html
    if ($file =~ /\.pod/) {

      my $chapter = Pod::HtmlPsPdf::Chapter->new
          ($self,
           $file,
           $src_root,
           $src_file,
           $verbose,
           $doc_root,
           $curr_page,
           $curr_page_index,
           $prev_page,
           $next_page,
          );

        # podify
      $chapter->podify_items() 
	if $Pod::HtmlPsPdf::RunTime::options{podify_items};

        # pod2html
      $chapter->pod2html();
      $chapter->parse_html();


      # here's the hack to not write when building ps.
      # Write only if generating pdf/ps, or if not modified
      #(which would only be if building pdf and html at same time)

      $chapter->write_html_file() unless $generate_ps || !$not_modified;


        # html for ps creation
      $chapter->write_ps_html_file() if $generate_ps;

        # html 2 split html -- this destroys the title/body/index
        # structures! Therefore it MUST come last, when we won't need
        # them anymore. Othewise you should revise this function to
        # use copy instead.
      $chapter->write_split_html_files() 
	if $Pod::HtmlPsPdf::RunTime::options{split_html};

    }

  } # end of for (my $i=0; $i<@$ra_ordered_srcs; $i++)

  # refresh the last_modified flag
  Pod::HtmlPsPdf::Common::write_file($last_modified,[""]);

    # create the index.html files based on the toc from each pod file.
  $self->write_index_html_file();

    # copy non-pod files like images and stylesheets
  my $rel_root = $config->get_param('rel_root');
  $self->copy_the_rest($rel_root);

} # end of sub create_html_version



####################
sub copy_the_rest{
  my $self = shift;
  my $target_root = shift;

  print "+++ Copying the non-pod files to $target_root\n";

  my ($nonpod_files,$src_root)
    = $config->get_param(qw(nonpod_files src_root));

    # Handle the rest of the files

  foreach my $file (@$nonpod_files) {

    # The nonpod files are relative to the current directory now.
    my $src = "$src_root/$file";
    my $dst = "$target_root/$file";

    my $base_name = File::Basename::basename $dst;


    my $forced = $Pod::HtmlPsPdf::RunTime::options{rebuild_all}
                 ? ' / forced'
                 : '';

    if ( !$forced && -e $dst && -M $src > -M $dst ) {
        printf("--- %-22s: skipping (not modified)\n",$base_name);

    } else {

        $forced ||= 'modified';
    
        Pod::HtmlPsPdf::Common::copy_file( $src, $dst );
        printf("+++ %-22s: processing (%s)\n",$base_name, $forced);
    }

  }

} # end of sub copy_the_rest



# generate the PS version
####################
sub create_ps_version{
  my $self = shift;

  print "+++ Starting the Generation of a PostScript Book version\n";

  $self->make_indexps_file();

    # copy non-pod files like images and stylesheets
    # must copy these before PS gets generated
  my $ps_root      = $config->get_param('ps_root');
  $self->copy_the_rest($ps_root);

  print "+++ Generating a PostScript Book version\n";

  my $html2ps_exec = $config->get_param('html2ps_exec');
  my $html2ps_conf = $config->get_param('html2ps_conf');
  my $ra_ordered_srcs  = $config->get_param('pod_files');
  my $out_name = $config->get_param('out_name');

  my $command = "$html2ps_exec -f $html2ps_conf -o $ps_root/${out_name}.ps ";
  $command .= join " ", map {"$ps_root/$_.html"} "index", @$ra_ordered_srcs;
  print "Doing $command\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
print "Doing $command\n";
  system $command;

} # end of sub create_ps_version

# generate the PDF version
####################
sub create_pdf_version{
  my $self = shift;

  print "+++ Converting from PS format to PDF format\n";
  my $ps_root = $config->get_param('ps_root');
  my $out_name = $config->get_param('out_name');
  my $command = "ps2pdf $ps_root/$out_name.ps $ps_root/$out_name.pdf";
  print "Doing $command\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
  system $command;

} # end of sub create_pdf_version


# generate the PDF version
####################
sub create_split_html_version{
  my $self = shift;

  print "+++ Completing the Split HTML version\n";

    # copy non-pod files like images and stylesheets
  my $split_root = $config->get_param('split_root');
  $self->copy_the_rest($split_root);

  $self->write_split_index_html_file()

} # end of sub create_split_html_version


# Using the same template file create the long and the short index
# html files
###################
sub write_index_html_file{
  my $self = shift;

  my $ra_ordered_srcs  = $config->get_param('pod_files');
  my @ordered_srcs  = @$ra_ordered_srcs;

  # build a complete toc
  my $main_long_toc  = join "\n", 
    map { s/$/.html/;
	  $self->{rh_main_toc}->{$_} ||= ''} @ordered_srcs;

  my $main_short_toc = join "\n", 
  map {   $self->{rh_main_toc}->{$_} =~ s|<UL>.*</UL>||ism;
	  $self->{rh_main_toc}->{$_} =~ s|<B><FONT SIZE=\+1>([^<]+)</FONT></B></A></LI><P>|$1</A></LI>|ism;
	  $self->{rh_main_toc}->{$_} =~ s^<P>^^gism;
	  $self->{rh_main_toc}->{$_} } @ordered_srcs;

  #  $self->{rh_main_toc}->{$_} =~ s^<B>|</B>|<P>^^gism;
  #  $self->{rh_main_toc}->{$_} =~ s^<FONT.*?>|</FONT>^^gism;

  my %toc = (
	     long  => $main_long_toc,
	     short => $main_short_toc,
	    );

  my $rel_root  = $config->get_param('rel_root');
  my %file = (
	      long  => "$rel_root/index_long.html",
	      short => "$rel_root/index.html",
	     );

  my %toc_link = (
		  long  => qq{[ <A HREF="#toc">TOC</A> ] 
			      [ <A HREF="index.html">Dense TOC</A> ]},
		  short => qq{[ <A HREF="#toc">TOC</A> ] 
			      [ <A HREF="index_long.html">Full TOC</A> ]},
		 );

  my ($mon,$day,$year) = (localtime ( time ) )[4,3,5];
  my $time_stamp = sprintf "%02d/%02d/%04d", ++$mon,$day,1900+$year;
  my $date = sprintf "%s %d, %d", (split /\s+/, scalar localtime)[1,2,4];

  my $template = $config->get_param('tmpl_index_html');
  my @orig_content = ();
  Pod::HtmlPsPdf::Common::read_file($template,\@orig_content);

  use vars qw($VERSION);
  my $version_file = $config->get_param('version_file');
  my $package_name = $config->get_param('package_name');
  require $version_file;
  {
      no strict 'refs';
      $VERSION = ${$package_name."::VERSION"};
  }

  my %replace_map = 
    (
     VERSION  => $VERSION,
     DATE     => $date,
     MODIFIED => $time_stamp,
    );

  for (qw(short long)) {

    $replace_map{MAIN_TOC} = $toc{$_};
    $replace_map{TOC} = $toc_link{$_};

    my @content = @orig_content;

    foreach (@content) {
      s/\[(\w+)\]/$replace_map{$1}/g;
    }

    print "+++ Creating $file{$_}\n";
    Pod::HtmlPsPdf::Common::write_file($file{$_},\@content);
  } # end of for (qw(short long))


} # end of sub write_index_html_file

# index html files for the split html version
###################
sub write_split_index_html_file{
  my $self = shift;

  my $rel_root   = $config->get_param('rel_root');
  my $split_root = $config->get_param('split_root');

  # we simply copy the index.html and index_long.html
  # from $rel_root and adjust the links

  for (qw(index index_long)) {
    my $index_file = "$rel_root/$_.html";
    my @content = ();
    Pod::HtmlPsPdf::Common::read_file($index_file,\@content);

    foreach (@content) {
      s|<LI><A HREF="([^.]+)\.html"|<LI><A HREF="$1/index.html"|gi;
      s|<LI><A.HREF="([^.]+)\.html#([^\"]*)"|<LI><A HREF="$1/$2.html"|gsi;
      s|HREF="([^.]+)\.html#([^\"]*)"|HREF="$1/$2.html"|gsi;
    }

    $index_file = "$split_root/$_.html";
    print "+++ Creating $index_file\n";
    Pod::HtmlPsPdf::Common::write_file($index_file,\@content);

  } # end of for (qw(index index_long))

} # end of sub write_split_index_html_file



######################
sub make_indexps_file{
  my $self = shift;

  use vars qw($VERSION);
  require $config->get_param('version_file');

  my ($mon,$day,$year) = (localtime ( time ) )[4,3,5];
  $year = 1900 + $year;
  my $time_stamp = sprintf "%02d/%02d/%04d", ++$mon,$day,$year;

  my $date = sprintf "%s, %d %d", (split /\s+/, scalar localtime)[1,2,4];

  my %replace_map = 
    (
     VERSION  => $VERSION,
     DATE     => $date,
     MODIFIED => $time_stamp,
    );

  my $tmpl_index_ps = $config->get_param('tmpl_index_ps');
  my @content = ();
  Pod::HtmlPsPdf::Common::read_file($tmpl_index_ps,\@content);

  for (@content){
    s/\[(\w+)\]/$replace_map{$1}/g 
  }
  
  my $ps_root = $config->get_param('ps_root');
  print "+++ Creating $ps_root/index.html\n";
  Pod::HtmlPsPdf::Common::write_file("$ps_root/index.html",\@content);

} # end of sub make_indexps_file

# Validate pod's L<> links
###################
sub validate_links{
  my $self = shift;

  my %links_to_check = %{$self->{rh_links_to_check}};

  print "+++ Validating anchors\n";
  my $count = 0;
  foreach my $srcpage (sort keys %links_to_check) {
    foreach (@{$links_to_check{$srcpage}}) {
      $count++,
	print "!!! Broken $srcpage.pod: $_\n"
	  unless exists $self->{rh_valid_anchors}->{$_};
    }
  }

  print $count
    ? "!!! $count anchors are broken\n"
    : "!!! Nothing broken\n";

} # end of sub validate_links

# print the available target anchors by the pod's L<> fashion, to be
# copied and pasted directly into a pod.
###################
sub print_anchors{
  my $self = shift;

  my %valid_anchors = %{$self->{rh_valid_anchors}};
  
  print "+++ Generating a list of available anchors\n";
  foreach my $key (sort keys %valid_anchors) {
    print "L<$valid_anchors{$key}|$key>\n";
  }

} # end of sub print_anchors


################
sub make_tar_gz{
  my $self = shift;

  print "+++ Creating tar.gz version\n";

  my $root     = $config->get_param('root');
  my $out_dir  = $config->get_param('out_dir');
  my $dir_mode = $config->get_param('dir_mode');
  mkdir $out_dir, $dir_mode unless -d $out_dir;

    # copy all to an out dir
  my $rel_root = $config->get_param('rel_root');
  system "cp -r $rel_root/* $out_dir";

  my $ps_root  = $config->get_param('ps_root');
  my $out_name = $config->get_param('out_name');

    # if we create pdf, we don't want the PS version, therefore it pdf
    # comes first in this if statement
  if ($Pod::HtmlPsPdf::RunTime::options{generate_pdf}) {
    my $file = "$ps_root/${out_name}.pdf";
    print "gzip -9 $file\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
    system("gzip -9 $file");
    print "mv $file.gz $root/$out_name\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
    move("$file.gz","$root/$out_name");

  } elsif ($Pod::HtmlPsPdf::RunTime::options{generate_ps}) {
    my $file = "$ps_root/${out_name}.ps";
    print "gzip -9 $file\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
    system("gzip -9 $file");
    print "mv $file.gz $root/$out_name\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};
    move("$file.gz","$root/$out_name");

  } else {
    # do nothing
  }

  chdir $root;

  print "mv $out_name.tar.gz $out_name.tar.gz.old\n" 
    if -e "$out_name.tar.gz" and $Pod::HtmlPsPdf::RunTime::options{verbose};
  rename "$out_name.tar.gz", "$out_name.tar.gz.old" if -e "$out_name.tar.gz";
  system "tar cvf $out_name.tar $out_name";
  system "gzip -9 $out_name.tar";

		# Clean up
  system "rm -rf $out_name";

#  print "*** Error: PDF did not enter the release package!\n"
#    unless $Pod::HtmlPsPdf::RunTime::options{generate_pdf}


} # end of sub make_tar_gz




# cleanup
############
sub cleanup{
  my $self = shift;

  if (Pod::HtmlPsPdf::RunTime::has_storable_module()){
    # update TOC
    my $toc_file = $config->get_param('toc_file');
    print "+++ Storing the TOC file $toc_file for a later reuse\n"
      if $Pod::HtmlPsPdf::RunTime::options{verbose};
    Storable::store($self->{rh_main_toc},$toc_file);
  }
} # end of sub cleanup


1;
__END__
