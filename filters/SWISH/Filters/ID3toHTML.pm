package SWISH::Filters::ID3toHTML;

use strict;

#use MP3::Tag;
#use HTML::Entities;

use vars qw/ @ISA $VERSION /;

$VERSION = '0.03';

@ISA = ('SWISH::Filter');


# Convert known ID3v2 tags to metanames.

my %id3v2_tags = (
    TIT2 => 'song',            # 4.2.1 TIT2 Title/songname/content description
    TYER => 'year',            # 4.2.1 TYER Year
    TRCK => 'track',           # 4.2.1 TRCK Track number/Position in set 
    TCOP => 'copyright',       # 4.2.1 TCOP Copyright message
                               # * WinAMP seems to prepend a (C) to this value.

    TPE1 => 'artist',          # 4.2.1 TPE1 Lead performer(s)/Soloist(s)
    TALB => 'album',           # 4.2.1 TALB Album/Movie/Show title
    TENC => 'encoded',         # 4.2.1 TENC Encoded by
    TOPE => 'artist_original', # 4.2.1 TOPE Original artist(s)/performer(s)
    TCOM => 'composer',        # 4.2.1 TCOM Composer
    TCON => 'genre',           # 4.2.1 TCON Content type

                                            # 4.3.2 WXXX User defined URL link frame
    WXXX_URL => 'url',                      #  * URL => http://URL/HERE
    WXXX_Description => 'url_description',  #  * Description => WinAMP provides no description

                                       # 4.11 COMM Comments
    COMM_Text => 'comment',            # * Text => COMMENT
    COMM_Language => 'comment_lang',   # * Language => eng
    COMM_short => 'comment_short'      # * short => WinAMP provides no short

);


sub new {
    my ( $pack, %params ) = @_;

    my $self = bless {
        name => $params{name} || $pack,
    }, $pack;


    return unless $self->use_modules( qw/ MP3::Tag  HTML::Entities / );

    return $self;

}

sub name { $_->{name} || 'unknown' };


sub filter {
    my ( $self, $filter) = @_;

    # Do we care about this document?
    return unless $filter->content_type =~ m!audio/mpeg!;

    # We need a file name to pass to the conversion function
    my $file = $filter->fetch_filename;

    my $content_ref = get_id3_content_ref( $file );
    return unless $content_ref;

    # update the document's content type
    $filter->set_content_type( 'text/html' );
    
    # If filtered must return either a reference to the doc or a pathname.
    return \$content_ref;
}

# =======================================================================
sub get_id3_content_ref {
    my $filename = shift;
    my $mp3 = MP3::Tag->new($filename);
    
    # return unless we have a file with tags
    return format_empty_doc( $filename )
        unless ref $mp3 && $mp3->get_tags();

    # Here we will store all of the tag info
    my %metadata;
    # Convert tags to metadata giving ID3v2 precedence
    get_id3v1_tags($mp3, \%metadata);
    get_id3v2_tags($mp3, \%metadata);  # will replace any v1 tags that are the same

    # HTML or bust
    return %metadata
        ? format_as_html( \%metadata )
        : format_empty_doc( $filename );
}
    
sub get_id3v1_tags {
    my ($mp3, $metadata) = @_;

    return unless exists $mp3->{ID3v1};

    # Read all ID3v1 tags into metadata hash
    my $id3v1 = $mp3->{ID3v1};
    for ( qw/ artist album comment genre song track year / ) {
        $metadata->{$_} = $id3v1->$_ if $id3v1->$_;
    }
}

sub get_id3v2_tags {
    my ($mp3, $metadata) = @_;

    # Do we even have an ID3 v2 tag?
    return unless exists $mp3->{ID3v2};

    
    # Get the tag and a hash of frame ids.
    my $id3v2 = $mp3->{ID3v2};

    # keys are 4-character-codes and values are the long names
    my $frameIDs_hash = $id3v2->get_frame_ids;


    # Go through each frame and translate it to usable metadata
    foreach my $frame (keys %$frameIDs_hash) {
        my ($info, $name) = $id3v2->get_frame($frame);


        # We have a user defined frame
        if (ref $info) { 
            # $$$ We really only want COMM and WXXX
            while(my ($key,$val)=each %$info) {

                next if $key =~ /^_/ || !$val;  # leading underscore means binary data

                # Concatenate frame and key for our lookup hash
                my $code = ${frame} . "_" .${key};

                # fails when frame is appended with digits (e.g. "COMM01");
                my $metaname = $id3v2_tags{$code} || $code;

                # Assign value if not empty and has a key
                $metadata->{$metaname} = encode_entities($val) if $val;
            }
        }
        # We have a simple frame
        else {
            my $metaname = $id3v2_tags{$frame} || $frame || 'blank frame';
            $metadata->{$metaname} = encode_entities($info) unless !$info;
        }
    }
}
    
sub format_as_html {
    my $metadata = shift;

    my $title = $metadata->{song} || $metadata->{album} || $metadata->{artist} || 'No Title';
    my $metas = join "\n", map { qq[<meta name="$_" content="$metadata->{$_}">] } sort keys %$metadata;

    my $url = '';
    if ( $metadata->{url} ) {
        my $desc = $metadata->{url_description} || $metadata->{url};
        $url = qq[<p><a href="$metadata->{url}">$desc</a>];
    }

    my $comment = '';
    if ( $metadata->{comment} ) {
        my $lang = get_iso_lang($metadata->{comment_lang} || 'en');  # wrong assuming "en"?
        $comment = qq[<p name="comment" lang="$lang">$metadata->{comment}</p>];
    }


    return <<EOF;
<html>
 <head>
   <title>$title</title>
   $metas
 </head>
 <body>
   $url
   $comment
  </body>
</html>
EOF

}

sub format_empty_doc {
    my $filename = shift;
    require File::Basename;
    my $base = File::Basename::basename( $filename, '.mp3' );
    
    return format_as_html( { song => $base, notag => 1 } );
}

sub get_iso_lang {
    my $lang = shift;
    # Do we need to translate undocumented ID3 Lang codes to ISO?
    # 4.11.Comments
    #   Language $xx xx xx 
    #   *  WinAMP may be mistaken for using "eng" instead of an ISO designator

    return $lang unless $lang == "eng";
    return "en";
}


1;
__END__

=head1 NAME

SWISH::Filters::ID3toHTML - ID3 tag to HTML filter module

=head1 DESCRIPTION

SWISH::Filters::ID3toHTML translates ID3 tags into HTML
metadata for use by the SWISH::Filter module and SWISH-E.

Depends on two perl modules:

    MP3::Tag;
    HTML::Entities;


=head1 SUPPORT

Please contact the Swish-e discussion list.  
http://swish-e.org/

=cut

