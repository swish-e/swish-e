package SWISH::Filters::Pdf2HTML;
use strict;

use vars qw/ @ISA $VERSION /;

$VERSION = '0.01';

@ISA = ('SWISH::Filter');

sub new {
    my ( $pack, %params ) = @_;

    my $self = bless {
	name => $params{name} || $pack,
    }, $pack;


    # check for helpers
    for my $prog ( qw/ pdftotext pdfinfo / ) {
        my $path = $self->find_binary( $prog );
        unless ( $path ) {
            $self->mywarn("Can not use Filter $pack -- need to install $prog");
            return;
        }
        $self->{$prog} = $path;
    }

    return $self;

}

# set sort order for this filter

sub name { $_->{name} || 'unknown' };        


sub filter {
    my ( $self, $filter) = @_;

    return unless $filter->content_type =~ m!application/pdf!;


    my $user_data = $filter->user_data;
    my $title_tag = $user_data->{pdf}{title_tag} if ref $user_data eq 'HASH';
    

    my $metadata = $self->get_pdf_headers( $filter );

    my $headers = format_metadata( $metadata );

    if ( $title_tag && exists $metadata->{ $title_tag } ) {
        my $title = escapeXML( $metadata->{ $title_tag } );

        $headers = "<title>$title</title>\n" . $headers
    }
    

    # Check for encrypted content

    my $content_ref;

    # patch provided by Martial Chartoire
    if ( $metadata->{encrypted} && $metadata->{encrypted} =~ /yes\.*\scopy:no\s\.*/i ) {
        $content_ref = \'';

    } else {
        $content_ref = $self->get_pdf_content_ref( $filter );
    }

    # update the document's content type
    $filter->set_content_type( 'text/html' );


    my $txt = <<EOF;
<html>    
<head>
$headers
</head>
<body>
<pre>
$$content_ref
</pre>
</body>
</html>
EOF

    return \$txt;

    

}

sub get_pdf_headers {

    my ($self, $filter ) = @_;

    # We need a file name to pass to the pdf conversion programs
    
    
    my %metadata;
    my $headers = $filter->run_program( $self->{pdfinfo}, $filter->fetch_filename );
    return \%metadata unless $headers;

    for (split /\n/, $headers ) {
        if ( /^\s*([^:]+):\s+(.+)$/ ) {
            my ( $metaname, $value ) = ( lc( $1 ), $2 );
            $metaname =~ tr/ /_/;
            $metadata{$metaname} = $value;
        }
    }

    return \%metadata;
}

sub format_metadata {

    my $metadata = shift;

    my $metas = join "\n", map {
        qq[<meta name="$_" content="] . escapeXML( $metadata->{$_} ) . '">';
    } sort keys %$metadata;


    return $metas;
}

sub get_pdf_content_ref {
    my ( $self, $filter )  = @_;

    my $content = escapeXML( $filter->run_program($self->{pdftotext}, $filter->fetch_filename, '-' ) );

    return \$content;
}



# How are URLs printed with pdftotext?
sub escapeXML {

   my $str = shift;

   return '' unless $str;

   for ( $str ) {
       s/&/&amp;/go;
       s/"/&quot;/go;
       s/</&lt;/go;
       s/>/&gt;/go;
    }
   return $str;
}

1;
__END__

=head1 NAME

SWISH::Filters::Pdf2HTML - Perl extension for filtering PDF documents with Swish-e

=head1 DESCRIPTION

This is a plug-in module that uses the xpdf package to convert PDF documents
to html for indexing by Swish-e.  Any info tags found in the PDF document are created
as meta tags.

This filter plug-in requires the xpdf package available at:

    http://www.foolabs.com/xpdf/

You may pass into SWISH::Filter's new method a tag to use as the html
<title> if found in the PDF info tags:

    my %user_data;
    $user_data{pdf}{title_tag} = 'title';

    $was_filtered = $filter->filter(
        document  => $filename,
        user_data => \%user_data,
    );

Then if a PDF info tag of "title" is found that will be used as the HTML <title>.    



=head1 AUTHOR

Bill Moseley

=head1 SEE ALSO

L<SWISH::Filter>


