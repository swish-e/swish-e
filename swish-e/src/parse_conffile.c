/*
$Id$
**
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
*/

/* New file created from file.c 02/2001 jmruiz */
/* Contains routines for parsing the configuration file */

/*
** 2001-02-15 rasc    ResultExtFormatName 
** 2001-03-03 rasc    EnableAltaVistaSyntax
**                    code optimize: getYesNoOrAbort 
** 2001-03-13 rasc    SwishSearchOperators, SwishSearchDefaultRule
** 2001-03-16 rasc    TruncateDocSize nbytes
** 2001-04-09 rasc    Filters: options (opt.)
**
*/


#include "swish.h"
#include "string.h"
#include "mem.h"
#include "list.h"
#include "file.h"
#include "metanames.h"
#include "hash.h"
#include "error.h"
#include "entities.h"
#include "filter.h"
#include "result_output.h"
#include "index.h"
#include "search.h"
#include "search_alt.h"
#include "parse_conffile.h"
#include "merge.h"   /* Argh, needed for docprop.h */
#include "docprop.h"
/* removed stuff 
#include "deflate.h"
*/
#include "result_sort.h"
#include "db.h"
#include "extprog.h"
#ifdef HAVE_ZLIB
#include "zlib.h"
#endif



static void Build_ReplaceRules( char *name, char **params, regex_list **reg_list );
static  void add_ExtractPath( char * name, SWISH *sw, struct metaEntry *m, char **params );
static int     getDocTypeOrAbort(StringList * sl, int n);
static void readstopwordsfile(SWISH *, IndexFILE *, char *);
static void readusewordsfile(SWISH *, IndexFILE *, char *);
static void readbuzzwordsfile(SWISH *, IndexFILE *, char *);
static int parseconfline(SWISH *, StringList *);
static void get_undefined_meta_flags( char *w0, StringList * sl, UndefMetaFlag *setting );




/* Reads the configuration file and puts all the right options
** in the right variables and structures.
*/

void    getdefaults(SWISH * sw, char *conffile, int *hasdir, int *hasindex, int hasverbose)
{
    int     i,
            gotdir,
            gotindex;
    char    line[MAXSTRLEN];
    FILE   *fp;
    int     linenumber = 0;
    int     baddirective = 0;
    StringList *sl;
    struct IndexContents *ic;
    struct StoreDescription *sd;
    IndexFILE *indexf = NULL;
    unsigned char *StringValue = NULL;
    struct swline *tmplist;
    char   *w0;

    gotdir = gotindex = 0;

    if ((fp = fopen(conffile, FILEMODE_READ)) == NULL || !isfile(conffile))
    {
        progerrno("Couldn't open the configuration file '%s': ", conffile);
    }

    /* Init default index file */
    indexf = sw->indexlist = (IndexFILE *) addindexfile(sw->indexlist, INDEXFILE);


    sl = NULL;

    while (fgets(line, MAXSTRLEN, fp) != NULL)
    {
        linenumber++;

        if ( sl )
            freeStringList(sl);

        /* Parse line */
        if (!(sl = parse_line(line)))
            continue;

        if (!sl->n)
            continue;

        w0 = sl->word[0];       /* Config Direct. = 1. word */

        if (w0[0] == '#')
            continue;           /* comment */



        if (strcasecmp(w0, "IndexDir") == 0)
        {
            if (sl->n > 1)
            {
                if (!*hasdir)
                {
                    gotdir = 1;
                    grabCmdOptions(sl, 1, &sw->dirlist);
                }
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;                
        }

        
        if (strcasecmp(w0, "IncludeConfigFile") == 0)
        {
            if (sl->n == 2)
            {
                getdefaults(sw, sl->word[1], hasdir, hasindex, hasverbose);
            }
            else
                progerr("%s: requires one value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "NoContents") == 0)
        {
            if (sl->n > 1)
            {
                grabCmdOptions(sl, 1, &sw->nocontentslist);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "IndexFile") == 0)
        {
            if (!(*hasindex))
            {
                if (sl->n == 2)
                {
                    gotindex = 1;
                    if (indexf->line)
                        efree(indexf->line);
                    indexf->line = estrdup(sl->word[1]);
                }
                else
                    progerr("%s: requires one value", w0);
            }
            
            continue;
        }

        
        if (strcasecmp(w0, "IndexReport") == 0)
        {
            if (sl->n == 2)
            {
                if (!hasverbose)
                {
                    sw->verbose = atoi(sl->word[1]);
                }
            }
            else
                progerr("%s: requires one value", w0);
            continue;
        }

        if (strcasecmp(w0, "ParserWarnLevel") == 0)
        {
            if (sl->n == 2)
                sw->parser_warn_level = atoi(sl->word[1]);
            else
                progerr("%s: requires one value", w0);
            continue;
        }


        if (strcasecmp(w0, "obeyRobotsNoIndex") == 0)
        {
            sw->obeyRobotsNoIndex = getYesNoOrAbort(sl, 1, 1);
            continue;
        }


        if (strcasecmp(w0, "AbsoluteLinks") == 0)
        {
            sw->AbsoluteLinks = getYesNoOrAbort(sl, 1, 1);
            continue;
        }




        if (strcasecmp(w0, "MinWordLimit") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.minwordlimit = atoi(sl->word[1]);
            }
            else
                progerr("%s: requires one value", w0);
            continue;
        }

        if (strcasecmp(w0, "MaxWordLimit") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.maxwordlimit = atoi(sl->word[1]);
            }
            else
                progerr("%s: requires one value", w0);

            continue;
        }
        

        if (strcasecmp(w0, "IndexComments") == 0)
        {
            sw->indexComments = getYesNoOrAbort(sl, 1, 1);
            continue;
        }


        if (strcasecmp(w0, "WordCharacters") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars, sl->word[1], &indexf->header.lenwordchars);
                sortstring(indexf->header.wordchars);
                makelookuptable(indexf->header.wordchars, indexf->header.wordcharslookuptable);
            }
            else
                progerr("%s: requires one value", w0);

            continue;
        }


        if (strcasecmp(w0, "BeginCharacters") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars, sl->word[1], &indexf->header.lenbeginchars);
                sortstring(indexf->header.beginchars);
                makelookuptable(indexf->header.beginchars, indexf->header.begincharslookuptable);
            }
            else
                progerr("%s: requires one value", w0);
            continue;
        }

        if (strcasecmp(w0, "EndCharacters") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.endchars = SafeStrCopy(indexf->header.endchars, sl->word[1], &indexf->header.lenendchars);
                sortstring(indexf->header.endchars);
                makelookuptable(indexf->header.endchars, indexf->header.endcharslookuptable);
            }
            else
                progerr("%s: requires one value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "IgnoreLastChar") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar, sl->word[1], &indexf->header.lenignorelastchar);
                sortstring(indexf->header.ignorelastchar);
                makelookuptable(indexf->header.ignorelastchar, indexf->header.ignorelastcharlookuptable);
            }                   /* Do nothing */
            /* else progerr("%s: requires one value",w0); */

            continue;
        }
        
        if (strcasecmp(w0, "IgnoreFirstChar") == 0)
        {
            if (sl->n == 2)
            {
                indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar, sl->word[1], &indexf->header.lenignorefirstchar);
                sortstring(indexf->header.ignorefirstchar);
                makelookuptable(indexf->header.ignorefirstchar, indexf->header.ignorefirstcharlookuptable);
            }                   /* Do nothing */
            /*  else progerr("%s: requires one value",w0); */

            continue;
        }

        
        if (strcasecmp(w0, "ReplaceRules") == 0)
        {
            if (sl->n > 2)
                Build_ReplaceRules( w0, sl->word, &sw->replaceRegexps );
            else
                progerr("%s: requires at least two values", w0);

            continue;
        }

        
        if (strcasecmp(w0, "IndexName") == 0)
        {
            if (sl->n > 1)
            {
                StringValue = StringListToString(sl, 1);
                indexf->header.indexn = SafeStrCopy(indexf->header.indexn, StringValue, &indexf->header.lenindexn);
                efree(StringValue);
            }
            else
                progerr("%s: requires a value", w0);
            continue;
        }
        

        if (strcasecmp(w0, "IndexDescription") == 0)
        {
            if (sl->n > 1)
            {
                StringValue = StringListToString(sl, 1);
                indexf->header.indexd = SafeStrCopy(indexf->header.indexd, StringValue, &indexf->header.lenindexd);
                efree(StringValue);
            }
            else
                progerr("%s: requires a value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "IndexPointer") == 0)
        {
            if (sl->n > 1)
            {
                StringValue = StringListToString(sl, 1);
                indexf->header.indexp = SafeStrCopy(indexf->header.indexp, StringValue, &indexf->header.lenindexp);
                efree(StringValue);
            }
            else
                progerr("%s: requires a value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "IndexAdmin") == 0)
        {
            if (sl->n > 1)
            {
                StringValue = StringListToString(sl, 1);
                indexf->header.indexa = SafeStrCopy(indexf->header.indexa, StringValue, &indexf->header.lenindexa);
                efree(StringValue);
            }
            else
                progerr("%s: requires one value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "UseStemming") == 0)
        {
            indexf->header.applyStemmingRules = getYesNoOrAbort(sl, 1, 1);
            continue;
        }

        
        if (strcasecmp(w0, "IgnoreTotalWordCountWhenRanking") == 0)
        {
            indexf->header.ignoreTotalWordCountWhenRanking = getYesNoOrAbort(sl, 1, 1);
            continue;
        }

        
        if (strcasecmp(w0, "UseSoundex") == 0)
        {
            indexf->header.applySoundexRules = getYesNoOrAbort(sl, 1, 1);
            continue;
        }


        if (strcasecmp(w0, "TranslateCharacters") == 0)
        {
            if (sl->n >= 2)
            {
                if (!BuildTranslateChars(indexf->header.translatecharslookuptable, sl->word[1], sl->word[2]))
                {
                    progerr("%s: requires two values (same length) or one translation rule", w0);
                }
            }
            continue;
        }


        if (strcasecmp(w0, "ExtractPath") == 0)
        {
            struct metaEntry *m;
            char **words;

            if (sl->n < 4)
                progerr("%s: requires at least three values: metaname expression type and a expression/strings", w0);

            if ( !( m = getMetaNameByName( &indexf->header, sl->word[1])) )
                progerr("%s - name '%s' is not a MetaName", w0, sl->word[1] );

            words = sl->word;
            words++;  /* past metaname */
            add_ExtractPath( w0, sw, m, words );

            continue;
        }

                


        if (strcasecmp(w0, "MetaNames") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires at least one value", w0);

            for (i = 1; i < sl->n; i++)
            {
                if ( getMetaNameByName( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a MetaName", w0, sl->word[i] );

                addMetaEntry(&indexf->header, sl->word[i], META_INDEX, 0);
            }

            continue;
        }


        if (strcasecmp(w0, "MetaNameAlias") == 0)
        {
            struct metaEntry *meta_entry;
            struct metaEntry *new_meta;
            
            if (sl->n < 3)
                progerr("%s: requires at least two values", w0);


            /* Make sure first entry is not an alias */
            /* Lookup entry, and do not follow alias */
            if ( !(meta_entry = getMetaNameByNameNoAlias( &indexf->header, sl->word[1]) ) )
                progerr("%s - name '%s' not a MetaName", w0, sl->word[1] );


            if ( meta_entry->alias )                
                progerr("%s - name '%s' must not be an alias", w0, sl->word[1] );

                
            for (i = 2; i < sl->n; i++)
            {
                if ( getMetaNameByNameNoAlias( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a MetaName or MetaNameAlias", w0, sl->word[i] );
                    
                new_meta = addMetaEntry(&indexf->header, sl->word[i], meta_entry->metaType, 0);
                new_meta->alias = meta_entry->metaID;
            }

            continue;
        }


        /* Meta name to extract out <a href> links */
        if (strcasecmp(w0, "HTMLLinksMetaName") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires one value", w0);

            if ( !( sw->links_meta = getMetaNameByName( &indexf->header, sl->word[1]) ))
                sw->links_meta = addMetaEntry(&indexf->header, sl->word[1], META_INDEX, 0);

            continue;
        }

        
        /* Meta name to extract out <img src> links */
        if (strcasecmp(w0, "ImageLinksMetaName") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires one value", w0);

            if ( !( sw->images_meta = getMetaNameByName( &indexf->header, sl->word[1]) ))
                sw->images_meta = addMetaEntry(&indexf->header, sl->word[1], META_INDEX, 0);

            continue;
        }

        if (strcasecmp(w0, "PropCompressionLevel") == 0)
        {

#ifdef HAVE_ZLIB
            if (sl->n == 2)
            {
                sw->PropCompressionLevel = atoi(sl->word[1]);
                if ( sw->PropCompressionLevel < 0 || sw->PropCompressionLevel > 9 )
                    progerr("%s: requires a number from 0 to 9, with 0=no compression 9=max", w0 );
            }
            else
                progerr("%s: requires one value", w0);
#else
            progwarn("%s: Swish not built with zlib support -- cannot compress", w0);
#endif
            continue;
        }
        

        if (strcasecmp(w0, "PropertyNames") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires at least one value", w0);

            for (i = 1; i < sl->n; i++)
            {
                if ( getPropNameByName( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a PropertyName", w0, sl->word[i] );

                addMetaEntry(&indexf->header, sl->word[i], META_PROP, 0);
            }

            continue;
        }

        if (strcasecmp(w0, "PropertyNamesNumeric") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires at least one value", w0);

            for (i = 1; i < sl->n; i++)
            {
                if ( getPropNameByName( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a PropertyName", w0, sl->word[i] );

                addMetaEntry(&indexf->header, sl->word[i], META_PROP|META_NUMBER, 0);
            }

            continue;
        }
        if (strcasecmp(w0, "PropertyNamesDate") == 0)
        {
            if (sl->n <= 1)
                progerr("%s: requires at least one value", w0);

            for (i = 1; i < sl->n; i++)
            {
                if ( getPropNameByName( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a PropertyName", w0, sl->word[i] );

                addMetaEntry(&indexf->header, sl->word[i], META_PROP|META_DATE, 0);
            }

            continue;
        }
        

        if (strcasecmp(w0, "PropertyNameAlias") == 0)
        {
            struct metaEntry *meta_entry;
            struct metaEntry *new_meta;
            
            if (sl->n < 3)
                progerr("%s: requires at least two values", w0);


            /* Make sure first entry is not an alias */
            /* Lookup entry, and do not follow alias */
            if ( !(meta_entry = getPropNameByNameNoAlias( &indexf->header, sl->word[1]) ) )
                progerr("%s - name '%s' not a PropertyName", w0, sl->word[1] );


            if ( meta_entry->alias )                
                progerr("%s - name '%s' must not be an alias", w0, sl->word[1] );

                
            for (i = 2; i < sl->n; i++)
            {
                if ( getPropNameByNameNoAlias( &indexf->header, sl->word[i]) )
                    progerr("%s - name '%s' is already a PropertyName or PropertyNameAlias", w0, sl->word[i] );
                    
                new_meta = addMetaEntry(&indexf->header, sl->word[i], meta_entry->metaType, 0);
                new_meta->alias = meta_entry->metaID;
            }

            continue;
        }




        if (strcasecmp(w0, "IgnoreWords") == 0)
        {
            if (sl->n > 1)
            {
                if (lstrstr(sl->word[1], "SwishDefault"))
                {
                    progwarn("SwishDefault is obsolete. See the CHANGES file.");
                }
                else if (lstrstr(sl->word[1], "File:"))
                {               /* 2000-06-15 rasc */
                    if (sl->n == 3)
                        readstopwordsfile(sw, indexf, sl->word[2]);
                    else
                        progerr("IgnoreWords File: requires path");
                }
                else
                    for (i = 1; i < sl->n; i++)
                    {
                        addstophash(&indexf->header, sl->word[i]);
                    }
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }


        if (strcasecmp(w0, "BuzzWords") == 0)  /* 2001-04-24 moseley */
        {
            if (sl->n > 1)
            {
                if (lstrstr(sl->word[1], "File:"))
                {
                    if (sl->n == 3)
                        readbuzzwordsfile(sw, indexf, sl->word[2]);
                    else
                        progerr("BuzzWords File: requires path");
                }
                else
                    for (i = 1; i < sl->n; i++)
                    {
                        addbuzzwordhash(&indexf->header, sl->word[i]);
                    }
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }
        

        if (strcasecmp(w0, "UseWords") == 0)
        {                       /* 11/00 Jmruiz */
            indexf->header.is_use_words_flag = 1;
            if (sl->n > 1)
            {
                if (lstrstr(sl->word[1], "File:"))
                {               /* 2000-06-15 rasc */
                    if (sl->n == 3)
                        readusewordsfile(sw, indexf, sl->word[2]);
                    else
                        progerr("UseWords File: requires path");
                }
                else
                    for (i = 1; i < sl->n; i++)
                    {
                        addusehash(&indexf->header, sl->word[i]);
                    }
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }

        
        /* IndexVerbose is supported for backwards compatibility */
        if (strcasecmp(w0, "IndexVerbose") == 0)
        {
            sw->verbose = getYesNoOrAbort(sl, 1, 1);
            if (sw->verbose)
                sw->verbose = 3;

            continue;
        }

        
        if (strcasecmp(w0, "IndexOnly") == 0)
        {
            if (sl->n > 1)
            {
                grabCmdOptions(sl, 1, &sw->suffixlist);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;    
        }

        
        if (strcasecmp(w0, "IndexContents") == 0)
        {
            if (sl->n > 2)
            {
                ic = (struct IndexContents *) emalloc(sizeof(struct IndexContents));

                ic->DocType = getDocTypeOrAbort(sl, 1);
                ic->patt = NULL;

                for (i = 2; i < sl->n; i++)
                    ic->patt = addswline(ic->patt, sl->word[i]);

                if (sw->indexcontents)
                    ic->next = sw->indexcontents;
                else
                    ic->next = NULL;

                sw->indexcontents = ic;
            }
            else
                progerr("%s: requires at least two values", w0);

            continue;
        }


        /* $$$ this needs fixing */
        if (strcasecmp(w0, "StoreDescription") == 0)
        {
            if (sl->n == 3 || sl->n == 4)
            {
                sd = (struct StoreDescription *) emalloc(sizeof(struct StoreDescription));

                sd->DocType = getDocTypeOrAbort(sl, 1);
                sd->size = 0;
                sd->field = NULL;
                i = 2;

                if (sl->word[i][0] == '<' && sl->word[i][strlen(sl->word[i]) - 1] == '>')
                {
                    sl->word[i][strlen(sl->word[i]) - 1] = '\0';
                    sd->field = estrdup(sl->word[i] + 1);
                    i++;
                }

                if (i < sl->n && isnumstring(sl->word[i]))
                {
                    sd->size = atoi(sl->word[i]);
                }
                if (sl->n == 3 && !sd->field && !sd->size)
                    progerr("%s: second parameter must be <fieldname> or a number", w0);
                if (sl->n == 4 && sd->field && !sd->size)
                    progerr("%s: third parameter must be empty or a number", w0);
                if (sw->storedescription)
                    sd->next = sw->storedescription;
                else
                    sd->next = NULL;

                sw->storedescription = sd;

                /* Make sure there's a property name */
                if ( !getPropNameByName( &indexf->header, AUTOPROPERTY_SUMMARY) )
                    addMetaEntry(&indexf->header, AUTOPROPERTY_SUMMARY, META_PROP, 0);
            }
            else
                progerr("%s: requires two or three values", w0);

            continue;
        }

        
        if (strcasecmp(w0, "DefaultContents") == 0)
        {
            if (sl->n > 1)
            {
                sw->DefaultDocType = getDocTypeOrAbort(sl, 1);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }

        
        if (strcasecmp(w0, "BumpPositionCounterCharacters") == 0)
        {
            if (sl->n > 1)
            {
                indexf->header.bumpposchars = SafeStrCopy(indexf->header.bumpposchars, sl->word[1], &indexf->header.lenbumpposchars);
                sortstring(indexf->header.bumpposchars);
                makelookuptable(indexf->header.bumpposchars, indexf->header.bumpposcharslookuptable);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }


        /* #### Added UndefinedMetaTags as defined by Bill Moseley */
        if (strcasecmp(w0, "UndefinedMetaTags") == 0)
        {
            get_undefined_meta_flags( w0, sl, &sw->UndefinedMetaTags );
            if ( !sw->UndefinedMetaTags )
                progerr("%s: possible values are error, ignore, index or auto", w0);

            continue;
        }


        if (strcasecmp(w0, "UndefinedXMLAttributes") == 0)
        {
            get_undefined_meta_flags( w0, sl, &sw->UndefinedXMLAttributes );
            continue;
        }


        
        if (strcasecmp(w0, "IgnoreMetaTags") == 0)
        {
            if (sl->n > 1)
            {
                grabCmdOptions(sl, 1, &sw->ignoremetalist);
                /* Go lowercase */
                for (tmplist = sw->ignoremetalist; tmplist; tmplist = tmplist->next)
                    tmplist->line = strtolower(tmplist->line);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }


        if (strcasecmp(w0, "XMLClassAttributes") == 0)
        {
            if (sl->n > 1)
            {
                grabCmdOptions(sl, 1, &sw->XMLClassAttributes);
                /* Go lowercase */
                for (tmplist = sw->XMLClassAttributes; tmplist; tmplist = tmplist->next)
                    tmplist->line = strtolower(tmplist->line);
            }
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }


        if (strcasecmp(w0, "DontBumpPositionOnStartTags") == 0)
        {
            if (sl->n > 1)
                grabCmdOptions(sl, 1, &sw->dontbumpstarttagslist);
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }

        if (strcasecmp(w0, "DontBumpPositionOnEndTags") == 0)
        {
            if (sl->n > 1)
                grabCmdOptions(sl, 1, &sw->dontbumpendtagslist);
            else
                progerr("%s: requires at least one value", w0);

            continue;
        }
        
        if (strcasecmp(w0, "TruncateDocSize") == 0)
        {                       /* rasc 2001-03 */
            if (sl->n == 2 && isnumstring(sl->word[1]))
                sw->truncateDocSize = atol(sl->word[1]);
            else
                progerr("%s: requires size parameter in bytes", w0);

            continue;
        }


        else if (configModule_Entities(sw, sl));
        else if (configModule_Filter(sw, sl)); /* rasc */
        else if (configModule_ResultOutput(sw, sl)); /* rasc */
        else if (configModule_SearchAlt(sw, sl)); /* rasc */
		/* Removed , patents ...
        else if (configModule_Deflate(sw, sl));*/ /* jmruiz */
        else if (configModule_ResultSort(sw, sl)); /* jmruiz */
        else if (configModule_DB(sw, sl)); /* jmruiz */
        else if (configModule_Search(sw, sl)); /* jmruiz */
        else if (configModule_Index(sw, sl)); /* jmruiz */
        else if (configModule_Prog(sw, sl));
        else if (!parseconfline(sw, sl))
        {
            printf("Bad directive on line #%d of file %s: %s\n", linenumber, conffile, line);
            if ( ++baddirective > 30 )
                progerr("Too many errors.  Can not continue.");
        }

    }

    freeStringList(sl);

    fclose(fp);

    if (baddirective)
        exit(1);
    if (gotdir && !(*hasdir))
        *hasdir = 1;
    if (gotindex && !(*hasindex))
        *hasindex = 1;
}



/*
  -- some config helper routines
*/

/*
  --  check if word "n" in StringList  is yes/no
  --  "lastparam": 0/1 = is param last one for config directive?
  --  returns 1 (yes) or 0 (no)
  --  aborts if not "yes" or "no" (and prints first word of array)
  --  aborts if lastparam set and is not last param...
  --  2001-03-04 rasc
*/

int     getYesNoOrAbort(StringList * sl, int n, int lastparam)
{
    if (lastparam && n < (sl->n - 1))
    {
        progerr("%s has too many paramter", sl->word[0], n);
        return 0;
    }

    if (n < sl->n)
    {
        if (!strcasecmp(sl->word[n], "yes") || !strcasecmp(sl->word[n], "on") || !strcasecmp(sl->word[n], "1") )
            return 1;

        if (!strcasecmp(sl->word[n], "no") || !strcasecmp(sl->word[n], "off") ||!strcasecmp(sl->word[n], "0"))
            return 0;
    }
    progerr("%s requires parameter #%d of yes|on|1 or no|off|0", sl->word[0], n);
    return 0;
}


static  void add_ExtractPath( char *name, SWISH *sw, struct metaEntry *m, char **params )
{
    path_extract_list *list = sw->pathExtractList;
    path_extract_list *last = NULL;

    
    while ( list && list->meta_entry != m  )
    {
        last = list;
        list = list->next;
    }


    /* need to create a meta entry */
    if ( !list )
    {
        list = emalloc( sizeof( path_extract_list ));
        if ( last )
            last->next = list;
        else
            sw->pathExtractList = list;

        list->meta_entry = m;
        list->regex = NULL;
        list->next  = NULL;
    }

    /* now add regular expression to list */
    Build_ReplaceRules( name, params, &list->regex ); /* compile and add to list of expression */
}


/********************************************************
*  Free a ExtractPath list
*
*********************************************************/
void free_Extracted_Path( SWISH *sw )
{
    path_extract_list *list = sw->pathExtractList;
    path_extract_list *next;

    while ( list )
    {
        next = list->next;
        free_regex_list( &list->regex );
        efree( list );
        list = next;
    }

    sw->pathExtractList = NULL;
}

/*********************************************************************
*  Builds regex substitution strings of the FileRules type
*  But also includex ExtractPath
*
*********************************************************************/

static void Build_ReplaceRules( char *name, char **params, regex_list **reg_list )
{
    char *pattern = NULL;
    char *replace = NULL;
    int   cflags = REG_EXTENDED;
    int   global = 0;
    char *ptr;

    params++;

    /* these two could be optimized, of course */
    
    if ( strcasecmp( params[0], "append") == 0 )
    {
        pattern = estrdup("$");
        replace = estrdup( params[1] );
    }

    else if  ( strcasecmp( params[0], "prepend") == 0 )
    {
        pattern = estrdup("^");
        replace = estrdup(params[1]);
    }

       
    else if  ( strcasecmp( params[0], "remove") == 0 )
    {
        pattern = estrdup(params[1]);
        replace = estrdup( "" );
        global++;
    }
        

    else if  ( strcasecmp( params[0], "replace") == 0 )
    {
        pattern = estrdup(params[1]);
        replace = estrdup(params[2]);
        global++;
    }


    else if  ( strcasecmp( params[0], "regex") == 0 )
    {
        char    *word = params[1];
        int     delimiter = (int)*word;
        char    *pos;

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
                progerr("%s regexp %s: unknown flag '%c'", name, params[1], *word );
        }
        
    }

    else
        progerr("%s: unknown argument '%s'.  Must be prepend|append|remove|replace|regex.", name, params[0] );


    add_regular_expression( reg_list, pattern, replace, cflags, global, 0 );

    efree( pattern );
    efree( replace );
}




/*
  --  check if word "n" in StringList  is a DocumentType
  --  returns (doctype-id)
  --  aborts if not a DocumentType, or no param
  --  2001-03-04 rasc
*/


int	strtoDocType( char * s )
{
    static struct
    {
        char    *type;
        int      id;
    }
    doc_map[] =
    {
        {"TXT", TXT},
        {"HTML", HTML},
#ifdef HAVE_LIBXML2        
        {"XML2", XML2 },
        {"HTML2", HTML2 },
        {"TXT2", TXT2 },
#endif
        {"XML", XML},
        {"WML", WML}
    };
    int i;

    for (i = 0; i < sizeof(doc_map) / sizeof(doc_map[0]); i++)
        if ( strcasecmp(doc_map[i].type, s) == 0 )
		    return doc_map[i].id;

    return 0;
}

static int     getDocTypeOrAbort(StringList * sl, int n)
{
    int doctype;

    if (n < sl->n)
    {
        doctype = strtoDocType( sl->word[n] );

        if (!doctype )
            progerr("%s: Unknown document type \"%s\"", sl->word[0], sl->word[n]);
        else
            return doctype;
    }

    progerr("%s: missing %d. parameter", sl->word[0], n);
    return 0;                   /* never happens */
}


/*
  -- helper routine for misc. indexing methods
  -- (called via "jump" function array)
 02/2001 Rewritten Jmruiz
 */

void    grabCmdOptions(StringList * sl, int start, struct swline **listOfWords)
{
    int     i;

    for (i = start; i < sl->n; i++)
        *listOfWords = (struct swline *) addswline(*listOfWords, sl->word[i]);
    return;
}



/* --------------------------------------------------------- */




/*
  read stop words from file
  lines beginning with # are comments
  2000-06-15 rasc

*/

static void    readstopwordsfile(SWISH * sw, IndexFILE * indexf, char *stopw_file)
{
    char    line[MAXSTRLEN];
    FILE   *fp;
    StringList *sl;
    int     i;


    if ((fp = fopen(stopw_file, FILEMODE_READ)) == NULL || !isfile(stopw_file))
    {
        progerr("Couldn't open the stopword file \"%s\".", stopw_file);
    }


    /* read all lines and store each word as stopword */

    while (fgets(line, MAXSTRLEN, fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        sl = parse_line(line);
        if (sl && sl->n)
        {
            for (i = 0; i < sl->n; i++)
            {
                addstophash(&indexf->header, sl->word[i]);
            }
            freeStringList(sl);
        }
    }

    fclose(fp);
    return;
}

/* Read in buzzwords from file -- based on Rainer's readstopwordsfile, of course. */
/* Might be nice to combine all these routines that do the same thing */


static void    readbuzzwordsfile(SWISH * sw, IndexFILE * indexf, char *stopw_file)
{
    char    line[MAXSTRLEN];
    FILE   *fp;
    StringList *sl;
    int     i;


    if ((fp = fopen(stopw_file, FILEMODE_READ)) == NULL || !isfile(stopw_file))
    {
        progerr("Couldn't open the buzzword file \"%s\".", stopw_file);
    }


    /* read all lines and store each word as stopword */

    while (fgets(line, MAXSTRLEN, fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        sl = parse_line(line);
        if (sl && sl->n)
        {
            for (i = 0; i < sl->n; i++)
            {
                addbuzzwordhash(&indexf->header, sl->word[i]);
            }
            freeStringList(sl);
        }
    }

    fclose(fp);
    return;
}


static int     parseconfline(SWISH * sw, StringList * sl)
{
    /* invoke routine to parse config file lines */
    return (*IndexingDataSource->parseconfline_fn) (sw, (void *) sl);
}


/*
  read "use" words from file
  lines beginning with # are comments
  2000-11 jmruiz
  
  Based on readstopwordsfile (from rasc) 

*/

static void    readusewordsfile(SWISH * sw, IndexFILE * indexf, char *usew_file)
{
    char    line[MAXSTRLEN];
    FILE   *fp;
    StringList *sl;
    int     i;


    if ((fp = fopen(usew_file, FILEMODE_READ)) == NULL || !isfile(usew_file))
    {
        progerr("Couldn't open the useword file \"%s\".", usew_file);
    }


    /* read all lines and store each word as useword */

    while (fgets(line, MAXSTRLEN, fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        sl = parse_line(line);
        if (sl && sl->n)
        {
            for (i = 0; i < sl->n; i++)
            {
                addusehash(&indexf->header, sl->word[i]);
            }
            freeStringList(sl);
        }
    }

    fclose(fp);
    return;
}

static void get_undefined_meta_flags( char *w0, StringList * sl, UndefMetaFlag *setting )
{
    if (sl->n != 2)
        progerr("%s: requires one value", w0);
        
    if (strcasecmp(sl->word[1], "error") == 0)
        *setting = UNDEF_META_ERROR;
        
    else if (strcasecmp(sl->word[1], "ignore") == 0)
        *setting = UNDEF_META_IGNORE;

    else if (strcasecmp(sl->word[1], "disable") == 0)  // default for xml attributes
        *setting = UNDEF_META_DISABLE;

    else if (strcasecmp(sl->word[1], "auto") == 0)
        *setting = UNDEF_META_AUTO;

    else if (strcasecmp(sl->word[1], "index") == 0)
        *setting = UNDEF_META_INDEX;
    else
        progerr("%s: possible values are error, ignore, index or auto", w0);
}

