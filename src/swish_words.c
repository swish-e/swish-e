/*
$Id$
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
**
** 2001-05-23  moseley  created - replaced parser in search.c
**
** 2001-12-11  moseley, updated to deal with swish operators inside of phrases
**                      Still broken with regard to double-quotes inside of phrases
**                      Very unlikely someone would want to search for a single double quote
**                      within a phrase.  It currently works if the double-quotes doesn't have
**                      white space around.  Really should tag the words as being operators, or
**                      or "swish words", or let the backslash stay in the query until searching.
**
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "list.h"
#include "hash.h"
#include "stemmer.h"
#include "soundex.h"
#include "double_metaphone.h"
#include "error.h"
#include "metanames.h"
#include "config.h"         // for _AND_WORD...
//#include "search_alt.h"     // for AND_WORD... humm maybe needs better organization
#include "swish_words.h"

static struct swline *tokenize_query_string( SEARCH_OBJECT *srch, char *words, INDEXDATAHEADER *header );
static struct swline *ignore_words_in_query(DB_RESULTS *db_results, struct swline *searchwordlist);

static struct swline *fixmetanames(struct swline *);
static struct swline *fixnot1(struct swline *);
static struct swline *fixnot2(struct swline *);
static struct swline *expandphrase(struct swline *, char);



struct MOD_Swish_Words
{
    char   *word;
    int     lenword;
};

/* 
  -- init structures for this module
*/

void initModule_Swish_Words (SWISH  *sw)
{
    struct MOD_Swish_Words *self;

    self = (struct MOD_Swish_Words *) emalloc(sizeof(struct MOD_Swish_Words));
    sw->SwishWords = self;

    /* initialize buffers used by indexstring */
    self->word = (char *) emalloc((self->lenword = MAXWORDLEN) + 1);

    return;
}

void freeModule_Swish_Words (SWISH *sw)
{
  struct MOD_Swish_Words *self = sw->SwishWords;

  efree( self->word );
  efree ( self );
  sw->SwishWords = NULL;

  return;
}





/* Returns true if the character is a search operator */
/* this could be a macro, but gcc is probably smart enough */

static int isSearchOperatorChar( int c, int phrase_delimiter, int inphrase )
{
    return inphrase
        ? ( '*' == c || c == phrase_delimiter )
        : ( '(' == c || ')' == c || '=' == c || '*' == c || c == phrase_delimiter );
}


/* This simply tokenizes by whitespace and by the special characters "()=" */
/* If within a phrase, then just splits by whitespace */

/* Funny how argv was joined into a string just to be split again... */


// $$$ BUG in next_token() is that "search *" or "searh*" becomes two tokens, and then
//   is patched back to at the end (so "search *" becomes "search*").



static int next_token( char **buf, char **word, int *lenword, int phrase_delimiter, int inphrase )
{
    int     i;
    int     backslash;

    **word = '\0';

    /* skip any leading whitespace */
    while ( **buf && isspace( (unsigned char) **buf) )
        (*buf)++;



    /* extract out word */
    i = 0;
    backslash = 0;
    
    while ( **buf && !isspace( (unsigned char) **buf) )
    {

        // This should be looking at swish words, not raw input
        //if ( i > max_size + 4 )   /* leave a little room for operators */
        //    progerr( "Search word exceeded maxwordlimit setting." ); 

    
        /* reallocate buffer, if needed -- only if maxwordlimit was set larger than MAXWORDLEN (1000) */
        if ( i == *lenword )
        {
            *lenword *= 2;
            *word = erealloc(*word, *lenword + 1);
        }


        /* backslash says take next char as-is */
        /* note that you cannot backslash whitespace */
        if ( '\\' == **buf && ! backslash++ )
        {
            (*buf)++;
            continue;
        }


        if ( backslash || !isSearchOperatorChar( (unsigned char) **buf, phrase_delimiter, inphrase ) )
        {
            backslash = 0;
            
            (*word)[i++] = **buf;  /* can't this be done in one line? */
            (*buf)++;
        }

        else  /* this is a search operator char */
        {
            if ( **word )  /* break if characters already found - end of this token */
                break;

            (*word)[i++] = **buf;  /* save the search operator char as it's own token, and end. */
            (*buf)++;
            break;
        }

    }


    /* flag if we found a token */
    if ( i )
    {
        (*word)[i] = '\0';
        return 1;
    }

    return 0;
}


static int next_swish_word(INDEXDATAHEADER *header, char **buf, char **word, int *lenword )
{
    int     i;

    /* skip non-wordchars */
    while ( **buf && !header->wordcharslookuptable[tolower((unsigned char)(**buf))] )
        (*buf)++;

    i = 0;
    while ( **buf && header->wordcharslookuptable[tolower((unsigned char)(**buf))] )
    {
        /* reallocate buffer, if needed */
        if ( i + 1 == *lenword )
        {
            *lenword *= 2;
            *word = erealloc(*word, *lenword + 1);
        }

        (*word)[i++] = **buf;
        (*word)[i] = '\0';
        (*buf)++;
    }


    if ( i )
    {
        stripIgnoreLastChars( header, *word);
        stripIgnoreFirstChars(header, *word);


        return **word ? 1 : 0;
    }

    return 0;
}


/* Convert a word into swish words */

static struct swline *parse_swish_words( SWISH *sw, INDEXDATAHEADER *header, char *word, int max_size )
{
    struct  swline  *swish_words = NULL;
    char   *curpos;
    struct  MOD_Swish_Words *self = sw->SwishWords;

    /* Some initial adjusting of the word */


    TranslateChars(header->translatecharslookuptable, (unsigned char *)word);



    curpos = word;
    while( next_swish_word( header, &curpos, &self->word, &self->lenword ) )
    {
        /* Check Begin & EndCharacters */
        if (!header->begincharslookuptable[(int) ((unsigned char) self->word[0])])
            continue;


        if (!header->endcharslookuptable[(int) ((unsigned char) self->word[strlen(self->word) - 1])])
            continue;


        /* limit by stopwords, min/max length, max number of digits, ... */
        /* ------- processed elsewhere for search ---------
        if (!isokword(sw, self->word, indexf))
            continue;
        - stopwords are processed in search.c because removing them may have side effects
        - maxwordlen is checked when first tokenizing for security reasons
        - limit by vowels, consonants and digits is not needed since search will just fail
        ----------- */
        if ( strlen( self->word ) > max_size )
        {
            sw->lasterror = SEARCH_WORD_TOO_BIG;
            return NULL;
        }

        if (!*self->word)
            continue;

        switch ( header->fuzzy_data.fuzzy_mode )
        {
            case FUZZY_NONE:
                swish_words = (struct swline *) addswline( swish_words, self->word );
                break;

            case FUZZY_STEMMING:
#ifdef SNOWBALL
            case FUZZY_STEMMING_ES:
            case FUZZY_STEMMING_FR:
            case FUZZY_STEMMING_IT:
            case FUZZY_STEMMING_PT:
            case FUZZY_STEMMING_DE:
            case FUZZY_STEMMING_NL:
#endif
                header->fuzzy_data.fuzzy_routine(&self->word, &self->lenword);
                if ( *self->word ) // should not happen
                    swish_words = (struct swline *) addswline( swish_words, self->word );
                break;

                
            case FUZZY_SOUNDEX:
                soundex(self->word);
                if ( *self->word )
                    swish_words = (struct swline *) addswline( swish_words, self->word );
                break;

            case FUZZY_METAPHONE:
            case FUZZY_DOUBLE_METAPHONE:
                {
                    char *codes[2];
                    DoubleMetaphone(self->word, codes);

                    if ( !(*codes[0]) )
                    {
                        efree( codes[0] );
                        efree( codes[1] );
                        swish_words = (struct swline *) addswline( swish_words, self->word );
                        break;
                    }


                    /* check if just METAPHONE or only one word returned (e.g. they are the same) */
                
                    if ( header->fuzzy_data.fuzzy_mode == FUZZY_METAPHONE || !(*codes[1]) || !strcmp(codes[0], codes[1]) )
                    {
                        swish_words = (struct swline *) addswline( swish_words, codes[0] );
                    }
                    else
                    {
                        /* yuck! */
                        swish_words = (struct swline *) addswline( swish_words, "(" );
                        swish_words = (struct swline *) addswline( swish_words, codes[0] );
                        swish_words = (struct swline *) addswline( swish_words, "or" );
                        swish_words = (struct swline *) addswline( swish_words, codes[1] );
                        swish_words = (struct swline *) addswline( swish_words, ")" );
                    }

                    efree( codes[0] );
                    efree( codes[1] );
                }
                break;

            default:
                progerr("Invalid FuzzyMode '%d'", (int)header->fuzzy_data.fuzzy_mode );
        }
    }

    return swish_words;


}

/* This is really dumb.  swline needs a ->prev entry, really search needs its own linked list */
/* Replaces a given node with another node (or nodes) */

static void  replace_swline( struct swline **original, struct swline *entry, struct swline *new_words )
{
    struct swline *temp;


    temp = *original;


    /* check for case of first one */
    if ( temp == entry )
    {
        if ( new_words )
        {
            new_words->nodep->next = temp->next;
            new_words->nodep = temp->nodep;
            *original = new_words;
        } 
        else /* just delete first node */
        {
            if ( entry->next )
                entry->next->nodep = entry->nodep; /* point next one to last one */
            *original = entry->next;
        }
    }



    else /* not first node */
    {
        /* search for the preceeding node */
        for ( temp = *original; temp && temp->next != entry; temp = temp->next );

        if ( !temp )
            progerr("Fatal Error: Failed to find insert point in replace_swline");

        if ( new_words )
        {
            /* set the previous record to point to the start of the new entry (or entries) */
            temp->next = new_words;

            /* set the end of the new string to point to the next entry */
            new_words->nodep->next = entry->next;
        }
        else /* delete the entry */
            temp->next = temp->next->next;
    }
    

    /* now free the removed item */
    efree( entry->line );
    efree( entry );


}


static int checkbuzzword(INDEXDATAHEADER *header, char *word )
{
    if ( !header->hashbuzzwordlist.count )
        return 0;

        
    /* only strip when buzzwords are being used since stripped again as a "swish word" */
    stripIgnoreLastChars( header, word );
    stripIgnoreFirstChars( header, word );
    
    if ( !*word ) /* stripped clean? */
        return 0;


    return is_word_in_hash_table( header->hashbuzzwordlist, word );
}

/* I hope this doesn't live too long */

static void fudge_wildcard( struct swline **original, struct swline *entry )
{
    char    *tmp;
    struct swline *wild_card;

    wild_card = entry->next;        

    /* reallocate a string */
    tmp = entry->line;
    entry->line = emalloc( strlen( entry->line ) + 2 );
    strcpy( entry->line, tmp);
    efree( tmp );
    strcat( entry->line, "*");

    efree( wild_card->line );


    /* removing last entry - set pointer to new end */
    if ( (*original)->nodep == wild_card )
        (*original)->nodep = entry;

    /* and point next to the one after next */
    entry->next = wild_card->next;


    efree( wild_card );
}

    
    
/******************** Public Functions *********************************/

char *isBooleanOperatorWord( char * word )
{
    /* don't need strcasecmp here, since word should alrady be lowercase -- need to check alt-search first */
    if (!strcasecmp( word, _AND_WORD))
        return AND_WORD;

    if (!strcasecmp( word, _OR_WORD))
        return OR_WORD;

    if (!strcasecmp( word, _NOT_WORD))
        return NOT_WORD;

    return (char *)NULL;
}




static struct swline *tokenize_query_string( SEARCH_OBJECT *srch, char *words, INDEXDATAHEADER *header )
{
    char   *curpos;               /* current position in the words string */
    struct  swline *tokens = NULL;
    struct  swline *temp;
    struct  swline *swish_words;
    struct  swline *next_node;
    SWISH  *sw = srch->sw;
    struct  MOD_Swish_Words *self = sw->SwishWords;
    unsigned char PhraseDelimiter;
    int     max_size;
    int     inphrase = 0;



    PhraseDelimiter = (unsigned char) srch->PhraseDelimiter;
    max_size        = header->maxwordlimit;

    curpos = words;  

    /* split into words by whitespace and by the swish operator characters */
    
    while ( next_token( &curpos, &self->word, &self->lenword, PhraseDelimiter, inphrase ) )
    {
        tokens = (struct swline *) addswline( tokens, self->word );


        if ( self->word[0] == PhraseDelimiter && !self->word[1] )
            inphrase = !inphrase;
    }


    /* no search words found */
    if ( !tokens )
        return NULL;


    inphrase = 0;

    temp = tokens;
    while ( temp )
    {

        /* do look-ahead processing first -- metanames */

        if ( !inphrase && isMetaNameOpNext(temp->next) )
        {

            if( !getMetaNameByName( header, temp->line ) )
            {
                set_progerr( UNKNOWN_METANAME, sw, "'%s'", temp->line );
                freeswline( tokens );
                return NULL;
            }


            /* this might be an option with XML */
            strtolower( temp->line );

            temp = temp->next;
            continue;
        }
                

        /* skip operators */
        if ( strlen( temp->line ) == 1 && isSearchOperatorChar( (unsigned char) temp->line[0], PhraseDelimiter, inphrase ) )
        {

            if ( temp->line[0] == PhraseDelimiter && !temp->line[1] )
                inphrase = !inphrase;

            temp = temp->next;
            continue;
        }

        /* this might be an option if case sensitive searches are used */
        strtolower( temp->line );


        /* check Boolean operators -- currently doesn't change it (search.c does) */
        if ( !inphrase )
        {
            char *operator, *nextoperator;

            if ( (operator = isBooleanOperatorWord( temp->line )) )
            {
                /* replace the common "and not" with simply not" */
                /* probably not the best place to do this level of processing */
                /* since should also check for things like "and this" and "and and and not this" */
                /* should probably be moved to end and recursively check for these (to catch "and and not") */
                if (
                    temp->next
                    && ( strcmp( operator, AND_WORD ) == 0)
                    && ( (nextoperator = isBooleanOperatorWord( temp->next->line)))
                    && ( strcmp( nextoperator, NOT_WORD ) == 0)
                ) {
                    struct swline *andword = temp; /* save position */

                    temp = temp->next;  /* now point to "not" word */
                    replace_swline( &tokens, andword, (struct swline *)NULL ); /* cut it out */
                    continue;
                }

                temp = temp->next;
                continue;
            }
        }
    

        /* buzzwords */
        if ( checkbuzzword( header, temp->line ) )
        {
            temp = temp->next;
            continue;
        }



        /* query words left.  Turn into "swish_words" */
        swish_words = NULL;
        swish_words = parse_swish_words( sw, header, temp->line, max_size);

        if ( sw->lasterror )
            return NULL;

        
        next_node = temp->next;

        /* move into list.c at some point */
        replace_swline( &tokens, temp, swish_words );
        temp = next_node;
        
    }

    /* fudge wild cards back onto preceeding word */
    /* $$$ This is broken because a query of "foo *" ends up "foo*" */

    
    for ( temp = tokens ; temp; temp = temp->next )
        if ( temp->next && strcmp( temp->next->line, "*") == 0 )
            fudge_wildcard( &tokens, temp );


    return tokens;
}

/**********************************************************************************
*  parse_swish_query -- convert a string in a SEARCH_OBJECT into a list of tokens
*
*   Pass in:
*       db_results - container for the search results for a single index
*
*   Returns:
*       false on error
*       sets sw->lasterror on fatal errors.
*
*       but some errors are cleared and set in the search object to allow
*       processing to continue.  For example, when searching multiple index
*       files one index may end up removing all the words in a query (if they
*       are all stopwords) where another index will not have the same stopword
*       list and produce a query.  Room for improvement, of course.
*
*   Notes:
*       calls tokenize_query_string() as first pass
*       ignore_words_in_query() to remove stop words (could be combined with tokenize, I think -- but removing stop words is a bit tricky)
*       expandpharse() prepares for phrase searching
*       fixmetanames() fixnot1() and fixnot2() make other adjustments
*
*       Clearly, a better query parser is in order.
*
*       Sep 29, 2002 - moseley
*
***********************************************************************************/

struct swline *parse_swish_query( DB_RESULTS *db_results )
{
    struct swline *searchwordlist;
    IndexFILE     *indexf = db_results->indexf;
    SEARCH_OBJECT *srch = db_results->srch;
    


    /* tokenize the query into swish words based on this current index */
    /* returns false if no words or error */
    /* may set sw->lasterror on unknown metanames or word too big */

    if (!(searchwordlist = tokenize_query_string(srch, srch->query, &indexf->header)))
        return NULL;




    /* Remove stopwords from the query -- also sets db_results->removed_stopwords */

    /* This can set QUERY_SYNTAX_ERROR which should abort */
    /* WORDS_TOO_COMMON & NO_WORDS_IN_SEARCH should be used if no index files are searched */
    /* This is a bit ugly */

    searchwordlist = ignore_words_in_query(db_results, searchwordlist);
  
    if ( !searchwordlist || srch->sw->lasterror )
    {
        if ( searchwordlist )
            freeswline( searchwordlist );
        return NULL;
    }

    db_results->parsed_words = dupswline(searchwordlist);


    /* Now hack up the query for searh processing */
    /* $$$ please fix this!  Let's get a real parser */

        
    /* Expand phrase search: "kim harlow" becomes (kim PHRASE_WORD harlow) */
    searchwordlist = expandphrase(searchwordlist, (char)srch->PhraseDelimiter);

    searchwordlist = fixmetanames(searchwordlist);
    searchwordlist = fixnot1(searchwordlist);
    searchwordlist = fixnot2(searchwordlist);


    return searchwordlist;
}

static int     u_isrule(SWISH * sw, char *word)
{
    if (!strcmp(word, _AND_WORD) || !strcmp(word, _OR_WORD) || !strcmp(word, _NOT_WORD))
        return 1;
    else
        return 0;
}

static int     isnotrule(char *word)
{
    if (!strcmp(word, NOT_WORD))
        return 1;
    else
        return 0;
}

static int     u_isnotrule(SWISH * sw, char *word)
{
    if (!strcmp(word, _NOT_WORD))
        return 1;
    else
        return 0;
}



/******************************************************************************
*   Remove the stop words from the tokenized query
*   rewritten Nov 24, 2001 - moseley
*   Still horrible!  Need a real parse tree.
*******************************************************************************/


static struct swline *ignore_words_in_query(DB_RESULTS *db_results, struct swline *searchwordlist)
{
    IndexFILE      *indexf = db_results->indexf;
    SEARCH_OBJECT  *srch = db_results->srch;
    SWISH          *sw = srch->sw;
    struct swline  *cur_token = searchwordlist;
    struct swline  *prev_token = NULL;
    struct swline  *prev_prev_token = NULL;  // for removing two things
    int             in_phrase = 0;
    int             word_count = 0; /* number of search words found */
    int             paren_count = 0;
    int             stop_word_removed = 0;
    unsigned char   phrase_delimiter = (unsigned char)srch->PhraseDelimiter;
    


    

    while ( cur_token )
    {
        int remove = 0;
        char first_char = cur_token->line[0];

        if ( cur_token == searchwordlist )
        {
            prev_token = prev_prev_token = NULL;
            word_count = 0;
            paren_count = 0;
            in_phrase = 0;
        }

        while ( 1 )  // so we can use break.
        {

            /* Can't backslash here -- (because this code should really be include in swish_words.c) */
            
            if ( first_char == phrase_delimiter )
            {
                in_phrase = !in_phrase;

                if ( !in_phrase && prev_token && prev_token->line[0] == phrase_delimiter )
                    remove = 2;
                    
                break;
            }


            /* leave everything alone inside a pharse */
            
            if ( in_phrase )
            {
                if ( is_word_in_hash_table( indexf->header.hashstoplist, cur_token->line ) )
                {
                    db_results->removed_stopwords = addswline( db_results->removed_stopwords, cur_token->line );
                    stop_word_removed++;
                    remove = 1;
                }
                else
                    word_count++;

                break;                    
            }
            

            /* Allow operators */

            if ( first_char == '=' )
                break;
            
            if ( first_char == '(' )
            {
                paren_count++;
                break;
            }

            if ( first_char == ')' )
            {
                paren_count--;

                if ( prev_token && prev_token->line[0] == '(' )
                    remove = 2;

                break;
            }
                


            /* Allow all metanames */
            
            if ( isMetaNameOpNext(cur_token->next) )
                break;



            /* Look for AND OR NOT - remove AND OR at start, and remove second of doubles */

            if ( u_isrule(sw, cur_token->line)  )
            {
                if ( prev_token )
                {
                    /* remove double tokens */
                    if ( u_isrule( sw, prev_token->line ) )
                        remove = 1;
                }
                /* allow NOT at the start */
                else if ( !u_isnotrule(sw, cur_token->line) )
                    remove = 1;

                break;
            }
                    


            /* Finally, is it a stop word? */

            if ( is_word_in_hash_table( indexf->header.hashstoplist, cur_token->line ) )
            {
                db_results->removed_stopwords = addswline( db_results->removed_stopwords, cur_token->line );
                stop_word_removed++;
                remove = 1;
            }
            else
                word_count++;
            

            
            break;
        }


        /* Catch dangling metanames */
        if ( !remove && !cur_token->next && isMetaNameOpNext( cur_token ) )
            remove = 2;


        if ( remove )
        {
            struct swline *tmp = cur_token;
                
            if ( cur_token == searchwordlist )      // we are removing first token
                searchwordlist = cur_token->next;
            else
            {
                prev_token->next = cur_token->next; // remove one in the middle
                cur_token = prev_token;  // save if remove == 2
            }


            efree( tmp->line );
            efree( tmp );

            if ( remove == 2 )
            {
                tmp = cur_token;

                if ( cur_token == searchwordlist )      // we are removing first token
                    searchwordlist = cur_token->next;
                else
                    prev_prev_token->next = cur_token->next; // remove one in the middle
                
                efree( tmp->line );
                efree( tmp );
            }


            /* start at the beginning again */

            cur_token = searchwordlist;
            continue;
        }
        
        if ( prev_token )
            prev_prev_token = prev_token;
            
        prev_token = cur_token;
        cur_token = cur_token->next;
    }


    if ( in_phrase || paren_count )
        sw->lasterror = QUERY_SYNTAX_ERROR;

    else if ( !word_count )
        sw->lasterror = stop_word_removed ? WORDS_TOO_COMMON : NO_WORDS_IN_SEARCH;


    return searchwordlist;

}


/* 2001-09 jmruiz - Rewriting
** This puts parentheses in the right places around meta searches
** to avoid problems whith them. Basically "metaname = bla" 
** becomes "(metanames = bla)" */

static struct swline *fixmetanames(struct swline *sp)
{
    int     metapar;
    struct swline *tmpp,
           *newp;

    tmpp = sp;
    newp = NULL;

    /* Fix metanames with parenthesys eg: metaname = bla => (metanames = bla) */
    while (tmpp != NULL)
    {
        if (isMetaNameOpNext(tmpp->next))
        {
            /* If it is a metaName add the name and = and skip to next */
            newp = (struct swline *) addswline(newp, "(");
            newp = (struct swline *) addswline(newp, tmpp->line);
            newp = (struct swline *) addswline(newp, "=");
            tmpp = tmpp->next;
            tmpp = tmpp->next;
            if ( !tmpp )
                return NULL;  /* no more words! */
            
            /* 06/00 Jose Ruiz
               ** Fix to consider parenthesys in the
               ** content of a MetaName */
            if (tmpp->line[0] == '(')
            {
                metapar = 1;
                newp = (struct swline *) addswline(newp, tmpp->line);
                tmpp = tmpp->next;
                while (metapar && tmpp)
                {
                    if (tmpp->line[0] == '(')
                        metapar++;
                    else if (tmpp->line[0] == ')')
                        metapar--;
                    newp = (struct swline *) addswline(newp, tmpp->line);
                    if (metapar)
                        tmpp = tmpp->next;
                }
                if (!tmpp)
                    return (newp);
            }
            else
                newp = (struct swline *) addswline(newp, tmpp->line);
            newp = (struct swline *) addswline(newp, ")");
        }
        else 
            newp = (struct swline *) addswline(newp, tmpp->line);
        /* next one */
        tmpp = tmpp->next;
    }

    freeswline(sp);
    return newp;
}

/* 2001 -09 jmruiz Rewritten 
** This optimizes some NOT operator to be faster.
**
** "word1 not word" is changed by "word1 and_not word2"
**
** In the old way the previous query was...
**    get results if word1
**    get results of word2
**    not results of word2 (If we have 100000 docs and word2 is in
**                          just 3 docs, this means read 99997
**                          results)
**    intersect both list of results
**
** The "new way"
**    get results if word1
**    get results of word2
**    intersect (and_not_rule) both lists of results
** 
*/

static struct swline *fixnot1(struct swline *sp)
{
    struct swline *tmpp,
           *prev;

    if (!sp)
        return NULL;
    /* 06/00 Jose Ruiz - Check if first word is NOT_RULE */
    /* Change remaining NOT by AND_NOT_RULE */
    for (tmpp = sp, prev = NULL; tmpp; prev = tmpp, tmpp = tmpp->next)
    {
        if (tmpp->line[0] == '(')
            continue;
        else if (isnotrule(tmpp->line))
        {
            if(prev && prev->line[0]!='=' && prev->line[0]!='(')
            {
                efree(tmpp->line);
                tmpp->line = estrdup(AND_NOT_WORD);
            }
        }
    }
    return sp;
}

/* 2001 -09 jmruiz - Totally new - Fix the meta=(not ahsg) bug
** Add parentheses to avoid the way operator NOT confuse complex queries */
static struct swline *fixnot2(struct swline *sp)
{
    int     openparen, found;
    struct swline *tmpp, *newp;
    char    *magic = "<__not__>";  /* magic avoids parsing the
                                   ** "not" operator twice
                                   ** and put the code in an 
                                   ** endless loop */                            

    found = 1;
    while(found)
    {
        openparen = 0;
        found = 0;

        for (tmpp = sp , newp = NULL; tmpp ; tmpp = tmpp->next)
        {
            if (isnotrule(tmpp->line))
            {
                found = 1;
                /* Add parentheses */
                newp = (struct swline *) addswline(newp, "(");
                /* Change "NOT" by magic to avoid find it in next iteration */
                newp = (struct swline *) addswline(newp, magic);
                for(tmpp = tmpp->next; tmpp; tmpp = tmpp->next)
                {
                    if ((tmpp->line)[0] == '(')
                        openparen++;
                    else if(!openparen)
                    {
                        newp = (struct swline *) addswline(newp, tmpp->line);
                        /* Add parentheses */
                        newp = (struct swline *) addswline(newp, ")");
                        break;
                    }
                    else if ((tmpp->line)[0] == ')')
                        openparen--;
                    newp = (struct swline *) addswline(newp, tmpp->line);
                }
                if(!tmpp)
                    break;
            }
            else
                newp = (struct swline *) addswline(newp, tmpp->line);
        }
        freeswline(sp);
        sp = newp;
    }

    /* remove magic and put the "real" NOT in place */
    for(tmpp = newp; tmpp ; tmpp = tmpp->next)
    {
        if(!strcmp(tmpp->line,magic))
        {
            efree(tmpp->line);
            tmpp->line = estrdup(NOT_WORD);
        }
    }

    return newp;
}

/* expandstar removed - Jose Ruiz 04/00 */

/* Expands phrase search. Berkeley University becomes Berkeley PHRASE_WORD University */
/* It also fixes the and, not or problem when they appeared inside a phrase */
static struct swline *expandphrase(struct swline *sp, char delimiter)
{
    struct swline *tmp,
           *newp;
    int     inphrase;

    if (!sp)
        return NULL;
    inphrase = 0;
    newp = NULL;
    tmp = sp;
    while (tmp != NULL)
    {
        if ((tmp->line)[0] == delimiter)
        {
            if (inphrase)
            {
                inphrase = 0;
                newp = (struct swline *) addswline(newp, ")");
            }
            else
            {
                inphrase++;
                newp = (struct swline *) addswline(newp, "(");
            }
        }
        else
        {
            if (inphrase)
            {
                if (inphrase > 1)
                    newp = (struct swline *) addswline(newp, PHRASE_WORD);
                inphrase++;
                newp = (struct swline *) addswline(newp, tmp->line);
            }
            else
            {
                char *operator;

                if ( ( operator = isBooleanOperatorWord( tmp->line )) )
                    newp = (struct swline *) addswline(newp, operator);
                else
                    newp = (struct swline *) addswline(newp, tmp->line);
            }
        }
        tmp = tmp->next;
    }
    freeswline(sp);
    return newp;
}



/*  These 2 routines fix the problem when a word ends with mutiple
**  IGNORELASTCHAR's (eg, qwerty'. ).  The old code correctly deleted
**  the ".", but didn't check if the new last character ("'") is also
**  an ignore character.
*/
void     stripIgnoreLastChars(INDEXDATAHEADER *header, char *word)
{
    int     k,j,i = strlen(word);

    /* Get rid of specified last char's */
    /* for (i=0; word[i] != '\0'; i++); */
    /* Iteratively strip off the last character if it's an ignore character */
    while ((i > 0) && (isIgnoreLastChar(header, word[--i])))
    {
        word[i] = '\0';

        /* We must take care of the escaped characeters */
        /* Things like hello\c hello\\c hello\\\c can appear */
        for(j=0,k=i-1;k>=0 && word[k]=='\\';k--,j++);
        
        /* j contains the number of \ */
        if(j%2)   /* Remove the escape if even */
        {
             word[--i]='\0';    
        }
    }
}

void    stripIgnoreFirstChars(INDEXDATAHEADER *header, char *word)
{
    int     j,
            k;
    int     i = 0;

    /* Keep going until a char not to ignore is found */
    /* We must take care of the escaped characeters */
    /* Things like \chello \\chello can appear */

    while (word[i])
    {
        if(word[i]=='\\')   /* Jump escape */
            k=i+1;
        else
            k=i;
        if(!word[k] || !isIgnoreFirstChar(header, word[k]))
            break;
        else
            i=k+1;
    }

    /* If all the char's are valid, just return */
    if (0 == i)
        return;
    else
    {
        for (k = i, j = 0; word[k] != '\0'; j++, k++)
        {
            word[j] = word[k];
        }
        /* Add the NULL */
        word[j] = '\0';
    }
}


