#=======================================================================
#  Default Highlighting Code
#
#  Context highlighting & deals with stemming, but not phrases or stopwords
#
#    $Id$
#=======================================================================
package SWISH::DefaultHighlight;
use strict;

sub new {
    my ( $class, $settings, $headers ) = @_;


    my $self = bless {
        settings=> $settings,
        headers => $headers,
    }, $class;


    if ( $self->header('stemming applied') =~ /^(?:1|yes)$/i ) {
        eval { require SWISH::Stemmer };
        if ( $@ ) {
            warn('Stemmed index needs Stemmer.pm to highlight: ' . $@);
        } else {
            $self->{stemmer_function} = \&SWISH::Stemmer::SwishStem;
        }
    }

    $self->set_match_regexp;


    return $self;
}

sub header {
    my $self = shift;
    return '' unless ref $self->{headers} eq 'HASH';
    return $self->{headers}{$_[0]} || '';
}



#==========================================================================
# Returns true IF prop was HTML escaped.

sub highlight {

    my ( $self, $text_ref, $phrase_array, $prop_name ) = @_;

    my $wc_regexp = $self->{wc_regexp};
    my $extract_regexp = $self->{extract_regexp};
    my $match_regexp = $self->match_string( $phrase_array, $prop_name );


    my $last = 0;

    my $settings = $self->{settings};

    my $Show_Words = $settings->{show_words} || 10;
    my $Occurrences = $settings->{occurrences} || 5;
    my $Max_Words = $settings->{max_words} || 100;
    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';

    my $on_flag  = 'sw' . time . 'on';
    my $off_flag = 'sw' . time . 'off';

    my $stemmer_function = $self->{stemmer_function};


    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.
    my @words = split /$wc_regexp/, $$text_ref;
    return unless @words;


    my @flags;
    $flags[$#words] = 0;  # Extend array.

    my $occurrences = $Occurrences ;


    my $pos = 0;

    while ( $Show_Words && $pos <= $#words ) {

        # Check if the word is a swish word (ignoring begin and end chars)
        if ( $words[$pos] =~ /$extract_regexp/ ) {


            my ( $begin, $word, $end ) = ( $1, $2, $3 );

            my $test = $stemmer_function
                       ? $stemmer_function->($word)
                       : lc $word;

            $test ||= lc $word;

            # Not check if word matches
            if ( $test =~ /$match_regexp/ ) {

                $words[$pos] = "$begin$on_flag$word$off_flag$end";


                my $start = $pos - ($Show_Words-1)* 2;
                my $end   = $pos + ($Show_Words-1)* 2;
                if ( $start < 0 ) {
                    $end = $end - $start;
                    $start = 0;
                }

                $end = $#words if $end > $#words;

                $flags[$_]++ for $start .. $end;


                # All done, and mark where to stop looking
                if ( $occurrences-- <= 0 ) {
                    $last = $end;
                    last;
                }
            }
        }

       $pos += 2;  # Skip to next wordchar word
    }


    my $dotdotdot = ' ... ';


    my @output;

    my $printing;
    my $first = 1;
    my $some_printed;

    if ( $Show_Words && @words > 50 ) {  # don't limit context if a small number of words
        for my $i ( 0 ..$#words ) {


            if ( $last && $i >= $last && $i < $#words ) {
                push @output, $dotdotdot;
                last;
            }

            if ( $flags[$i] ) {

                push @output, $dotdotdot if !$printing++ && !$first;
                push @output, $words[$i];
                $some_printed++;

            } else {
                $printing = 0;
            }

        $first = 0;


        }
    }

    if ( !$some_printed ) {
        for my $i ( 0 .. $Max_Words ) {
            if ( $i > $#words ) {
                $printing++;
                last;
            }
            push @output, $words[$i];
        }
    }



    push @output, $dotdotdot if !$printing;

    $$text_ref = join '', @output;

    my %entities = (
        '&' => '&amp;',
        '>' => '&gt;',
        '<' => '&lt;',
        '"' => '&quot;',
    );
    my %highlight = (
        $on_flag => $On,
        $off_flag => $Off,
    );


    $$text_ref =~ s/([&"<>])/$entities{$1}/ge;  # " fix emacs

    $$text_ref =~ s/($on_flag|$off_flag)/$highlight{$1}/ge;

    return 1;  # Means that prop was processed AND was html escaped.


}

#============================================
# Returns compiled regular expressions for matching
#

sub match_string {
    my ($self, $phrases, $prop_name ) = @_;

    # Already cached?
    return $self->{cache}{$prop_name}
	if $self->{cache}{$prop_name};

    my $wc = quotemeta $self->header('wordcharacters');
    # Yuck!
    $wc .= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';  # Warning: dependent on tolower used while indexing

    # Get all the unique words;

    my %words;
    for my $phrase ( @$phrases ) {
       $words{$_}++ for @$phrase;
    }


    my $match_string =
        join '|',
           map { substr( $_, -1, 1 ) eq '*'
                    ? quotemeta( substr( $_, 0, -1) ) . "[$wc]*?"
                    : quotemeta
               } keys %words;

    my $re = qr/^(?:$match_string)$/;

    $self->{cache}{$prop_name} = $re;

    return $re;
}

#============================================
# Returns compiled regular expressions for splitting the source text into "swish words"
#
#

sub set_match_regexp {
    my $self = shift;

    my $ignoref = $self->header('ignorefirstchar');
    my $ignorel = $self->header('ignorelastchar');

    for ( $ignoref, $ignorel ) {
        if ( $_ ) {
            $_ = quotemeta;
            $_ = "([$_]*)";
        } else {
            $_ = '()';
        }
    }


    my $wc = quotemeta $self->header('wordcharacters');
    $wc .= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';  # Warning: dependent on tolower used while indexing


    # Now, wait a minute.  Look at this more, as I'd hope that making a
    # qr// go out of scope would release the compiled pattern.
    # doesn't really matter, as $wc probably never changes

    $self->{wc_regexp}      = qr/([^$wc]+)/;                     # regexp for splitting into swish-words
    $self->{extract_regexp} = qr/^$ignoref([$wc]+?)$ignorel$/i;  # regexp for extracting out the words to compare
}

1;

