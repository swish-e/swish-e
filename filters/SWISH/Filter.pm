cd /data/_a/internetindexpackage SWISH::Filter;

use 5.005;
use strict;
use File::Basename;
#use MIME::Types;  # require below

use vars qw/ $VERSION %extra_methods $testing/;

$VERSION = '0.01';

# Define the available parameters
%extra_methods = map {$_ => 1} qw/name user_data /;


# Map content types to swish-e parsers.

my %swish_parser_types = (
    'text/html'     => 'HTML*',
    'text/xml'      => 'XML*',
    'text/plain'    => 'TXT*',
);

$testing = 0;


# For testing only 

if ( $0 eq 'Filter.pm' && @ARGV >= 2 && shift =~ /^test/i) {

    print STDERR "Testing mode for $0\n\n",'=-' x 35, "=\n";
    $testing++;    

    my $filter = SWISH::Filter->new();

    while ( my $file = shift @ARGV ) {

        my %user_data;
        $user_data{pdf}{title_tag} = 'producer';

        if ( $filter->filter( document => $file, user_data => \%user_data ) ) {

            print "File: $file\nContent-type: ",
                  $filter->content_type, "\n\n",
                  substr( ${$filter->fetch_doc}, 0, 800 );

        } else {
            print "File: $file\nContent-type: ",
                  $filter->content_type, "\n\n",
                  "** NOT FILTERED **\n";
        }

        print "\n",'=-' x 35, "=\n";
    }

    exit;
}

    


=head1 NAME

SWISH::Filter - Perl extension for filtering documents with Swish-e

=head1 SYNOPSIS

  use SWISH::Filter;
  my $filter = SWISH::Filter->new;

  my $was_filtered = $filter->filter(
        document     => \$scalar_ref,  # path or ref to a doc
        content_type => $content_type, # content type if doc reference
        name         => $real_path,    # optional name for this file (useful for debugging)
        user_data    => $whatever,     # optional data to make available to filters
   );


  # Print out the doc
  if ( $was_filtered ) {
      my $doc_ref = $filter->fetch_doc;
      print $$doc_ref;
  }

  # Fetch the content type of the document
  my $content_type = $filter->content_type;

  # Fetch swish-e parser type (TXT*, XML*, HTML*)
  my $doc_type = $filter->swish_parser_type;



=head1 DESCRIPTION

SWISH::Filter provides a unified way to convert documents into a type that swish-e
can index.  Individual filters are installed as separate perl modules.  For example,
there might be a filter that converts from PDF format to HTML format.

The filters are automatically loaded when C<SWISH::Filters::new()> is called.  Filters define
a type and priority that determines the processing order of the filter.  Filters are processed
in this sort order until a filter accepts the document for filtering.
Filters use the documents's content type to determine if the filter should handle the current document.

Normally, once a document is filtered processing stops.  Some filters can filter the document
yet set a flag saying that filtering should continue (for example a filter that uncompresses a
MS Word document before passing on to the filter that converts from MS Word to text).

The idea is that new filters can be created, and then downloaded and installed to provide new
filtering capabilities.  For example, if you needed to index MS Excel documents you might be able to
download a filter from the Swish-e site and magically next time you run indexing MS Excel docs would be
indexed.

The SWISH::Filter setup can be used with -S prog or -S http.  The -S http method uses a
Perl helper script called F<swishspider>, which by default will attempt to use the
SWISH::Filter module.  (This default action can be disabled in the swishspider script.)
The F<spider.pl> program included for spidering with -S prog method can also use the SWISH::Filter
module.  See the example provided in the F<prog-bin/SwishSpiderConfig.pl> file.
You will see better performance using -S prog (i.e. with spider.pl) than with -S http because
with -S http the modules must be loaded and compiled for every URL processed instead of once
with -S prog method.

Note: This is just one of a few methods for filtering documents with Swish-e.  Swish-e has
a FileFilter configuration directive for filtering from within Swish-e.
Also, the F<prog-bin/SwishSpiderConfig.pl> example spider configuration file shows 
examples of calling filter modules directly.

=head1 METHODS

=over 4

=item $swish_filter = SWISH::Filter->new()

This creates a SWISH::Filter object.  You may pass in options as a list or a hash reference.

=back

=head2 Options

There is currently only one option that can be passed in to new():

=over 4

=item ignore_filters

Pass in a reference to a list of filter names to ignore.  For example, if you have two filters installed
"Pdf2HTML" and "Pdf2XML" and want to avoid using "Pdf2XML":

    my $swish_filter = SWISH::Filter->new( ignore_filters => ['Pdf2XML'];

=cut

sub new {
    my $class = shift;
    $class = ref( $class ) || $class;

    my %attr = ref $_[0] ? %{$_[0]} : @_ if @_;

    my $self = bless {}, $class;

    $self->{skip_filters} = {};

    $self->ignore_filters( $attr{ignore_filters} ) if $attr{ignore_filters};

    $self->get_filter_list;

    eval { require MIME::Types };
    if ( $@ ) {
        $class->mywarn( "Failed to load MIME::Types\n$@\nInstall MIME::Types for more complete MIME support");
   
        # handle the lookup for a small number of types locally
        $self->{mimetypes} = $self;

    } else {
        $self->{mimetypes} = MIME::Types->new;
    }
 

    return $self;
}

# Here's some common mime types
my %mime_types = (
    doc   => 'application/msword',
    pdf   => 'application/pdf',
    html  => 'text/html',
    htm   => 'text/html',
    txt   => 'text/plain',
    text  => 'text/plain',
    xml   => 'text/xml',
    mp3   => 'audio/mpeg',
);    

sub mimeTypeOf {
    my ( $self, $file ) = @_;
    $file =~ s/.*\.//;
    return $mime_types{$file} || undef;
}
    
    

sub ignore_filters {
    my ( $self, $filters ) = @_;

    unless ( $filters ) {
        return unless $self->{ignore_filter_list};
        return @{$self->{ignore_filter_list}};
    }

    @{$self->{ignore_filter_list}} = @$filters;
    

    # create lookup hash for filters to skip
    $self->{skip_filters} = map { $_, 1 } @$filters;
}

=item $filtered = $swish_filter->filter();

This method filters a document.  Returns true if the document was filtered.
You must pass in a hash (or hash reference) of parameters.  The possible parameters are:

=over 8

=item document

This can be either a path to a file, or a scalar reference to a document in memory.

=item content_type

The MIME type of the document.  This is only required when passing in a scalar reference
to a document is passed. The content type string is what the filters used to match
a document type.

When passing in a file name and C<content_type> is not set, then the content type will
be determined from the file's extension by using the MIME::Types Perl module (available on CPAN).

=item name

Optional name to pass in to filters that will be used in error and warning messages.

=item user_data

Optional data structure that all filters may access.

This can be fetched in a filter by:

    my $user_data = $filter->user_data;

And an example use might be:    

    if ( ref $user_data && $user_data->{pdf2html}{title} ) {
       ...
    }


=back

Example:

    $filtered = $swish_filter->filter(
        document     => $doc_ref,
        content-type => 'application/pdf',
    );

This method returns true if the document was filtered, otherwise false.

=cut


sub filter {
    my $self = shift;
    my %attr = ref $_[0] ? %{$_[0]} : @_ if @_;

    # Any filters?
    return unless $self->{filters};

    delete $self->{doc_ref};
    delete $self->{doc_type};  # html|txt|xml
    $self->{content_type} = '';

    my $doc = delete $attr{document};
    die "Failed to supply document attribute 'document' when calling filter()\n"
        unless $doc;

    my $content_type = delete $attr{content_type};


    # Allow a reference to a file name (why?)

    if ( ref $content_type ) {
        my $type = $self->decode_content_type( $$content_type );

        unless ( $type ) {
            warn "Failed to set content type for file reference '$$content_type'\n";
            return;
        }
        $content_type = $type;
    }



    if ( ref $doc ) {
        die "Must supply a content type when passing in a reference to a document\n"
            unless $content_type;

    } else {
        $content_type ||= $self->decode_content_type( $doc );
        unless ( $content_type ) {
            warn "Failed to set content type for document '$doc'\n";
            return;
        }

        $attr{name} ||= $doc;
    }


    my $doc_object = SWISH::Filter::document->new( $doc, $content_type );
    return unless $doc_object;

    $self->{content_type} = $doc_object->content_type;

    local $SIG{__DIE__};
    local $SIG{__WARN__}; 


    # Look for left over config settings that we do not know about

    for ( keys %extra_methods ) {
        next unless $attr{$_};
        my $method = "set_" . $_;
        $doc_object->$method(delete $attr{$_});

        if ( $_ eq 'name' ) {
            $SIG{__DIE__} = sub { die "$$ Error- ", $doc_object->name, ": ", @_ };
            $SIG{__WARN__} = sub { warn "$$ Warning - ",$doc_object->name, ": ", @_ };
        }
    }

    die "Unknown filter config setting '$_'\n"
        for keys %attr;
        

    my @filter_set =  @{$self->{filters}};
    my @cur_filters;

    for my $filter ( @filter_set )  {
        $doc_object->continue( 0 );  # reset just in case a non-filtering filter set this

        
        my $filtered_doc;
        eval {
            local $SIG{__DIE__};
            $filtered_doc = $filter->filter($doc_object);
        };
        
        if ( $@ ) {
            $self->mywarn("Problems with filter '$filter'.  Filter disabled:\n -> $@");
            next;
        }
        
        # save the working filters in this list
        
        push @cur_filters, $filter;

        if ( $filtered_doc ) {  # either a file name or a reference to the doc

            $self->{converted}++;                   # flag that it was converted
            $doc_object->cur_doc($filtered_doc);    # and save it (filename or reference)

            # All done?
            last unless $doc_object->continue( 0 );
        }
    }
    
    
    # Replace the list of filters with the current working ones
    $self->{filters} = \@cur_filters;



    # Save the reference to the doc (filtered or not)
    # to make it easy for the caller to access
    
    $self->{doc_ref} = $doc_object->fetch_doc_reference
        if $self->{converted} || !exists $attr{no_fetch_unfiltered};  # or some such thing.



    # Return false if the doc was not converted            
    return unless delete $self->{converted};

    # Get the content-type of the last conversion
    $self->{content_type} = $doc_object->content_type;

    return 1;  # true that there was a conversion.
}

=item $doc_ref = $swish_filter->fetch_doc;

Returns a reference to a scalar that contains the document passed in.
If the file is on disk it will be read from disk into memory.

Example:

    $filtered = $swish_filter->filter( $doc_ref, 'application/pdf' );
    if ( $filtered ) {
        $doc_ref = $swish_filter->fetch_doc;
        print $$doc_ref;
    }

=cut

sub fetch_doc {
    my ( $self ) = @_;
    return $self->{doc_ref} || undef;
}

=item $type = $swish_filter->content_type

Returns the content type of the document after filtering.

=cut

sub content_type {
    return $_[0]->{content_type};
}

=item $type = $swish_filter->swish_parser_type

Returns the swish parser type for the document, or undefined.

For example, in a -S prog program you can set the parser that swish-e will
use:

    if ( my $type = $swish_filter->swish_parser_type ) {
        print "Document-Type: $type\n";
    }


=cut

sub swish_parser_type {
    my $self = shift;

    my $content_type = $self->content_type || return;

    for ( keys %swish_parser_types ) {
        return $swish_parser_types{$_} if 
            $content_type =~ /^\Q$_/;
    }

    return;
}


=item $self->mywarn()

Internal function used for writing warning messages to STDERR if $ENV{FILTER_DEBUG} is set.
Set the environment variable FILTER_DEBUG before running to see the verbose warning messages.

=cut

sub mywarn {
    my $self = shift;
    
    print STDERR @_,"\n" if $ENV{FILTER_DEBUG};
}


=item $path = $self->find_binary( $prog );

Use in a filter's new() method to test for a necesary program located in $PATH.
Returns the path to the program or undefined if not found or does not pass the -x 
file test.

=cut

use Config;
my @path_segments;

sub find_binary {
    my ( $self, $prog ) = @_;

    warn "Find path of [$prog] in $ENV{PATH}\n" if $testing;

    unless ( @path_segments ) {
        my $path_sep = $Config{path_sep} || ':';

        @path_segments = split /\Q$path_sep/, $ENV{PATH};
    }
    
    for ( @path_segments ) {
        my $path = "$_/$prog";
        $path .= '.exe' if $^O =~ /Win32/ && $path !~ /\.exe$/;

        warn "Looking at [$path]\n" if $testing;
        return $path if -x $path;
    }
    return; 
}
    

=item $bool = $self->use_modules( @module_list );

Attempts to load each of the module listed and calls its import() method.

Use to test and load required modules within a filter without aborting.

    return unless $self->use_modules( qw/ Spreadsheet::ParseExcel  HTML::Entities / );

A warning message is displayed if the FILTER_DEBUG environment variable is true.

=back

=cut

sub use_modules {
    my ( $self, @modules ) = @_;

    for my $mod ( @modules ) {
        warn "trying to load [$mod]\n" if $testing;

        eval { eval "require $mod" or die "$!\n" };

        if ( $@ ) {
            my $caller = caller();
            $self->mywarn("Can not use Filter $caller -- need to install $mod: $@");
            return;
        }

        warn " ** Loaded $mod **\n" if $testing;

        # Export back to caller
        $mod->export_to_level( 1 );
    }
    return 1;
}
                                

# Fetches the list of filters installed

sub get_filter_list {
    my ( $self ) = @_;

    my @filters;

    # Look for filters to load
    for my $inc_path ( @INC ) {
        my $cur_path = "$inc_path/SWISH/Filters";

       
        next unless opendir( DIR, $cur_path );


        while ( my $file = readdir( DIR ) ) {
            my $full_path = "$cur_path/$file";

            next unless -f $full_path;

            my ($base,$path,$suffix) = fileparse( $full_path,"\.pm");


            next unless $suffix eq '.pm';
            

            # Should this filter be skipped?
            
            next if $self->{skip_filters}{$base};

            

            eval { require "SWISH/Filters/${base}$suffix" };
            if ( $@ ) {
                if ( $ENV{FILTER_DEBUG} ) {
                    print STDERR "Failed to load 'SWISH/Filters/${base}$suffix'\n",
                    '-+' x 40, "\n",
                    $@,
                    '-+' x 40, "\n";
                }
                next;
            }

            my $package =  "SWISH::Filters::" . $base;
            
            my $filter = $package->new( name => $full_path );
            
            next unless $filter;  # may not get installed
            
            push @filters, $filter;
        }
    }

    unless ( @filters ) {
        warn "No SWISH filters found\n";
        return;
    }


    # Now sort the filters in order.
    $self->{filters} = [ sort { $a->type <=> $b->type || $a->priority <=> $b->priority } @filters ];
}


# Set default method for name() type() and priority()

sub name { "Filter did not set a name" }

sub type { 2 }
sub priority{ 50 }


sub decode_content_type {
    my ( $self, $file ) = @_;

    return unless $file;

    return ($self->{mimetypes})->mimeTypeOf($file);
}



#=========================================================================
package SWISH::Filter::document;
use strict;
use File::Temp;
use Symbol;

use vars '$AUTOLOAD';

=head1 WRITING FILTERS

Filters are standard perl modules that are installed into the SWISH::Filters name space.
Filters are not complicated -- see the existing filters for examples.

Filters are linked together in a chain, and have a type and priority that set the order of the
filter in the chain.  Filters check the content type of the document to see if they should process
the document.  If the filter processes the document it returns either a file name or a reference to a scalar.
Normally a reference to a scalar of the converted document should be returned.
If the filter does not process the document then it returns undef and the next filter in the chain has a chance
to process the document.

If a filter calls die then the filter is removed from the chain and will not be
called again I<during the same run>.  Calling die when running with -S http or -S fs has no effect since the
program is run once per document.

Once a filter returns something other than undef no more filters will be called.  If the filter calls $filter->set_continue
then processing will continue as if the file was not filtered.  For example, a filter can uncompress data and then
set $filter->set_continue and let other filters process the document.


This is the list of methods the filter should or may define (as specificed):

=over 4

=item new()  * required *

This method returns either an object which provides access to the filter, or undefined
if the filter is not to be used.

The new() method is a good place to check for required modules or helper programs.
Returning undefined prevents the filter from being included in the filter chain.

=item filter() * required *

This is the function that does the work of converting a document from one content type
to another.

=item name()

This should return the name of the filter.  The name is passed in in the new() call.
This is not required, but is recommended.

See example below.

=item type()

Returns a number from 1 to 10.  Filters are sorted (for processing in a specific order)
and this number is simply the primary key used in sorting.  If not specified
the filter's type used for sorting is 2.


=item priority()

Returns a number from 1 to 10.  Filters are sorted (for processing in a specific order)
and this number is simply the primary key used in sorting.  If not specified
the filter's priority is 50.

=back

Again, the point of the type() and priority() methods is to allow setting the sort order
of the filters.  Useful if you have two filters that for filtering similar content-types,
but prefer to use one over the other.

Here's a module to index MS Word documents using the program "catdoc":

    package SWISH::Filters::Doc2txt;
    use vars qw/ @ISA $VERSION /;

    $VERSION = '0.01';
    @ISA = ('SWISH::Filter');

    sub new {
        my ( $pack, %params ) = @_;

        my $self = bless {
            name => $params{name} || $pack,
        }, $pack;


        # check for helpers
        for my $prog ( qw/ catdoc / ) {
            my $path = $self->find_binary( $prog );
            unless ( $path ) {
                $self->mywarn("Can not use Filter $pack -- need to install $prog");
                return;
            }
            $self->{$prog} = $path;
        }

        return $self;

    }

    sub name { $_->{name} || 'unknown' };



    sub filter {
    my ( $self, $filter) = @_;

        # Do we care about this document?
        return unless $filter->content_type =~ m!application/msword!;

        # We need a file name to pass to the catdoc program
        my $file = $filter->fetch_filename;


        # Grab output from running program
        my $content = $filter->run_program( $self->{catdoc}, $file );

        return unless $content;

        # update the document's content type
        $filter->set_content_type( 'text/plain' );

        # return the document
        return \$content;
    }
    1;


=head2 Methods Available to Filters

The filter's "filter" function is passed an object.  Method calls may be made on this object to
check the document's current content type, or to fetch the document as either a file name or
a reference to a scalar containing the document content.

=over 4

=cut

sub new {
    my ( $class, $doc, $content_type ) = @_;

    return unless $doc && $content_type;

    my $self = bless {}, $class;
    
    if ( ref $doc ) {
        unless ( length $$doc ) {
            warn "Empty document passed to filter\n";
            return;
        }

        die "Must supply a content type when passing in a reference to a document\n"
            unless $content_type;

        
    } else {

        unless ( -r $doc ) {
            warn "Filter unable to read doc '$doc': $!\n";
            return;
        }
    }

    $self->set_content_type( $content_type );            

    $self->{cur_doc} = $doc;

    return $self;
}    

sub DESTROY {
    my $self = shift;
    $self->remove_temp_file;
}

sub cur_doc {
    my ( $self, $doc_ref ) = @_;
    $self->{cur_doc} = $doc_ref if $doc_ref;
    return $self->{cur_doc};
}

sub remove_temp_file {
    my $self = shift;

    unlink delete $self->{temp_file} if $self->{temp_file};
}
    
=item $file_name = $filter->fetch_filename;

Returns a path to the document as stored on disk.
This name can be passed to external programs (e.g. catdoc) that expect input
as a file name.

If the document is currently in memory then a temporary file will be created.  Do not expect
the file name passed to be the real path of the document.

=cut


# This will create a tempoary file if file is in memory

sub fetch_filename {
    my ( $self ) = @_;

    return ref $self->{cur_doc}
        ? $self->create_temp_file
        : $self->{cur_doc};
}

=item $doc_ref = $filter->fetch_doc_reference;

Returns a scalar reference to the document.  This can be used when the filter
can operate on the document in memory (or if an external program expects the input
to be from standard input).

If the file is currently on disk then it will be read into memory.  If the file was stored
in a temporary file on disk the file will be deleted once read into memory.

The file is stored with binmode set unless the content type is "text/".

=cut

sub fetch_doc_reference {
    my ( $self ) = @_;

    return ref $self->{cur_doc}  # just $self->read_file should work
        ? $self->{cur_doc}
        : $self->read_file;
}

=item $filter->set_continue;

Processing will continue to the next filter if this is set to a true value.
This should be set for filters that change encodings or uncompress documents.

=cut

sub set_continue {
    my ( $self ) = @_;
    return $self->continue(1);
}

sub continue {
    my ( $self, $continue ) = @_;
    my $old = $self->{continue} || 0;
    $self->{continue}++ if $continue;
    return $old;
}


=item $content_type = $filter->content_type;

Fetches the current content type for the document.

Example:

    return unless $filter->content_type =~ m!application/pdf!;


=cut

sub content_type {
    return $_[0]->{content_type} || '';
}


=item $filter->set_content_type( $type );

Sets the content type for a document.

=cut

sub set_content_type {
    my ( $self, $type ) = @_;
    die "Failed to pass in new content type\n" unless $type;
    $self->{content_type} = $type;
}


sub read_file {
    my $self = shift;
    my $doc = $self->{cur_doc};
    return $doc if ref $doc;

    my $sym = gensym();
    open($sym, "<$doc" ) or die "Failed to open file '$doc': $!";
    binmode $sym unless $self->{content_type} =~ /^text/;
    local $\ = undef;
    my $content = <$sym>;
    close $sym;
    $self->{cur_doc} = \$content;

    # Remove the temporary file, if one was created.
    $self->remove_temp_file;

    
    return $self->{cur_doc};
}


# write file out to a temporary file

sub create_temp_file {
    my $self = shift;
    my $doc = $self->{cur_doc};

    return $doc unless ref $doc;

    my ( $fh, $file_name ) = File::Temp::tempfile();

    # assume binmode if we need to filter...
    binmode $fh unless $self->{content_type} =~ /^text/;

    print $fh $$doc or die "Failed to write to '$file_name': $!";
    close $fh or die "Failed to close '$file_name' $!";

    $self->{cur_doc} = $file_name;
    $self->{temp_file} = $file_name;

    return $file_name;
}


=item $filter->name

Fetches the name of the current file.  This is useful for printing out the
name of the file in an error message.
This is the name passed in to the SWISH::Filter->new method.
It is optional and thus may not always be set.  

    my $name = $filter->name || 'Unknown name';
    warn "File '$name': failed to convert -- file may be corrupt\n";


=item $filter->user_data

Fetches the the user_data passed in to the filter.
This can be any data or data structure passed into SWISH::Filter->new.

This is an easy way to pass special parameters into your filters.

Example:

    my $data = $filter->user_data;
    # see if a choice for the <title> was passed in
    if ( ref $data eq 'HASH' && $data->{pdf2html}{title_field}  {
       ...
       ...
    }

=cut


sub AUTOLOAD {
    my ( $self, $newval ) = @_;
    no strict 'refs';


    if ($AUTOLOAD =~ /.*::set_(\w+)/ && $SWISH::Filter::extra_methods{$1})
    {
       my $attr_name=$1;
       *{$AUTOLOAD} = sub { $_[0]->{$attr_name} = $_[1]; return };
       return $self->{$attr_name} = $newval;
    }

    elsif ($AUTOLOAD =~ /.*::(\w+)/ && $SWISH::Filter::extra_methods{$1})
    {
       my $attr_name=$1;
       *{$AUTOLOAD} = sub { return $_[0]->{$attr_name} };
       return $self->{$attr_name};
    }  

    die "No such method: $AUTOLOAD\n";
}


=item $doc_ref = $filter->run_program( $program, @args );

Runs $program with @args.  Must pass in @args.

Under Windows calls IPC::Open2, which may pass data through the shell.  Double-quotes are
escaped (backslashed) and each parameter is wrapped in double-quotes.

On other platforms a fork and exec is used to avoid passing any data through the shell.
Returns a reference to a scalar containing the output from your program, or dies.

This method is intended to read output from a program that converts one format into text.
The output is read back in text mode -- on systems like Windows this means \r\n (CRLF) will
be convertet to \n. 

=cut

sub run_program {
    my $self = shift;

    die "No arguments passed to run_program()\n"
        unless @_;

    die "Must pass arguments to program '$_[0]'\n"
        unless @_ > 1;

    my $fh = $^O =~ /Win32/i || $^O =~ /VMS/i
         ? $self->windows_fork( @_ )
         : $self->real_fork( @_ );

    local $/ = undef;

    return <$fh>;
}


#==================================================================
# Run swish-e by forking
#

use Symbol;

sub real_fork {
    my ( $self, @args ) = @_;


    # Run swish 
    my $fh = gensym;
    my $pid = open( $fh, '-|' );

    die "Failed to fork: $!\n" unless defined $pid;

    return $fh if $pid;

    delete $self->{temp_file};  # in child, so don't want to delete on destroy.

    exec @args or exit;  # die "Failed to exec '$args[0]': $!\n";
}


#=====================================================================================
# Need
#
sub windows_fork {
    my ( $self, @args ) = @_;


    require IPC::Open2;
    my ( $rdrfh, $wtrfh );

    my @command = map { s/"/\\"/g; qq["$_"] }  @args;

     
    my $pid = IPC::Open2::open2($rdrfh, $wtrfh, @command );

    # IPC::Open3 uses binmode for some reason (5.6.1)
    # Assume that the output from the program will be in text
    # Maybe an invalid assumption if running through a binary filter

    binmode $rdrfh, ':crlf';  # perhpaps: unless delete $self->{binary_output};

    $self->{pid} = $pid;

    return $rdrfh;
}

=back

=cut

1;
__END__

=head1 TESTING

This module can be run as a program directly.  Change directory to the location of the Filter.pm
module and run:

  perl -I.. Filter.pm test  foo.pdf  bar.doc

replace foo.pdf and bar.doc with real paths on your system.  The -I.. is needed for
loading the filter modules.

Setting the environment variable "FILTER_DEBUG" to a true value will report
errors when loading filters.  Otherwise error are suppressed.

  export FILTER_DEBUG=1

=head1 SUPPORT

Please contact the Swish-e discussion list.  http://swish-e.org

=head1 AUTHOR

Bill Moseley

=head1 COPYRIGHT

This library is free software; you can redistribute it
and/or modify it under the same terms as Perl itself.


=cut
