/*
$Id$
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
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
    
** Mon May  9 15:51:39 CDT 2005
** added GPL

**---------------------------------------------------------
** ** ** PATCHED 5/13/96, CJC
** Added MatchAndChange for regex in replace rule G.Hill 2/10/98
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** fixed cast to SWINT_T problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
** 2001-03-17  rasc  save real_filename as title (instead full real_path)
**                   was: compatibility issue to v 1.x.x
**
** 2001-05-09  rasc  entities completly rewritten  (new module)
**                   small fix in parseHTMLsummary
**
** 
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "index.h"
#include "compress.h"
#include "merge.h"
#include "search.h"
#include "docprop.h"
#include "metanames.h"
#include "html.h"
#include "entities.h"
#include "fs.h"
#include "error.h"

/* #### */

static char   *parsetag(SWISH *sw, char *parsetag, char *buffer, SWINT_T max_lines, SWINT_T case_sensitive);

static struct metaEntry *getHTMLMeta(IndexFILE * indexf, char *tag, SWISH *sw, char *name,
                                     char **parsed_tag, char *filename)
{
    char   *temp;
    SWINT_T lenword = 0;
    char *word = NULL;
    char buffer[MAXSTRLEN + 1];
    SWINT_T     i;
    struct metaEntry *e = NULL;


	word = buffer;
	lenword = sizeof(buffer) - 1;

    if (!name)
    {
        if (!(temp = (char *) lstrstr((char *) tag, (char *) "NAME")))
            return NULL;
    }
    else
        temp = name;
    temp += 4;                  /* strlen("NAME") */

    /* Get to the '=' sign disreguarding any other char */
    while (*temp)
    {
        if (*temp && (*temp != '=')) /* TAB */
            temp++;
        else
        {
            temp++;
            break;
        }
    }

    /* Get to the beginning of the word disreguarding blanks and quotes */
    /* TAB */
    while (*temp)
    {
        if (*temp == ' ' || *temp == '"')
            temp++;
        else
            break;
    }

    /* Copy the word and convert to lowercase */
    /* TAB */
    /* while (temp !=NULL && strncmp(temp," ",1) */
    /* && strncmp(temp,"\"",1) && i<= MAXWORDLEN ) { */

    /* and the above <= was wrong, should be < which caused the
       null insertion below to be off by two bytes */


    for (i = 0; temp != NULL && *temp && *temp != ' ' && *temp != '"';)
    {
        if (i == lenword)
        {
            lenword *= 2;
            if(word == buffer)
            {
                word = (char *) emalloc(lenword + 1);
                memcpy(word,buffer,sizeof(buffer));
            }
            else
                word = (char *) erealloc(word, lenword + 1);
        }
        word[i] = *temp++;
        i++;
    }
    if (i == lenword)
    {
        lenword *= 2;
        word = (char *) erealloc(word, lenword + 1);
    }
    word[i] = '\0';

    /* Use Rainer's routine */
    strtolower(word);

    *parsed_tag = word;

    if ((e = getMetaNameByName(&indexf->header, word)))
        return e;

    if ( (sw->UndefinedMetaTags == UNDEF_META_AUTO) && word && *word)
    {
        if (sw->verbose)
            printf("Adding automatic MetaName '%s' found in file '%s'\n", word, filename);

        return addMetaEntry(&indexf->header, word, META_INDEX, 0);
    }

    /* If it is ok not to have the name listed, just index as no-name */
    if (sw->UndefinedMetaTags == UNDEF_META_ERROR)
        progerr("UndefinedMetaNames=error.  Found meta name '%s' in file '%s', not listed as a MetaNames in config", word, filename);

    if(word != buffer)
        efree(word);

    return NULL;

}


/* Parses the Meta tag */
static SWINT_T parseMetaData(SWISH * sw, IndexFILE * indexf, char *tag, SWINT_T filenum, SWINT_T structure, char *name, char *content, FileRec *thisFileEntry,
                         SWINT_T *position, char *filename)
{
    SWINT_T     metaName;
    struct metaEntry *metaNameEntry;
    char   *temp,
           *start,
           *convtag;
    SWINT_T     wordcount = 0;      /* Word count */
    char   *parsed_tag;


    /* Lookup (or add if "auto") meta name for tag */

    metaNameEntry = getHTMLMeta(indexf, tag, sw, name, &parsed_tag, filename);
    metaName = metaNameEntry ? metaNameEntry->metaID : 1;


    temp = content + 7;         /* 7 is strlen("CONTENT") */

    /* Get to the " sign disreguarding other characters */
    if ((temp = strchr(temp, '\"')))
    {
        structure |= IN_META;

        start = temp + 1;

        /* Jump escaped \" */
        temp = strchr(start, '\"');
        while (temp)
        {
            if (*(temp - 1) == '\\')
                temp = strchr(temp + 1, '\"');
            else
                break;
        }

        if (temp)
            *temp = '\0';       /* terminate CONTENT, temporarily */


        /* Convert entities, if requested, and remove newlines */
        convtag = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)start);
        remove_newlines(convtag);  /** why isn't this just done for the entire doc? */



        /* Index only if a metaEntry was found, or if not not ReqMetaName */
        if ( sw->UndefinedMetaTags != UNDEF_META_IGNORE || metaNameEntry)
        {
            /* Meta tags get bummped */
            /* I'm not clear this works as well as I'd like because it always bumps on a new Meta tag, 
             * but in order to disable this behavior the name MUST be a meta name.
             * Probably better to let getHTMLMeta() return the name as a string.
             */


            if (!metaNameEntry || !isDontBumpMetaName(sw->dontbumpstarttagslist, metaNameEntry->metaName))
                position[0]++;

            wordcount = indexstring(sw, convtag, filenum, structure, 1, &metaName, position);

            if (!metaNameEntry || !isDontBumpMetaName(sw->dontbumpendtagslist, metaNameEntry->metaName))
                position[0]++;

        }


        /* If it is a property store it */

        if ((metaNameEntry = getPropNameByName(&indexf->header, parsed_tag)))
            if (!addDocProperty(&thisFileEntry->docProperties, metaNameEntry, (unsigned char*)convtag, strlen(convtag), 0))
                progwarn("property '%s' not added for document '%s'\n", metaNameEntry->metaName, filename);

        if (temp)
            *temp = '\"';       /* restore string */
    }

    return wordcount;
}


/* Extracts anything in <title> tags from an HTML file and returns it.
** Otherwise, only the file name without its path is returned.
*/

char   *parseHTMLtitle(SWISH *sw, char *buffer)
{
    char   *title;
    char   *empty_title;

    empty_title = (char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone,1);
    *empty_title = '\0';

    if (!buffer)
        return empty_title;

    if ((title = parsetag(sw, "title", buffer, TITLETOPLINES, CASE_SENSITIVE_OFF)))
        return title;

    return empty_title;
}


/* Check if a particular title (read: file!) should be ignored
** according to the settings in the configuration file.
*/
/* This is to check "title contains" option in config file */

SWINT_T     isoktitle(SWISH * sw, char *title)
{
    struct MOD_FS *fs = sw->FS;

    return !match_regex_list(title, fs->filerules.title, "FileRules title");
}




/* This returns the value corresponding to the HTML structures
** a word is in.
*/

static SWINT_T getstructure(char *tag, SWINT_T structure)
{

/* SWINT_T len; *//* not used - 2/22/00 */
    char    oldChar = 0;
    char   *endOfTag = NULL;
    char   *pos;

    pos = tag;
    while (*pos)
    {
        if (isspace((SWINT_T) ((unsigned char) *pos)))
        {
            endOfTag = pos;     /* remember where we are... */
            oldChar = *pos;     /* ...and what we saw */
            *pos = '\0';        /* truncate string, for now */
        }
        else
            pos++;
    }
    /*      Store Word Context
       **      Modified DLN 1999-10-24 - Comments and Cleaning
       **  TODO: Make sure that these allow for HTML attributes
       * */

    /* HEAD  */
    if (strcasecmp(tag, "/head") == 0)
        structure &= ~IN_HEAD;  /* Out  */
    else if (strcasecmp(tag, "head") == 0)
        structure |= IN_HEAD;   /* In  */
    /* TITLE  */
    else if (strcasecmp(tag, "/title") == 0)
        structure &= ~IN_TITLE;
    else if (strcasecmp(tag, "title") == 0)
        structure |= IN_TITLE;
    /* BODY */
    else if (strcasecmp(tag, "/body") == 0)
        structure &= ~IN_BODY;  /* In */
    else if (strcasecmp(tag, "body") == 0)
        structure |= IN_BODY;   /* Out */
    /* H1, H2, H3, H4, H5, H6  */
    else if (tag[0] == '/' && tolower((SWINT_T)((unsigned char)tag[1])) == 'h' && isdigit((SWINT_T)((unsigned char)tag[2]))) /* cast to SWINT_T - 2/22/00 */
        structure &= ~IN_HEADER; /* In */
    else if (tolower((SWINT_T)((unsigned char)tag[0])) == 'h' && isdigit((SWINT_T)(unsigned char)tag[1])) /* cast to SWINT_T - 2/22/00 */
        structure |= IN_HEADER; /* Out */
    /* EM, STRONG  */
    else if ((strcasecmp(tag, "/em") == 0) || (strcasecmp(tag, "/strong") == 0))
        structure &= ~IN_EMPHASIZED; /* Out */
    else if ((strcasecmp(tag, "em") == 0) || (strcasecmp(tag, "strong") == 0))
        structure |= IN_EMPHASIZED; /* In */
    /* B, I are seperate for semantics  */
    else if ((strcasecmp(tag, "/b") == 0) || (strcasecmp(tag, "/i") == 0))
        structure &= ~IN_EMPHASIZED; /* Out */
    else if ((strcasecmp(tag, "b") == 0) || (strcasecmp(tag, "i") == 0))
        structure |= IN_EMPHASIZED; /* In */
    /* The End  */

    if (endOfTag != NULL)
    {
        *endOfTag = oldChar;
    }
    return structure;
}



/* Get the MetaData index when the whole tag is passed */

/* Patch by Tom Brown */
/* TAB, this routine is/was somewhat pathetic... but it was pathetic in
 1.2.4 too ... someone needed a course in defensive programming... there are
 lots of tests below for temp != NULL, but what is desired is *temp != '\0'
 (e.g. simply *temp) ... I'm going to remove some strncmp(temp,constant,1)
 which are must faster as *temp != constant ...

 Anyhow, the test case I've got that's core dumping is:
    <META content=3D"MSHTML 5.00.2614.3401" name=3DGENERATOR>
 no trailing quote, no trailing space... and with the missing/broken check for+  end of string it scribbles over the stack...

*/



static char *parseHtmlSummary(char *buffer, char *field, SWINT_T size, SWISH * sw)
{
    char   *p,
           *q,
           *tag,
           *endtag,
            c = '\0';
    char   *summary,
           *beginsum,
           *endsum,
           *tmp,
           *tmp2,
           *tmp3;
    SWINT_T     found,
            lensummary;

    /* Get the summary if no metaname/field is given */
    if (!field && size)
    {
        /* Jump title if it exists */
        if ((p = lstrstr(buffer, "</title>")))
        {
            p += 8;
        }
        else
            p = buffer;
        /* Let us try to find <body> */
        if ((q = lstrstr(p, "<body")))
        {
            q = strchr(q, '>');
        }
        else
            q = p;
        summary = (char *) Mem_ZoneAlloc(sw->Index->perDocTmpZone,strlen(p)+1);
        strcpy(summary,p);
        remove_newlines(summary);

//$$$$ Todo: remove tag and content of scripts, css, java, embeddedobjects, comments, etc  

        remove_tags(summary);

        summary = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)summary);


        /* use only the required memory -save those not used */
        /* 2001-03-13 rasc  copy only <size> bytes of string */
        if((SWINT_T) strlen(summary) > size)
            summary[size]='\0';
        return summary;
    }

    for (p = buffer, summary = NULL, found = 0, beginsum = NULL, endsum = NULL; p && *p;)
    {
        if ((tag = strchr(p, '<')) && ((tag == p) || (*(tag - 1) != '\\')))
        {                       /* Look for non escaped '<' */
            tag++;
            for (endtag = tag;;)
                if ((endtag = strchr(endtag, '>')))
                {
                    if (*(endtag - 1) != '\\')
                        break;
                    else
                        endtag++;
                }
                else
                    break;
            if (endtag)
            {
                c = *endtag;
                *endtag++ = '\0';
                if ((tag[0] == '!') && lstrstr(tag, "META") && (lstrstr(tag, "START") || lstrstr(tag, "END")))
                {               /* Check for META TAG TYPE 1 */
                    if (lstrstr(tag, "START"))
                    {
                        if ((tmp = lstrstr(tag, "NAME")))
                        {
                            tmp += 4;
                            if (lstrstr(tmp, field))
                            {
                                beginsum = endtag;
                                found = 1;
                            }
                            p = endtag;
                        }
                        else
                            p = endtag;
                    }
                    else if (lstrstr(tag, "END"))
                    {
                        if (!found)
                        {
                            p = endtag;
                        }
                        else
                        {
                            endsum = tag - 1;
                            *(endtag - 1) = c;
                            break;
                        }
                    }
                }               /* Check for META TAG TYPE 2 */
                else if ((tag[0] != '!') && lstrstr(tag, "META") && (tmp = lstrstr(tag, "NAME")) && (tmp2 = lstrstr(tag, "CONTENT")))
                {
                    tmp += 4;
                    tmp3 = lstrstr(tmp, field);
                    if (tmp3 && tmp3 < tmp2)
                    {
                        tmp2 += 7;
                        if ((tmp = strchr(tmp2, '=')))
                        {
                            for (++tmp; isspace((SWINT_T) ((unsigned char) *tmp)); tmp++);
                            if (*tmp == '\"')
                            {
                                beginsum = tmp + 1;
                                for (tmp = endtag - 1; tmp > beginsum; tmp--)
                                    if (*tmp == '\"')
                                        break;
                                if (tmp == beginsum)
                                    endsum = endtag - 1;
                                else
                                    endsum = tmp;
                            }
                            else
                            {
                                beginsum = tmp;
                                endsum = endtag - 1;
                            }
                            found = 1;
                            *(endtag - 1) = c;
                            break;

                        }
                    }
                    p = endtag;
                }               /* Default: Continue */
                else
                {
                    p = endtag;
                }
            }
            else
                p = NULL;       /* tag not closed ->END */
            if (endtag)
                *(endtag - 1) = c;
        }
        else
        {                       /* No more '<' */
            p = NULL;
        }
    }
    if (found && beginsum && endsum && endsum > beginsum)
    {
        lensummary = endsum - beginsum;
        summary = (char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, lensummary + 1);
        memcpy(summary, beginsum, lensummary);
        summary[lensummary] = '\0';
    }
    /* If field is set an no metaname is found, let us search */
    /* for something like <field>bla bla </field> */
    if (!summary && field)
    {
        summary = parsetag(sw, field, buffer, 0, CASE_SENSITIVE_OFF);
    }
    /* Finally check for something after title (if exists) and */
    /* after <body> (if exists) */

    if (!summary)
    {
        /* Jump title if it exists */
        if ((p = lstrstr(buffer, "</title>")))
        {
            p += 8;
        }
        else
            p = buffer;

        /* Let us try to find <body> */
        if ((q = lstrstr(p, "<body")))
        {
            q = strchr(q, '>');
        }
        else
            q = p;

        summary = (char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone,strlen(q) + 1);
        strcpy(summary,q);
    }

    if (summary)
    {
        remove_newlines(summary);
        remove_tags(summary);
        summary = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)summary);
    }

    if (summary && size && ((SWINT_T) strlen(summary)) > size)
        summary[size] = '\0';
    return summary;
}


#define NO_TAG 0
#define TAG_CLOSE 1
#define TAG_FOUND 2

/* Gets the content between "<parsetag>" and "</parsetag>" from buffer
limiting the scan to the first max_lines lines (0 means all lines) */
static char   *parsetag(SWISH *sw, char *parsetag, char *buffer, SWINT_T max_lines, SWINT_T case_sensitive)
{
    register SWINT_T c,
            d;
    register char *p,
           *r;
    char   *tag;
    SWINT_T     lencontent;
    char   *content;
    SWINT_T     i,
            j,
            lines,
            status,
            tagbuflen,
            totaltaglen,
            curlencontent;
    char   *begintag;
    char   *endtag;
    char   *newbuf;
    char   *(*f_strstr) ();



    if (case_sensitive)
        f_strstr = strstr;
    else
        f_strstr = lstrstr;

    lencontent = strlen(parsetag);
    begintag = (char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, lencontent + 3);
    endtag = (char *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, lencontent + 4);
    sprintf(begintag, "<%s>", parsetag);
    sprintf(endtag, "</%s>", parsetag);

    tag = (char *) Mem_ZoneAlloc(sw->Index->perDocTmpZone, 1);
    tag[0] = '\0';

    content = (char *) Mem_ZoneAlloc(sw->Index->perDocTmpZone, (lencontent = MAXSTRLEN) + 1);
    lines = 0;
    status = NO_TAG;
    p = content;
    *p = '\0';


    for (r = buffer;;)
    {
        c = *r++;
        if (c == '\n')
        {
            lines++;
            if (max_lines && lines == max_lines)
                break;
        }
        if (!c)
            return NULL;

        switch (c)
        {
        case '<':
            tag = (char *) Mem_ZoneAlloc(sw->Index->perDocTmpZone, (tagbuflen = MAXSTRLEN) + 1);
            totaltaglen = 0;
            tag[totaltaglen++] = '<';

            /* Collect until find '>' */
            while (1)
            {
                d = *r++;
                if (!d)
                    return NULL;
                if (totaltaglen == tagbuflen)
                {
                    newbuf = (char *) Mem_ZoneAlloc(sw->Index->perDocTmpZone, tagbuflen + 200 + 1);
		            memcpy(newbuf,tag,tagbuflen + 1);
                    tag = newbuf;
                    tagbuflen += 200;
                }
                tag[totaltaglen++] = d;
                if (d == '>')
                {
                    tag[totaltaglen] = '\0';
                    break;
                }
            }


            if (f_strstr(tag, endtag))
            {
                status = TAG_CLOSE;
                *p = '\0';

                /* nulls to spaces */
                for (i = 0; content[i]; i++)
                    if (content[i] == '\n')
                        content[i] = ' ';

                /* skip over initial spaces and quotes */
                for (i = 0; isspace((SWINT_T) ((unsigned char) content[i])) || content[i] == '\"'; i++)
                    ;

                /* shift buffer to left */
                for (j = 0; content[i]; j++)
                    content[j] = content[i++];

                content[j] = '\0';


                /* remove trailing spaces, nulls, quotes */
                for (j = strlen(content) - 1; ( j >= 0 ) && ( isspace((SWINT_T) ((unsigned char) content[j])) || content[j] == '\0' || content[j] == '\"'); j--)
                    content[j] = '\0';

                /* replace double quotes with single quotes -- why? */
                for (j = 0; content[j]; j++)
                    if (content[j] == '\"')
                        content[j] = '\'';

                if (*content)
                    return (content);
                else
                    return NULL;
            }
            else if (f_strstr(tag, begintag))
            {
                status = TAG_FOUND;
            }
            break;
        default:
            if (status == TAG_FOUND)
            {
                curlencontent = p - content;
                if (curlencontent == lencontent)
                {
                    newbuf = Mem_ZoneAlloc(sw->Index->perDocTmpZone,(lencontent * 2) + 1);
                    memcpy(newbuf,content,lencontent + 1);
                    lencontent *= 2;
                    content = newbuf;
                    p = content + curlencontent;
                }
                *p = c;
                p++;
            }
        }
    }
    return NULL;
}







/* Parses the words in a comment.
*/

SWINT_T     parsecomment(SWISH * sw, char *tag, SWINT_T filenum, SWINT_T structure, SWINT_T metaID, SWINT_T *position)
{
    structure |= IN_COMMENTS;
    return indexstring(sw, tag + 1, filenum, structure, 1, &metaID, position);
}

/* Indexes all the words in a html file and adds the appropriate information
** to the appropriate structures.
*/

/* Indexes all the words in a html file and adds the appropriate information
** to the appropriate structures.
*/

SWINT_T countwords_HTML(SWISH *sw, FileProp *fprop, FileRec *fi, char *buffer)
{
    SWINT_T     ftotalwords;
    SWINT_T    *metaID;
    SWINT_T     metaIDlen;
    SWINT_T     position;           /* Position of word in file */
    SWINT_T     currentmetanames;
    char   *p,
           *newp,
           *tag,
           *endtag;
    SWINT_T     structure;
    FileRec *thisFileEntry = fi;
    struct metaEntry *metaNameEntry;
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;
    char   *Content = NULL,
           *Name = NULL,
           *summary = NULL;
    char   *title = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)parseHTMLtitle(sw, buffer));

    if (!isoktitle(sw, title))
        return -2;


    if (fprop->stordesc)
        summary = parseHtmlSummary(buffer, fprop->stordesc->field, fprop->stordesc->size, sw);

    addCommonProperties( sw, fprop, fi, title, summary, 0 );

    /* Init meta info */
    metaID = (SWINT_T *) Mem_ZoneAlloc(sw->Index->perDocTmpZone,(metaIDlen = 16) * sizeof(SWINT_T));

    currentmetanames = 0;
    ftotalwords = 0;
    structure = IN_FILE;
    metaID[0] = 1;
    position = 1;
    

    for (p = buffer; p && *p;)
    {

        /* Look for non escaped '<' */
        if ((tag = strchr(p, '<')) && ((tag == p) || (*(tag - 1) != '\\')))
        {
            /* Index up to the tag */
            *tag++ = '\0';

            newp = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)p);

            if ( ! currentmetanames )
                currentmetanames++;
            ftotalwords += indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &position);

            /* Now let us look for a not escaped '>' */
            for (endtag = tag;;)
                if ((endtag = strchr(endtag, '>')))
                {
                    if (*(endtag - 1) != '\\')
                        break;
                    else
                        endtag++;
                }
                else
                    break;


            if (endtag)
            {
                *endtag++ = '\0';

                if ((tag[0] == '!') && lstrstr(tag, "META") && (lstrstr(tag, "START") || lstrstr(tag, "END")))
                {
                    /* Check for META TAG TYPE 1 */
                    structure |= IN_META;
                    if (lstrstr(tag, "START"))
                    {
                        char   *parsed_tag;

                        if (
                            (metaNameEntry =
                             getHTMLMeta(indexf, tag, sw, NULL, &parsed_tag, fprop->real_path)))
                        {
                            /* realloc memory if needed */
                            if (currentmetanames == metaIDlen)
                            {
                                SWINT_T *newbuf = (SWINT_T *)Mem_ZoneAlloc(sw->Index->perDocTmpZone, metaIDlen * 2 * sizeof(SWINT_T));
                                memcpy((char *)newbuf,(char *)metaID,metaIDlen * sizeof(SWINT_T));
                                metaID = newbuf;
                                metaIDlen *= 2;
                            }

                            /* add metaname to array of current metanames */
                            metaID[currentmetanames] = metaNameEntry->metaID;

                            /* Bump position for all metanames unless metaname in dontbumppositionOnmetatags */
                            if (!isDontBumpMetaName(sw->dontbumpstarttagslist, metaNameEntry->metaName))
                                position++;

                            currentmetanames++;


                            p = endtag;

                            /* If it is also a property store it until a < is found */
                            if ((metaNameEntry = getPropNameByName(&indexf->header, parsed_tag)))
                            {
                                if ((endtag = strchr(p, '<')))
                                    *endtag = '\0';


                                p = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)p);

                                remove_newlines(p);  /** why isn't this just done for the entire doc? */

                                if (!addDocProperty(&thisFileEntry->docProperties, metaNameEntry, (unsigned char *)p, strlen(p), 0))
                                    progwarn("property '%s' not added for document '%s'\n", metaNameEntry->metaName, fprop->real_path);


                                if (endtag)
                                    *endtag = '<';

                                continue;
                            }
                        }

                    }

                    else if (lstrstr(tag, "END"))
                    {
                        /* this will close the last metaname */
                        if (currentmetanames)
                        {
                            currentmetanames--;
                            if (!currentmetanames)
                                metaID[0] = 1;
                        }
                    }

                    p = endtag;
                }

                /* Check for META TAG TYPE 2 */
                else if ((tag[0] != '!') && lstrstr(tag, "META") && (Name = lstrstr(tag, "NAME")) && (Content = lstrstr(tag, "CONTENT")))
                {
                    ftotalwords += parseMetaData(sw, indexf, tag, idx->filenum, structure, Name, Content, thisFileEntry, &position, fprop->real_path);
                    p = endtag;
                }               /*  Check for COMMENT */

                else if ((tag[0] == '!') && sw->indexComments)
                {
                    ftotalwords += parsecomment(sw, tag, idx->filenum, structure, 1, &position);
                    p = endtag;
                }               /* Default: Continue */

                else
                {
                    structure = getstructure(tag, structure);
                    p = endtag;
                }
            }
            else
                p = tag;        /* tag not closed: continue */
        }

        else
        {                       /* No more '<' */

            newp = (char *)sw_ConvHTMLEntities2ISO(sw, (unsigned char *)p);

            if ( ! currentmetanames )
                currentmetanames++;
            ftotalwords += indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &position);

            p = NULL;
        }
    }
    return ftotalwords;
}
