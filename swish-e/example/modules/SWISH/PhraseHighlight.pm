#=======================================================================
#  Phrase Highlighting Code
#
#    $Id$
#    This code is slow, slow, SLOW!
#=======================================================================
package SWISH::PhraseHighlight;
use strict;

use constant DEBUG_HIGHLIGHT => 0;

sub new {
    my ( $class, $settings, $headers, $options ) = @_;



    my $self = bless {
        settings => $settings,
        headers  => $headers,
        options  => $options,
    }, $class;



    if ( $self->header('stemming applied') =~ /^(?:1|yes)$/i ) {
        $self->{stemming_applied}++;  # flag that stemming is used

        unless ( $options && $options->{swish} ) {  # are we running under SWISH::API?
            # if not, then try to load SWISH::Stemmer
            eval { require SWISH::Stemmer };
            if ( $@ ) {
                warn('Stemmed index needs Stemmer.pm to highlight: ' . $@);
            } else {
                $self->{stemmer_function} = \&SWISH::Stemmer::SwishStem;
            }
        }
    }


    $self->{stopwords} = { map { $_, 1 } split /\s+/, $self->header('stopwords') };


    $self->set_match_regexp;


    return $self;
}


sub header {
    my $self = shift;
    return '' unless ref $self->{headers} eq 'HASH';
    return $self->{headers}{$_[0]} || '';
}


#=========================================================================
# Highlight a single property -- returns true if any words highlighted
# no, no returns true really means that the text was processed and most
# importantly HTML escaped.

sub highlight {


    my ( $self, $text_ref, $phrase_list, $prop_name, $result_object ) = @_;
    # $phrase_list is an array of arrays of phrases (where a phrase might be a single word)
    # and should be sorted by longest to shortest phrase.

    my $wc_regexp = $self->{wc_regexp};  # used to split text into words and non-word tokens
    my $extract_regexp = $self->{extract_regexp};  # used to strip out the ignore chars at begin and end of word


    my $last = 0;

    my $found_phrase = 0;  # count of number of phrases found in the text
    my $show_all_words = 0;  # true (in some cases) when every word will be displayed

    my $settings = $self->{settings};

    my $Show_Words = $settings->{show_words} || 10;  # These define how the final output will display
    my $Occurrences = $settings->{occurrences} || 5;
    my $Max_Words = $settings->{max_words} || 100;



    my $On = $settings->{highlight_on} || '<b>';
    my $Off = $settings->{highlight_off} || '</b>';

    my $on_flag  = 'sw' . time . 'on';  # used to flag where to turn on highlighting
    my $off_flag = 'sw' . time . 'off'; # can't use $On/$Off because of html escaping needs to be done later

    my $stemmer_function; 
    # Now check for running under SWISH::API and check for stemming
    if ( $result_object && $result_object->FuzzyMode ne 'None' ) {
        # This doesn't work with double-metaphone which might return more than one stem
        $stemmer_function = sub {
            ($result_object->FuzzyWord( shift )->WordList)[0] 
        };
    } else {
        $stemmer_function = $self->{stemmer_function};
    }

    # Should really call unescapeHTML(), but then would need to escape <b> from escaping.


    # Split into "swish" words.  For speed, should work on a stream method instead.  TODO:
    my @words = split /$wc_regexp/, $$text_ref;
    return unless @words;

    my @flags;  # This marks where to start and stop display -- maps to @words.
    $flags[$#words] = 0;  # Extend array.

    my $occurrences = $Occurrences;


    # $word_pos is current pointer into @words/2 (it indexes just the "swish words") -- where to start looking for a phrase match.
    my $word_pos = $words[0] eq '' ? 2 : 0;  # Start depends on if first word was wordcharacters or not
    # $$$ There's a bug when the text starts: '; foo'

    # These try and cache the last stemmed word.
    my $last_stemmed_word;
    my $last_word_pos;




    # Remember, that the swish words are every other in @words.

    WORD:
    while ( $Show_Words && $word_pos * 2 < @words ) {

        PHRASE:
        foreach my $phrase ( @$phrase_list ) {


            print STDERR "  Search phrase '@$phrase'\n" if DEBUG_HIGHLIGHT;
            next PHRASE if ($word_pos + @$phrase -1) * 2 > @words;  # phrase is longer than what's left


            my $end_pos = 0;  # end offset of the current phrase, that is if looking at second word
                              # in a phrase, then $end_pos = 1 and $end_pos is used to index into the
                              # text starting at $word_pos.

            # now compare all the words in the phrase

            my ( $begin, $word, $end );

            for my $match_word ( @$phrase ) {

                # get the current word from the property and convert it into a "swish word" for comparing with the phrase
                # by stripping out the ignorefirst and ignore last chars

                my $word_pos_idx = $word_pos + $end_pos;  # offset for this word of this phrase.

                my $cur_word = $words[ $word_pos_idx * 2 ];



                unless ( $cur_word =~ /$extract_regexp/ ) {  # something fishy is going on

                    my $idx = ($word_pos + $end_pos) * 2;
                    my ( $s, $e ) = ( $idx - 10, $idx + 10 );
                    $s = 0 if $s < 0;
                    $e = @words-1 if $e >= @words;

                    warn  "Failed to  IgnoreFirst/Last from word '"
                    . (defined $cur_word ? $cur_word : '*undef')
                    . "' (index: $idx) word_pos:$word_pos end_pos:$end_pos total:"
                    . scalar @words
                    . "\n-search pharse words-\n"
                    . join( "\n", map { "$_ '$phrase->[$_]'" } 0..@$phrase -1 )
                    . "\n-Words-\n"
                    . join( "\n", map { "$_ '$words[$_]'" . ($_ == $idx ? ' <<< this word' : '') } $s..$e )
                    . "\n";

                    next PHRASE;
                }




                # Strip ignorefirst and ignorelast
                ( $begin, $word, $end ) = ( $1, $2, $3 );  # this is a waste, as it can operate on the same word over and over

                my $check_word = lc $word;
                my $unstemmed = $check_word;

                # Is the word a stop word?  If so, then just ignore unless in the middle of a phrase

                if ( exists $self->{stopwords}{$check_word} ) {
                    next WORD unless $end_pos;  # skip work unless in the middle of a phrase

                    $end_pos++;
                    print STDERR " Found stopword '$check_word' in the middle of phrase - * MATCH *\n" if DEBUG_HIGHLIGHT;
                    redo if  $word_pos_idx * 2 < @words;  # go on to check this match word with the next word.

                    # No more words to match with, so go on to next pharse.
                    next PHRASE;
                }



                # Now stem the word.
                # Note that this can end up stemming the same word more than once
                # when there's more than one phrase in the query
                # A small attempt is made to cache.
                if ( $stemmer_function ) {
                    if ( $last_stemmed_word && $last_word_pos == $word_pos_idx) {
                        $check_word = $last_stemmed_word;
                    } else {
                        my $w = $stemmer_function->($check_word);
                        $check_word = $w if $w;
                        $last_word_pos = $word_pos_idx;
                        $last_stemmed_word = $check_word;
                    }
                }



                print STDERR "     comparing source # (word:$word_pos offset:$end_pos) '$check_word ($unstemmed)' == '$match_word'\n" if DEBUG_HIGHLIGHT;

                if ( substr( $match_word, -1 ) eq '*' ) {
                    next PHRASE if index( $check_word, substr($match_word, 0, length( $match_word ) - 1) ) != 0;

                } else {
                    next PHRASE if $check_word ne $match_word;
                }


                print STDERR "      *** Word Matched '$check_word' *** \n" if DEBUG_HIGHLIGHT;
                $end_pos++;
            }

            print STDERR "      *** PHRASE MATCHED (word:$word_pos offset:$end_pos) *** \n" if DEBUG_HIGHLIGHT;

            $found_phrase++;


            # We are currently at the end word, so it's easy to set that highlight
            # we just modify the word with a (I hope) unique bit of text that can be substitued later with
            # the HTML to enable highlighting

            $end_pos--;

            if ( !$end_pos ) { # only one word
                $words[$word_pos * 2] = "$begin$on_flag$word$off_flag$end";
            } else {
                $words[($word_pos + $end_pos) * 2 ] = "$begin$word$off_flag$end";

                #Now, reload first word of match
                $words[$word_pos * 2] =~ /$extract_regexp/ or die "2 Why didn't '$words[$word_pos]' =~ /$extract_regexp/?";
                # Strip ignorefirst and ignorelast
                ( $begin, $word, $end ) = ( $1, $2, $3 );  # probably should cache this!
                $words[$word_pos * 2] = "$begin$on_flag$word$end";
            }


            # Now, flag the words around to be shown by using the @flags array.

            my $start = ($word_pos - $Show_Words) * 2;
            my $stop   = ($word_pos + $end_pos + $Show_Words) * 2;
            $start = 0 if $start < 0;
            $stop = $#words if $stop > $#words;  # shift back if went too far



            # Here's Bill Schell's optimization -- if $Show_Words is huge this avoids setting @flags when not needed
            #
            unless ( $show_all_words ) {

                if ( $start == 0 && $stop == $#words ) {
                    $show_all_words = 1;
                    $Max_Words = $stop;  # and to make code simpler below
                } else {
                    $flags[$_]++ for $start .. $stop;
                }
            }


            # All done, and mark where to stop looking
            if ( --$occurrences <= 0 ) {
                $last = $stop;
                last WORD;
            }


            # Now reset $word_pos to word following
            $word_pos += $end_pos; # continue will still be executed
            next WORD;
        }
    } continue {
        $word_pos ++;
    }




    my $dotdotdot = ' ... ';


    my @output;

    my $first = 1;


    # Print context based on the @flags array
    # If $show_all_words was flagged (i.e. everything in @flags is marked then skip this
    # Now, there's the case when the property doesn't contain the search word, so @flags will be empty
    # in that case print $Max_Words.  $found_phrase catches this.

    if ( $show_all_words || !$found_phrase) {
        $Max_Words = $#words if $Max_Words > $#words;
        $$text_ref  = join '', @words[0..$Max_Words];
        $$text_ref .= $dotdotdot if $Max_Words < $#words;  # Add dots if there's more text

    } else {

        # walk the @flags array looking for words to display

        my $printing;  # flag if there's more text.

        for my $i ( 0 ..$#words ) {


            if ( $last && $i >= $last && $i < $#words ) {
                push @output, $dotdotdot;
                last;
            }

            if ( $flags[$i] ) {

                push @output, $dotdotdot if !$printing++ && !$first;
                push @output, $words[$i];

            } else {
                $printing = 0;
            }

            $first = 0;
        }

        push @output, $dotdotdot if !$printing;
        $$text_ref = join '', @output;
    }





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
#

sub set_match_regexp {
    my $self = shift;



    my $wc = $self->header('wordcharacters');
    my $ignoref = $self->header('ignorefirstchar');
    my $ignorel = $self->header('ignorelastchar');


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



