package Pod::HtmlPsPdf::Chapter;

use strict;

use Pod::HtmlPsPdf::Html ();
use Pod::HtmlPsPdf::Common ();

use Pod::HtmlPsPdf::Config ();
my $config = Pod::HtmlPsPdf::Config->new();


########
sub new{
    my $class = shift;

    # book object
    my $book_obj = shift;

    my ($file_name, $src_root, $src_file, $verbose, $doc_root,
        $curr_page, $curr_page_index, $prev_page, $next_page) = @_;

    # extract the base name
    # Note that this must match Book.pm pod_newer_split()
    my ($base_name) = ($file_name =~ /^(.+)\.pod$/);

    # make it html ext if it was a pod
    $file_name =~ s/\.pod$/.html/;

    my $self = bless {
		      book_obj  => $book_obj,
		      base_name => $base_name,
		      file_name => $file_name,
		      src_file  => $src_file,
		      src_root  => $src_root,
		      verbose   => $verbose,
		      curr_page => $curr_page,
		      curr_page_index => $curr_page_index,
		      prev_page => $prev_page,
		      next_page => $next_page,
                      doc_root  => $doc_root,
		      content => [],
		      title   => '',
		      body    => '',
		      index   => '',
		      change_time_stamp => '',
		     }, ref($class)||$class;
    
    # read the file and attributes
    $self->get_pod_file();
    
    return $self;
    
}	  # end of sub new


# you can only retrieve data from this class, you cannot modify it.
##############
sub get_param{
  my $self = shift;

  return () unless @_;
  return unless defined wantarray;	
  my @values = map {defined $self->{$_} ? $self->{$_} : ''} @_;

  return wantarray ? @values : $values[0];

} # end of sub get_param


# read the file set the content and attibutes
#############
sub get_pod_file{
  my $self = shift;

  my $src_file = $self->{src_file};
  Pod::HtmlPsPdf::Common::read_file($src_file,$self->{content});

    # file change timestamp
  my ($mon,$day,$year) = (localtime ( (stat($src_file))[9] ) )[4,3,5];
  $self->{change_time_stamp} = sprintf "%02d/%02d/%04d", ++$mon,$day,1900+$year;

} # end of sub get_pod_file

=item podify_items()

  podify_items(\@pars);

Podify text to represent items in pod, e.g:

 1 Some text from item Item1
  
 2 Some text from item Item2

becomes:

 =over 4
 
 =item 1
 
 Some text from item Item1

 =item 2
 
 Some text from item Item2

 =back

podify_items() accepts 'C<*>' and digits as bullets

podify_items() receives a ref to array of paragraphs as a parameter
and modifies it. Nothing returned.

=cut

#################
sub podify_items{
  my $self = shift;

    # tmp result
  my @pars = ();
  my $items = 0;
  foreach (@{$self->{content}}) {
      # is it an item?
    if (s/^(\*|\d+)\s+/=item $1\n\n/) {
      $items++;
        # first time insert the =over pod tag
      push @pars, "=over 4" if $items == 1;
      push @pars, split /\n\n/, $_;
    } else {
        # comlete the =over =item =back tag
      push @pars, "=back" if $items;
      push @pars, $_;
        # not a tag item
      $items = 0;
    }
  } # end of foreach (@$r_pars)

    # update the content
   @{$self->{content}} = @pars;
} # end of sub podify_items


# convert POD => HTML
#############
sub pod2html{
  my $self = shift;
  my @podpath = qw(.);

  my $book_obj = $self->{book_obj};
  my ($rh_main_toc,$rh_valid_anchors, $rh_links_to_check) = 
    $self->{book_obj}->get_param(qw(rh_main_toc rh_valid_anchors rh_links_to_check));

#  print "###: $rh_valid_anchors, $rh_links_to_check\n";

      # @content enters as pod, when returns - it's html
  Pod::HtmlPsPdf::Html::pod2html(\@podpath,
			      $self->{src_root},
			      $self->{doc_root},
			      $self->{verbose},
			      $self->{content},
			      $rh_main_toc,
			      $self->{curr_page},
			      $self->{curr_page_index},
			      $rh_valid_anchors,
			      $rh_links_to_check
			     );

#  print "###: $rh_valid_anchors, $rh_links_to_check\n";

} # end of sub pod2html


# splits the content into parts like title, index, body 
#############
sub parse_html{
  my $self = shift;

  my $content = join "", @{$self->{content}};
  # extract the body 
  my ($title) = ($content =~ m|<TITLE>(.*)</TITLE>|si);
  my ($body)  = ($content =~ m|<BODY[^>]*>(.*)</BODY>|si);
  $title ||= '';
  $body  ||= '';

  # extract index
  my $index = 
    $body =~ s|<!-- INDEX BEGIN -->(.*)<!-- INDEX END -->||si
      ? $1 : '';

  # remove first header
  $body =~ s|<H1[^<]+</H1>||si;

  # add a left colour bar for <pre></pre> sections
  $body =~ 
    s{
      <PRE>(.*?)</PRE>
     }
    {
    <table>
      <tr>

	<td bgcolor="#eeeeee" width="1">
	  &nbsp;
        </td>

	<td>
	  <pre>$1</pre>
        </td>
	    
      </tr>
    </table>
    }gsix;




  $self->{title} =  $title || '';
  $self->{body}  = $body || '';
  $self->{index} = $index || '';

#  for (qw(title index)) {
#    print "\n" x 3, "$_ :\n",$self->{$_},"\n";
#  }
#  exit;

} # end of sub parse_html


# writes an html file
##################
sub write_html_file{
  my $self = shift;

    # read the template page
  my $template = $config->get_param('tmpl_page_html');
  my @page_tmpl = ();
  Pod::HtmlPsPdf::Common::read_file($template,\@page_tmpl);

    # convert
  $self->template2release(\@page_tmpl);

  ### apply html version specific changes

#  # add the <a name="anchor##"> for each para
#  my $anchor_count = 0;
#  for (@page_tmpl){
#    s|\n<P>\n|qq{\n<P><A NAME="anchor}.$anchor_count++.qq{"></A>\n}|seg
#  }

  # add the links to #toc, before the <HR> tags. But skip the first
  # <HR> tgas, so we will start the replace after the TOC itself.
  my $after_toc = 0;
  for (@page_tmpl){
    $after_toc = 1 if !$after_toc and m|</UL>|i;
    next unless $after_toc;
    s|<HR>|[ <B><FONT SIZE=-1><A HREF="#toc">TOC</A></FONT></B> ]\n<HR>|sgi
  }

    # write the file
  my $rel_root = $config->get_param('rel_root');
  Pod::HtmlPsPdf::Common::write_file("$rel_root/$self->{file_name}",\@page_tmpl);

} # end of sub write_html_file



# writes an html file for the later PS generation
##################
sub write_ps_html_file{
  my $self = shift;

    # read the template page
  my $template = $config->get_param('tmpl_page_ps');
  my @page_tmpl = ();
  Pod::HtmlPsPdf::Common::read_file($template,\@page_tmpl);

    # convert
  $self->template2release(\@page_tmpl);

  ### apply html PS version specific changes

  for (@page_tmpl){
 
    if ($Pod::HtmlPsPdf::RunTime::options{slides_mode}) {
      # create the page breakers for slides 
      s|<HR>|<HR class=PAGE-BREAK>|gsi;
    } else {
      # remove the <HR> tags
      s|<HR>||gsi;
    }

      # bump up the $digit in the <h$digit></h$digit> by one to create a
      # nice structured PS/PDF, must skip the first <h1> standing for
      # the name of the chapter 
    s|(</?H)(\d)>|$1.($2+1).">"|egsi;
  }

    # write the file
  my $ps_root = $config->get_param('ps_root');
  Pod::HtmlPsPdf::Common::write_file("$ps_root/$self->{file_name}",\@page_tmpl);

} # end of sub write_ps_html_file


# convert the template into the release version
# input: ref to template array
###################
sub template2release{
  my $self    = shift;
  my $ra_tmpl = shift;

  my %replace_map = 
    (
     DOC_ROOT => $self->{doc_root},
     PREVPAGE => ($self->{prev_page} 
                  ? qq{<a href="$self->{prev_page}">Prev</a>}
                  : ''
                  ),
     NEXTPAGE => ($self->{next_page} 
                  ? qq{<a href="$self->{next_page}">Next</a>}
                  : ''
                  ),
     TITLE    => $self->{title},
     INDEX    => $self->{index},
     BODY     => $self->{body},
     MODIFIED => $self->{change_time_stamp},
    );

  for (@{$ra_tmpl}){
    s/\[(\w+)\]/$replace_map{$1}/g 
  }

} # end of sub template2release




# writes a split html files for easier searching if the original html
# is too big
#
# note that this function destroyes all the parsed structures, if you
# intend to make a use of them later, consider using the copy instead.
#
##################
sub write_split_html_files{
  my $self = shift;

  print "+++ Making split\n" if $Pod::HtmlPsPdf::RunTime::options{verbose};

    # prepare directory
  my $split_root  = $config->get_param('split_root');
  my $split_base_dir = "$split_root/".$self->{base_name};
  my $dir_mode   = $config->get_param('dir_mode');
  mkdir $split_base_dir, $dir_mode unless -d $split_base_dir;

    # read the template page
  my $template = $config->get_param('tmpl_page_split_html');
  my @page_tmpl = ();
  Pod::HtmlPsPdf::Common::read_file($template,\@page_tmpl);

### parse html ###

# 1. parse index
# 2. parse body

### parse index ###

  #########################################
  # the datastructure we are going to build
  #
  # $node->[$idx_link]->$
  # $node->[$idx_level]->$
  # $node->[$idx_parent]->$
  # $node->[$idx_kids]->[]
  #########################################

  my $root_anchor = 'index.html';
  my @parents = ($root_anchor);
  my @keys = qw(link level parent kids);
  my %keys = ();
  @keys{@keys} = 0..$#keys;
  my %index = ();
  my $level = 0;

    # initialize the root entry
  $index{$root_anchor}->[$keys{link}]   
    = qq{<LI><A HREF="index.html"><b>}.$self->{title}.qq{</b></A>};
  $index{$root_anchor}->[$keys{level}]  = $level;
  $index{$root_anchor}->[$keys{parent}] = '';
  $index{$root_anchor}->[$keys{kids}]   = [];

  while ( $self->{index}  =~ /(.*\n)/g ){
    local $_ = $1 || '';
      # assumption: there can be one tag/item per line!
    chomp;

      # start of level
    if (m|<ul>|i){
      $level++;
      next;
    }

      # end of level
    if (m|</ul>|i){
      $level--;
      pop @parents;
      next;
    }

    next unless m|<li>|i;

    s/A HREF="#(\w+)"/A HREF="$1.html"/i;
    my $anchor = $1 || '';
    warn("!!! No anchor found"), next unless $anchor;
      # make it html
    $anchor .= '.html';

    $index{$anchor}->[$keys{link}] = $_;
    $index{$anchor}->[$keys{level}] = $level;

    my $parent_anchor = $parents[$level-1];
    $index{$anchor}->[$keys{parent}] = $parent_anchor;
    splice @parents,$level,1,$anchor;

      # now update parent's kids field by pushing yourself to the end
    push @{ $index{$parent_anchor}->[$keys{kids}] }, $anchor;

      # init your kids
    $index{$anchor}->[$keys{kids}] = [];

  } # end of while ( $self->{index}  =~ /(.*\n)/g )

### parse body ###

    # create the split version:
  my %sections = ();
  my $prev_anchor = '';
  my %item = ();

  while ( $self->{body}  =~ /(.*\n)/g ){
    my $line = $1;

    if ( $line =~ m|<H(\d)><A NAME="([^\"]+)">([^>]+)</A></H|){

      my $anchor = $2;

        # was a previous item already?
      if (%item) {

	  # curr anchor is the 'next' for the 'prev' item
	$item{next} = $anchor;

	  # store it away
	%{$sections{$item{anchor}}} = %item;

	  # set the previous anchor
	$prev_anchor = $item{anchor};

	  # reset the prev item
	%item = ();
      }

        # start a new item
      $item{level}  = $1;
      $item{anchor} = $anchor;
      $item{title}  = $3;
      $item{prev}   = $prev_anchor;

#        # find out who are your parents
#      splice @parents, $item{level}-1;
#        # remember who are your parents
#      @{$item{parents}} = @parents;
#        # add yourself as a father
#      push @parents,$item{anchor} ;

    } else {

        # %item wasn't started yet
      next unless %item;

      $item{content} .= "$line";
    } # end of if ( $line =~ m|<H(\d)><A NAME....

  } # end of while ( $self->{body}  =~ /(.*)/g ){


      # last item add (cannot be added in while loop)
  if (%item) {

      # curr anchor is the 'next' for the 'prev' item
    $item{next} = '';

      # store it away
    %{$sections{$item{anchor}}} = %item;

      # reset the prev item
    %item = ();

  } # end of if (%item)


### generate split html pages ###

  my $pod_title = $self->{title};
  for my $sec (keys %sections) {

    my $anchor = "$sec.html";

    ### prepare the top index with parents dynasty only ###
    my $top_index = '';
    my $parent_anchor = $anchor;
    my @parents = ();
    while ($parent_anchor = $index{$parent_anchor}->[$keys{parent}]) {
      unshift @parents, $parent_anchor;
    }
    my @close_top_index = ();

      # top level
    {
      local $_ = $anchor;
      $top_index .= "<UL>\n";
      $top_index .=  qq{<LI><a href="../index.html">Split Version TOC</a>\n};
      unshift @close_top_index,"</UL>";
    }

    for (@parents,$anchor) {
      my $level = $index{$_}->[$keys{level}] + 1;
      $level *= 4;
      $top_index .= ' ' x $level . "<UL>\n";
      if ($_ eq $anchor) {
	my $title =  $sections{$sec}->{title};
	$top_index .= qq{<h1><font color="brown">$title</font></h1>\n};
      } else {
	$top_index .= $index{$_}->[$keys{link}] . "\n";
      }
      unshift @close_top_index,' ' x $level ."</UL>";
    }


    $top_index .= join "\n",@close_top_index,"\n";

    ### generate sub-index for this section ###
    my $level =  $index{$anchor}->[$keys{level}];
    my @close_bot_index = ();
    my $bot_index = "\n";
    for (0..$level) {
      $bot_index .= "    " x $level . "<UL>\n";
    }
    $bot_index .= nested_kids(\%index,$anchor,$keys{kids},$keys{link});
    for (0..$level) {
      $bot_index .= "    " x $level . "</UL>\n";
    }

    # recursive function
    ################
    sub nested_kids{
      my $r_index = shift;
      my $anchor  = shift;
      my $kids_key = shift;
      my $link_key = shift;
      
      my @kids = @{$r_index->{$anchor}->[$kids_key]};
      return '' unless @kids;
      
      my $subidx .= "<UL>\n";
      for my $kid_anchor (@kids){
	$subidx .= $r_index->{$kid_anchor}->[$link_key] . "\n";
	$subidx .= nested_kids($r_index,$kid_anchor,$kids_key,$link_key);
      }
      $subidx .= "</UL>\n";
      return $subidx;
    } # end of sub nested_kids


      # set values for template conversion.
      # these destroy the original values
    $self->{full_title} = $top_index;
    $self->{title} = $sections{$sec}->{title};
    $self->{body}  = $sections{$sec}->{content};

      # fix internal links to work with subdirs created by the split version
    $self->{body} =~ s|A HREF="(?:[\./]*)?([^.]+)\.html#?"|A HREF="../$1/index.html"|g;
    $self->{body} =~ s|A HREF="(?:[\./]*)?([^.]+)\.html#(.+?)"|A HREF="../$1/$2.html"|g;

      # append the submenu
    $self->{body}  .= $bot_index;

      # convert
    my @tmpl_copy = @page_tmpl;
    $self->split_template2release(\@tmpl_copy);

      # write the section as a file
    my $filename = "$split_base_dir/".$sections{$sec}->{anchor}.".html";
    Pod::HtmlPsPdf::Common::write_file($filename,\@tmpl_copy);

  } # end of for (keys %sections)

### now write the index.html for original file ###
    # correct the links
  $self->{index} =~ s/A HREF="#([^\"]*)"/A HREF="$1.html"/g;
  $self->{body} = $self->{index}."\n<hr>";
  $self->{full_title} = qq{<h1><font color="brown">$pod_title</font></h1>\n};
  $self->{title} = $pod_title;

  my @tmpl_copy = @page_tmpl;
  $self->split_template2release(\@tmpl_copy);

    # write the file
  my $filename = "$split_base_dir/index.html";
  Pod::HtmlPsPdf::Common::write_file($filename,\@tmpl_copy);


} # end of sub write_split_html_files

# convert the template into the release version
# input: ref to template array
###################
use File::Basename;

sub split_template2release{
  my $self    = shift;
  my $ra_tmpl = shift;

  my $full_file_name = $self->{curr_page};
  #$full_file_name =~ s|^.*?(\w+)\.\w+$|$1|;
  # Just want the basename, correct?  Is it always .pod?
  $full_file_name = $1 if $full_file_name =~ m!([^/]+)\.\w+$!;  
  
  my %replace_map = 
    (
     PAGE     => $full_file_name,
     DOC_ROOT => $self->{doc_root},
     PREVPAGE => ($self->{prev_page} 
                  ? qq{<a href="$self->{prev_page}">Prev</a>}
                  : ''
                  ),
     NEXTPAGE => ($self->{next_page} 
                  ? qq{<a href="$self->{next_page}">Next</a>}
                  : ''
                  ),
     FULLTITLE=> $self->{full_title},
     TITLE    => $self->{title},
#     INDEX    => $self->{index},
     BODY     => $self->{body},
     MODIFIED => $self->{change_time_stamp},
    );

  for (@{$ra_tmpl}){
    s/\[(\w+)\]/$replace_map{$1}/g 
  }

} # end of sub split_template2release






1;
__END__
