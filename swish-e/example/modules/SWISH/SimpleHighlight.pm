#=======================================================================
#  Simple Highlighting Code
#    $Id$
# This one is not accurate and is more for speed
#=======================================================================
package SWISH::SimpleHighlight;
use strict;

sub new {
    my ( $class, $settings, $headers ) = @_;


    return bless {
        settings=> $settings,
	headers => $headers,
    }, $class;
}

sub header {
    my $self = shift;
    return '' unless ref $self->{headers} eq 'HASH';
    return $self->{headers}{$_[0]} || '';
}


#==========================================================================
#

sub highlight {

    my ( $self, $text_ref, $phrase_array, $prop_name ) = @_;


    my $settings = $self->{settings};

    my $Max_Words = $settings->{max_words} || 100;
    my $max_chars = 8 * $Max_Words;


    # first trim down the property - would likely be faster to use substr()
    # limits what is searched, but also means some man not show highlighting
    # is also not limited to the description property

    my $text = length( $$text_ref ) > $max_chars
	? substr( $$text_ref, 0, $max_chars ) . " ..."
        : substr( $$text_ref, 0, $max_chars );

    my $start = "\007";  # Unlikely chars
    my $end   = "\010";


    my @matches = $self->set_match_regexp( $phrase_array, $prop_name );
    $text =~ s/($_)/${start}$1${end}/gi for @matches;

    # Replace entities

    my %entities = (
        '&' => '&amp;',
        '>' => '&gt;',
        '<' => '&lt;',
        '"' => '&quot;',
    );

    $text =~ s/([&"<>])/$entities{$1}/ge;  # " fix emacs

    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';

    $text =~ s/$start/$On/g;
    $text =~ s/$end/$Off/g;
    my $wc = quotemeta $self->header('wordcharacters');

    $$text_ref = $text;


    return 1;  # return true because the property was trimmed and escaped.
}


#============================================
# Returns compiled regular expressions for matching
#
# This builds a list of expressions to match against the text.

sub set_match_regexp {
    my ( $self, $phrases, $prop_name ) = @_;

    my $wc = quotemeta $self->header('wordcharacters');

    my @matches;

    # convert each phrase for this meta into a regular expression

    for ( @$phrases ) {

        # Fix up wildcards
        my $exp = join "[^$wc]+",
            map { '\b' . $_ . '\b' }
            map { substr( $_, -1, 1 ) eq '*'
                ? quotemeta( substr( $_, 0, -1) ) . "[$wc]*?"
                : quotemeta
            } @$_;


        push @matches, qr/$exp/i;
    }

    return @matches;


}
1;

