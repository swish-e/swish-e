#!/usr/local/bin/perl -w
use strict;
use SWISH::Filter;


=pod

$id$

This is an example of how to use the SWISH::Filter module to filter documents using
Swish-e's C<FileFilter> feature.  This will filter any number of document types,
depending on what filter modules are installed.

This program should typically only be used for the -S fs indexing method.  For -S http
the F<swishspider> program calls SWISH::Filter directly.  And -S prog programs written
in Perl can also make use of SWISH::Filter directly.

In general, you will not want to filter with this program if you have a lot of files
to filter.  Running a perl program for many documents will be slow (due to the compiliation
of the perl program).  If you have many documents to convert with the -S fs method of indexing
then consider using -S prog with F<prog-bin/DirTree.pl> and use the SWISH::Filter module (see
F<filters/README>).

This program uses the SWISH::Filter module.  SWISH::Filter must be in Perl's
include path @INC.  The easy way to do this is by setting the environment variable PERL5LIB
before running Swish-e.

For example:

    export PERL5LIB=$HOME/swish-e-2.2/filters


Swish-e configuration:

    FileFilter .pdf /path/to/swish_filter.pl
    FileFilter .doc /path/to/swish_filter.pl
    FileFilter .mp3 /path/to/swish_filter.pl
    IndexContents HTML2 .pdf .mp3
    IndexContents TXT2 .doc

Then when indexing those type of documents this program will attempt to filter (convert)
them into a text format.

See SWISH-CONFIG documentation on Filtering for more information.

=cut    


    my ( $work_path, $real_path ) = @ARGV;
    my $filter = SWISH::Filter->new;

    my $filtered = $filter->filter(
        document => $work_path,
        name     => $real_path,
        content_type => \$real_path, # use the real path to lookup the content type
    );

    print STDERR $filtered ? " - Filtered: $real_path\n" : " - Not filtered: $real_path ($work_path)\n";

    print $filtered
        ? ${$filter->fetch_doc}
        : $real_path;

    



