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

void dump_memory_file_list( SWISH *sw, IndexFILE *indexf ) 
{
    int     i;
    struct  file *fi = NULL;
   
    printf("\n\n-----> FILES in index %s <-----\n", indexf->line );
    for (i = 0; i < indexf->filearray_cursize; i++)
    {
        fi = indexf->filearray[ i ];

        fflush(stdout);
        printf("%d: %s\n", i+1, fi->fi.filename);


        dump_file_properties( indexf, fi );
        
        printf("\n");
    }
    printf("\nNumber of File Entries: %d\n", indexf->header.totalfiles);
    fflush(stdout);
}



void dump_index_file_list( SWISH *sw, IndexFILE *indexf ) 
{
    int     i;
    struct  file *fi = NULL;
   
    printf("\n\n-----> FILES in index %s <-----\n", indexf->line );
    for (i = 0; i < indexf->header.totalfiles; i++)
    {
        fi = readFileEntry(sw, indexf, i + 1);

        fflush(stdout);
        printf("%d: %s\n", i+1, fi->fi.filename);


        dump_file_properties( indexf, fi );
        
        printf("\n");
        freefileinfo(fi);
    }
    printf("\nNumber of File Entries: %d\n", indexf->header.totalfiles);
    fflush(stdout);
}


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
    unsigned long    nextposmetaname;
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

    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_HEADER) )
        resultPrintHeader(sw, 0, &indexf->header, indexf->line, 0);

    fieldnum = 0;


    /* Do metanames first as that will be helpful for decoding next */
    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_METANAMES)  )
    {
        printf("\n\n-----> METANAMES <-----\n");
        for(i = 0; i < indexf->header.metaCounter; i++)
        {
            printf("%s id:%d type:%d\n",indexf->header.metaEntryArray[i]->metaName,indexf->header.metaEntryArray[i]->metaID,indexf->header.metaEntryArray[i]->metaType);
        }
        printf("\n");
    }
    


    if (DEBUG_MASK & DEBUG_INDEX_WORDS_ONLY)
    {
        DB_InitReadWords(sw, indexf->DB);

        for( j = 0; j < 256; j++ )
        {
            word[0] = (unsigned char) j;
            word[1] = '\0';
            DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

            while(wordID)
            {
                printf("%s\n",resultword);

                
                efree(resultword);
                DB_ReadNextWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

            }
        }
        DB_EndReadWords(sw, indexf->DB);
    }


    else if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_WORDS | DEBUG_INDEX_WORDS_FULL | DEBUG_INDEX_WORDS_META)  )
    {
        int     *meta_used;
        int     end_meta = 0;

        printf("\n-----> WORD INFO <-----\n");

        for(i = 0; i < indexf->header.metaCounter; i++)
            if ( indexf->header.metaEntryArray[i]->metaID > end_meta )
                end_meta = indexf->header.metaEntryArray[i]->metaID;

        meta_used = emalloc( sizeof(int) * ( end_meta + 1) );  
    
        /* _META only reports which tags the words are found in */
        for(i = 0; i <= end_meta; i++)
            meta_used[i] = 0;


        DB_InitReadWords(sw, indexf->DB);

        for(j=0;j<256;j++)
        {
            word[0] = (unsigned char) j; word[1] = '\0';
            DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

            while(wordID)
            {
                printf("\n%s",resultword);


                /* Read Word's data */
                DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, indexf->DB);

                /* parse and print word's data */
                s = worddata;

                x = uncompress2(&s);     /* tfrequency */
                x = uncompress2(&s);     /* metaname */
                metaname = x;
                if (metaname)
                {
                    nextposmetaname = UNPACKLONG2(s); s += sizeof(long);
                    x = uncompress2(&s); /* First file */
                }
                while (x)
                {
                    filenum = x;
                    index_structfreq = uncompress2(&s);
                    frequency = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[0];
                    index_structure = indexf->header.structfreqlookup->all_entries[index_structfreq - 1]->val[1];
                    structure = indexf->header.structurelookup->all_entries[index_structure - 1]->val[0];

                    // if (sw->verbose >= 4)
                    if (DEBUG_MASK & (DEBUG_INDEX_ALL|DEBUG_INDEX_WORDS_FULL))
                    {
                        struct file *fileInfo;

                        printf("\n Meta:%d", metaname);
                        fileInfo = readFileEntry(sw, indexf, filenum);
                        printf(" %s", fileInfo->fi.filename);
                        printf(" Struct:%x", structure);
                        printf(" Freq:%d", frequency);
                        printf(" Pos:");
                    }
                    else if ( DEBUG_MASK & DEBUG_INDEX_WORDS_META)
                        meta_used[ metaname ]++;
                    else
                    {
                        printf(" [%d", metaname);
                        printf(" %d", filenum);
                        printf(" %x", structure);
                        printf(" %d (", frequency);
                    }
                    

                    for (i = 0; i < frequency; i++)
                    {
                        x = uncompress2(&s);

                        if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_WORDS_FULL))
                        //if (sw->verbose >= 4)
                        {
                            if (i)
                                printf(",%d", x);
                            else
                                printf("%d", x);
                        }
                        else if ( DEBUG_MASK & DEBUG_INDEX_WORDS)
						{
							if (i)
								printf(" %d", x);
							else
								printf("%d", x);
						}
                    }

                    if ( DEBUG_MASK & DEBUG_INDEX_WORDS )
						printf(")]");


                    if ((unsigned long)(s - worddata) == nextposmetaname)
                    {
                        x = uncompress2(&s);
                        metaname = x;
                        if (metaname)
                        {
                            nextposmetaname = UNPACKLONG2(s); 
                            s += sizeof(long);
                            x = uncompress2(&s);
                        }
                        else
                            nextposmetaname = 0L;
                    }
                    else
                        x = uncompress2(&s);
                }

                if ( DEBUG_MASK & DEBUG_INDEX_WORDS_META)
                {
                    for(i = 0; i <= end_meta; i++)
                    {
                        if ( meta_used[i] )
                            printf( "\t%d", i );
                        meta_used[i] = 0;
                    }
                }
                

                if ( !( DEBUG_MASK & DEBUG_INDEX_WORDS_META ))
                    printf("\n");

                efree(worddata);
                efree(resultword);
                DB_ReadNextWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);
            }
        }
        DB_EndReadWords(sw, indexf->DB);

        efree( meta_used );
    }



    /* Decode Stop Words: All them are in just one line */
    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_STOPWORDS)  )
    {
        printf("\n\n-----> STOP WORDS <-----\n");
        for(i=0;i<indexf->header.stopPos;i++)
            printf("%s ",indexf->header.stopList[i]);
        printf("\n");
    }



    /* Decode File Info */
    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_FILES)  )
        dump_index_file_list( sw, indexf );


    DB_Close(sw, indexf->DB);

}
