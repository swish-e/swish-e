/*
$Id$
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
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
**---------------------------------------------------------
** ** ** PATCHED 5/13/96, CJC
** Added MatchAndChange for regex in replace rule G.Hill 2/10/98
**
** change sprintf to snprintf to avoid corruption
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** fixed cast to int problems pointed out by "gcc -Wall"
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
#include "string.h"
#include "index.h"
#include "compress.h"
#include "merge.h"
#include "docprop.h"
#include "metanames.h"
#include "html.h"
#include "entities.h"
#include "fs.h"
#include "error.h"

/* #### */

static struct metaEntry *getHTMLMeta(IndexFILE *indexf, char *tag, int *applyautomaticmetanames, int verbose, int OkNoMeta, char *name)
{
    char   *temp;
    static int lenword = 0;
    static char *word = NULL;
    int     i;
    struct metaEntry *e = NULL;

    /*** $$$ NOTE: memory for "word" is never freed... ***/
    if (!lenword)
        word = (char *) emalloc((lenword = MAXWORDLEN) + 1);

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

    while (1)
    {
        if ((e = getMetaNameData(&indexf->header, word)))
        {
            if ((!is_meta_index(e)) && (*applyautomaticmetanames))
                e->metaType |= META_INDEX;

            return e;
        }

        /* 06/00 Jose Ruiz
           ** If automatic MetaNames enabled add the metaName
           ** else break
         */

        if (*applyautomaticmetanames && word && *word )
        {
            if (verbose)
                printf("\nAdding automatic MetaName %s\n", word);

            /* $$$ addMetaEntry needs to return something here */    
            addMetaEntry(&indexf->header, word, 0, 0, 0, applyautomaticmetanames);
        }
        else
            break;
    }

    /* If it is ok not to have the name listed, just index as no-name */
    if (OkNoMeta)
    {
        /*    printf ("\nwarning: metaName %s does not exist in the user config file", word); */
        return NULL;
    }
    else
    {
        printf("\nerr: INDEXING FAILURE: The metaName %s does not exist in the user config file\n", word);
        exit(-1);
    }

}


/* Parses the Meta tag */
static int     parseMetaData(SWISH * sw, IndexFILE * indexf, char *tag, int filenum, int structure, char *name, char *content, struct file *thisFileEntry,
                      int *position, char *filename)
{
    int     metaName;
    struct metaEntry *metaNameEntry;
    char   *temp,
           *start,
           *convtag;
    int     wordcount = 0;      /* Word count */


    metaNameEntry = getHTMLMeta(indexf, tag, &sw->applyautomaticmetanames, sw->verbose, sw->OkNoMeta, name);

    /* 10/11/99 - Bill Moseley - don't index meta tags not specified in MetaNames */
    if (sw->ReqMetaName && !metaNameEntry)
    {
        return 0;
    }

    if (!metaNameEntry)
        metaName = 1;
    else
        metaName = metaNameEntry->metaID;

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


        /* Convert entities before saving as a property */
        convtag = sw_ConvHTMLEntities2ISO(sw, start);

        remove_newlines(convtag);  /** why isn't this just done for the entire doc? */


        /* If it is a property store it */
        if (metaNameEntry && is_meta_property(metaNameEntry))
            if ( !addDocProperty(&thisFileEntry->docProperties, metaNameEntry, convtag, strlen(convtag),0) )
                progwarn("prop not added for doc '%s'\n", filename );



        /* Do not index as a metaName */
        if (metaNameEntry && !is_meta_index(metaNameEntry))
            metaName = 1;


        /* Meta tags get bummped */
        /* I'm not clear this works as well as I'd like because it always bumps on a new Meta tag, 
         * but in order to disable this behavior the name MUST be a meta name.
         * Probably better to let getHTMLMeta() return the name as a string.
         */
        if(!metaNameEntry || !isDontBumpMetaName( sw, metaNameEntry->metaName ) )
                position[0]++;

        wordcount = indexstring(sw, convtag, filenum, structure, 1, &metaName, position);

        if (convtag != start)
            efree(convtag);
        if (temp)
            *temp = '\"';       /* restore string */
    }
    return wordcount;
}


/* Extracts anything in <title> tags from an HTML file and returns it.
** Otherwise, only the file name without its path is returned.
*/

char   *parsetitle(char *buffer, char *alttitle)
{
    char   *shorttitle,
           *title;


    shorttitle = estrdup(alttitle);
    if (!buffer)
    {
        return shorttitle;
    }

    if ((title = parsetag("title", buffer, TITLETOPLINES, CASE_SENSITIVE_OFF)))
    {
        efree(shorttitle);
        return title;
    }

    return shorttitle;
}


/* Check if a particular title (read: file!) should be ignored
** according to the settings in the configuration file.
*/
/* This is to check "title contains" option in config file */

static int     isoktitle(sw, title)
     SWISH  *sw;
     char   *title;
{
    int     badfile;
    struct swline *tmplist;
    struct MOD_FS *fs = sw->FS;

    badfile = 0;
    tmplist = fs->titconlist;
    while (tmplist != NULL)
    {
        if (matchARegex(title, tmplist->line))
        {
            badfile = 1;
            break;
        }
        tmplist = tmplist->next;
    }
    if (badfile)
        return 0;
    else
        return 1;
}




/* This returns the value corresponding to the HTML structures
** a word is in.
*/

static int     getstructure(tag, structure)
     char   *tag;
     int     structure;
{

/* int len; *//* not used - 2/22/00 */
    char    oldChar = 0;
    char   *endOfTag = NULL;
    char   *pos;

    pos = tag;
    while (*pos)
    {
        if (isspace((int) ((unsigned char) *pos)))
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
    else if (tag[0] == '/' && tolower(tag[1]) == 'h' && isdigit((int) tag[2])) /* cast to int - 2/22/00 */
        structure &= ~IN_HEADER; /* In */
    else if (tolower(tag[0]) == 'h' && isdigit((int) tag[1])) /* cast to int - 2/22/00 */
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



static char   *parseHtmlSummary(char *buffer, char *field, int size, SWISH * sw)
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
    int     found,
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
        tmp = estrdup(p);
        remove_newlines(tmp);
//$$$$ Todo: remove tag and content of scripts, css, java, embeddedobjects, comments, etc  
        remove_tags(tmp);

        /* use only the required memory -save those not used */
        /* 2001-03-13 rasc  copy only <size> bytes of string */
        summary = estrndup(tmp, size);
        efree(tmp);
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
                            for (++tmp; isspace((int) ((unsigned char) *tmp)); tmp++);
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
        summary = emalloc(lensummary + 1);
        memcpy(summary, beginsum, lensummary);
        summary[lensummary] = '\0';
    }
    /* If field is set an no metaname is found, let us search */
    /* for something like <field>bla bla </field> */
    if (!summary && field)
    {
        summary = parsetag(field, buffer, 0, CASE_SENSITIVE_OFF);
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

        summary = estrdup(q);
    }

    if (summary)
    {
        remove_newlines(summary);
        remove_tags(summary);
    }

    if (summary && size && ((int) strlen(summary)) > size)
        summary[size] = '\0';
    return summary;
}

/* Parses the words in a comment.
*/

int     parsecomment(SWISH * sw, char *tag, int filenum, int structure, int metaID, int *position)
{
    structure |= IN_COMMENTS;
    return indexstring(sw, tag + 1, filenum, structure, 1, &metaID, position);
}

/* Indexes all the words in a html file and adds the appropriate information
** to the appropriate structures.
*/

int     countwords_HTML(SWISH * sw, FileProp * fprop, char *buffer)
{
    int     ftotalwords;
    int    *metaID;
    int     metaIDlen;
    int     positionMeta;       /* Position of word in file */
    int     position_no_meta = 1; /* Counter for words in doc (excluding metanames) */
    int     position_meta = 1;  /* Counter for words in doc (only for metanames) */
    int     currentmetanames;
    int     n;
    char   *p,
           *newp,
           *tag,
           *endtag;
    int     structure;
    struct file *thisFileEntry = NULL;
    struct metaEntry *metaNameEntry;
    IndexFILE *indexf = sw->indexlist;
    struct MOD_Index *idx = sw->Index;
    char   *Content = NULL,
           *Name = NULL,
           *summary = NULL;
    char   *title = parsetitle(buffer, fprop->real_filename);

    if (!isoktitle(sw, title))
    {
        efree(title);
        return -2;
    }
    idx->filenum++;

    if (fprop->index_no_content)
    {
    	addtofilelist(sw, indexf, fprop->real_path, NULL );
        addCommonProperties( sw, indexf, fprop->mtime, title, summary, 0, fprop->fsize );


        addtofwordtotals(indexf, idx->filenum, 100);
        if (idx->economic_flag)
            SwapFileData(sw, indexf->filearray[idx->filenum - 1]);
        n = countwordstr(sw, title, idx->filenum);
        efree(title);
        return n;
    }


    if (fprop->stordesc)
        summary = parseHtmlSummary(buffer, fprop->stordesc->field, fprop->stordesc->size, sw);

	addtofilelist(sw, indexf, fprop->real_path, &thisFileEntry );
    addCommonProperties( sw, indexf, fprop->mtime, title, summary, 0, fprop->fsize );

    /* Init meta info */
    metaID = (int *) emalloc((metaIDlen = 1) * sizeof(int));

    currentmetanames = ftotalwords = 0;
    structure = IN_FILE;
    metaID[0] = 1;
    positionMeta = 1;

    for (p = buffer; p && *p;)
    {

        /* Look for non escaped '<' */
        if ((tag = strchr(p, '<')) && ((tag == p) || (*(tag - 1) != '\\')))
        {
            /* Index up to the tag */
            *tag++ = '\0';

            newp = sw_ConvHTMLEntities2ISO(sw, p);

            ftotalwords += indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &positionMeta);

            if (newp != p)
                efree(newp);
            structure = IN_FILE;

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
                        if ((metaNameEntry = getHTMLMeta(indexf, tag, &sw->applyautomaticmetanames, sw->verbose, sw->OkNoMeta, NULL)))
                        {
                            /* If must be indexed add the metaName to the currentlist of metaNames */
                            if (is_meta_index(metaNameEntry))
                            {
                                /* realloc memory if needed */
                                if (currentmetanames == metaIDlen)
                                {
                                    metaID = (int *) erealloc(metaID, (metaIDlen *= 2) * sizeof(int));
                                }

                                /* add metaname to array of current metanames */
                                metaID[currentmetanames] = metaNameEntry->metaID;

                                /* Preserve position */
                                if (!currentmetanames)
                                {
                                    position_no_meta = positionMeta;
                                     /* Init word counter for the metaname */
                                    positionMeta = position_meta;
                                }


                                /* Bump position for all metanames unless metaname in dontbumppositionOnmetatags */
                                if( !isDontBumpMetaName( sw, metaNameEntry->metaName ) )
                                        positionMeta++;

                                currentmetanames++;


                                
                            }
                            p = endtag;

                            /* If it is also a property store it until a < is found */
                            if (is_meta_property(metaNameEntry))
                            {
                                if ((endtag = strchr(p, '<')))
                                    *endtag = '\0';

                                    
                                p = sw_ConvHTMLEntities2ISO(sw, p);

                                remove_newlines(p);  /** why isn't this just done for the entire doc? */

                                if ( !addDocProperty(&thisFileEntry->docProperties, metaNameEntry, p, strlen(p), 0) )
                                    progwarn("prop not added for doc '%s'\n", fprop->real_path );

                                
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
                            {
                                metaID[0] = 1;
                                position_meta = positionMeta;
                                /* Restore position counter */
                                positionMeta = position_no_meta;
                            }
                        }
                    }

                    p = endtag;
                }

                /* Check for META TAG TYPE 2 */
                else if ((tag[0] != '!') && lstrstr(tag, "META") && (Name = lstrstr(tag, "NAME")) && (Content = lstrstr(tag, "CONTENT")))
                {
                    ftotalwords += parseMetaData(sw, indexf, tag, idx->filenum, structure, Name, Content, thisFileEntry, &position_meta, fprop->real_path);
                    p = endtag;
                }               /*  Check for COMMENT */

                else if ((tag[0] == '!') && sw->indexComments)
                {
                    ftotalwords += parsecomment(sw, tag, idx->filenum, structure, 1, &positionMeta);
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

            newp = sw_ConvHTMLEntities2ISO(sw, p);

            ftotalwords += indexstring(sw, newp, idx->filenum, structure, currentmetanames, metaID, &positionMeta);

            if (newp != p)
                efree(newp);
            p = NULL;
        }
    }

    efree(metaID);

    addtofwordtotals(indexf, idx->filenum, ftotalwords);

    if (idx->economic_flag)
        SwapFileData(sw, indexf->filearray[idx->filenum - 1]);

    efree(title);
    if(summary)
        efree(summary);
    return ftotalwords;
}


