package SWISH::Filters::ID3toHTML;

use strict;

use MP3::Tag;
use HTML::Entities;

# Define the sort order for this filter
use vars qw/ %FilterInfo $VERSION /;

$VERSION = '0.01';

%FilterInfo = (
    type     => 2,  # normal filter 1-10
    priority => 50, # normal priority 1-100
);

sub filter {
    my $filter = shift;

    # Do we care about this document?
    return unless $filter->content_type =~ m!audio/mpeg!;

    # We need a file name to pass to the conversion function
    my $file = $filter->fetch_filename;

    my $content_ref = get_id3_content_ref( $file ) || return;
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
    return unless ref $mp3 && $mp3->get_tags();

    # Here we will store all of the tag info
    my %metadata;
    # Convert tags to metadata giving ID3v2 precedence
    %metadata = get_id3v1_tags($mp3, \%metadata);
    %metadata = get_id3v2_tags($mp3, \%metadata);

    # HTML or bust
    return %metadata
        ? format_as_html( \%metadata )
        : format_empty_doc();
}
    
sub get_id3v1_tags {
    my ($mp3, $metadata) = @_;
    # Read all ID3v1 tags into metadata hash
    if( exists $mp3->{ID3v1} ) {
        my $id3v1 = $mp3->{ID3v1};
        for ( qw/ artist album comment genre song track year / ) {
            $metadata->{$_} = $id3v1->$_ if $id3v1->$_;
        }
    }
    return %$metadata;
}

sub get_id3v2_tags {
    my ($mp3, $metadata) = @_;

    # Do we even have an ID3 v2 tag?
    if( exists $mp3->{ID3v2} ){
        # List of ID3v2 tags we wish to retrieve
        my %id3v2_tags = (
            # 4.2.1 TIT2 Title/songname/content description
            TIT2 => 'song',
            # 4.2.1 TYER Year
            TYER => 'year',
            # 4.2.1 TRCK Track number/Position in set 
            TRCK => 'track',
            # 4.2.1 TCOP Copyright message
            # * WinAMP seems to prepend a (C) to this value.
            TCOP => 'copyright',
            # 4.2.1 TPE1 Lead performer(s)/Soloist(s)
            TPE1 => 'artist',
            # 4.2.1 TALB Album/Movie/Show title
            TALB => 'album',
            # 4.2.1 TENC Encoded by
            TENC => 'encoded',
            # 4.2.1 TOPE Original artist(s)/performer(s)
            TOPE => 'artist_original',
            # 4.2.1 TCOM Composer
            TCOM => 'composer',
            # 4.2.1 TCON Content type
            TCON => 'genre',
            # 4.3.2 WXXX User defined URL link frame
            # * Description => WinAMP provides no description
            # * URL => http://URL/HERE
            WXXX_URL => 'url',
            WXXX_Description => 'url_description',
            # 4.11 COMM Comments
            # * Text => COMMENT
            # * Language => eng
            # * short => WinAMP provides no short
            COMM_Text => 'comment',
            COMM_Language => 'comment_lang',
            COMM_short => 'comment_short'
        );
    
        # Get the tag and a hash of frame ids.
        my $id3v2 = $mp3->{ID3v2};
        my $frameIDs_hash = $id3v2->get_frame_ids;
        # Go through each frame and translate it to usable metadata
        foreach my $frame (keys %$frameIDs_hash) {
            my ($info, $name) = $id3v2->get_frame($frame);
            # We have a user defined frame
            if (ref $info) { 
                # $$$ We really only want COMM and WXXX
                while(my ($key,$val)=each %$info) {
                    # Concatenate frame and key for our lookup hash
                    $key = $id3v2_tags{${frame} . "_" .${key}}; 
                    # Assign value if not empty and has a key
                    $metadata->{$key} = encode_entities($val) if $val && $key;
                }
            }
            # We have a simple frame
            else { 
                $metadata->{$id3v2_tags{$frame}} = encode_entities($info) unless !$info;
            }
        }
    }
    return %$metadata;
}
    
sub format_as_html {
    my $metadata = shift;
    
    my $return_value = "<html>\n<head>\n";
    $return_value .= "<title>"  . $metadata->{song} . "</title>\n" if $metadata->{song};
    $return_value .= "<meta name=\"" . $_ . "\" content=\"" . $metadata->{$_} . "\">\n" for (sort keys %$metadata);
    $return_value .= "<body>\n";
    $return_value .= "<p><a href=\"" . $metadata->{url} . "\">". ($metadata->{url_description} or $metadata->{url}) . "</a></p>\n" if $metadata->{url};
    $return_value .= "<p name=\"comment\" lang=\"". (get_iso_lang($metadata->{comment_lang})) . "\">" . $metadata->{comment} . "</p>\n" if $metadata->{comment};
    $return_value .= "</body>\n</html>\n";
    
    return $return_value;
}

sub format_empty_doc {
    # perhaps we might want to do something here
    return;
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

SWISH::Filters::ID3toHTML translates ID3 tags into HTML metadata for use by the SWISH::Filter module and SWISH-E.

=head1 SUPPORT

Please contact the Swish-e discussion list.  
http://swish-e.org/

=cut

