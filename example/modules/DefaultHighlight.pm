#=======================================================================
#  Default Highlighting Code
#
#    $Id$
#=======================================================================
package DefaultHighlight;
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

    $self->{description_prop} = $results->config('description_prop') || '';

    if ( $results->header('stemming applied') =~ /^(?:1|yes)$/i ) {
        eval { require SWISH::Stemmer };
        if ( $@ ) {
            $results->errstr('Stemmed index needs Stemmer.pm to highlight: ' . $@);
        } else {
            $self->{stemmer_function} = \&SWISH::Stemmer::SwishStem;
        }
    }


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

    my %checked;

    for ( @props ) {
        if ( $properties->{$_} ) {
            $checked{$_}++;
            $self->highlight_text( \$properties->{$_}, $phrase_array );
        }
    }


    # Truncate the description, if not processed.
    my $description = $self->{description_prop};
    if ( $description && !$checked{ $description } && $properties->{$description} ) {
        my $max_words = $settings->{max_words} || 100;
        my @words = split /\s+/, $properties->{$description};
        if ( @words > $max_words ) {
            $properties->{$description} = join ' ', @words[0..$max_words], '<b>...</b>';
        }
    }

}



#==========================================================================
#

sub highlight_text {

    my ( $self, $text_ref, $phrase_array ) = @_;
    
    my $wc_regexp = $self->{wc_regexp};
    my $extract_regexp = $self->{extract_regexp};
    my $match_regexp = $self->{match_regexp};


    my $last = 0;

    my $settings = $self->{settings};

    my $Show_Words = $settings->{show_words} || 10;
    my $Occurrences = $settings->{occurrences} || 5;
    my $Max_Words = $settings->{max_words} || 100;
    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';


    my $stemmer_function = $self->{stemmer_function};


    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.
    my @words = split /$wc_regexp/, $$text_ref;


    return 'No Content saved: Check StoreDescription setting' unless @words;

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

                $words[$pos] = "$begin$On$word$Off$end";


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


    my $dotdotdot = ' <b>...</b> ';


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


}

#============================================
# Returns compiled regular expressions for matching
#

sub set_match_regexp {
    my $self = shift;

    my $results = $self->{results};


    my $wc = $results->header('wordcharacters');
    my $ignoref = $results->header('ignorefirstchar');
    my $ignorel = $results->header('ignorelastchar');



    my $query = join ' ', map { join ' ', @$_} @{$self->{query}};  # join everything together!


    $wc = quotemeta $wc;


    my $match_string =
        join '|',
           map { substr( $_, -1, 1 ) eq '*'
                    ? quotemeta( substr( $_, 0, -1) ) . "[$wc]*?"
                    : quotemeta
               }
                grep { ! /^(and|or|not|["()=])$/oi }  # left over code
                    split /\s+/, $query;



    return unless $match_string;

    for ( $ignoref, $ignorel ) {
        if ( $_ ) {
            $_ = quotemeta;
            $_ = "([$_]*)";
        } else {
            $_ = '()';
        }
    }


    # Yuck!
    $wc .= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';  # Warning: dependent on tolower used while indexing


    # Now, wait a minute.  Look at this more, as I'd hope that making a
    # qr// go out of scope would release the compiled pattern.

    if ( $ENV{MOD_PERL} ) {
        $self->{wc_regexp}      = qr/([^$wc]+)/;                     # regexp for splitting into swish-words
        $self->{extract_regexp} = qr/^$ignoref([$wc]+?)$ignorel$/i;  # regexp for extracting out the words to compare
        $self->{match_regexp}   = qr/^(?:$match_string)$/;           # regexp for comparing extracted words to query

     } else {
        $self->{wc_regexp}      = qr/([^$wc]+)/o;                    # regexp for splitting into swish-words
        $self->{extract_regexp} = qr/^$ignoref([$wc]+?)$ignorel$/oi;  # regexp for extracting out the words to compare
        $self->{match_regexp}   = qr/^(?:$match_string)$/o;          # regexp for comparing extracted words to query
     }
}    


1;

