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

   -- This module does result output for swish-e
   -- This module implements some methods about the 
   -- "-x fmt" cmd option.
   -- basically: handle output fmts like:  -x "%c|<swishtitle fmt=/%20s/>\n"
   -- 
   -- License: see swish licence file

   -- 2001-01  R. Scherg  (rasc)   initial coding

   -- 2001-02-09 rasc    make propertynames always lowercase!  (may change) 
				 this is get same handling as metanames...
   -- 2001-02-28 rasc    -b and counter corrected...
   -- 2001-03-13 rasc    result header output routine  -H <n>
   -- 2001-04-12 rasc    Module init rewritten

*/





/* Prints the final results of a search.
   2001-01-01  rasc  Standard is swish 1.x default output

   if option extended format string is set, an alternate
   userdefined result output is possible (format like strftime or printf)
   in this case -d (delimiter is obsolete)
     e.g. : -x "result: COUNT:%c \t URL:%u\n"
*/


/* $$$ Remark / ToDO:
   -- The code is a prototype and needs optimizing:
   -- format control string is parsed on each result entry. (very bad!)
   -- ToDO: build an "action array" from an initial parsing of fmt
   --       ctrl string.
   --       on each entry step thru this action output list
   --       seems to be simple, but has to be done.
   -- but for now: get this stuff running on the easy way. 
   -- (rasc 2000-12)
   $$$ 
*/





#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#include "swish.h"
#include "mem.h"
#include "string.h"
#include "merge.h"
#include "docprop.h"
#include "error.h"
#include "search.h"
#include "result_output.h"
#include "no_better_place_module.h"



/* private module prototypes */

static void printExtResultEntry(SWISH * sw, FILE * f, char *fmt, RESULT * r);
static char *printResultControlChar(FILE * f, char *s);
static char *printTagAbbrevControl(SWISH * sw, FILE * f, char *s, RESULT * r);
static char *parsePropertyResultControl(char *s, char **propertyname, char **subfmt);
static void printPropertyResultControl(SWISH * sw, FILE * f, char *propname, char *subfmt, RESULT * r);

static struct ResultExtFmtStrList *addResultExtFormatStr(struct ResultExtFmtStrList *rp, char *name, char *fmtstr);


/*
** ----------------------------------------------
** 
**  Module management code starts here
**
** ----------------------------------------------
*/



/* 
  -- init structures for this module
*/

void    initModule_ResultOutput(SWISH * sw)
{
    struct MOD_ResultOutput *md;

    md = (struct MOD_ResultOutput *) emalloc(sizeof(struct MOD_ResultOutput));

    sw->ResultOutput = md;

    md->resultextfmtlist = NULL;

    /* cmd options */
    md->extendedformat = NULL;  /* -x :cmd param  */
    md->headerOutVerbose = 1;   /* default = standard header */
    md->stdResultFieldDelimiter = NULL; /* -d :old 1.x result output delimiter */

    return;
}


/* 
  -- release all wired memory for this module
  -- 2001-04-11 rasc
*/

void    freeModule_ResultOutput(SWISH * sw)
{
    struct MOD_ResultOutput *md = sw->ResultOutput;
    struct ResultExtFmtStrList *l,
           *ln;


    if (md->stdResultFieldDelimiter)
        efree(md->stdResultFieldDelimiter); /* -d :free swish 1.x delimiter */
    /* was not emalloc!# efree (md->extendedformat);               -x stuff */


    l = md->resultextfmtlist;   /* free ResultExtFormatName */
    while (l)
    {
        efree(l->name);
        efree(l->fmtstr);
        ln = l->next;
        efree(l);
        l = ln;
    }
    md->resultextfmtlist = NULL;

    /* free module data */
    efree(sw->ResultOutput);
    sw->ResultOutput = NULL;

    return;
}



/*
** ----------------------------------------------
** 
**  Module config code starts here
**
** ----------------------------------------------
*/


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int     configModule_ResultOutput(SWISH * sw, StringList * sl)
{
    struct MOD_ResultOutput *md = sw->ResultOutput;
    char   *w0 = sl->word[0];
    int     retval = 1;



    /* $$$ this will not work unless swish is reading the config file also for search ... */

    if (strcasecmp(w0, "ResultExtFormatName") == 0)
    {                           /* 2001-02-15 rasc */
        /* ResultExt...   name  fmtstring */
        if (sl->n == 3)
        {
            md->resultextfmtlist = (struct ResultExtFmtStrList *) addResultExtFormatStr(md->resultextfmtlist, sl->word[1], sl->word[2]);
        }
        else
            progerr("%s: requires \"name\" \"fmtstr\"", w0);
    }
    else
    {
        retval = 0;             /* not a module directive */
    }

    return retval;
}



/*
  -- cmdline settings
  -- return:  # of args read
*/

int     cmdlineModule_ResultOutput(SWISH * sw, char opt, char **args)
{

    //$$$ still to do...
    //$$$ move code from swish.c
    return 0;                   /* quiet a warning */


}




/*
** ----------------------------------------------
** 
**  Module code starts here
**
** ----------------------------------------------
*/



/*
   -- Init the print of result entry in extented output format.
   -- The parsed propertynames will be stored for result handling
   -- Only user properties will be stored.
   -- Routine has to be executed prior to search/result storing...
   -- (This behavior is for historic reasons and may change)
   -- ($$ this routine may build the print action list in the future...)
   2001-02-07   rasc
*/

void    initPrintExtResult(SWISH * sw, char *fmt)
{
    FILE   *f;
    char   *propname;
    char   *subfmt;

    f = (FILE *) NULL;          /* no output, just parsing!!! */


    while (*fmt)
    {                           /* loop fmt string */

        switch (*fmt)
        {

        case '%':              /* swish abbrevation controls */
            /* ignore (dummy param), because autoprop */
            fmt = printTagAbbrevControl(sw, f, fmt, NULL);
            break;

        case '<':
            /* -- Property - Control: read Property Tag  <name> */
            /* -- Save User PropertyNames for result handling   */
            fmt = parsePropertyResultControl(fmt, &propname, &subfmt);

            /**** Not sure what this is for
            if (!isAutoProperty(propname))
            {
                addSearchResultDisplayProperty(sw, propname);
            }
            ****/
            efree(subfmt);
            efree(propname);
            break;

        case '\\':             /* format controls */
            fmt = printResultControlChar(f, fmt);
            break;


        default:               /* a output character in fmt string */
            fmt++;
            break;
        }

    }

}





/* ------------------------------------------------------------ */


/*
  -- Output search result 
  -- do sorting according to cmd opt  ($todo...)
  -- print header informations  ($$todo, move from search.c )
*/

void    printResultOutput(SWISH * sw)
{

    printSortedResults(sw);
}




/*
  -- Output the resuult entries in the given order
  -- outputformat depends on some cmd opt settings
*/

void    printSortedResults(SWISH * sw)
{
    struct MOD_ResultOutput *md = sw->ResultOutput;
    RESULT *r;
    int     resultmaxhits;
    int     resultbeginhits;
    int     counter;
    char   *delimiter;
    FILE   *f_out;


    f_out = stdout;
    resultmaxhits = sw->Search->maxhits;
    resultbeginhits = (sw->Search->beginhits > 0) ? sw->Search->beginhits - 1 : 0;
    delimiter = (md->stdResultFieldDelimiter) ? md->stdResultFieldDelimiter : " ";
    counter = resultbeginhits;


    /* jmruiz 02/2001 SwishSeek is faster because it does not read the
    ** unused data */
    SwishSeek(sw, resultbeginhits);


    /* -- resultmaxhits: >0 or -1 (all hits) */
    while ((r = SwishNext(sw)) && (resultmaxhits != 0))
    {
        r->count = ++counter;   /* set rec. counter for output */


        /* This may or man not be an optimization */
        // ReadAllDocPropertiesFromDisk( sw, r->indexf, r->filenum);
        

        if (md->extendedformat)
            printExtResultEntry(sw, f_out, md->extendedformat, r);

        else
        {
            char *format = emalloc( (3* strlen( delimiter )) + 100 );

            sprintf( format, "%%r%s%%p%s\"%%t\"%s%%l", delimiter, delimiter, delimiter );
            printExtResultEntry(sw, f_out, format, r);
            printStandardResultProperties(sw, f_out, r);

            fprintf(f_out, "\n");
            efree( format );
        }


        /* might as well free the memory as we go */
        freefileinfo( r->indexf->filearray[r->filenum - 1] );
        r->indexf->filearray[r->filenum - 1] = NULL;

        if (resultmaxhits > 0)
            resultmaxhits--;
    }

}





/*
   -- print a result entry in extented output format
   -- Format characters: see switch cases...
   -- f_out == NULL, use STDOUT
   -- fmt = output format
   -- count = current result record counter
   2001-01-01   rasc
*/

static void printExtResultEntry(SWISH * sw, FILE * f_out, char *fmt, RESULT * r)
{
    FILE   *f;
    char   *propname;
    char   *subfmt;


    f = (f_out) ? f_out : stdout;

    while (*fmt)
    {                           /* loop fmt string */

        switch (*fmt)
        {

        case '%':              /* swish abbrevation controls */
            fmt = printTagAbbrevControl(sw, f, fmt, r);
            break;

        case '<':
            /* Property - Control: read and print Property Tag  <name> */
            fmt = parsePropertyResultControl(fmt, &propname, &subfmt);
            printPropertyResultControl(sw, f, propname, subfmt, r);
            efree(subfmt);
            efree(propname);
            break;

        case '\\':             /* print format controls */
            fmt = printResultControlChar(f, fmt);
            break;


        default:               /* just output the character in fmt string */
            if (f)
                fputc(*fmt, f);
            fmt++;
            break;
        }

    }


}







/*  -- parse print control and print it
    --  output on file <f>
    --  *s = "\....."
    -- return: string ptr to char after control sequence.
*/

static char *printResultControlChar(FILE * f, char *s)
{
    char    c,
           *se;

    if (*s != '\\')
        return s;

    c = charDecode_C_Escape(s, &se);
    if (f)
        fputc(c, f);
    return se;
}





/*  -- parse % control and print it
    --  in fact expand shortcut to fullnamed autoproperty tag
    --    output on file <f>, NULL = parse only mode
    --  *s = "%.....
    -- return: string ptr to char after control sequence.
*/

static char *printTagAbbrevControl(SWISH * sw, FILE * f, char *s, RESULT * r)
{
    char   *t;
    char    buf[MAXWORDLEN];

    if (*s != '%')
        return s;
    t = NULL;

    switch (*(++s))
    {
    case 'c':
        t = AUTOPROPERTY_REC_COUNT;
        break;
    case 'd':
        t = AUTOPROPERTY_SUMMARY;
        break;
    case 'D':
        t = AUTOPROPERTY_LASTMODIFIED;
        break;
    case 'I':
        t = AUTOPROPERTY_INDEXFILE;
        break;
    case 'p':
        t = AUTOPROPERTY_DOCPATH;
        break;
    case 'r':
        t = AUTOPROPERTY_RESULT_RANK;
        break;
    case 'l':
        t = AUTOPROPERTY_DOCSIZE;
        break;
    case 'S':
        t = AUTOPROPERTY_STARTPOS;
        break;
    case 't':
        t = AUTOPROPERTY_TITLE;
        break;

    case '%':
        if (f)
            fputc('%', f);
        break;
    default:
        progerr("Formatstring: unknown abbrev '%%%c'", *s);
        break;

    }

    if (t)
    {
        sprintf(buf, "<%s>", t); /* create <...> tag */
        if (f)
            printExtResultEntry(sw, f, buf, r);
        else
            initPrintExtResult(sw, buf); /* parse only ! */
    }
    return ++s;
}




/*  -- parse <tag fmt="..."> control
    --  *s = "<....." format control string
    --       possible subformat:  fmt="...", fmt=/..../, etc. 
    -- return: string ptr to char after control sequence.
    --         **propertyname = Tagname  (or NULL)
    --         **subfmt = NULL or subformat
*/

static char *parsePropertyResultControl(char *s, char **propertyname, char **subfmt)
{
    char   *s1;
    char    c;
    int     len;


    *propertyname = NULL;
    *subfmt = NULL;

    s = str_skip_ws(s);
    if (*s != '<')
        return s;
    s = str_skip_ws(++s);


    /* parse propertyname */

    s1 = s;
    while (*s)
    {                           /* read to end of propertyname */
        if ((*s == '>') || isspace((unsigned char) *s))
        {                       /* delim > or whitespace ? */
            break;              /* break on delim */
        }
        s++;
    }
    len = s - s1;
    *propertyname = (char *) emalloc(len + 1);
    strncpy(*propertyname, s1, len);
    *(*propertyname + len) = '\0';


    if (*s == '>')
        return ++s;             /* no fmt, return */
    s = str_skip_ws(s);


    /* parse optional fmt=<c>...<c>  e.g. fmt="..." */

    if (!strncmp(s, "fmt=", 4))
    {
        s += 4;                 /* skip "fmt="  */
        c = *(s++);             /* string delimiter */
        s1 = s;
        while (*s)
        {                       /* read to end of delim. char */
            if (*s == c)
            {                   /* c or \c */
                if (*(s - 1) != '\\')
                    break;      /* break on delim c */
            }
            s++;
        }

        len = s - s1;
        *subfmt = (char *) emalloc(len + 1);
        strncpy(*subfmt, s1, len);
        *(*subfmt + len) = '\0';
    }


    /* stupid "find end of tag" */

    while (*s && *s != '>')
        s++;
    if (*s == '>')
        s++;

    return s;
}




/*
  -- Print the result value of propertytag <name> on file <f>
  -- if a format is given use it (data type dependend)
  -- string and numeric types are using printfcontrols formatstrings
  -- date formats are using strftime fromat strings.
*/


static void printPropertyResultControl(SWISH * sw, FILE * f, char *propname, char *subfmt, RESULT * r)
{
    char   *fmt;
    PropValue *pv;
    char   *s;
    int     n;


    pv = getResultPropValue(sw, r, propname, 0);

#ifdef USE_DOCPATH_AS_TITLE
    if ( strcmp( AUTOPROPERTY_TITLE, propname ) == 0 && strcmp( "", pv->value.v_str ) == 0 )
    {
        char *c;
        efree( pv );
        pv = getResultPropValue(sw, r, AUTOPROPERTY_DOCPATH, 0);
        if ( pv )
        {
            c = estrdup( str_basename( pv->value.v_str ) );
            efree( pv->value.v_str );
            pv->value.v_str = c;
        }
    }
#endif        


    if (!pv)
    {
        if (f)
            fprintf(f, "(NULL)"); /* Null value, no propname */
        return;
    }


    switch (pv->datatype)
    {
        /* use passed or default fmt */

    case INTEGER:
        fmt = (subfmt) ? subfmt : "%d";
        if (f)
            fprintf(f, fmt, pv->value.v_int);
        break;

    case ULONG:
        fmt = (subfmt) ? subfmt : "%lu";
        if (f)
            fprintf(f, fmt, pv->value.v_ulong);
        break;

        

    case STRING:
        fmt = (subfmt) ? subfmt : "%s";

        /* -- get rid of \n\r in string! */  // there shouldn't be any in the first place, I believe
        for (s = pv->value.v_str; *s; s++)
        {
            if (isspace((unsigned char) *s))
                *s = ' ';
        }

        /* $$$ ToDo: escaping of delimiter characters  $$$ */
        /* $$$ Also ToDo, escapeHTML entities (need config directive) */

        if (f)
            fprintf(f, fmt, (char *) pv->value.v_str);

        /* Free the string, if neede */
        if ( pv->destroy )
            efree( pv->value.v_str );


        break;


    case DATE:
        fmt = (subfmt) ? subfmt : "%Y-%m-%d %H:%M:%S";
        if (!strcmp(fmt, "%ld"))
        {
            /* special: Print date as serial int (for Bill) */
            if (f)
                fprintf(f, fmt, (long) pv->value.v_date);
        }
        else
        {
            /* fmt is strftime format control! */
            s = (char *) emalloc(MAXWORDLEN + 1);
            n = strftime(s, (size_t) MAXWORDLEN, fmt, localtime(&(pv->value.v_date)));
            if (n && f)
                fprintf(f, s);
            efree(s);
        }
        break;

    case FLOAT:
        fmt = (subfmt) ? subfmt : "%f";
        if (f)
            fprintf(f, fmt, (double) pv->value.v_float);
        break;

    default:
        fprintf(stdout, "err:(unknown datatype <%s>)\n", propname);
        break;


    }
    efree(pv);

}



/*
  -------------------------------------------
  Result config stuff
  -------------------------------------------
*/


/*
  -- some code for  -x fmtByName:
  -- e.g.  ResultExtendedFormat   myformat   "<swishtitle>|....\n"
  --       ResultExtendedFormat   yourformat "%c|%t|%p|<author fmt=/%20s/>\n"
  --
  --    swish -w ... -x myformat ...
  --
  --  2001-02-15 rasc
*/


/*
   -- add name and string to list 
*/

static struct ResultExtFmtStrList *addResultExtFormatStr(struct ResultExtFmtStrList *rp, char *name, char *fmtstr)
{
    struct ResultExtFmtStrList *newnode;


    newnode = (struct ResultExtFmtStrList *) emalloc(sizeof(struct ResultExtFmtStrList));

    newnode->name = (char *) estrdup(name);
    newnode->fmtstr = (char *) estrdup(fmtstr);

    newnode->next = NULL;

    if (rp == NULL)
        rp = newnode;
    else
        rp->nodep->next = newnode;

    rp->nodep = newnode;
    return rp;
}



/* 
   -- check if name is a predefined format
   -- case sensitive
   -- return fmtstring for formatname or NULL
*/


char   *hasResultExtFmtStr(SWISH * sw, char *name)
{
    struct ResultExtFmtStrList *rfl;

    rfl = sw->ResultOutput->resultextfmtlist;
    if (!rfl)
        return (char *) NULL;

    while (rfl)
    {
        if (!strcmp(name, rfl->name))
            return rfl->fmtstr;
        rfl = rfl->next;
    }

    return (char *) NULL;
}





/*
  -------------------------------------------
  result header stuff
  -------------------------------------------
*/




/*
  -- print a line for the result output header
  -- the verbose level is checked for output
  -- <min_verbose> has to be >= sw->...headerOutVerbose
  -- outherwise nothing is outputted
  -- return: 0/1  (not printed/printed)
  -- 2001-03-13  rasc
*/

int     resultHeaderOut(SWISH * sw, int min_verbose, char *printfmt, ...)
{
    va_list args;

    /* min_verbose to low, no output */
    if (min_verbose > sw->ResultOutput->headerOutVerbose)
        return 0;

    /* print header info... */
    va_start(args, printfmt);
    vfprintf(stdout, printfmt, args);
    va_end(args);
    return 1;
}




/* 
  -- print a standard result header (according to -H <n>)
  -- for the header of an index file
*/

void    resultPrintHeader(SWISH * sw, int min_verbose, INDEXDATAHEADER * h, char *pathname, int merged)
{
    char   *fname;
    int     v;

    v = min_verbose;
    fname = str_basename(pathname);

    resultHeaderOut(sw, v, "%s\n", INDEXVERSION);
    /* why the blank merge header? */
    resultHeaderOut(sw, v, "# %s\n", (merged) ? "MERGED INDEX" : "");
    resultHeaderOut(sw, v, "%s %s\n", NAMEHEADER, (h->indexn[0] == '\0') ? "(no name)" : h->indexn);
    resultHeaderOut(sw, v, "%s %s\n", SAVEDASHEADER, fname);
    resultHeaderOut(sw, v, "%s %d words, %d files\n", COUNTSHEADER, h->totalwords, h->totalfiles);
    resultHeaderOut(sw, v, "%s %s\n", INDEXEDONHEADER, h->indexedon);
    resultHeaderOut(sw, v, "%s %s\n", DESCRIPTIONHEADER, (h->indexd[0] == '\0') ? "(no description)" : h->indexd);
    resultHeaderOut(sw, v, "%s %s\n", POINTERHEADER, (h->indexp[0] == '\0') ? "(no pointer)" : h->indexp);
    resultHeaderOut(sw, v, "%s %s\n", MAINTAINEDBYHEADER, (h->indexa[0] == '\0') ? "(no maintainer)" : h->indexa);
    resultHeaderOut(sw, v, "%s %s\n", DOCPROPENHEADER, "Enabled");
    resultHeaderOut(sw, v, "%s %d\n", STEMMINGHEADER, h->applyStemmingRules);
    resultHeaderOut(sw, v, "%s %d\n", SOUNDEXHEADER, h->applySoundexRules);
    resultHeaderOut(sw, v, "%s %d\n", IGNORETOTALWORDCOUNTWHENRANKING, h->ignoreTotalWordCountWhenRanking);
    resultHeaderOut(sw, v, "%s %s\n", WORDCHARSHEADER, h->wordchars);
    resultHeaderOut(sw, v, "%s %d\n", MINWORDLIMHEADER, h->minwordlimit);
    resultHeaderOut(sw, v, "%s %d\n", MAXWORDLIMHEADER, h->maxwordlimit);
    resultHeaderOut(sw, v, "%s %s\n", BEGINCHARSHEADER, h->beginchars);
    resultHeaderOut(sw, v, "%s %s\n", ENDCHARSHEADER, h->endchars);
    resultHeaderOut(sw, v, "%s %s\n", IGNOREFIRSTCHARHEADER, h->ignorefirstchar);
    resultHeaderOut(sw, v, "%s %s\n", IGNORELASTCHARHEADER, h->ignorelastchar);
/*
	resultHeaderOut (sw,v, "%s %d\n", FILEINFOCOMPRESSION, h->applyFileInfoCompression);
*/
    translatecharHeaderOut(sw, v, h);


    return;
}
