#=======================================================================
#  Simple Highlighting Code
#    $Id$
#=======================================================================
package SimpleHighlight;
use strict;

sub new {
    my ( $class, $results, $metaname ) = @_;


    my $self = bless {
        results => $results,  # just in case we need a method
        settings=> $results->config('highlight'),
        metaname=> $metaname,
    }, $class;

    # parse out the query into words
    my $query = $results->extract_query_match;


    # Do words exist for this layer (all text at this time) and metaname?
    # This is a reference to an array of phrases and words



    if ( $query && exists $query->{text}{$metaname} ) {
        $self->{query} = $query->{text}{$metaname};

        $self->set_match_regexp;
    }


    return $self;
}

sub highlight {
    my ( $self, $properties ) = @_;


    return unless $self->{query};

    my $phrase_array = $self->{query};

    my $settings = $self->{settings};
    my $metaname = $self->{metaname};

    # Do we care about this meta?
    return unless exists $settings->{meta_to_prop_map}{$metaname};

    # Get the related properties
    my @props = @{ $settings->{meta_to_prop_map}{$metaname} };

    for ( @props ) {
        if ( $properties->{$_} ) {
            $self->highlight_text( \$properties->{$_}, $phrase_array );
        }
    }

}



#==========================================================================
#

sub highlight_text {

    my ( $self, $text_ref, $phrase_array ) = @_;
    

    my $settings = $self->{settings};

    my $Show_Words = $settings->{show_words} || 10;
    my $Occurrences = $settings->{occurrences} || 5;
    my $Max_Words = $settings->{max_words} || 100;
    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';

    my @words = split /\s+/, $$text_ref;
    if ( @words > $Max_Words ) {
        $$text_ref = join ' ', @words[0..$Max_Words], '<b>...</b>';
    }


    for ( @{ $self->{matches} } ) {
        $$text_ref =~ s/($_)/$On$1$Off/g;
    }

    

}

#============================================
# Returns compiled regular expressions for matching
#
# This builds a list of expressions to match against the text.

sub set_match_regexp {
    my $self = shift;

    my $results = $self->{results};


    my $wc = $results->header('wordcharacters');
    $wc = quotemeta $wc;

    my @matches;

    # loop through all the phrase 
    for ( @{$self->{query}} ) {

        # Fix up wildcards
        my $exp = join '[^$wc]+',
            map { '\b' . $_ . '\b' }
            map { substr( $_, -1, 1 ) eq '*'
                ? quotemeta( substr( $_, 0, -1) ) . "[$wc]*?"
                : quotemeta
            } @$_;

        push @matches, qr/$exp/i;
    }


    $self->{matches} = \@matches;

}    
1;

