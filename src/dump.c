/*
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
**
** 2001-05-07 jmruiz init coding
**
*/

#include "swish.h"
#include "merge.h"
#include "docprop.h"
#include "hash.h"
#include "string.h"
#include "mem.h"
#include "db.h"
#include "compress.h"
#include "index.h"
#include "search.h"
#include "result_output.h"


/* Prints out the data in an index DB */

void    DB_decompress(SWISH * sw, IndexFILE * indexf)
{
    int     i,
            j,
            c,
            x,
            fieldnum,
            frequency,
            metaname,
            index_structure,
            structure,
            index_structfreq,
            filenum;
    long    nextposmetaname;
    struct  file *fi = NULL;
    struct  docPropertyEntry *docProperties = NULL;
    char    ISOTime[20];
    char    word[2];
    char   *resultword;
    unsigned char   *worddata, *s;
    int     sz_worddata;
    long    wordID;
    
    indexf->DB = DB_Open(sw, indexf->line);

    metaname = 0;

    nextposmetaname = 0L;

    c = 0;

    frequency = 0;

        /* Read header */
    read_header(sw, &indexf->header, indexf->DB);
    
        /* Allocate size for fileinfo */
    indexf->filearray_cursize = indexf->header.totalfiles;
    indexf->filearray_maxsize = indexf->header.totalfiles;
    indexf->filearray = emalloc(indexf->header.totalfiles * sizeof(struct file *));
    for(i = 0; i < indexf->header.totalfiles; i++)
        indexf->filearray[i] = NULL;

    resultPrintHeader(sw, 0, &indexf->header, indexf->line, 0);

    fieldnum = 0;

    printf("\n-----> WORD INFO <-----\n");

    DB_InitReadWords(sw, indexf->DB);

    for(j=0;j<256;j++)
    {
        word[0] = (unsigned char) j; word[1] = '\0';
        DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

        while(wordID)
        {
            printf("%s:",resultword);

                /* Read Word's data */
            DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, indexf->DB);

                /* parse and print word's data */
            s = worddata;

            x = uncompress2(&s);     /* tfrequency */
            x = uncompress2(&s);     /* metaname */
            metaname = x;
            if (metaname)
            {
                UNPACKLONG2(nextposmetaname,s); s += sizeof(long);
                x = uncompress2(&s); /* First file */
            }
            while (x)
            {
                filenum = x;
                index_structfreq = uncompress2(&s);
                frequency = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[0];
                index_structure = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[1];
                structure = indexf->header.structurelookup->all_entries[index_structure - 1]->val[0];

                if (sw->verbose >= 4)
                {
                    struct file *fileInfo;

                    printf(" Meta:%d", metaname);
                    fileInfo = readFileEntry(sw, indexf, filenum);
                    printf(" %s", fileInfo->fi.filename);
                    printf(" Strct:%x", structure);
                    printf(" Freq:%d", frequency);
                    printf(" Pos:");
                }
                else
                {
                    printf(" %d", metaname);
                    printf(" %d", filenum);
                    printf(" %d", structure);
                    printf(" %d", frequency);
                }
                for (i = 0; i < frequency; i++)
                {
                    x = uncompress2(&s);
                    if (sw->verbose >= 4)
                    {
                        if (i)
                            printf(",%d", x);
                        else
                            printf("%d", x);
                    }
                    else
                        printf(" %d", x);
                }
                if ((s - worddata) == nextposmetaname)
                {
                    x = uncompress2(&s);
                    metaname = x;
                    if (metaname)
                    {
                        UNPACKLONG2(nextposmetaname,s); 
                        s += sizeof(long);
                        x = uncompress2(&s);
                    }
                    else
                        nextposmetaname = 0L;
                }
                else
                    x = uncompress2(&s);
            }
            putchar((int) '\n');

            efree(worddata);
            efree(resultword);
            DB_ReadNextWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);
        }
    }
    DB_EndReadWords(sw, indexf->DB);

    /* Decode Stop Words: All them are in just one line */
    printf("\n\n-----> STOP WORDS <-----\n");
    for(i=0;i<indexf->header.stopPos;i++)
        printf("%s ",indexf->header.stopList[i]);
    putchar((int) '\n');

    /* Decode File Info */
    printf("\n\n-----> FILES <-----\n");
    fflush(stdout);
    for (i = 0; i < indexf->header.totalfiles; i++)
    {
        fi = readFileEntry(sw, indexf, i + 1);

        strftime(ISOTime, sizeof(ISOTime), "%Y/%m/%d %H:%M:%S", (struct tm *) localtime((time_t *) & fi->fi.mtime));

        fflush(stdout);
        if (fi->fi.summary)
        {
            printf("%s \"%s\" \"%s\" \"%s\" %d %d", fi->fi.filename, ISOTime, fi->fi.title, fi->fi.summary, fi->fi.start, fi->fi.size);
            fflush(stdout);     /* filename */
        }
        else
        {
            printf("%s \"%s\" \"%s\" \"\" %d %d", fi->fi.filename, ISOTime, fi->fi.title, fi->fi.start, fi->fi.size);
            fflush(stdout);     /* filename */
        }
        for (docProperties = fi->docProperties; docProperties; docProperties = docProperties->next)
        {
            printf(" PROP_%d: \"%s\"", docProperties->metaID, getDocPropAsString(indexf, fi, docProperties->metaID));
        }
        putchar((int) '\n');
        fflush(stdout);
        freefileinfo(fi);
    }
    printf("\nNumber of File Entries: %d\n", indexf->header.totalfiles);
    fflush(stdout);

    printf("\n\n-----> METANAMES <-----\n");
    for(i = 0; i < indexf->header.metaCounter; i++)
    {
        printf("%s\"%d\"%d ",indexf->header.metaEntryArray[i]->metaName,indexf->header.metaEntryArray[i]->metaID,indexf->header.metaEntryArray[i]->metaType);
    }
    putchar((int) '\n');

    DB_Close(sw, indexf->DB);

    if (sw->verbose != 4)
        printf("\nUse -v 4 for a more complete info\n");
}
