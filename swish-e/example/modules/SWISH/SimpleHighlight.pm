#=======================================================================
#  Simple Highlighting Code
#    $Id$
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


    # first trim down the property - would likely be faster to use substr()
    # limits what is searched, but also means some man not show highlighting
    # is also not limited to the description property

    my @words = split /\s+/, $$text_ref;
    if ( @words > $Max_Words ) {
        $$text_ref = join ' ', @words[0..$Max_Words], '<b>...</b>';
    }

    my %entities = (
        '&' => '&amp;',
        '>' => '&gt;',
        '<' => '&lt;',
        '"' => '&quot;',
    );

    $$text_ref =~ s/([&"<>])/$entities{$1}/ge;  # " fix emacs




    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';

    my @matches = $self->set_match_regexp( $phrase_array, $prop_name );

    for ( @matches ) {
        $$text_ref =~ s/($_)/$On$1$Off/g;
    }

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

