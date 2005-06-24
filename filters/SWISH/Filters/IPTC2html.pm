package SWISH::Filters::IPTC2html;
use strict;
use vars qw/ $VERSION /;
use Image::IPTCInfo;

$VERSION = '0.01';
sub new {

    my ( $class ) = @_;
    my $self = bless {

        mimetypes => [ qr!image/jpeg! ],# list of types this filter handles

    }, $class;
     return $self->use_modules( qw/ Image::IPTCInfo / ); }

sub filter {

    my ( $self, $doc ) = @_;
    my $file = $doc->fetch_filename;
    # Create new info object
    my $info = new Image::IPTCInfo($file);

    # Check if file had IPTC data
    unless (defined($info)) { return; }
    # Get specific attributes...
    my $caption = $info->Attribute('caption/abstract');     my $headers = "<title>$caption</title>\n";

    # update the document's content type     $doc->set_content_type( 'text/html' );

        my $xml = $info->ExportXML('image');

     my $txt = <<EOF;
<html>
<head>
$headers
</head>
<body>
$xml
</body>
</html>
EOF

    return \$txt;
}

1;
__END__ =head1 NAME

SWISH::Filters::iptc2html - Perl extension for filtering image files with Swish-e

=head1 DESCRIPTION

This is a plug-in module that uses the Perl Image::IPTCInfo package to extract meta-data into html for indexing by Swish-e.

This filter plug-in requires the Image::IPTC package available at:

    http://search.cpan.org/~jcarter/Image-IPTCInfo-1.9/IPTCInfo.pm

=head1 AUTHOR

Bill Conlon

=head1 SEE ALSO

L<SWISH::Filter>

=head1 SUPPORT

Please contact the Swish-e discussion list. http://swish-e.org/

=cut

