package Pod::HtmlPsPdf::Config;

use strict;
use Carp;

# META: probably move FindBin here
use File::Basename ();

# passed only once and then cached 
use vars qw($config_file);

# called in BEGIN block before the rest of the code
####################
sub set_config_file{
    $config_file = shift || '';

    croak "Configuration file $config_file is not readable: $!" 
	unless -r $config_file;

} # end of sub set_config_file


########
sub new{
    my ($class) = shift;

    my $self = bless {}, ref($class)||$class;
    $self->_init(@_);

    return $self;
} # end of sub new

########## 
sub _init{ 
    my $self = shift;
    my $new_config_file = shift || '';

    set_config_file($new_config_file) if $new_config_file;

    croak "Configuration file is not specified" 
	unless $config_file;
 

    # The root directory of the project 
    my $root      = File::Basename::dirname $config_file;
    $self->{root} = $root;

    # do all the work from the base directory
    chdir $root;
    
    # process user's configuration file
    require $config_file;
    use vars qw(%c);
    local *c = \%Pod::HtmlPsPdf::Config::Local::c;

    # dirs (will be mostly created dynamically
    $self->{src_root}   = $c{dir}{src}        || cnf_err('$c{dir}{src}');
    $self->{ps_root}    = $c{dir}{rel_ps}     || cnf_err('$c{dir}{rel_ps}');
    $self->{rel_root}   = $c{dir}{rel_html}   || cnf_err('$c{dir}{rel_html}');
    $self->{split_root} = $c{dir}{split_html} || cnf_err('$c{dir}{split_html}');
    $self->{out_dir}    = $c{dir}{out}        || cnf_err('$c{dir}{out}');

    # setting the filenames to process and checking their existance
    $self->{pod_files}    = $c{ordered_pod_files} || cnf_err('$c{ordered_pod_files}');

    $self->fixup_pm_files;



    
    $self->{nonpod_files} = $c{non_pod_files}     || cnf_err('$c{non_pod_files}');
    for (@{$self->{pod_files}}, @{$self->{nonpod_files}}) {
	croak "Can't find @{[$self->{src_root}]}/$_: $!" 
	    unless -r $self->{src_root}."/$_";
    }

    


    # set and check that we can read the template files
    my @tmpl_files = qw(index_html index_ps page_html page_ps page_split_html);
    for (@tmpl_files) {
	$self->{'tmpl_'.$_} = $c{tmpl}{$_} || cnf_err('$c{tmpl}{'.$_.'}');
	croak "Can't find @{[$self->{'tmpl_'.$_}]}: $!" 
	    unless -r $self->{'tmpl_'.$_};
    }

    $self->{out_name} = $c{out_name} || cnf_err('$c{out_name}');

    $self->{toc_file} = $c{file}{toc_file}           || cnf_err('$c{file}{toc_file}');
    
    $self->{version_file} = $c{file}{version_file}   || cnf_err('$c{file}{version_file}');
    $self->{package_name} = $c{package_name}         || cnf_err('$c{package_name}');

    $self->{html2ps_conf} = $c{file}{html2ps_conf}   || cnf_err('$c{file}{html2ps_conf}');

    $self->{dir_mode} = $c{mode}{dir}     || cnf_err('$c{mode}{dir}');
    $self->{html2ps_exec} = which( 'html2ps' ); 
    chomp $self->{html2ps_exec};
    
    return $self;
    
} # end of sub _init

use File::Basename;

# HACK ALERT

sub fixup_pm_files {
    my $self = shift;

    my @remove_files;

    for ( @{$self->{pod_files}} ) {
        next unless /\.(pm|cgi|pl)(\.in)?$/;
        my $orig_file = "$self->{src_root}/$_";


        my ($name,$path,$suffix) = fileparse($_,'\.pm\.in', '\.pm', '\.cgi\.in', '\.cgi', '\.pl', '\.pl\.in');

        $_ = "$name.pod";

        my $out_file = "$self->{src_root}/$_";

        open(IN,"<$orig_file")   or die "Can't open $orig_file: $!";
        open(OUT, ">$out_file" ) or die "Failed to open '$out_file':$!";

        push @remove_files, $out_file;

        my $cut = 1;
        local $_;
        while (my $line = <IN>) {
            $cut = $1 eq 'cut' if $line =~ /^=(\w+)/;
            next if $cut;
            print OUT $line
                or die "Can't print $out_file: $!";
        }
        close OUT;
    }

    $self->{remove_files} = \@remove_files if @remove_files;
}

sub DESTROY {
    my $self = shift;

    if ( $self->{remove_files} ) {
        print "Removing temporary pod files [@{$self->{remove_files}}]\n";

        unlink @{$self->{remove_files}};
    }
}



# user configuration parsing errors reporter
############
sub cnf_err{
    croak "$_[0] from package Pod::HtmlPsPdf::Config::Local is not
    defined\n";
}# end of sub cnf_err


# you can only retrieve data from this class, you cannot modify it.
##############
sub get_param{
  my $self = shift;

  return () unless @_;
  return unless defined wantarray;	
  my @values = map {defined $self->{$_} ? $self->{$_} : ''} @_;

  return wantarray ? @values : $values[0];

} # end of sub get_param


sub which {
    my $cmd = shift;
    
    foreach my $dir (split( ':', $ENV{PATH})) {
        return "$dir/$cmd" if -x "$dir/$cmd";
    }

	return;
}



1;
__END__
