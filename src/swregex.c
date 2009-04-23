/*
$Id$
**


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
Mon May  9 10:57:22 CDT 2005 -- added GPL notice


**
**
** March 16, 2002 - Bill Moseley: moved regex routines out of string.c
**
** This is a collection of routines for building and testing regular expressions
** for use with swish-e.
**
*/

//#include <ctype.h>
#include "swish.h"
#include "mem.h"
//#include "index.h"
//#include "swish_qsort.h"
#include "swstring.h"
#include "error.h"
#include "swregex.h"

static char *regex_replace( char *str, regex_list *regex, SWINT_T offset, SWINT_T *matched );


/*********************************************************************
*   Adds a list of patterns to a reg_list.  Calls progerr on failure.
*   Call With:
*       name        = Descriptive name for errors - e.g. the name of the directive currently being processed
*       regex_list  = pointer to the list of regular expressions
*       params      = null-terminated list of pointers to strings
*       regex_pattern = flag to indicate that it's a delimited pattern (instead of just the pattern)
*
*   Returns:
*       void
*
*   ToDO:
*       Really should get passed in *SWISH so can set error string and return
*
*   Notes:
*       An expression can be proceeded by the word "not" to negate the matching of the pattern.
*   
*
**********************************************************************/
void add_regex_patterns( char *name, regex_list **reg_list, char **params, SWINT_T regex_pattern )
{
    SWINT_T     negate;
    char    *word;
    char    *pos;
    char    *ptr;
    SWINT_T     delimiter;
    SWINT_T     cflags;
    SWINT_T     global;
    

    while ( *params )
    {
        negate = 0;
        global = 0;
        cflags = REG_EXTENDED;

        
        if ( (strcasecmp( *params, "not" ) == 0) && *(params+1) )
        {
            negate = 1;
            params++;
        }

        /* Simple case of a string pattern */
        if ( !regex_pattern )
        {
            add_regular_expression( reg_list, *params, NULL, cflags, global, negate );
            params++;
            continue;
        }

        word = *params;       
        delimiter = (SWINT_T)*word;

        word++; /* past the first delimiter */

        if ( !(pos = strchr( word, delimiter )))
            progerr("%s regex: failed to find search pattern delimiter '%c' in pattern '%s'", name, (char)delimiter, *params );

        *pos = '\0';            


        /* now check for flags */
        for ( ptr = pos + 1; *ptr; ptr++ )
        {
            if ( *ptr == 'i' )
                cflags |= REG_ICASE;
            else if ( *ptr == 'm' )
                cflags |= REG_NEWLINE;
            else
                progerr("%s regexp %s: unknown flag '%c'", name, *params, *ptr );
        }

        add_regular_expression( reg_list, word, NULL, cflags, global, negate );

        *pos = delimiter;  /* put it back */
        params++;
    }
}

/*********************************************************************
*   Adds a single regex replacement pattern
*
*   Call With:
*       name        = Descriptive name for errors - e.g. the name of the directive currently being processed
*       regex_list  = pointer to the list of regular expressions
*       word        = delimited regex pattern
*
*   Returns:
*       void
*
*
*
**********************************************************************/

void  add_replace_expression( char *name, regex_list **reg_list, char *expression )
   
{
    char    *word = estrdup( expression );
    char    *save = word;
    SWINT_T     delimiter = (SWINT_T)*word;
    char    *pos;
    char    *pattern = NULL;
    char    *replace = NULL;
    SWINT_T     cflags = REG_EXTENDED;
    SWINT_T     global = 0;
    char    *ptr;
    

    word++; /* past the first delimiter */

    if ( !(pos = strchr( word, delimiter )))
        progerr("%s regex: failed to find search pattern delimiter '%c' in pattern '%s'", name, (char)delimiter, word );

    *pos = '\0';            
    pattern = estrdup(word);

    word = pos + 1;  /* now at replace pattern */

    if ( !(pos = strchr( word, delimiter )))
        progerr("%s regex: failed to find replace pattern delimiter '%c' in pattern '%s'", name, (char)delimiter, word );

    *pos = '\0';            
    replace = estrdup(word);


    /* now check for flags */
    for ( ptr = pos + 1; *ptr; ptr++ )
    {
        if ( *ptr == 'i' )
            cflags |= REG_ICASE;

        else if ( *ptr == 'm' )
            cflags |= REG_NEWLINE;

        else if ( *ptr == 'g' )
            global++;
        else
            progerr("%s regexp %s: unknown flag '%c'", name, expression, *ptr );
    }

    add_regular_expression( reg_list, pattern, replace, cflags, global, 0 );

    efree( pattern );
    efree( replace );
    efree( save );
}



/*********************************************************************
*   Match regular expressions
*   Works on a list of expressions, and returns true if *ANY* match
*   
*
**********************************************************************/
SWINT_T match_regex_list( char *str, regex_list *regex, char *comment )
{
    regmatch_t pmatch[1];
    SWINT_T        matched;

    while ( regex )
    {
        matched = regex->negate
            ? regexec(&regex->re, str, (size_t) 1, pmatch, 0) != 0
            : regexec(&regex->re, str, (size_t) 1, pmatch, 0) == 0;

        if ( DEBUG_MASK & DEBUG_REGEX )
            printf("%s match %s %c~ m[%s] : %s\n", comment, str, (int)(regex->negate ? '!' : '='), regex->pattern, matched ? "matched" : "nope" );  // no rw64, %c needs (int)

        if ( matched )
            return 1;

        regex = regex->next;            
    }

    return 0;
}


/*********************************************************************
*   Process all the regular expressions in a regex_list
*
*
**********************************************************************/
char *process_regex_list( char *str, regex_list *regex, SWINT_T *matched )
{
    if ( DEBUG_MASK & DEBUG_REGEX && regex )
        printf("\nOriginal String: '%s'\n", str );

    while ( regex )
    {
        str = regex_replace( str, regex, 0, matched );
        regex = regex->next;

        if ( DEBUG_MASK & DEBUG_REGEX )
            printf("  Result String: '%s'\n", str );

    }

    return str;
}

/*********************************************************************
*  Regular Expression Substitution
*
*   Rewritten 7/31/2001 - general purpose regexp
*
*   Pass in a string and a regex_list pointer
*
*   Returns:
*       a string.  Either the original, or a replacement string
*       Frees passed in string if return is different.
*
*   Notes:
*       Clearly, there must be a library to do this already.  For /g I'm
*       recursively calling this.
*
*
**********************************************************************/
static char *regex_replace( char *str, regex_list *regex, SWINT_T offset, SWINT_T *matched )
{
    regmatch_t pmatch[MAXPAR];
    char   *c;
    char   *newstr;
    SWINT_T     escape = 0;
    SWINT_T     pos = 0;
    SWINT_T     j;
    SWINT_T     last_offset = 0;

    if ( DEBUG_MASK & DEBUG_REGEX )
        printf("replace %s =~ m[%s][%s]: %s\n", str + offset, regex->pattern, regex->replace,
                regexec(&regex->re, str + offset, (size_t) MAXPAR, pmatch, 0) ? "No Match" : "Matched" );
    
    /* Run regex - return original string if no match (might be nice to print error msg? */
    if ( regexec(&regex->re, str + offset, (size_t) MAXPAR, pmatch, 0) )
        return str;


    /* Flag that a pattern matched */
    (*matched)++;        


    /* allocate a string SWINT_T enough */
    newstr = (char *) emalloc( offset + strlen( str ) + regex->replace_length + (regex->replace_count * strlen( str )) + 1 );

    /* Copy everything before string */
    for ( j=0; j < offset; j++ )
        newstr[pos++] = str[j];


    /* Copy everything before the match */
    if ( pmatch[0].rm_so > 0 )
        for ( j = offset; j < pmatch[0].rm_so + offset; j++ )
            newstr[pos++] = str[j];


    /* ugly section */
    for ( c = regex->replace; *c; c++ )
    {
        if ( escape )
        {
            newstr[pos++] = *c;
            last_offset = pos;
            escape = 0;
            continue;
        }
        
        if ( *c == '\\' && *(c+1) )
        {
            escape = 1;
            continue;
        }

        if ( '$' == *c && *(c+1) )
        {
            char   *start = NULL;
            char   *end = NULL;

            c++;

            /* chars before match */
            if ( '`' == *c  ) 
            {
                if ( pmatch[0].rm_so + offset > 0 )
                {
                    start = str;
                    end   = str + pmatch[0].rm_so + offset;
                }
            }

            /* chars after match */
            else if ( '\'' == *c )
            {
                start = str + pmatch[0].rm_eo + offset;
                end = str + strlen( str );
            }

            else if ( *c >= '0' && *c <= '9' )
            {
                SWINT_T i = (SWINT_T)( *c ) - (SWINT_T)'0';

                if ( pmatch[i].rm_so != -1 )
                {
                    start = str + pmatch[i].rm_so + offset;
                    end   = str + pmatch[i].rm_eo + offset;
                }
            }

            else  /* just copy the pattern */
            {
                start = c - 1;
                end   = c + 1;
            }

            if ( start )
                for ( ; start < end; start++ )
                    newstr[pos++] = *start;
        }

        /* not a replace pattern, just copy the char */
        else
            newstr[pos++] = *c;

        last_offset = pos;
    }

    newstr[pos] = '\0';

    /* Append any pattern after the string */
    strcat( newstr, str+pmatch[0].rm_eo + offset );
    

    efree( str );
    

    /* This allow /g processing to match repeatedly */
    /* I'm sure there a way to mess this up and end up with a regex loop... */
    
    if ( regex->global && last_offset < (SWINT_T)strlen( newstr ) )
        newstr = regex_replace( newstr, regex, last_offset, matched );

    return newstr;
}

/*********************************************************
*  Free a regular express list
*
*********************************************************/

void free_regex_list( regex_list **reg_list )
{
    regex_list *list = *reg_list;
    regex_list *next;
    while ( list )
    {
        if ( list->replace )
            efree( list->replace );

        if ( list->pattern )
            efree( list->pattern );

        regfree(&list->re);

        next = list->next;
        efree( list );
        list = next;
    }
    *reg_list = NULL;
}
        
/****************************************************************************
*  Create or Add a regular expression to a list
*  pre-compiles expression to check for errors and for speed
*
*  Pattern and replace string passed in are duplicated
*
*
*****************************************************************************/

void add_regular_expression( regex_list **reg_list, char *pattern, char *replace, SWINT_T cflags, SWINT_T global, SWINT_T negate )
{
    regex_list *new_node = emalloc( sizeof( regex_list ) );
    regex_list *last;
    char       *c;
    SWINT_T         status;
    SWINT_T         escape = 0;

    if ( (status = regcomp( &new_node->re, pattern, cflags )))
        progerr("Failed to complie regular expression '%s', pattern. Error: %lld", pattern, status );



    new_node->pattern = pattern ? estrdup(pattern) : estrdup("");  /* only used for -T debugging */
    new_node->replace = replace ? estrdup(replace) : estrdup("");
    new_node->negate  = negate;

    new_node->global = global;  /* repeat flag */

    new_node->replace_length = strlen( new_node->replace );

    new_node->replace_count = 0;
    for ( c = new_node->replace; *c; c++ )
    {
        if ( escape )
        {
            escape = 0;
            continue;
        }
        
        if ( *c == '\\' )
        {
            escape = 1;
            continue;
        }

        if ( *c == '$' && *(c+1) )
            new_node->replace_count++;
    }
         
            
    new_node->next = NULL;


    if ( *reg_list == NULL )
        *reg_list = new_node;
    else
    {
        /* get end of list */
        for ( last = *reg_list; last->next; last = last->next );

        last->next = new_node;
    }

}

