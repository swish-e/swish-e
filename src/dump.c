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
#include "mem.h"
#include "merge.h"
#include "search.h"
#include "docprop.h"
#include "hash.h"
#include "swstring.h"
#include "db.h"
#include "compress.h"
#include "index.h"
#include "search.h"
#include "result_output.h"
#include "metanames.h"
#include "dump.h"
#include "headers.h"
#include "error.h"


void dump_index_file_list( SWISH *sw, IndexFILE *indexf, int begin, int maxhits ) 
{
    int     i, filenum, totalfiles;
    int     end;

    i = begin ? begin - 1 : 0;

    /* We must keep in mind that some files can be marked as deleted */
    totalfiles = indexf->header.totalfiles - indexf->header.removedfiles;

    if ( i >= totalfiles )
    {
        printf("Hey, there are only %d files\n", totalfiles );
        exit(-1);
    }

    end = totalfiles;

    if ( maxhits > 0 )
    {
        end = i + maxhits;
        if ( end > totalfiles)
            end = totalfiles;
    }

   
    printf("\n\n-----> FILES in index %s <-----\n", indexf->line );

    for (filenum = i + 1; i < end; i++)
    {
        FileRec fi;

        memset( &fi, 0, sizeof( FileRec ) );

        /* 2004/09 jmruiz. Need to check for files marked as deleted */
        if (indexf->header.removedfiles) 
        {
            while(!DB_CheckFileNum(sw,filenum,indexf->DB))
                filenum++;
        }

        fi.filenum = filenum;

        fflush(stdout);
        printf("Dumping File Properties for File Number: %d\n", filenum);


        dump_file_properties( indexf, &fi );
        printf("\n");


        printf("ReadAllDocProperties:\n");
        fi.docProperties =  ReadAllDocPropertiesFromDisk( indexf, i+1 );
        dump_file_properties( indexf, &fi );
	
	if (! indexf->header.ignoreTotalWordCountWhenRanking )
	{
		printf("Filenum and words in this file:");
		dump_words_per_file( sw, indexf, &fi );
	}
        freefileinfo( &fi );

        printf("\n");


        /* dump one at a time */
        {
            propEntry *p;
            int j;
            struct metaEntry *meta_entry;
            INDEXDATAHEADER *header = &indexf->header;
            int count = header->property_count;

            printf("ReadSingleDocPropertiesFromDisk:\n");

            for (j=0; j< count; j++) // just for testing
            {
                int metaID = header->propIDX_to_metaID[j];

                if ( !(p = ReadSingleDocPropertiesFromDisk(indexf, &fi, metaID, 0 )) )
                    continue;

                meta_entry = getPropNameByID( &indexf->header, metaID );
                dump_single_property( p, meta_entry );

                { // show compression
                    char    *buffer;
                    int     uncompressed_len;
                    int     buf_len;

                    if ( (buffer = DB_ReadProperty( sw, indexf, &fi, meta_entry->metaID, &buf_len, &uncompressed_len, indexf->DB )))
                    {
                        if ( uncompressed_len )
                            printf("  %20s: %d -> %d (%4.2f%%)\n", "**Compressed**", uncompressed_len , buf_len, (float)buf_len/(float)uncompressed_len * 100.00f );

                        efree(buffer);
                    }
                }
        

        
                freeProperty( p );
            }
        }
        printf("\n");


        freefileinfo(&fi);
        filenum++;
    }
    printf("\nNumber of File Entries: %d\n", totalfiles);
    fflush(stdout);
}


/* prints out the number of words in every file in the index */

void	dump_words_per_file( SWISH *sw, IndexFILE * indexf, FileRec *fi )
{
#ifndef USE_BTREE
INDEXDATAHEADER *header;
#endif

int words;
int filenum;

filenum = fi->filenum-1;

#ifdef USE_BTREE
    getTotalWordsPerFile(sw, indexf, fi->filenum-1, &words);
#else

/* this depends currently that IgnoreTotalWordCountWhenRanking is set to 0 
otherwise TotalWordsPerFile is not indexed in non-BTREE indexes */

	header = &indexf->header;

	if ( indexf->header.ignoreTotalWordCountWhenRanking ) {
		fprintf(stderr, "IgnoreTotalWordCountWhenRanking must be 0 to use IDF ranking\n");
		
		words = 0;
		
	} else {

		words = header->TotalWordsPerFile[filenum];
		
	}

#endif

	printf("%d %d\n", filenum, words );


}

void	dump_word_count( SWISH *sw, IndexFILE *indexf, int begin, int maxhits ) 
{
    int     i, filenum, totalfiles;
    int     end;

    totalfiles = indexf->header.totalfiles - indexf->header.removedfiles;
    i = begin ? begin - 1 : 0;

    if ( i >= totalfiles )
    {
        printf("Hey, there are only %d files\n", totalfiles );
        exit(-1);
    }

    end = totalfiles;

    if ( maxhits > 0 )
    {
        end = i + maxhits;
        if ( end > totalfiles )
            end = totalfiles;
    }

    for (filenum = i + 1 ; i < end; i++)
    {
        FileRec fi;

        memset( &fi, 0, sizeof( FileRec ) );
     
        /* 2004/09 jmruiz. Need to check for files marked as deleted */
        if (indexf->header.removedfiles)
        {
            while(!DB_CheckFileNum(sw,filenum,indexf->DB))
                filenum++;
        }

        fi.filenum = filenum;
  
	dump_words_per_file( sw, indexf, &fi );
	
        freefileinfo( &fi );
        filenum++;
    }

}

/* Prints out the data in an index DB */
void    DB_decompress(SWISH * sw, IndexFILE * indexf, int begin, int maxhits)
{
    int     i,
            j,
            c,
            fieldnum,
            frequency,
            metaID,
            tmpval,
            printedword,
            filenum;
    unsigned int       *posdata;
    int     metadata_length;
    char    word[2];
    char   *resultword;
    unsigned char   *worddata, *s, *start, flag;
    int     sz_worddata, saved_bytes;
    sw_off_t    wordID;


    
    indexf->DB = DB_Open(sw, indexf->line,DB_READ);
    if ( sw->lasterror )
        SwishAbortLastError( sw );

    metaID = 0;

    metadata_length = 0;

    c = 0;

    frequency = 0;

        /* Read header */
    read_header(sw, &indexf->header, indexf->DB);
    

    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_HEADER) )
    {
        sw->headerOutVerbose = 255;
        print_index_headers( indexf );
    }

    fieldnum = 0;


    /* Do metanames first as that will be helpful for decoding next */
    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_METANAMES)  )
        dump_metanames( sw, indexf, 1 );

    if (DEBUG_MASK & DEBUG_INDEX_WORDS_ONLY)
    {
        DB_InitReadWords(sw, indexf->DB);

        for( j = 0; j < 256; j++ )
        {
            word[0] = (unsigned char) j;
            word[1] = '\0';
            DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

            while(wordID && (((int)((unsigned char)resultword[0]))== j))
            {
              if(indexf->header.removedfiles)
              {
                /* We need to Read Word's data to check that there is 
                ** at least one file that has not been removed */
                DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, &saved_bytes, indexf->DB);
                uncompress_worddata(&worddata, &sz_worddata, saved_bytes);

                /* parse and print word's data */
                s = worddata;

                tmpval = uncompress2(&s);     /* tfrequency */
                metaID = uncompress2(&s);     /* metaID */
                metadata_length = uncompress2(&s);

                filenum = 0;
                start = s;
                while(1)
                {                   /* Read on all items */
                    uncompress_location_values(&s,&flag,&tmpval,&frequency);
                    filenum += tmpval;
                    posdata = (unsigned int *) emalloc(frequency * sizeof(int));
                    uncompress_location_positions(&s,flag,frequency,posdata);

                    /* 2004/09 jmruiz. Need to check for one file not being marked as deleted */
                    if (DB_CheckFileNum(sw,filenum,indexf->DB))
                    {
                        printf("%s\n",resultword);
                        break;
                    }
                    /* Check for end of worddata */
                    if ((s - worddata) == sz_worddata)
                        break;   /* End of worddata */

                    /* Check for end of current metaID data */
                    if ( metadata_length == (s - start))
                    {
                        filenum = 0;
                        metaID = uncompress2(&s);
                        metadata_length = uncompress2(&s);
                        start = s;
                    }

                }
                efree(posdata);
                efree(worddata);
              } 
              else
                printf("%s\n",resultword);
                
              efree(resultword);
              DB_ReadNextWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);
              if (wordID && ((int)((unsigned char)resultword[0]))!= j)
                efree(resultword);

            }
        }
        DB_EndReadWords(sw, indexf->DB);
    }


    else if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_WORDS | DEBUG_INDEX_WORDS_FULL | DEBUG_INDEX_WORDS_META)  )
    {
        int     *meta_used;
        int     end_meta = 0;

        printf("\n-----> WORD INFO in index %s <-----\n", indexf->line);

        for(i = 0; i < indexf->header.metaCounter; i++)
            if ( indexf->header.metaEntryArray[i]->metaID > end_meta )
                end_meta = indexf->header.metaEntryArray[i]->metaID;

        meta_used = emalloc( sizeof(int) * ( end_meta + 1) );  
 
        /* _META only reports which tags the words are found in */
        for(i = 0; i <= end_meta; i++)
            meta_used[i] = 0;


        DB_InitReadWords(sw, indexf->DB);

        for(j=1;j<256;j++)
        {
            word[0] = (unsigned char) j; word[1] = '\0';
            DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

            while(wordID && (((int)((unsigned char)resultword[0]))== j))
            {
                /* Flag to know if we must print a word or not */
                /* Words with all the files marked as deleted shoud not be
                ** printed */
                printedword = 0;  
                /* Read Word's data */
                DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, &saved_bytes, indexf->DB);
                uncompress_worddata(&worddata, &sz_worddata, saved_bytes);

                /* parse and print word's data */
                s = worddata;

                tmpval = uncompress2(&s);     /* tfrequency */
                metaID = uncompress2(&s);     /* metaID */
                metadata_length = uncompress2(&s);

                filenum = 0;
                start = s;
                while(1)
                {                   /* Read on all items */
                    uncompress_location_values(&s,&flag,&tmpval,&frequency);
                    filenum += tmpval;
                    posdata = (unsigned int *) emalloc(frequency * sizeof(int));
                    uncompress_location_positions(&s,flag,frequency,posdata);

                    /* 2004/09 jmruiz. Need to check for files marked as deleted */
                    if ((!indexf->header.removedfiles) || DB_CheckFileNum(sw,filenum,indexf->DB))
                    {
                    if(!printedword) 
                    {
                        printf("\n%s",resultword);
                        printedword = 1;
                    }
                           
                    // if (sw->verbose >= 4)
                    if (DEBUG_MASK & (DEBUG_INDEX_ALL|DEBUG_INDEX_WORDS_FULL))
                    {
                        struct metaEntry    *m;
                            
                        printf("\n Meta:%d", metaID);

                        
                        /* Get path from property list */
                        if ( (m = getPropNameByName( &sw->indexlist->header, AUTOPROPERTY_DOCPATH )) )
                        {
                            RESULT r;
                            DB_RESULTS db_results;
                            char  *s;

                            memset( &r, 0, sizeof( RESULT ) );
                            memset( &db_results, 0, sizeof( DB_RESULTS ) );
                            db_results.indexf = indexf;
    
                            r.db_results = &db_results;
                            r.filenum = filenum;
                            r.fi.filenum = filenum;
   
                            s = getResultPropAsString( &r, m->metaID);
    
                            printf(" %s", s );
                            efree( s );
                            
                        }
                        else
                            printf(" Failed to lookup meta entry");
                            

                        printf(" Freq:%d", frequency);
                        printf(" Pos/Struct:");
                    }
                    else if ( DEBUG_MASK & DEBUG_INDEX_WORDS_META)
                        meta_used[ metaID ]++;
                    else
                    {
                        printf(" [%d", metaID);
                        printf(" %d", filenum);
                        printf(" %d (", frequency);
                    }

                    for (i = 0; i < frequency; i++)
                    {
                        if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_WORDS_FULL))
                        //if (sw->verbose >= 4)
                        {
                            if (i)
                                printf(",%d/%x", GET_POSITION(posdata[i]),GET_STRUCTURE(posdata[i]));
                            else
                                printf("%d/%x", GET_POSITION(posdata[i]), GET_STRUCTURE(posdata[i]));
                        }
                        else if ( DEBUG_MASK & DEBUG_INDEX_WORDS)
                        {
                            if (i)
                                 printf(" %d/%x", GET_POSITION(posdata[i]),GET_STRUCTURE(posdata[i]));
                            else
                                 printf("%d/%x", GET_POSITION(posdata[i]),GET_STRUCTURE(posdata[i]));
                        }
                    }
                    if ( DEBUG_MASK & DEBUG_INDEX_WORDS )
                        printf(")]");

                    }  /* End of DB_CheckFileNum */

                    efree(posdata);

                    /* Check for end of worddata */
                    if ((s - worddata) == sz_worddata)
                        break;   /* End of worddata */

                    /* Check for end of current metaID data */
                    if ( metadata_length == (s - start))
                    {
                        filenum = 0;
                        metaID = uncompress2(&s);
                        metadata_length = uncompress2(&s);
                        start = s;
                    }
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
                if (wordID && ((int)((unsigned char)resultword[0]))!= j)
                  efree(resultword);
            }
        }
        DB_EndReadWords(sw, indexf->DB);

        efree( meta_used );
    }






    /* Decode File Info */
    if (DEBUG_MASK & (DEBUG_INDEX_ALL | DEBUG_INDEX_FILES)  )
        dump_index_file_list( sw, indexf, begin, maxhits );
	
	
    /* just print filenums and number of words per file for word ranking */
    if (DEBUG_MASK & DEBUG_INDEX_WORD_COUNT )
    	dump_word_count( sw, indexf, begin, maxhits );

    DB_Close(sw, indexf->DB);
}


int check_sorted_index( SWISH *sw, IndexFILE *indexf, struct metaEntry *m )
{
    unsigned char *buffer = NULL;
    int     sz_buffer = 0;

    DB_InitReadSortedIndex(sw, indexf->DB);
   
    /* Get the sorted index of the property */
    DB_ReadSortedIndex(sw, m->metaID, &buffer, &sz_buffer, indexf->DB);

    /* For incremental index. It will be released when closing indexf->DB */
#ifndef USE_BTREE
    if ( sz_buffer )
        efree( buffer );
#endif

    /* Table doesn't exist */
    return sz_buffer;
}


void dump_metanames( SWISH *sw, IndexFILE *indexf, int check_presorted )
{
    struct metaEntry *meta_entry;
    int i;

    printf("\n\n-----> METANAMES for %s <-----\n", indexf->line );
    for(i = 0; i < indexf->header.metaCounter; i++)
    {
        meta_entry = indexf->header.metaEntryArray[i];
        
        printf("%20s : id=%2d type=%2d ",meta_entry->metaName, meta_entry->metaID, meta_entry->metaType);

        if ( is_meta_index( meta_entry ) )
            printf(" META_INDEX  Rank Bias=%3d", meta_entry->rank_bias );
            
            

        if ( is_meta_internal( meta_entry ) )
            printf(" META_INTERNAL");
            

        if ( is_meta_property( meta_entry ) )
        {
            printf(" META_PROP:");

            if  ( is_meta_string(meta_entry) )
            {
                printf("STRING(case:%s) ",
                    is_meta_use_strcoll(meta_entry)
                        ? "strcoll"
                        : is_meta_ignore_case(meta_entry)
                            ? "ignore" 
                            : "compare");
                printf("SortKeyLen: %d ", meta_entry->sort_len );
            }

            else if ( is_meta_date(meta_entry) )
                printf("DATE");

            else if ( is_meta_number(meta_entry) )
                printf("NUMBER");

            else
                printf("unknown!");
        }



        if ( check_presorted && check_sorted_index( sw, indexf, meta_entry)  )
            printf(" *presorted*");


        if ( meta_entry->alias )
        {
            struct metaEntry *m = is_meta_index( meta_entry )
                                  ? getMetaNameByID( &indexf->header, meta_entry->alias )
                                  : getPropNameByID( &indexf->header, meta_entry->alias );

            printf(" [Alias for %s (%d)]", m->metaName, m->metaID );
        }


        printf("\n");
        
    }
    printf("\n");
}

/***************************************************************
* Dumps what's currently in the fi->docProperties structure
*
**************************************************************/

void dump_file_properties(IndexFILE * indexf, FileRec *fi )
{
    int j;
	propEntry *prop;
    struct metaEntry *meta_entry;

	if ( !fi->docProperties )  /* may not be any properties */
	{
	    printf(" (No Properties)\n");
	    return;
	}

    for (j = 0; j < fi->docProperties->n; j++)
    {
        if ( !fi->docProperties->propEntry[j] )
            continue;

        meta_entry = getPropNameByID( &indexf->header, j );
        prop = fi->docProperties->propEntry[j];
        
        dump_single_property( prop, meta_entry );
    }
}


void dump_single_property( propEntry *prop, struct metaEntry *meta_entry )
{
    char *propstr;
    char proptype = '?';
    int  i;


    if  ( is_meta_string(meta_entry) )
        proptype = 'S';

    else if ( is_meta_date(meta_entry) )
        proptype = 'D';

    else if ( is_meta_number(meta_entry) )
        proptype = 'N';


    i = prop ? prop->propLen : 0;

    printf("  %20s:%2d (%3d) %c:", meta_entry->metaName, meta_entry->metaID, i, proptype );
    

    if ( !prop )
    {
        printf(" propEntry=NULL\n");
        return;
    }

    propstr = DecodeDocProperty( meta_entry, prop );
    i = 0;
    printf(" \"");

    while ( i < (int)strlen( propstr ) )
    {
        if ( 1 ) // ( isprint( (int)propstr[i] ))
            printf("%c", propstr[i] );
            
        else if ( propstr[i] == '\n' )
            printf("\n");

        else
            printf("..");

        i++;
        if ( i > 300 )
        {
            printf(" ...");
            break;
        }
    }
    printf("\"\n");
            
    efree( propstr );
}


