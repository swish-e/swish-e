#=======================================================================
#  Phrase Highlighting Code
#
#    $Id$
#=======================================================================
package PhraseHighlight;
use strict;

use constant DEBUG_HIGHLIGHT => 0;

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



    my %stopwords =  map { $_, 1 } split /\s+/, $results->header('stopwords');
    $self->{stopwords} = \%stopwords;


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


    my $last = 0;

    my $settings = $self->{settings};

    my $Show_Words = $settings->{show_words} || 10;
    my $Occurrences = $settings->{occurrences} || 5;
    my $Max_Words = $settings->{max_words} || 100;
    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';


    my $stemmer_function = $self->{stemmer_function};

    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.

    # Split into words.  For speed, should work on a stream method.
    my @words = split /$wc_regexp/, $$text_ref;


    return 'No Content saved: Check StoreDescription setting' unless @words;

    my @flags;  # This marks where to start and stop display.
    $flags[$#words] = 0;  # Extend array.

    my $occurrences = $Occurrences ;


    my $word_pos = $words[0] eq '' ? 2 : 0;  # Start depends on if first word was wordcharacters or not

    my @phrases = @{ $self->{query} };

    # Remember, that the swish words are every other in @words.

    WORD:
    while ( $Show_Words && $word_pos * 2 < @words ) {

        PHRASE:
        foreach my $phrase ( @phrases ) {

            print STDERR "  Search phrase '@$phrase'\n" if DEBUG_HIGHLIGHT;
            next PHRASE if ($word_pos + @$phrase -1) * 2 > @words;  # phrase is longer than what's left
            

            my $end_pos = 0;  # end offset of the current phrase

            # now compare all the words in the phrase

            my ( $begin, $word, $end );
            
            for my $match_word ( @$phrase ) {

                my $cur_word = $words[ ($word_pos + $end_pos) * 2 ];
                $cur_word =~ /$extract_regexp/ or
                    die "Why didn't '$cur_word' =~ /$extract_regexp/? word_pos:$word_pos end_pos:$end_pos total:" . scalar @words
                    . "\n"
                    . join( "\n", map { "$_ '$words[$_]'" } 0..$#words );


                # Strip ignorefirst and ignorelast
                ( $begin, $word, $end ) = ( $1, $2, $3 );  # this is a waste, as it can operate on the same word over and over

                my $check_word = lc $word;

                if ( $end_pos && exists $self->{stopwords}{$check_word} ) {
                    $end_pos++;
                    print STDERR " Found stopword '$check_word' in the middle of phrase - * MATCH *\n" if DEBUG_HIGHLIGHT;
                    redo;
                }

                if ( $stemmer_function ) {
                    my $w = $stemmer_function->($check_word);
                    $check_word = $w if $w;
                }



                print STDERR "     comparing source # (word:$word_pos offset:$end_pos) '$check_word' == '$match_word'\n" if DEBUG_HIGHLIGHT;
    
                if ( substr( $match_word, -1 ) eq '*' ) {
                    next PHRASE if index( $check_word, substr($match_word, 0, length( $match_word ) - 1) ) != 0;

                } else {
                    next PHRASE if $check_word ne $match_word;
                }


                print STDERR "      *** Word Matched '$check_word' *** \n" if DEBUG_HIGHLIGHT;
                $end_pos++;  
            }

            print STDERR "      *** PHRASE MATCHED (word:$word_pos offset:$end_pos) *** \n" if DEBUG_HIGHLIGHT;


            # We are currently at the end word, so it's easy to set that highlight

            $end_pos--;

            if ( !$end_pos ) { # only one word
                $words[$word_pos * 2] = "$begin$On$word$Off$end";
            } else {
                $words[($word_pos + $end_pos) * 2 ] = "$begin$word$Off$end";

                #Now, reload first word of match
                $words[$word_pos * 2] =~ /$extract_regexp/ or die "2 Why didn't '$words[$word_pos]' =~ /$extract_regexp/?";
                # Strip ignorefirst and ignorelast
                ( $begin, $word, $end ) = ( $1, $2, $3 );  # probably should cache this!
                $words[$word_pos * 2] = "$begin$On$word$end";
            }


            # Now, flag the words around to be shown
            my $start = ($word_pos - $Show_Words + 1) * 2;
            my $stop   = ($word_pos + $end_pos + $Show_Words - 2) * 2;
            if ( $start < 0 ) {
                $stop = $stop - $start;
                $start = 0;
            }
            
            $stop = $#words if $stop > $#words;

            $flags[$_]++ for $start .. $stop;


            # All done, and mark where to stop looking
            if ( $occurrences-- <= 0 ) {
                $last = $end;
                last WORD;
            }


            # Now reset $word_pos to word following
            $word_pos += $end_pos; # continue will still be executed
            next WORD;
        }
    } continue {
        $word_pos ++;
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
    # $$text_ref = join '', @words;  # interesting that this seems reasonably faster


}

#============================================
# Returns compiled regular expressions for matching
#
#

sub set_match_regexp {
    my $self = shift;

    my $results = $self->{results};


    my $wc = $results->header('wordcharacters');
    my $ignoref = $results->header('ignorefirstchar');
    my $ignorel = $results->header('ignorelastchar');


    $wc = quotemeta $wc;

    #Convert query into regular expressions


    for ( $ignoref, $ignorel ) {
        if ( $_ ) {
            $_ = quotemeta;
            $_ = "([$_]*)";
        } else {
            $_ = '()';
        }
    }


    $wc .= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';  # Warning: dependent on tolower used while indexing


    # Now, wait a minute.  Look at this more, as I'd hope that making a
    # qr// go out of scope would release the compiled pattern.

    if ( $ENV{MOD_PERL} ) {
        $self->{wc_regexp}      = qr/([^$wc]+)/;                     # regexp for splitting into swish-words
        $self->{extract_regexp} = qr/^$ignoref([$wc]+?)$ignorel$/i;  # regexp for extracting out the words to compare

     } else {
        $self->{wc_regexp}      = qr/([^$wc]+)/o;                    # regexp for splitting into swish-words
        $self->{extract_regexp} = qr/^$ignoref([$wc]+?)$ignorel$/oi;  # regexp for extracting out the words to compare
     }
}    

1;



