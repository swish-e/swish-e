/*
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
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
**-----------------------------------------------------------------
**
**  rewritten from scratch - moseley Oct 17, 2001
**
*/

#include <assert.h>             /* for bug hunting */
#include "swish.h"
#include "mem.h"
#include "string.h"
#include "merge.h"
#include "error.h"
#include "search.h"
#include "index.h"
#include "hash.h"
#include "file.h"
#include "docprop.h"
#include "list.h"
#include "compress.h"
#include "metanames.h"
#include "db.h"
#include "dump.h"
#include "result_sort.h"
#include "swish_qsort.h"
#include "result_output.h"
#include "parse_conffile.h"
static void dup_header( SWISH *sw_input, SWISH *sw_output );
static void check_header_match( IndexFILE *in_index, SWISH *sw_output );
static void make_meta_map( IndexFILE *in_index, SWISH *sw_output);
static void load_filename_sort( SWISH *sw, IndexFILE *cur_index );
static IndexFILE *get_next_file_in_order( SWISH *sw_input );
static void add_file( FILE *filenum_map, IndexFILE *cur_index, SWISH *sw_input, SWISH *sw_output );
static int *get_map( FILE *filenum_map, IndexFILE *cur_index );
static void dump_index(SWISH * sw, IndexFILE * indexf, SWISH *sw_output, int *filenum_map );
static ENTRY *write_word_pos( SWISH *sw_input, IndexFILE *indexf, SWISH *sw_output, int *file_num_map, int filenum, char *resultword, int metaID, int posdata );


// #define DEBUG_MERGE

/****************************************************************************
*  merge_indexes -- reads from input indexes, and outputs a new index
*
*
*****************************************************************************/

void merge_indexes( SWISH *sw_input, SWISH *sw_output )
{
    IndexFILE   *cur_index;
    FILE        *filenum_map;
    char        *tmpfilename;




    /*******************************************************************************
    * Get ready to merge the indexes.  For each index:
    *   - check that it has the correct headers
    *   - create meta entries in output index, and create a map to convert metas
    *   - load an array of file numbers sorted by filename so can merge sort the filesnames
    *   - set some initial defaults.
    *********************************************************************************/

    cur_index = sw_input->indexlist;
    while( cur_index  )
    {
        printf("Input index '%s' has %d files and %d words\n", cur_index->line, cur_index->header.totalfiles, cur_index->header.totalwords);

        if ( cur_index == sw_input->indexlist )
            /* Duplicate the first index's header into the output index */
            dup_header( sw_input, sw_output );
        else
            check_header_match( cur_index, sw_output );  // errors if headers don't match - don't really need to check first one since it was the one that was dupped 


        make_meta_map( cur_index, sw_output);        // add metas to new index, and create map

        load_filename_sort( sw_input, cur_index );   // so can read in filename order

        cur_index->current_file = 0;  
        cur_index->cur_prop = NULL;

#ifdef DEBUG_MERGE
        dump_metanames( sw_input, cur_index, 1 );
        dump_metanames( sw_output, sw_output->indexlist, 0 );
#endif

        cur_index = cur_index->next;
    }


#ifdef DEBUG_MERGE
    printf("----- Output Header ----------\n");
    resultPrintHeader(sw_output, 0, &sw_output->indexlist->header, sw_output->indexlist->line, 0);
#endif



    /****************************************************************************
    *  Now, read in filename order (so can throw out duplicates)
    *  - read properties and write out to new index
    *  - write a temporay of records to identify
    *       - indexfile
    *       - old filenum to new filenum mapping
    *       - total words per file, if set
    ****************************************************************************/

    /* place to store file number map and total words per file */
    filenum_map = create_tempfile(sw_input, F_WRITE_BINARY, "fnum", &tmpfilename, 0 );

    while( (cur_index = get_next_file_in_order( sw_input )) )
        add_file( filenum_map, cur_index, sw_input, sw_output );



    /* Don't need the pre-sorted indexes any more */
    for ( cur_index = sw_input->indexlist; cur_index; cur_index = cur_index->next )
    {
        efree( cur_index->path_order );
        cur_index->path_order = NULL;
    }

    fclose( filenum_map );

    if ( !(filenum_map = fopen( tmpfilename, F_READ_BINARY )) )
        progerrno("failed to reopen '%s' :", tmpfilename );



    /****************************************************************************
    *  Finally, read the indexes one-by-one to read word and position data
    *  - reads through the temp file for each index to build a filenumber map
    *
    ****************************************************************************/

    cur_index = sw_input->indexlist;
    while( cur_index )
    {
        int *file_num_map = get_map( filenum_map, cur_index );

        dump_index(sw_input, cur_index, sw_output, file_num_map );

        /* free the maps */
        efree( file_num_map );
        efree( cur_index->meta_map );
        cur_index->meta_map = NULL;

        cur_index = cur_index->next;

    }


#ifdef DEBUG_MERGE
    printf("----- Final Output Header ----------\n");
    resultPrintHeader(sw_output, 0, &sw_output->indexlist->header, sw_output->indexlist->line, 0);
#endif

    remove( tmpfilename );
    efree( tmpfilename );
}

/****************************************************************************
*  dup_header -- duplicates a header
*
*  rereads the header from the data base, and clears out some values
*
*****************************************************************************/
       
static void dup_header( SWISH *sw_input, SWISH *sw_output )
{
    INDEXDATAHEADER *out_header = &sw_output->indexlist->header;

    // probably need to free the sw_output header from what's created in swishnew.

    /* Read in the header from the first merge file and store in the output file */
    read_header(sw_input, out_header, sw_input->indexlist->DB);

    out_header->totalfiles = 0;
    out_header->totalwords = 0;

    freeMetaEntries( out_header );

    if ( out_header->indexedon )
    {
        efree( out_header->indexedon );
        out_header->indexedon = NULL;
        out_header->lenindexedon = 0;
    }
}
    
/****************************************************************************
*  check_header_match -- makes sure that the imporant settings match
*
*
*****************************************************************************/

// This assumes that the size will always preceed the content.
typedef struct
{
    int     len;
    char    *str;
} *HEAD_CMP;

static void compare_header( char *index, char *name, void *in, void *out )
{
    HEAD_CMP    in_item = (HEAD_CMP)in;
    HEAD_CMP    out_item = (HEAD_CMP)out;

    if ( in_item->len != out_item->len )
        progerr("Header %s in index %s doesn't match length in length with output header", name, index );

    if ( strcmp( (const char *)in_item->str, (const char *)out_item->str )) 
        progerr("Header %s in index %s doesn't match output header", name, index );
    
    //if ( memcmp( (const void *)in_item->str, (const void *)out_item->str, in_item->len ) )
    //    progerr("Header %s in index %s doesn't match output header", name, index );
        

        

}

   
static void check_header_match( IndexFILE *in_index, SWISH *sw_output )
{
    INDEXDATAHEADER *out_header = &sw_output->indexlist->header;
    INDEXDATAHEADER *in_header = &in_index->header;

    compare_header( in_index->line, "WordCharacters", &in_header->lenwordchars,  &out_header->lenwordchars );
    compare_header( in_index->line, "BeginCharacters", &in_header->lenbeginchars,  &out_header->lenbeginchars );
    compare_header( in_index->line, "EndCharacters", &in_header->lenendchars,  &out_header->lenendchars );

    compare_header( in_index->line, "IgnoreLastChar", &in_header->lenignorelastchar,  &out_header->lenignorelastchar );
    compare_header( in_index->line, "IgnoreFirstChar", &in_header->lenignorefirstchar,  &out_header->lenignorefirstchar );

    compare_header( in_index->line, "BumpPositionChars", &in_header->lenbumpposchars,  &out_header->lenbumpposchars );


    if ( in_header->fuzzy_mode != out_header->fuzzy_mode )
        progerr("FuzzyIndexingMode in index %s of '%s' doesn't match '%s'",
            in_index->line,
            fuzzy_mode_to_string( in_header->fuzzy_mode ),
            fuzzy_mode_to_string( out_header->fuzzy_mode ) );
            
        
    if ( in_header->ignoreTotalWordCountWhenRanking != out_header->ignoreTotalWordCountWhenRanking )
        progerr("ignoreTotalWordCountWhenRanking Rules doesn't match for index %s", in_index->line );

    if ( memcmp( &in_header->translatecharslookuptable, &out_header->translatecharslookuptable, sizeof(in_header->translatecharslookuptable) / sizeof( int ) ) )
        progerr("TranslateChars header doesn't match for index %s", in_index->line );


    //??? need to compare stopword lists

    //??? need to compare buzzwords

}

/****************************************************************************
*  make_meta_map - adds metanames to output index and creates map
*
*
*****************************************************************************/

static void make_meta_map( IndexFILE *in_index, SWISH *sw_output)
{
    INDEXDATAHEADER *out_header = &sw_output->indexlist->header;
    INDEXDATAHEADER *in_header = &in_index->header;
    int             i;
    struct metaEntry *in_meta;
    struct metaEntry *out_meta;
    int             *meta_map;
    

    meta_map = emalloc( sizeof( int ) * (in_header->metaCounter + 1) );
    memset( meta_map, 0, sizeof( int ) * (in_header->metaCounter + 1) );

    for( i = 0; i < in_header->metaCounter; i++ )
    {
        in_meta = in_header->metaEntryArray[i];


        /* Try to see if it's an existing metaname */
        out_meta = is_meta_index( in_meta )
                   ? getMetaNameByNameNoAlias( out_header, in_meta->metaName )
                   : getPropNameByNameNoAlias( out_header, in_meta->metaName );

        /* if it's not found, then add it */
        if ( !out_meta )
            out_meta = addMetaEntry(out_header, in_meta->metaName, in_meta->metaType, 0);
        else
            if (out_meta->metaType != in_meta->metaType )
                progerr("meta name %s in index %s is different type than in output index", in_meta->metaName, in_index->line );


        /* Now, save the mapping */
        meta_map[ in_meta->metaID ] = out_meta->metaID;                


        /* now here's a pain, and lots of room for screw up. */
        /* Basically, check for alias mappings, and that they are correct */
        /* you can say title is an alias for swishtitle in one index, and then say */
        /* title is an alias for doctitle in another index */

        /* If it's an alias, then make that mapping, too */
        if ( in_meta->alias )
        {
            struct metaEntry *in_alias;
            struct metaEntry *out_alias;

            /* Grab alias meta entry so we can look it up in the out_header */
            
            in_alias = is_meta_index( in_meta )
                   ? getMetaNameByID( in_header, in_meta->alias )
                   : getPropNameByID( in_header, in_meta->alias );

            if ( !in_alias )
                progerr("Failed to lookup alias for %s in index %s", in_meta->metaName, in_index->line );


            /* now lookup the alias in the out_header by name */
            out_alias = is_meta_index( in_alias )
                   ? getMetaNameByNameNoAlias( out_header, in_alias->metaName )
                   : getPropNameByNameNoAlias( out_header, in_alias->metaName );


            /* should be there, since it would have been added earlier - the real metas must be added before the aliases */
            if ( !out_alias )
                progerr("Failed to lookup alias for %s in output index", out_meta->metaName );


            /* If this is new (or doesn't point to the alias root, then just assign it */
            if ( !out_meta->alias )
                out_meta->alias = out_alias->metaID;

            /* else, if it is already an alias, but points someplace else, we have a problem */
            else if ( out_meta->alias != out_alias->metaID )
                progerr("In index %s metaname '%s' is an alias for '%s'(%d).  But another index already mapped '%s' to ID# '%d'", in_index->line, in_meta->metaName, in_alias->metaName, in_alias->metaID, out_meta->metaName, out_meta->alias );
        }
    }

    in_index->meta_map = meta_map;


#ifdef DEBUG_MERGE
    printf(" %s   ->   %s  ** Meta Map **\n", in_index->line, sw_output->indexlist->line );
    for ( i=0; i<in_header->metaCounter + 1;i++)
        printf("%4d  ->  %3d\n", i, meta_map[i] );
#endif
        
}

/****************************************************************************
*  load_filename_sort - creates an array for reading in filename order
*
*
*****************************************************************************/

static int  *sorted_data;

static int     compnums(const void *s1, const void *s2)
{
    int         a = *(int *)s1; // filenumber passed from qsort
    int         b = *(int *)s2;
    int         v1 = sorted_data[ a-1 ];
    int         v2 = sorted_data[ b-1 ];

    // return v1 <=> v2;
    
    if ( v1 < v2 )
        return -1;
    if ( v1 > v2 )
        return 1;

    return 0;
}


static void load_filename_sort( SWISH *sw, IndexFILE *cur_index )
{
    struct metaEntry *path_meta = getPropNameByName( &cur_index->header, AUTOPROPERTY_DOCPATH );
    int         i;
    int         *sort_array;
    int         totalfiles = cur_index->header.totalfiles;

    if ( !path_meta )
        progerr("Can't merge index %s.  It doesn't contain the property %s", cur_index->line, AUTOPROPERTY_DOCPATH );


    /* Save for looking up pathname when sorting */
    cur_index->path_meta = path_meta;

    /* Case is important for most OS when comparing file names */
    cur_index->path_meta->metaType &= ~META_IGNORE_CASE;



    cur_index->modified_meta = getPropNameByName( &cur_index->header, AUTOPROPERTY_LASTMODIFIED );

        
    if ( !LoadSortedProps( sw, cur_index, path_meta ) )
    {
        FileRec fi;
        memset( &fi, 0, sizeof( FileRec ));
        path_meta->sorted_data = CreatePropSortArray( sw, cur_index, path_meta, &fi, 1 );
    }


    /* So the qsort compare function can read it */
    sorted_data = path_meta->sorted_data;


    if ( !sorted_data )
        progerr("failed to load or create sorted properties for index %s", cur_index->line );


    sort_array = emalloc(  totalfiles * sizeof( int ) );
    memset( sort_array, 0, totalfiles * sizeof( int ) );


    /* build an array with file numbers and sort into filename order */
    for ( i = 0; i < totalfiles; i++ )
        sort_array[i] = i+1;  // filenumber starts a one


    swish_qsort( sort_array, totalfiles, sizeof( int ), &compnums);

    cur_index->path_order = sort_array;

    efree( path_meta->sorted_data );
    path_meta->sorted_data = NULL;
}

/****************************************************************************
*  get_next_file_in_order -- grabs the next file entry from all the indexes
*  in filename (and then modified date) order
*
*
*****************************************************************************/

/* This isn't really accurate, as some other file may come and replace the newer */

static void print_file_removed(IndexFILE *older, propEntry *op, IndexFILE *newer, propEntry *np )
{
    
    char *p1, *d1, *p2, *d2;
    p1 = DecodeDocProperty( older->path_meta, older->cur_prop );
    d1 = DecodeDocProperty( older->modified_meta, op );
    
    p2 = DecodeDocProperty( newer->path_meta, newer->cur_prop );
    d2 = DecodeDocProperty( newer->modified_meta, np );
    
    printf("Replaced file '%s %s' with '%s %s'\n", p1, d1, p2, d2);
}


static IndexFILE *get_next_file_in_order( SWISH *sw_input )
{
    IndexFILE   *winner = NULL;
    IndexFILE   *cur_index = sw_input->indexlist;
    FileRec     fi;
    int         ret;
    propEntry   *wp, *cp;

    memset(&fi, 0, sizeof( FileRec ));

    for ( cur_index = sw_input->indexlist; cur_index; cur_index = cur_index->next )
    {
        /* don't use cached props, as they belong to a different index! */
        if ( fi.prop_index )
            efree( fi.prop_index );
        memset(&fi, 0, sizeof( FileRec ));

        /* still some to read in this index? */
        if ( cur_index->current_file >= cur_index->header.totalfiles )
            continue;



        /* get file number from lookup table */
        fi.filenum = cur_index->path_order[cur_index->current_file];

        if ( !cur_index->cur_prop )
            cur_index->cur_prop = ReadSingleDocPropertiesFromDisk(sw_input, cur_index, &fi, cur_index->path_meta->metaID, 0 );


        if ( !winner )
        {
            winner = cur_index;
            continue;
        }

        ret = Compare_Properties( cur_index->path_meta, cur_index->cur_prop, winner->cur_prop );

        if ( ret != 0 )  
        {
            if ( ret < 0 )  /* take cur_index if it's smaller */
                winner = cur_index;

            continue;
        }



        /* if they are the same name, then take the newest, and increment the older one */


        /* read the modified time for the current file */
        /* Use the same fi record, because it has the cached prop seek locations */
        cp = ReadSingleDocPropertiesFromDisk(sw_input, cur_index, &fi, cur_index->modified_meta->metaID, 0 );


        /* read the modified time for the current winner */
        if ( fi.prop_index )
            efree( fi.prop_index );
        memset(&fi, 0, sizeof( FileRec ));

        fi.filenum = winner->path_order[winner->current_file];
        wp = ReadSingleDocPropertiesFromDisk(sw_input, winner, &fi, cur_index->modified_meta->metaID, 0 );

        ret = Compare_Properties( cur_index->modified_meta, cp, wp );



        /* If current is greater (newer) then throw away winner */
        if ( ret > 0 )
        {
            print_file_removed( winner, wp, cur_index, cp);
            winner->current_file++;
            if ( winner->cur_prop )
                efree( winner->cur_prop );
            winner->cur_prop = NULL;
            winner = cur_index;
        }
        /* else, keep winner, and throw away current */
        else
        {
            print_file_removed(cur_index, cp, winner, wp );
            cur_index->current_file++;
            if ( cur_index->cur_prop )
                efree( cur_index->cur_prop );

            cur_index->cur_prop = NULL;
        }

        freeProperty( cp );
        freeProperty( wp );

    }

    if ( fi.prop_index )
        efree( fi.prop_index );


    if ( !winner )
        return NULL;

        
    winner->filenum = winner->path_order[winner->current_file++];

#ifdef DEBUG_MERGE
printf("   Files in order: index %s file# %d winner\n", winner->line, winner->filenum );            
#endif

    /* free prop, as it's not needed anymore */
    if ( winner->cur_prop )
        efree( winner->cur_prop );
    winner->cur_prop = NULL;


    return winner;
}

        
/****************************************************************************
*  add_file
*
*  Now, read in filename order (so can throw out duplicates)
*  - read properties and write out to new index
*  - write a temporay of records to identify
*       - indexfile
*       - old filenum to new filenum mapping
*       - total words per file, if set
****************************************************************************/

static void add_file( FILE *filenum_map, IndexFILE *cur_index, SWISH *sw_input, SWISH *sw_output )
{
    FileRec             fi;
    IndexFILE           *indexf = sw_output->indexlist;
    struct MOD_Index    *idx = sw_output->Index;
    docProperties       *d;
    int                 i;
    propEntry           *tmp;
    docProperties       *docProperties=NULL;
    struct metaEntry    meta_entry;


    meta_entry.metaName = "(default)";  /* for error message, I think */


    memset( &fi, 0, sizeof( FileRec ));


#ifdef DEBUG_MERGE
    printf("Reading Properties from input index '%s' file %d\n", cur_index->line, cur_index->filenum);
#endif

    /* read the properties and map them as needed */
    d = ReadAllDocPropertiesFromDisk( sw_input, cur_index, cur_index->filenum );


#ifdef DEBUG_MERGE
    fi.docProperties = d;
    dump_file_properties( cur_index, &fi );
#endif
    


    /* all this off-by-one things are a mess */

    /* read through all the property slots, and map them, as needed */
    for ( i = 0; i < d->n; i++ )
        if ( (tmp = d->propEntry[i]) )
        {
            meta_entry.metaID = cur_index->meta_map[ i ];
            addDocProperty(&docProperties, &meta_entry, tmp->propValue, tmp->propLen, 1 );
        }

#ifdef DEBUG_MERGE
    printf(" after mapping file %s\n", indexf->line);
    fi.docProperties = docProperties;
    dump_file_properties( cur_index, &fi );
    printf("\n");
#endif

   
    /* Now bump the file counter  */
    idx->filenum++;
    indexf->header.totalfiles++;

    if ( docProperties )  /* always true */
    {
        fi.filenum = idx->filenum;
        fi.docProperties = docProperties;

        WritePropertiesToDisk( sw_output , &fi );

        freeDocProperties( d );
    }

    


    /* now write out the data to be used for mapping file for a given index. */
    //    compress1( cur_index->filenum, filenum_map, fputc );   // what file number this came from

    fwrite( &cur_index->filenum, sizeof(int), 1, filenum_map);
    fwrite( &cur_index, sizeof(IndexFILE *), 1, filenum_map);        // what index


    /* Save total words per file */
    if ( !indexf->header.ignoreTotalWordCountWhenRanking )
    {
        INDEXDATAHEADER *header = &indexf->header;
        int idx1 = fi.filenum - 1;

        if ( !header->TotalWordsPerFile || idx1 >= header->TotalWordsPerFileMax )
        {
            header->TotalWordsPerFileMax += 20000;  /* random guess -- could be a config setting */
            header->TotalWordsPerFile = erealloc( header->TotalWordsPerFile, header->TotalWordsPerFileMax * sizeof(int) );
        }

        header->TotalWordsPerFile[idx1] = cur_index->header.TotalWordsPerFile[cur_index->filenum-1];
    }
}

/****************************************************************************
*  Builds a old_filenum -> new_filenum map;
*
*  This makes is so you can lookup an old file number and map it to a new file number
*
****************************************************************************/

static int *get_map( FILE *filenum_map, IndexFILE *cur_index )
{
    int         *array = emalloc( (cur_index->header.totalfiles+1) * sizeof( int ) );
    IndexFILE   *idf;
    int         filenum;
    int         new_filenum = 0;

    

    memset( array, 0, (cur_index->header.totalfiles+1) * sizeof( int ) );


    clearerr( filenum_map );
    fseek( filenum_map, 0, 0 );  /* start at beginning */

    while ( 1 )
    {
        new_filenum++;

        if (!fread( &filenum, sizeof(int), 1, filenum_map))
            break;
        
        
        if(!fread( &idf, sizeof(IndexFILE *), 1, filenum_map))
            break;

        if ( idf == cur_index )
            array[filenum] = new_filenum;

    }

    return array;
}

/****************************************************************************
*  Reads the index and calls write_word_pos
*
*  This should be a common, shared function in swish.  Would also be good
*  written as an iterator function so it retuns a position structure.
*  Currently, calls write_word_pos for each position.  Would it be better
*  to call with a LOCATION structure?
*
****************************************************************************/
   
static void dump_index(SWISH * sw, IndexFILE * indexf, SWISH *sw_output, int *filenum_map )
{
    int         i;
    int         j;
    int         frequency = 0;
    int         tmpval;
    int         filenum;
    int         local_posdata[MAX_STACK_POSITIONS];
    int        *posdata;
    int         sz_worddata;
    int         metaname = 0;
    int         word_count = 0;
    int         loc_count = 0;
    char        word[2];
    char       *resultword;
    long        wordID;
    unsigned long    nextposmetaname = 0L;
    unsigned char   *worddata;
    unsigned char   *s;
    unsigned char   flag;
    ENTRY      *e, *tmp;
    int         hash;

    

    DB_InitReadWords(sw, indexf->DB);



    printf("Processing words in index '%s': %3d words\r", indexf->line, word_count);
    fflush(stdout);
    
    for(j=0;j<256;j++)
    {
        
        word[0] = (unsigned char) j; word[1] = '\0';
        DB_ReadFirstWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);

        while(wordID)
        {
            e = NULL;
            loc_count = 0;
            
            /* Read Word's data */
            DB_ReadWordData(sw, wordID, &worddata, &sz_worddata, indexf->DB);

            /* parse and print word's data */
            s = worddata;

            tmpval = uncompress2(&s);     /* tfrequency */

            metaname = uncompress2(&s);     /* metaID */

            if (metaname)
            {
                nextposmetaname = UNPACKLONG2(s);
                s += sizeof(long);
            }

            filenum = 0;

            while(1)
            {                   /* Read on all items */
                uncompress_location_values(&s,&flag,&tmpval,&frequency);
                filenum += tmpval;
                if(frequency > MAX_STACK_POSITIONS)
                    posdata = (int *) emalloc(frequency * sizeof(int));
                else
                    posdata = local_posdata;

                uncompress_location_positions(&s,flag,frequency,posdata);


                /* now we have the word data */
                for (i = 0; i < frequency; i++, loc_count++)
                {
                    tmp = write_word_pos( sw, indexf, sw_output, filenum_map, filenum, resultword, metaname, posdata[i]);
                    /* get the pointer to entry for later compress */
                    if(!e)
                        e = tmp;
                }

                if(e)
                {
                    /* 08/2002 jmruiz - We will call CompressCurrentLocEntry from time
                    ** to time to help addentry.
                    ** If we do not do this, addentry routine will have to run linked lists 
                    ** of positions with thousands of elements and makes the merge proccess
                    ** very slow
                    */
                    if(!(loc_count % 100))
                    {
                        hash = verybighash(e->word);
                        if(sw_output->Index->hashentriesdirty[hash])
                        {
                            sw_output->Index->hashentriesdirty[hash] = 0;
                            CompressCurrentLocEntry(sw_output, sw_output->indexlist, e);
                        }
                    }
                }

                if(posdata != local_posdata)
                    efree(posdata);

                if ((s - worddata) == sz_worddata)
                    break;   /* End of worddata */

                if ((unsigned long)(s - worddata) == nextposmetaname)
                {
                    filenum = 0;
                    metaname = uncompress2(&s);
                    if (metaname)
                    {
                        nextposmetaname = UNPACKLONG2(s); 
                        s += sizeof(long);
                    }
                    else
                        nextposmetaname = 0L;
                }
            }

            if(e)
            {
                /* Reset the hashentriesdirty flag - Not needed by merge */
                sw_output->Index->hashentriesdirty[verybighash(e->word)] = 0;
                CompressCurrentLocEntry(sw_output, sw_output->indexlist, e);

                word_count++;
            }


            /* Let's coalesce location from time to time to save memory */
            if(!(word_count % 1000))
            {
                printf("Processing words in index '%s': %6d words\r", indexf->line, word_count);
                fflush(stdout);

                coalesce_all_word_locations(sw_output, sw_output->indexlist);

                /* Make zone available for reuse */
                Mem_ZoneReset(sw_output->Index->currentChunkLocZone);
                sw_output->Index->freeLocMemChain = NULL;

            }

            efree(worddata);
            efree(resultword);
            DB_ReadNextWordInvertedIndex(sw, word,&resultword,&wordID,indexf->DB);
        }
    }
    printf("Processing words in index '%s': %6d words\n", indexf->line, word_count);

    DB_EndReadWords(sw, indexf->DB);

    /* Coalesce pending work*/
    coalesce_all_word_locations(sw_output, sw_output->indexlist);

    /* Make zone available for reuse */
    Mem_ZoneReset(sw_output->Index->currentChunkLocZone);
    sw_output->Index->freeLocMemChain = NULL;
}

/****************************************************************************
*  Writes a word out to the index
*
*
****************************************************************************/

static ENTRY  *write_word_pos( SWISH *sw_input, IndexFILE *indexf, SWISH *sw_output, int *file_num_map, int filenum, char *resultword, int metaID, int posdata )
{
    int         new_file;
    int         new_meta;

#ifdef DEBUG_MERGE
    printf("\nindex %s '%s' Struct: %d Pos: %d",
    indexf->line, resultword, structure, position );


    if ( !(new_file = file_num_map[ filenum ]) )
    {
        printf("  file: %d **File deleted!**\n", filenum);
        return NULL;
    }

    if ( !(new_meta = indexf->meta_map[ metaID ] ))
    {
        printf("  file: %d **Failed to map meta ID **\n", filenum);
        return NULL;
    }

    printf("  File: %d -> %d  Meta: %d -> %d\n", filenum, new_file, metaID, new_meta );

    return addentry( sw_output, resultword, new_file, structure, metaID, position );



    /* Compress the entries ?  */
    //compress_entries( sw_output );  // maybe after every 1000 words or so?
    // * but does this make it so addentry can't lookup a word?
#else


    if ( !(new_file = file_num_map[ filenum ]) )
        return NULL;

    if ( !(new_meta = indexf->meta_map[ metaID ] ))
        return NULL;

    return addentry( sw_output, resultword, new_file, GET_STRUCTURE(posdata), metaID, GET_POSITION(posdata) );


#endif


}

