package Pod::HtmlPsPdf::Common;

use strict;
use Symbol ();
use File::Basename ();
use File::Copy ();

use Pod::HtmlPsPdf::Config ();
my $config = Pod::HtmlPsPdf::Config->new();

### Common functions

# copy_file($src_filename,$dest_filename); $src_filename will be
# placed into the specified path, if one of the directories in the
# target path doesn't exist -- it'll be created.
###############
sub copy_file{
    my ($src,$dst) = @_;
    
    # make sure that the directory exist or create one
    
    my $base_dir = File::Basename::dirname $dst;
    create_dir($base_dir) unless (-d $base_dir);
    
    File::Copy::copy($src,$dst);
    
} # end of sub copy_file


# write_file($filename,$ref_to_array);
# content will be written to the file from the passed array of
# paragraphs
###############
sub write_file{
    my ($fn,$ra_content) = @_;
    
    # make sure that the directory exist or create one
    
    my $base_dir = File::Basename::dirname $fn;
    create_dir($base_dir) unless (-d $base_dir);
    
    my $fh = Symbol::gensym;
    open $fh, ">$fn" or die "Can't open $fn for writing: $!\n";
    print $fh @$ra_content;
    close $fh;

} # end of sub write_file


# recursively creates a multi-layer directory
###############
sub create_dir{
    my $dir = shift || '';
    return if !$dir or -d $dir;
    my $ancestor_dir = File::Basename::dirname $dir;
    create_dir($ancestor_dir);
    my $mode = $config->get_param('dir_mode');
    mkdir $dir, $mode;
}

# read_file_paras($filename,$ref_to_array);
# content will be returned in the passed array of paragraphs
###############
sub read_file{
  my ($fn,$ra_content) = @_;

  my $fh = Symbol::gensym;
  open $fh, $fn  or die "Can't open $fn for reading: $!\n";
  local $/ = "";
  @$ra_content = <$fh>;
  close $fh;

} # end of sub read_file


1;
__END__
