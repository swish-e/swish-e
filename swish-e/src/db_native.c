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

#include <time.h>
#include "swish.h"
#include "mem.h"
#include "file.h"
#include "error.h"
#include "string.h"
#include "compress.h"
#include "hash.h"
#include "db.h"
#include "swish_qsort.h"
#include "ramdisk.h"
#include "db_native.h"

#ifdef USE_BTREE
#define WRITE_WORDS_RAMDISK 0
#else
#define WRITE_WORDS_RAMDISK 1
#endif

// #define DEBUG_PROP 1

/*
  -- init structures for this module
*/

void    initModule_DBNative(SWISH * sw)
{
    struct MOD_DB *Db;

    Db = (struct MOD_DB *) emalloc(sizeof(struct MOD_DB));

    Db->DB_name = (char *) estrdup("native");

    Db->DB_Create = DB_Create_Native;
    Db->DB_Open = DB_Open_Native;
    Db->DB_Close = DB_Close_Native;
    Db->DB_Remove = DB_Remove_Native;

    Db->DB_InitWriteHeader = DB_InitWriteHeader_Native;
    Db->DB_WriteHeaderData = DB_WriteHeaderData_Native;
    Db->DB_EndWriteHeader = DB_EndWriteHeader_Native;

    Db->DB_InitReadHeader = DB_InitReadHeader_Native;
    Db->DB_ReadHeaderData = DB_ReadHeaderData_Native;
    Db->DB_EndReadHeader = DB_EndReadHeader_Native;

    Db->DB_InitWriteWords = DB_InitWriteWords_Native;
    Db->DB_GetWordID = DB_GetWordID_Native;
    Db->DB_WriteWord = DB_WriteWord_Native;

#ifndef USE_BTREE
    Db->DB_WriteWordHash = DB_WriteWordHash_Native;
#else
    Db->DB_UpdateWordID = DB_UpdateWordID_Native;
    Db->DB_DeleteWordData = DB_DeleteWordData_Native;
#endif

    Db->DB_WriteWordData = DB_WriteWordData_Native;
    Db->DB_EndWriteWords = DB_EndWriteWords_Native;

    Db->DB_InitReadWords = DB_InitReadWords_Native;
    Db->DB_ReadWordHash = DB_ReadWordHash_Native;
    Db->DB_ReadFirstWordInvertedIndex = DB_ReadFirstWordInvertedIndex_Native;
    Db->DB_ReadNextWordInvertedIndex = DB_ReadNextWordInvertedIndex_Native;
    Db->DB_ReadWordData = DB_ReadWordData_Native;
    Db->DB_EndReadWords = DB_EndReadWords_Native;

    Db->DB_InitWriteFiles = DB_InitWriteFiles_Native;
    Db->DB_WriteFile = DB_WriteFile_Native;
    Db->DB_EndWriteFiles = DB_EndWriteFiles_Native;

    Db->DB_InitReadFiles = DB_InitReadFiles_Native;
    Db->DB_ReadFile = DB_ReadFile_Native;
    Db->DB_EndReadFiles = DB_EndReadFiles_Native;

    Db->DB_InitWriteSortedIndex = DB_InitWriteSortedIndex_Native;
    Db->DB_WriteSortedIndex = DB_WriteSortedIndex_Native;
    Db->DB_EndWriteSortedIndex = DB_EndWriteSortedIndex_Native;

    Db->DB_InitReadSortedIndex = DB_InitReadSortedIndex_Native;
    Db->DB_ReadSortedIndex = DB_ReadSortedIndex_Native;
    Db->DB_ReadSortedData = DB_ReadSortedData_Native;
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_Native;

    Db->DB_WriteProperty = DB_WriteProperty_Native;
    Db->DB_WritePropPositions = DB_WritePropPositions_Native;
    Db->DB_ReadProperty = DB_ReadProperty_Native;
    Db->DB_ReadPropPositions = DB_ReadPropPositions_Native;
    Db->DB_Reopen_PropertiesForRead = DB_Reopen_PropertiesForRead_Native;

#ifdef USE_BTREE
    Db->DB_WriteTotalWordsPerFile = DB_WriteTotalWordsPerFile_Native;
    Db->DB_ReadTotalWordsPerFile = DB_ReadTotalWordsPerFile_Native;
#endif

    sw->Db = Db;

    return;
}


/*
  -- release all wired memory for this module
*/

void    freeModule_DBNative(SWISH * sw)
{
    efree(sw->Db->DB_name);
    efree(sw->Db);
    sw->Db = NULL;
    return;
}



/* ---------------------------------------------- */



/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int     configModule_DBNative(SWISH * sw, StringList * sl)
{
    // struct MOD_DBNative *md = sw->DBNative;
    // char *w0    = sl->word[0];
    int     retval = 1;


    retval = 0;                 // tmp due to empty routine

    return retval;
}






/* Does an index file have a readable format?
*/

static void DB_CheckHeader(struct Handle_DBNative *DB)
{
    long    swish_magic;

    fseek(DB->fp, 0, 0);
    swish_magic = readlong(DB->fp, fread);

    if (swish_magic != SWISH_MAGIC)
    {
        set_progerr(INDEX_FILE_ERROR, DB->sw, "File \"%s\" has an unknown format.", DB->cur_index_file);
        return;
    }



    {   
           long
#ifdef USE_BTREE
                btree,
                worddata,
                array,
                presorted,
#endif
                prop;

        DB->unique_ID = readlong(DB->fp, fread);
        prop = readlong(DB->prop, fread);

        if (DB->unique_ID != prop)
        {
            set_progerr(INDEX_FILE_ERROR, DB->sw, "Index file '%s' and property file '%s' are not related.", DB->cur_index_file, DB->cur_prop_file);
            return;
        }

#ifdef USE_BTREE
        btree = readlong(DB->fp_btree, fread);
        if (DB->unique_ID != btree)
        {
            set_progerr(INDEX_FILE_ERROR, DB->sw, "Index file '%s' and btree file '%s' are not related.", DB->cur_index_file, DB->cur_btree_file);
            return;
        }

        worddata = readlong(DB->fp_worddata, fread);
        if (DB->unique_ID != worddata)
        {
            set_progerr(INDEX_FILE_ERROR, DB->sw, "Index file '%s' and worddata file '%s' are not related.", DB->cur_index_file, DB->cur_worddata_file);
            return;
        }

        array = readlong(DB->fp_array, fread);
        if (DB->unique_ID != array)
        {
            set_progerr(INDEX_FILE_ERROR, DB->sw, "Index file '%s' and array file '%s' are not related.", DB->cur_index_file, DB->cur_array_file);
            return;
        }

        presorted = readlong(DB->fp_presorted, fread);

        if (DB->unique_ID != presorted)
        {
            set_progerr(INDEX_FILE_ERROR, DB->sw, "Index file '%s' and presorted index file '%s' are not related.", DB->cur_index_file, DB->cur_presorted_file);
            return;
        }
#endif
    }

}

static struct Handle_DBNative *newNativeDBHandle(SWISH *sw, char *dbname)
{
    struct Handle_DBNative *DB;

    /* Allocate structure */
    DB = (struct Handle_DBNative *) emalloc(sizeof(struct Handle_DBNative));
    memset( DB, 0, sizeof( struct Handle_DBNative ));

    DB->sw = sw;  /* for error messages */

    if (WRITE_WORDS_RAMDISK)
    {
        DB->w_tell = ramdisk_tell;
        DB->w_write = ramdisk_write;
        DB->w_seek = ramdisk_seek;
        DB->w_read = ramdisk_read;
        DB->w_close = ramdisk_close;
        DB->w_putc = ramdisk_putc;
        DB->w_getc = ramdisk_getc;
    }
    else
    {
        DB->w_tell = ftell;
        DB->w_write = fwrite;
        DB->w_seek = fseek;
        DB->w_read = fread;
        DB->w_close = fclose;
        DB->w_putc = fputc;
        DB->w_getc = fgetc;
    }

    DB->dbname = estrdup(dbname);

    return DB;
}


/* Open files */


static FILE   *openIndexFILEForRead(char *filename) 
{
    return fopen(filename, F_READ_BINARY);
}

static FILE   *openIndexFILEForReadAndWrite(char *filename)
{  
    return fopen(filename, F_READWRITE_BINARY);
}

  
static FILE   *openIndexFILEForWrite(char *filename)
{
    return fopen(filename, F_WRITE_BINARY);
}

static void    CreateEmptyFile(char *filename)
{
    FILE   *fp;
    
    if (!(fp = openIndexFILEForWrite(filename)))
    {
        progerrno("Couldn't write the file \"%s\": ", filename);
    }
    fclose(fp);
}

static int  is_directory(char *path)
{
    struct stat stbuf;

    if (stat(path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}


/**********************/



void   *DB_Create_Native(SWISH *sw, char *dbname)
{
    int     i;
    long    swish_magic;
    char   *filename;
#ifdef USE_BTREE
    FILE   *fp_tmp;
#endif
    struct Handle_DBNative *DB;


    if ( is_directory( dbname ) )
        progerr( "Index file '%s' is a directory", dbname );


    swish_magic = SWISH_MAGIC;
   /* Allocate structure */
    DB = (struct Handle_DBNative *) newNativeDBHandle(sw, dbname);
    DB->mode = DB_CREATE;
    DB->unique_ID = (long) time(NULL); /* Ok, so if more than one index is created the second... */

#ifdef USE_TEMPFILE_EXTENSION
    filename = emalloc(strlen(dbname) + strlen(USE_TEMPFILE_EXTENSION) + strlen(PROPFILE_EXTENSION) + strlen(BTREE_EXTENSION) + strlen(WORDDATA_EXTENSION) + strlen(ARRAY_EXTENSION) + strlen(PRESORTED_EXTENSION) + 1);
    strcpy(filename, dbname);
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_index = 1;
#else
    filename = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + +strlen(BTREE_EXTENSION) + strlen(WORDDATA_EXTENSION) + strlen(ARRAY_EXTENSION) + strlen(PRESORTED_EXTENSION) + 1);
    strcpy(filename, dbname);
#endif


    /* Create index File */

    CreateEmptyFile(filename);
    if (!(DB->fp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the index file \"%s\": ", filename);

    DB->cur_index_file = estrdup(filename);
    printlong(DB->fp, swish_magic, fwrite);
    printlong(DB->fp, DB->unique_ID, fwrite);


    /* Create property File */
    strcpy(filename, dbname);
    strcat(filename, PROPFILE_EXTENSION);

#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_prop = 1;
#endif

    CreateEmptyFile(filename);
    if (!(DB->prop = openIndexFILEForWrite(filename)))
        progerrno("Couldn't create the property file \"%s\": ", filename);

    DB->cur_prop_file = estrdup(filename);
    printlong(DB->prop, DB->unique_ID, fwrite);


#ifdef USE_BTREE
    /* Create Btree File */
    strcpy(filename, dbname);
    strcat(filename, BTREE_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_btree = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the btree file \"%s\": ", filename);
    DB->cur_btree_file = estrdup(filename);
    printlong(fp_tmp, DB->unique_ID, fwrite);
    DB->fp_btree = fp_tmp;
    DB->bt=BTREE_Create(DB->fp_btree,4096);


    /* Create WordData File */
    strcpy(filename, dbname);
    strcat(filename, WORDDATA_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_worddata = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the worddata file \"%s\": ", filename);
    printlong(fp_tmp, DB->unique_ID, fwrite);
    DB->fp_worddata = fp_tmp;
    DB->cur_worddata_file = estrdup(filename);
    DB->worddata=WORDDATA_Open(DB->fp_worddata);

    /* Create Array File */
    strcpy(filename, dbname);
    strcat(filename, ARRAY_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_array = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the array file \"%s\": ", filename);
    printlong(fp_tmp, DB->unique_ID, fwrite);
    DB->cur_array_file = estrdup(filename);
    DB->fp_array = fp_tmp;
    DB->totwords_array = ARRAY_Create(DB->fp_array);
    DB->props_array = ARRAY_Create(DB->fp_array);

    /* Create PreSorted Index File */
    strcpy(filename, dbname);
    strcat(filename, PRESORTED_EXTENSION);

#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_presorted = 1;
#endif

    CreateEmptyFile(filename);
    if (!(DB->fp_presorted = openIndexFILEForWrite(filename)))
        progerrno("Couldn't create the presorted index file \"%s\": ", filename);

    DB->cur_presorted_file = estrdup(filename);
    printlong(DB->fp_presorted, DB->unique_ID, fwrite);

#endif

    efree(filename);


    for (i = 0; i < MAXCHARS; i++)
        DB->offsets[i] = 0L;

#ifndef USE_BTREE
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        DB->hashoffsets[i] = 0L;
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        DB->lasthashval[i] = 0L;
#endif




    /* Reserve space for offset pointers */
    DB->offsetstart = ftell(DB->fp);
    for (i = 0; i < MAXCHARS; i++)
        printlong(DB->fp, (long) 0, fwrite);

#ifndef USE_BTREE
    DB->hashstart = ftell(DB->fp);
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        printlong(DB->fp, (long) 0, fwrite);
#endif

    return (void *) DB;
}


/*******************************************************************
*   DB_Open_Native
*
*******************************************************************/

void   *DB_Open_Native(SWISH *sw, char *dbname,int mode)
{
    struct Handle_DBNative *DB;
    int     i;
    FILE   *(*openRoutine)(char *) = NULL;
    char   *s;
#ifdef USE_BTREE
    FILE *fp_tmp;
#endif

    switch(mode)
    {
    case DB_READ:
        openRoutine = openIndexFILEForRead;
        break;
    case DB_READWRITE:
        openRoutine = openIndexFILEForReadAndWrite;
        break;
    default:
        openRoutine = openIndexFILEForRead;
    }

    DB = (struct Handle_DBNative *) newNativeDBHandle(sw, dbname);
    DB->mode = mode;

    /* Open index File */
    if (!(DB->fp = openRoutine(dbname)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Could not open the index file '%s': ", dbname);
        return (void *) DB;
    }

    DB->cur_index_file = estrdup(dbname);

    s = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PROPFILE_EXTENSION);

    if (!(DB->prop = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the property file \"%s\": ", s);
        return (void *) DB;
    }

    DB->cur_prop_file = s;

#ifdef USE_BTREE
    
    s = emalloc(strlen(dbname) + strlen(BTREE_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, BTREE_EXTENSION);

    if (!(fp_tmp = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the btree file \"%s\": ", s);
        return (void *) DB;
    }
        


    DB->fp_btree = fp_tmp;      
    DB->cur_btree_file = s;

    s = emalloc(strlen(dbname) + strlen(PRESORTED_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PRESORTED_EXTENSION);

    if (!(DB->fp_presorted = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the presorted index file \"%s\": ", s);
        return (void *) DB;
    }
        

    DB->cur_presorted_file = s;
    

    
    s = emalloc(strlen(dbname) + strlen(WORDDATA_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, WORDDATA_EXTENSION);

    if (!(fp_tmp = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the worddata file \"%s\": ", s);
        return (void *) DB;
    }
        

    DB->fp_worddata = fp_tmp;
    DB->cur_worddata_file = s;

    s = emalloc(strlen(dbname) + strlen(ARRAY_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, ARRAY_EXTENSION);

    if (!(fp_tmp = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the array file \"%s\": ", s);
        return (void *) DB;
    }
          
    DB->fp_array = fp_tmp;
    DB->cur_array_file = s;

    s = emalloc(strlen(dbname) + strlen(PRESORTED_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PRESORTED_EXTENSION);

    if (!(DB->fp_presorted = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, DB->sw, "Couldn't open the presorted index file \"%s\": ", s);
        return (void *) DB;
    }

    DB->cur_presorted_file = s;
    
#endif

    /* Validate index files */
    DB_CheckHeader(DB);
    if ( DB->sw->lasterror )
        return (void *) DB;

    /* Read offsets lookuptable */
    DB->offsetstart = ftell(DB->fp);
    for (i = 0; i < MAXCHARS; i++)
        DB->offsets[i] = readlong(DB->fp, fread);

#ifndef USE_BTREE
    /* Read hashoffsets lookuptable */
    DB->hashstart = ftell(DB->fp);
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        DB->hashoffsets[i] = readlong(DB->fp, fread);
#else
    DB->bt = BTREE_Open(DB->fp_btree,4096,DB->offsets[WORDPOS]);
    DB->worddata = WORDDATA_Open(DB->fp_worddata);
    DB->totwords_array = ARRAY_Open(DB->fp_array,DB->offsets[TOTALWORDSPERFILEPOS]);
    DB->props_array = ARRAY_Open(DB->fp_array,DB->offsets[FILEOFFSETPOS]);

    /* Put the file pointer of props file at the end of the file 
    ** This is very important because if we are in update mode
    ** we must avoid the properties to be overwritten
    */
    fseek(DB->prop,0,SEEK_END);
#endif

    return (void *) DB;
}

/****************************************************************
* This closes a file, and will rename if flagged as such
*  Frees the associated current file name
*
*****************************************************************/

static void DB_Close_File_Native(FILE ** fp, char **filename, int *tempflag)
{
    if (!*fp)
        return;

    if (fclose(*fp))
        progerrno("Failed to close file '%s': ", *filename);

    *fp = NULL;

#ifdef USE_TEMPFILE_EXTENSION
    if (*tempflag)
    {
        char   *newname = estrdup(*filename);

        newname[strlen(newname) - strlen(USE_TEMPFILE_EXTENSION)] = '\0';

#if defined(_WIN32) || defined (__VMS)
        if (isfile(newname))
            if (remove(newname))
                progerrno("Failed to unlink '%s' before renaming. : ", newname);
#endif

        if (rename(*filename, newname))
            progerrno("Failed to rename '%s' to '%s' : ", *filename, newname);


#ifdef INDEXPERMS
        chmod(newname, INDEXPERMS);
#endif

        *tempflag = 0;          /* no longer opened as a temporary file */
        efree(newname);
    }

#else

#ifdef INDEXPERMS
    chmod(*filename, INDEXPERMS);
#endif

#endif /* USE_TEMPFILE_EXTENTION */

    efree(*filename);
    *filename = NULL;
}




void    DB_Close_Native(void *db)
{
    int     i;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    /* Close (and rename) property file, if it's open */
    DB_Close_File_Native(&DB->prop, &DB->cur_prop_file, &DB->tmp_prop);

#ifdef USE_BTREE
    /* Close (and rename) array file, if it's open */
    if(DB->fp_array)
    {
        if(DB->totwords_array)
        {
            DB->offsets[TOTALWORDSPERFILEPOS] = ARRAY_Close(DB->totwords_array);
            DB->totwords_array = NULL;
        }
        if(DB->props_array)
        {
            DB->offsets[FILEOFFSETPOS] = ARRAY_Close(DB->props_array);
            DB->props_array = NULL;
        }
        DB_Close_File_Native(&DB->fp_array, &DB->cur_array_file, &DB->tmp_array);
    }
    /* Close (and rename) worddata file, if it's open */
    if(DB->worddata)
    {
        WORDDATA_Close(DB->worddata);
        DB_Close_File_Native(&DB->fp_worddata, &DB->cur_worddata_file, &DB->tmp_worddata);
        DB->worddata = NULL;
    }
    /* Close (and rename) btree file, if it's open */
    if(DB->bt)
    {
        DB->offsets[WORDPOS] = BTREE_Close(DB->bt);
        DB_Close_File_Native(&DB->fp_btree, &DB->cur_btree_file, &DB->tmp_btree);
        DB->bt = NULL;
    }

    /* Close (and rename) presorted index file, if it's open */
    if(DB->fp_presorted)
    {
        DB_Close_File_Native(&DB->fp_presorted, &DB->cur_presorted_file, &DB->tmp_presorted);
    }
    if(DB->presorted_array)
    {
        for(i = 0; i < DB->n_presorted_array; i++)
        {
            if(DB->presorted_array[i])
                ARRAY_Close(DB->presorted_array[i]);
            DB->presorted_array[i] = NULL;
        }
        efree(DB->presorted_array);
    }
    if(DB->presorted_root_node)
        efree(DB->presorted_root_node);
    if(DB->presorted_propid)
        efree(DB->presorted_propid);
#endif

    if (DB->mode == DB_CREATE)     /* If we are indexing update offsets to words and files */
    {
        /* Update internal pointers */

        fseek(fp, DB->offsetstart, 0);
        for (i = 0; i < MAXCHARS; i++)
            printlong(fp, DB->offsets[i], fwrite);

#ifndef USE_BTREE
        fseek(fp, DB->hashstart, 0);
        for (i = 0; i < VERYBIGHASHSIZE; i++)
            printlong(fp, DB->hashoffsets[i], fwrite);
#endif
    }

    /* Close (and rename) the index file */
    DB_Close_File_Native(&DB->fp, &DB->cur_index_file, &DB->tmp_index);


    if (DB->dbname)
        efree(DB->dbname);
    efree(DB);
}

void    DB_Remove_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;


    /* this is currently not used */
    /* $$$ remove the prop file too */
    fclose(DB->fp);
    remove(DB->dbname);
    efree(DB->dbname);
    efree(DB);
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Header stuff                  */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteHeader_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    if(DB->offsets[HEADERPOS])
    {
        /* If DB->offsets[HEADERPOS] is not 0 we are in update mode 
        ** So, put the pointer file in the header start position to overwrite
        ** the header 
        */
        fseek(DB->fp,DB->offsets[HEADERPOS],SEEK_SET);
    }
    else
    {
        /* The index file is being created. So put the header in the
        ** current file position (coincides with the end of the file
        */
        DB->offsets[HEADERPOS] = ftell(DB->fp);
    }
    return 0;
}


int     DB_EndWriteHeader_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    /* End of header delimiter */
    putc(0, fp);

    return 0;
}

int     DB_WriteHeaderData_Native(int id, unsigned char *s, int len, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    FILE   *fp = DB->fp;

    compress1(id, fp, fputc);
    compress1(len, fp, fputc);
    fwrite(s, len, sizeof(char), fp);

    return 0;
}


int     DB_InitReadHeader_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    fseek(DB->fp, DB->offsets[HEADERPOS], 0);
    return 0;
}

int     DB_ReadHeaderData_Native(int *id, unsigned char **s, int *len, void *db)
{
    int     tmp;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    tmp = uncompress1(fp, fgetc);
    *id = tmp;
    if (tmp)
    {
        tmp = uncompress1(fp, fgetc);
        *s = (unsigned char *) emalloc(tmp + 1);
        *len = tmp;
        fread(*s, *len, sizeof(char), fp);
    }
    else
    {
        len = 0;
        *s = NULL;
    }
    return 0;
}

int     DB_EndReadHeader_Native(void *db)
{
    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*                 Word Stuff                 */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteWords_Native(void *db)
{

#ifndef USE_BTREE
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    DB->offsets[WORDPOS] = ftell(DB->fp);
#endif

    return 0;
}

int     cmp_wordhashdata(const void *s1, const void *s2)
{
    int    *i = (int *) s1;
    int    *j = (int *) s2;

    return (*i - *j);
}

int     DB_EndWriteWords_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    FILE   *fp = (FILE *) DB->fp;
    int     i,
            wordlen;
    long    wordID,
            f_hash_offset,
            f_offset,
            word_pos;
#else
    FILE   *fp_tmp;
#endif

#ifdef USE_BTREE

    /* If we close the BTREE here we can save some memory bytes */
    /* Close (and rename) worddata file, if it's open */
    fp_tmp =DB->worddata->fp;
    WORDDATA_Close(DB->worddata);
    DB->worddata=NULL;
    DB_Close_File_Native(&fp_tmp, &DB->cur_worddata_file, &DB->tmp_worddata);

    fp_tmp = DB->bt->fp;
    DB->offsets[WORDPOS] = BTREE_Close(DB->bt);
    DB->bt = NULL;
    DB_Close_File_Native(&fp_tmp, &DB->cur_btree_file, &DB->tmp_btree);

    /* Restore file pointer at the end of file */
    fseek(DB->fp, 0, SEEK_END);
#else

    /* Free hash zone */
    Mem_ZoneFree(&DB->hashzone);

    /* Now update word's data offset into the list of words */
    /* Simple check  words and worddata must match */

    if (DB->num_words != DB->wordhash_counter)
        progerrno("Internal DB_native error - DB->num_words != DB->wordhash_counter: ");

    if (DB->num_words != DB->worddata_counter)
        progerrno("Internal DB_native error - DB->num_words != DB->worddata_counter: ");

    /* Sort wordhashdata to be writte to allow sequential writes */
    swish_qsort(DB->wordhashdata, DB->num_words, 3 * sizeof(long), cmp_wordhashdata);

    if (WRITE_WORDS_RAMDISK)
    {
        fp = (FILE *) DB->rd;
    }
    for (i = 0; i < DB->num_words; i++)
    {
        wordID = DB->wordhashdata[3 * i];
        f_hash_offset = DB->wordhashdata[3 * i + 1];
        f_offset = DB->wordhashdata[3 * i + 2];

        word_pos = wordID;
        if (WRITE_WORDS_RAMDISK)
        {
            word_pos -= DB->offsets[WORDPOS];
        }
        /* Position file pointer in word */
        DB->w_seek(fp, word_pos, SEEK_SET);
        /* Jump over word length and word */
        wordlen = uncompress1(fp, DB->w_getc); /* Get Word length */
        DB->w_seek(fp, (long) wordlen, SEEK_CUR); /* Jump Word */
        /* Write offset to next chain */
        printlong(fp, f_hash_offset, DB->w_write);
        /* Write offset to word data */
        printlong(fp, f_offset, DB->w_write);
    }

    efree(DB->wordhashdata);
    DB->wordhashdata = NULL;
    DB->worddata_counter = 0;
    DB->wordhash_counter = 0;

    if (WRITE_WORDS_RAMDISK)
    {
        unsigned char buffer[4096];
        long    ramdisk_size;
        long    read = 0;

        ramdisk_seek((FILE *) DB->rd, 0, SEEK_END);
        ramdisk_size = ramdisk_tell((FILE *) DB->rd);
        /* Write ramdisk to fp end free it */
        fseek((FILE *) DB->fp, DB->offsets[WORDPOS], SEEK_SET);
        ramdisk_seek((FILE *) DB->rd, 0, SEEK_SET);
        while (ramdisk_size)
        {
            read = ramdisk_read(buffer, 4096, 1, (FILE *) DB->rd);
            fwrite(buffer, read, 1, DB->fp);
            ramdisk_size -= read;
        }
        ramdisk_close((FILE *) DB->rd);
    }
    /* Get last word file offset - For the last word, this will be
    ** used to delimite the last word in the index file
    ** In other words. This is the file offset where no more words
    ** are added.
    */
    DB->offsets[ENDWORDPOS] = ftell(DB->fp);

    /* Restore file pointer at the end of file */
    fseek(DB->fp, 0, SEEK_END);
    fputc(0, DB->fp);           /* End of words mark */

#endif

    return 0;
}

#ifndef USE_BTREE
long    DB_GetWordID_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;
    long    pos = 0;

    if (WRITE_WORDS_RAMDISK)
    {
        if (!DB->rd)
        {
            /* ramdisk size as suggested by Bill Meier */
            DB->rd = ramdisk_create("RAM Disk: write words", 32 * 4096);
        }
        pos = DB->offsets[WORDPOS];
        fp = (FILE *) DB->rd;
    }
    pos += DB->w_tell(fp);

    return pos;                 /* Native database uses position as a Word ID */
}

int     DB_WriteWord_Native(char *word, long wordID, void *db)
{
    int     i,
            wordlen;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    FILE   *fp = DB->fp;

    i = (int) ((unsigned char) word[0]);

    if (!DB->offsets[i])
        DB->offsets[i] = wordID;


    /* Write word length, word and a NULL offset */
    wordlen = strlen(word);

    if (WRITE_WORDS_RAMDISK)
    {
        fp = (FILE *) DB->rd;
    }
    compress1(wordlen, fp, DB->w_putc);
    DB->w_write(word, wordlen, sizeof(char), fp);

    printlong(fp, (long) 0, DB->w_write); /* hash chain */
    printlong(fp, (long) 0, DB->w_write); /* word's data pointer */

    DB->num_words++;

    return 0;
}

long    DB_WriteWordData_Native(long wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;
    struct numhash *numhash;
    int     numhashval;

    /* We must be at the end of the file */

    if (!DB->worddata_counter)
    {
        /* We are starting writing worddata */
        /* If inside a ramdisk we must preserve its space */
        if (WRITE_WORDS_RAMDISK)
        {
            long    ramdisk_size;

            ramdisk_seek((FILE *) DB->rd, 0, SEEK_END);
            ramdisk_size = ramdisk_tell((FILE *) DB->rd);
            /* Preserve ramdisk size in DB file  */
            /* it will be write later */
            fseek((FILE *) DB->fp, ramdisk_size, SEEK_END);
        }
    }
    /* Search for word's ID */
    numhashval = bignumhash(wordID);
    for (numhash = DB->hash[numhashval]; numhash; numhash = numhash->next)
        if (DB->wordhashdata[3 * numhash->index] == wordID)
            break;
    if (!numhash)
        progerrno("Internal db_native.c error in DB_WriteWordData_Native: ");
    DB->wordhashdata[3 * numhash->index + 2] = ftell(fp);

    DB->worddata_counter++;

    /* Write the worddata to disk */
    /* Write in the form:  <data_size><saved_bytes><worddata> */
    /* If there is not any compression then saved_bytes is 0 */
    compress1(data_size, fp, fputc);
    compress1(saved_bytes, fp, fputc);
    fwrite(worddata, data_size, 1, fp);

    /* A NULL byte to indicate end of word data */
    fputc(0, fp);

    return 0;
}

#else

long    DB_GetWordID_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    return DB->worddata->lastid;
}

int     DB_WriteWord_Native(char *word, long wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    BTREE_Insert(DB->bt, (unsigned char *)word, strlen(word), (unsigned long) wordID);

    DB->num_words++;

    return 0;
}

int     DB_UpdateWordID_Native(char *word, long new_wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    BTREE_Update(DB->bt, (unsigned char *)word, strlen(word), (unsigned long) new_wordID);

    return 0;
}

int     DB_DeleteWordData_Native(long wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    int dummy;

    WORDDATA_Del(DB->worddata, wordID, &dummy);

    return 0;
}

long    DB_WriteWordData_Native(long wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db)
{
    unsigned char stack_buffer[8192]; /* just to avoid emalloc,efree overhead */
    unsigned char *buf, *p;
    int buf_size;

    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    /* WORDDATA_Put requires only 2 values (size and data). So
    ** we need to pack the saved bytes into data
    */

    /* Get total size */
    buf_size = data_size + sizeofcompint(saved_bytes);

    if(buf_size > sizeof(stack_buffer))
        buf = (unsigned char *) emalloc(buf_size);
    else
        buf = stack_buffer;

    /* Put saved_bytes in buf */
    p = compress3(saved_bytes, buf);
    /* Put bytes worddata buf */
    memcpy(p,worddata,data_size);

    DB->worddata_counter++;

    /* Write the worddata to disk */
    WORDDATA_Put(DB->worddata,buf_size,buf);

    if(buf != stack_buffer)
        efree(buf);
    return 0;
}

#endif

#ifndef USE_BTREE
int     DB_WriteWordHash_Native(char *word, long wordID, void *db)
{
    int     i,
            hashval,
            numhashval;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    struct numhash *numhash;

    if (!DB->wordhash_counter)
    {
        /* Init hash array */
        for (i = 0; i < BIGHASHSIZE; i++)
            DB->hash[i] = NULL;
        DB->hashzone = Mem_ZoneCreate("WriteWordHash", DB->num_words * sizeof(struct numhash), 0);

        /* If we are here we have finished WriteWord_Native */
        /* If using ramdisk - Reserve space upto the size of the ramdisk */
        if (WRITE_WORDS_RAMDISK)
        {
            long    ram_size = DB->w_seek((FILE *) DB->rd, 0, SEEK_END);

            fseek(DB->fp, ram_size, SEEK_SET);
        }

        DB->wordhashdata = emalloc(3 * DB->num_words * sizeof(long));
    }

    hashval = verybighash(word);

    if (!DB->hashoffsets[hashval])
    {
        DB->hashoffsets[hashval] = wordID;
    }

    DB->wordhashdata[3 * DB->wordhash_counter] = wordID;
    DB->wordhashdata[3 * DB->wordhash_counter + 1] = (long) 0;


    /* Add to the hash */
    numhash = (struct numhash *) Mem_ZoneAlloc(DB->hashzone, sizeof(struct numhash));

    numhashval = bignumhash(wordID);
    numhash->index = DB->wordhash_counter;
    numhash->next = DB->hash[numhashval];
    DB->hash[numhashval] = numhash;

    DB->wordhash_counter++;

    /* Update previous word in hashlist */
    if (DB->lasthashval[hashval])
    {
        /* Search for DB->lasthashval[hashval] */
        numhashval = bignumhash(DB->lasthashval[hashval]);
        for (numhash = DB->hash[numhashval]; numhash; numhash = numhash->next)
            if (DB->wordhashdata[3 * numhash->index] == DB->lasthashval[hashval])
                break;
        if (!numhash)
            progerrno("Internal db_native.c error in DB_WriteWordHash_Native: ");
        DB->wordhashdata[3 * numhash->index + 1] = (long) wordID;
    }
    DB->lasthashval[hashval] = wordID;

    return 0;
}
#endif

int     DB_InitReadWords_Native(void *db)
{
    return 0;
}

int     DB_EndReadWords_Native(void *db)
{
    return 0;
}

#ifndef USE_BTREE
int     DB_ReadWordHash_Native(char *word, long *wordID, void *db)
{
    int     wordlen,
            res,
            hashval;
    long    offset,
            dataoffset;
    char   *fileword = NULL;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;


    /* If there is not a star use the hash approach ... */
    res = 1;

    /* Get hash file offset */
    hashval = verybighash(word);
    if (!(offset = DB->hashoffsets[hashval]))
    {
        *wordID = 0;
        return 0;
    }
    /* Search for word */
    while (res)
    {
        /* Position in file */
        fseek(fp, offset, SEEK_SET);
        /* Get word */
        wordlen = uncompress1(fp, fgetc);
        fileword = emalloc(wordlen + 1);
        fread(fileword, 1, wordlen, fp);
        fileword[wordlen] = '\0';
        offset = readlong(fp, fread); /* Next hash */
        dataoffset = readlong(fp, fread); /* Offset to Word data */

        res = strcmp(word, fileword);
        efree(fileword);

        if (!res)
            break;              /* Found !! */
        else if (!offset)
        {
            dataoffset = 0;
            break;
        }
    }
    *wordID = dataoffset;
    return 0;
}

int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    int     wordlen,
            i,
            res,
            len,
            found;
    long    dataoffset = 0;
    char   *fileword = NULL;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;


    len = strlen(word);

    i = (int) ((unsigned char) word[0]);

    if (!DB->offsets[i])
    {
        *resultword = NULL;
        *wordID = 0;
        return 0;
    }
    found = 1;
    fseek(fp, DB->offsets[i], 0);

    /* Look for first occurrence */
    wordlen = uncompress1(fp, fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    while (wordlen)
    {
        fread(fileword, 1, wordlen, fp);
        fileword[wordlen] = '\0';
        readlong(fp, fread);    /* jump hash offset */
        dataoffset = readlong(fp, fread); /* Get offset to word's data */
        if (!(res = strncmp(word, fileword, len))) /*Found!! */
        {
            DB->nextwordoffset = ftell(fp); /* preserve next word pos */
            break;
        }
        if (res < 0)
        {
            dataoffset = 0;
            break;
        }
        /* Go to next value */
        wordlen = uncompress1(fp, fgetc); /* Next word */
        if (!wordlen)
        {
            dataoffset = 0;
            break;
        }
        efree(fileword);
        fileword = (char *) emalloc(wordlen + 1);
    }
    if (!dataoffset)
    {
        efree(fileword);
        *resultword = NULL;
    }
    else
        *resultword = fileword;
    *wordID = dataoffset;

    return 0;
}

int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    int     len,
            wordlen;
    long    dataoffset;
    char   *fileword;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    /* Check for end of words */
    if (!DB->nextwordoffset || DB->nextwordoffset == DB->offsets[ENDWORDPOS])
    {
        *resultword = NULL;
        *wordID = 0;
        return 0;
    }

    len = strlen(word);


    fseek(fp, DB->nextwordoffset, SEEK_SET);

    wordlen = uncompress1(fp, fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    fread(fileword, 1, wordlen, fp);
    fileword[wordlen] = '\0';
    if (strncmp(word, fileword, len))
    {
        efree(fileword);
        fileword = NULL;
        dataoffset = 0;         /* No more data */
        DB->nextwordoffset = 0;
    }
    else
    {
        readlong(fp, fread);    /* jump hash offset */
        dataoffset = readlong(fp, fread); /* Get data offset */
        DB->nextwordoffset = ftell(fp);
    }
    *resultword = fileword;
    *wordID = dataoffset;

    return 0;

}


long    DB_ReadWordData_Native(long wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db)
{
    unsigned char *buffer;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    fseek(fp, wordID, 0);
    *data_size = uncompress1(fp, fgetc);
    *saved_bytes = uncompress1(fp, fgetc);
    buffer = emalloc(*data_size);
    fread(buffer, *data_size, 1, fp);

    *worddata = buffer;

    return 0;
}


#else
int     DB_ReadWordHash_Native(char *word, long *wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    unsigned char *dummy;
    int dummy2;

    if((*wordID = (long)BTREE_Search(DB->bt,word,strlen(word),&dummy,&dummy2,1)) < 0)
        *wordID = 0;
    else
        efree(dummy);
    return 0;
}

int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    unsigned char *found;
    int found_len;


    if((*wordID = (long)BTREE_Search(DB->bt,word,strlen(word), &found, &found_len, 0)) < 0)
    {
        *resultword = NULL;
        *wordID = 0;
    }
    else
    {

        *resultword = emalloc(found_len + 1);
        memcpy(*resultword,found,found_len);
        (*resultword)[found_len]='\0';
        efree(found);
        if (strncmp(word, *resultword, strlen(word))>0)
            return DB_ReadNextWordInvertedIndex_Native(word, resultword, wordID, db);
    }

    return 0;
}

int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, long *wordID, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    unsigned char *found;
    int found_len;

    if((*wordID = (long)BTREE_Next(DB->bt, &found, &found_len)) < 0)
    {
        *resultword = NULL;
        *wordID = 0;
    }
    else
    {
        *resultword = emalloc(found_len + 1);
        memcpy(*resultword,found,found_len);
        (*resultword)[found_len]='\0';
        efree(found);
        if (strncmp(word, *resultword, strlen(word)))
        {
            efree(*resultword);
            *resultword = NULL;
            *wordID = 0;         /* No more data */
        }
    }
    return 0;
}

long    DB_ReadWordData_Native(long wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db)
{
    unsigned char *buf;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    *worddata = buf = WORDDATA_Get(DB->worddata,wordID,data_size);
    /* Get saved_bytes and adjust data_size */
    *saved_bytes = uncompress2(&buf);
    *data_size -= (buf - (*worddata));
    /* Remove saved_bytes from buffer 
    ** We need to use memmove because data overlaps */
    memmove(*worddata,buf, *data_size);

    return 0;
}

#endif


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              FileList  Stuff               */
/*--------------------------------------------*/
/*--------------------------------------------*/


int     DB_EndWriteFiles_Native(void *db)
{
    return 0;
}

int     DB_WriteFile_Native(int filenum, unsigned char *filedata, int sz_filedata, void *db)
{
    return 0;
}

int     DB_InitReadFiles_Native(void *db)
{
    return 0;
}

int     DB_ReadFile_Native(int filenum, unsigned char **filedata, int *sz_filedata, void *db)
{
    return 0;
}


int     DB_EndReadFiles_Native(void *db)
{

    return 0;
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Sorted data Stuff             */
/*--------------------------------------------*/
/*--------------------------------------------*/

#ifdef USE_BTREE
int     DB_InitWriteSortedIndex_Native(void *db, int n_props)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp_presorted;
   int i;

   DB->offsets[SORTEDINDEX] = ftell(fp);

   /* Write number of properties */
   printlong(fp,(unsigned long) n_props, fwrite);

   DB->n_presorted_array = n_props;
   DB->presorted_array = (ARRAY **)emalloc(n_props * sizeof(ARRAY *));
   DB->presorted_root_node = (unsigned long *)emalloc(n_props * sizeof(unsigned long));
   for(i = 0; i < n_props ; i++)
   {
       DB->presorted_array[i] = NULL;
       DB->presorted_root_node[i] = 0;
       /* Reserve space for propidx and Array Pointer */
       printlong(fp,(unsigned long) 0, fwrite);
       printlong(fp,(unsigned long) 0, fwrite);
   }
   DB->next_sortedindex = 0;
   return 0;
}

int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data, void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp_presorted;
   ARRAY *arr;
   int i;
   int *num = (int *)data;
   int n = sz_data/sizeof(int);

   arr = ARRAY_Create(fp);
   for(i = 0 ; i < n ; i++)
   {
       ARRAY_Put(arr,i,num[i]);
/*
       if(!(i%10000))
       {
            ARRAY_FlushCache(arr);
            printf("%d %d            \r",propID,i);
       }
*/
   }

   DB->presorted_root_node[DB->next_sortedindex] = ARRAY_Close(arr);

   fseek(fp,DB->offsets[SORTEDINDEX] + (1 + 2 * DB->next_sortedindex) *sizeof(unsigned long),SEEK_SET);
   printlong(fp,(unsigned long) propID, fwrite);
   printlong(fp,(unsigned long) DB->presorted_root_node[DB->next_sortedindex], fwrite);

   DB->next_sortedindex++;

   return 0;
}

int     DB_EndWriteSortedIndex_Native(void *db)
{
   return 0;
}

 
int     DB_InitReadSortedIndex_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp_presorted;
   int i;

   fseek(fp,DB->offsets[SORTEDINDEX],SEEK_SET);

   /* Read number of properties */
   DB->n_presorted_array = readlong(fp,fread);

   DB->presorted_array = (ARRAY **)emalloc(DB->n_presorted_array * sizeof(ARRAY *));
   DB->presorted_root_node = (unsigned long *)emalloc(DB->n_presorted_array * sizeof(unsigned long));
   DB->presorted_propid = (unsigned long *)emalloc(DB->n_presorted_array * sizeof(unsigned long));
   for(i = 0; i < DB->n_presorted_array ; i++)
   {
       DB->presorted_array[i] = NULL;
       DB->presorted_propid[i] = readlong(fp,fread);
       DB->presorted_root_node[i] = readlong(fp,fread);
   }
   return 0;

}

int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp_presorted;
   int i;

   if(!DB->cur_presorted_array || DB->cur_presorted_propid != (unsigned long)propID)
   {
       for(i = 0; i < DB->n_presorted_array ; i++)
       {
           if((unsigned long)propID == DB->presorted_propid[i])
           {
               DB->cur_presorted_propid = propID;
               DB->cur_presorted_array = DB->presorted_array[i] = ARRAY_Open(fp,DB->presorted_root_node[i]);
               break;
           }
       }
   }
   if(DB->cur_presorted_array)
   {
       *data = (unsigned char *)DB->cur_presorted_array;
       *sz_data = sizeof(DB->cur_presorted_array);
   }
   else
   {
       *data = NULL;
       *sz_data = 0;
   }

   return 0;
}


int     DB_ReadSortedData_Native(int *data,int index, int *value, void *db)
{
    *value = ARRAY_Get((ARRAY *)data,index);
    return 0;
}

int     DB_EndReadSortedIndex_Native(void *db)
{
   return 0;
}

#else
int     DB_InitWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   DB->offsets[SORTEDINDEX] = ftell(DB->fp);
   DB->next_sortedindex = DB->offsets[SORTEDINDEX];
   return 0;
}

int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data,void *db)
{
   long tmp1,tmp2;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;


    fseek(fp, DB->next_sortedindex, 0);


   tmp1 = ftell(fp);

   printlong(fp,(long)0,fwrite);  /* Pointer to next table if any */

    /* Write ID */
   compress1(propID,fp,fputc);

   /* Write len of data */
   compress1(sz_data,fp,putc);

   /* Write data */
   fwrite(data,sz_data,1,fp);

   DB->next_sortedindex = tmp2 = ftell(fp);


   if(DB->lastsortedindex)
   {
       fseek(fp,DB->lastsortedindex,0);
       printlong(fp,tmp1,fwrite);
       fseek(fp,tmp2,0);
   }
   DB->lastsortedindex = tmp1;
   return 0;
}

int     DB_EndWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   printlong(fp,(long)0,fwrite);  /* No next table mark - Useful if no presorted indexes */
         /* NULL meta id- Only useful if no presorted indexes  */
   fputc(0, fp);

   return 0;
}

 
int     DB_InitReadSortedIndex_Native(void *db)
{
   return 0;
}

int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db)
{
   long next, id, tmp;
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
   FILE *fp = DB->fp;

   fseek(fp,DB->offsets[SORTEDINDEX],0);


   next = readlong(fp,fread);
        /* read propID */
   id = uncompress1(fp,fgetc);


   while(1)
   {
       if(id == propID)
       {
           tmp = uncompress1(fp,fgetc);
           *sz_data = tmp;

           *data = emalloc(*sz_data);
           fread(*data,*sz_data,1,fp);
           return 0;
       }
       if(next)
       {
           fseek(fp,next,0);
           next = readlong(fp,fread);
           id = uncompress1(fp,fgetc);
       }
       else
       {
           *sz_data = 0;
           *data = NULL;
           return 0;
       }
   }
   return 0;
}

int     DB_ReadSortedData_Native(int *data,int index, int *value, void *db)
{
    *value = data[index];
    return 0;
}

int     DB_EndReadSortedIndex_Native(void *db)
{
   return 0;
}


#endif




/* 
** Jose Ruiz 04/00
** Store a portable long in a portable format
*/
void    printlong(FILE * fp, unsigned long num, size_t(*f_write) (const void *, size_t, size_t, FILE *))
{
    num = PACKLONG(num);        /* Make the number portable */
    f_write(&num, sizeof(long), 1, fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long from a portable format
*/
unsigned long readlong(FILE * fp, size_t(*f_read) (void *, size_t, size_t, FILE *))
{
    unsigned long num;

    f_read(&num, sizeof(long), 1, fp);
    return UNPACKLONG(num);     /* Make the number readable */
}



/****************************************************************************
*   Writing Properites  (not for USE_BTREE)
*
*   Properties are written sequentially to the .prop file.
*   Fixed length records of the seek position into the
*   property file are written sequentially to the main index (which is why
*   there's a separate .prop file).  
*
*   DB_InitWriteFiles is called first time a property is written
*   to save the offset of the property index table in the main index.
*   It's simply a ftell() of the current position in the index and that
*   seek position is stored in the main index "offsets" table.
*
*   DB_WriteProperty writes a property.
*
*   DB_WritePropPositions write the seek pointers to the main index and
*   *must* be called after processing each file.
*   This is all done in WritePropertiesToDisk().
*
*   The index tables are all based on the count of properties in the index.
*   So, to read you find the start of the prop pointers table by the value
*   stored in the offsets table.  Since we have a fixed number of properties
*   we know the size of an entry in the prop pointers table, one record per filenum.
*   Index into the prop seek positions table and grab the pointers to the properties.
*
*
*****************************************************************************/


int     DB_InitWriteFiles_Native(void *db)
{
#ifndef USE_BTREE
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    DB->offsets[FILELISTPOS] = ftell(DB->fp);

#ifdef DEBUG_PROP
    printf("InitWriteFiles: Start of property table in main index at offset: %ld\n", DB->offsets[FILELISTPOS] );
#endif

#endif

    return 0;
}


/****************************************************************************
*   Writes a property to the property file
*
*   Creates a PROP_INDEX structure in the file entry that caches all
*   the seek pointers into the .prop file, if it doesn't already exist.
*
*   Stores in the fi->prop_index structure the seek address of this property
*
*   Writes to the prop file:
*       <compressed length><saved_bytes><property (possibly compressed)>
*
*   compressed_length: length of the compressed property. If no compression was
*                      made this value is the real length
*   saved_bytes: number of bytes saved by the compression. If no compression
*                was made this value is 0.
*   property: buffer array containg the property (possibly compressed)
*
*   Important note: On entry to this routine, if uncompressed_len is zero, then
*                   no compression was made. So, saved_bytes is adjusted to zero
*
*****************************************************************************/

void    DB_WriteProperty_Native( IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    size_t             written_bytes;
    PROP_INDEX      *pindex = fi->prop_index;
    PROP_LOCATION   *prop_loc;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             propIDX = header->metaID_to_PropIDX[propID];
#ifdef DEBUG_PROP
    long            prop_start_pos;
#endif
    int             saved_bytes;

    if ( count <= 0 )
        return;
    

    if (!DB->prop)
        progerr("Property database file not opened\n");


    /* Create place to store seek positions on first call for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }


    /* make an alias */
    prop_loc = &pindex->prop_position[ propIDX ];


    /* Just to be sure */
    if ( !buf_len )
    {
        prop_loc->seek = 0;
        return;
    }

    /* write the property to disk */

    if ((prop_loc->seek = ftell(DB->prop)) == -1)
        progerrno("O/S failed to tell me where I am - file number %d metaID %d : ", fi->filenum, propID);


    /* First write the compressed size */
    /* should be smaller than uncompressed_len if any */
    /* NOTE: uncompressed_len is 0 if no compression was made */
    compress1( buf_len, DB->prop, putc);

    /* Second write the number of saved bytes */
    if( !uncompressed_len )   /* No compression */
        saved_bytes = 0;    
    else
        saved_bytes = uncompressed_len - buf_len;
    /* Write them */
    compress1( saved_bytes, DB->prop, putc);

#ifdef DEBUG_PROP
    prop_start_pos = ftell(DB->prop);
#endif
    


    if ((written_bytes = fwrite(buffer, 1, buf_len, DB->prop)) != buf_len) /* Write data */
        progerrno("Failed to write file number %d metaID %d to property file.  Tried to write %d, wrote %Zu : ", fi->filenum, propID, buf_len,
                  written_bytes);


#ifdef DEBUG_PROP
    printf("Write Prop: file %d  PropIDX %d  (meta %d) seek: %ld ",
                fi->filenum, propIDX, propID, prop_loc->seek );

    printf("data=[uncompressed_len: %d (%ld bytes), prop_data: (%ld bytes)]\n",
            uncompressed_len, prop_start_pos - prop_loc->seek, (long)written_bytes);
#endif
}


/****************************************************************************
*   Writes out the seek positions for the properties
*
*   This writes out a fixed size records, one for each property.  Each
*   record is a list of <seek pos> entries, one for
*   each property defined.  seek_pos is null if this file doesn't have a
*   property.
*
*   The advantage of the fixed width records is that they can be written
*   to disk after each file, saving RAM, and more importanly, all the
*   files don't need to be read when searhing.  Can just seek to the
*   file of interest, read the table, then read the property file.
*
*   This comes at a cost of disk space (and maybe disk access speed),
*   since much of the data in the table written to disk could be compressed.
*
*****************************************************************************/
void DB_WritePropPositions_Native(IndexFILE *indexf, FileRec *fi, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             i;
#ifdef DEBUG_PROP
    long            start_seek;
#endif
#ifdef USE_BTREE
    long            seek_pos;
#endif



    /* Just in case there were no properties for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }

#ifdef USE_BTREE
    /* now calculate index */
    seek_pos = (fi->filenum - 1) * count;
#endif

#ifdef DEBUG_PROP
    printf("Writing seek positions to index for file %d\n", fi->filenum );
#endif


    /* Write out the prop index */
    for ( i = 0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

#ifndef USE_BTREE

#ifdef DEBUG_PROP
        start_seek = ftell( DB->fp );
#endif
        
        /* Write in portable format */
        printlong( DB->fp, prop_loc->seek, fwrite );

#ifdef DEBUG_PROP
        printf("  PropIDX: %d  data=[seek: %ld]  main index location: %ld for %ld bytes (one print long)\n",
                 i,  prop_loc->seek, start_seek, ftell( DB->fp ) - start_seek );
#endif


#else
        ARRAY_Put( DB->props_array,seek_pos++, prop_loc->seek);
#endif
    }

    efree( pindex );
    fi->prop_index = NULL;;
}

/****************************************************************************
*   Reads in the seek positions for the properties
*
*
*****************************************************************************/
void DB_ReadPropPositions_Native(IndexFILE *indexf, FileRec *fi, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    long            seek_pos;
    int             i;

    if ( count <= 0 )
        return;
    

    /* create a place to store them */

    index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);

    pindex = fi->prop_index = emalloc( index_size );
    memset( pindex, 0, index_size );


#ifndef USE_BTREE
    /* now calculate seek_pos */
    /* printlong currently always writes sizeof(long) bytes (usually 4 bytes 
    ** for 32 bit architectures and 8 bytes for 64bit ones, so 4 or 8 bytes
    ** bytes are need for seek
    */
    seek_pos = ((fi->filenum - 1) * sizeof(long) * count)  + DB->offsets[FILELISTPOS];


    /* and seek to table */
    if (fseek(DB->fp, seek_pos, 0) == -1)
        progerrno("Failed to seek to property index located at %ld for file number %d : ", seek_pos, fi->filenum);


#ifdef DEBUG_PROP
        printf("\nFetching seek positions for file %d\n", fi->filenum );
        printf(" property index table at %ld, this file at %ld\n", DB->offsets[FILELISTPOS], seek_pos );
#endif


    /* Read in the prop indexes */
    for ( i=0; i < count; i++ )
    {
#ifdef DEBUG_PROP
        long    seek_start = ftell( DB->fp );
#endif
        
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

        prop_loc->seek = readlong( DB->fp, fread );

#ifdef DEBUG_PROP
        printf("   PropIDX: %d  data[Seek: %ld] at seek %ld read %ld bytes (one readlong)\n", i, prop_loc->seek, seek_start, ftell( DB->fp ) - seek_start  );
#endif


    }
#else

    /* now calculate index */
    seek_pos = (fi->filenum - 1) * count;

    /* Read in the prop indexes */
    for ( i=0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];
        prop_loc->seek = ARRAY_Get(DB->props_array, seek_pos++);
    }
#endif
}

    

/****************************************************************************
*   Reads a property from the property file
*
*   Returns:
*       *char (buffer -- must be destoryed by caller)
*
*   Important note: On returning, *buf_len contains the compressed lenth,
*                   *uncompressed_length contains the real length or 0 if
*                   no compression was made
*
*****************************************************************************/
char   *DB_ReadProperty_Native(IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    long            seek_pos;
    int             propIDX;
    PROP_LOCATION   *prop_loc;
    char            *buffer;
    int             saved_bytes;


    propIDX = header->metaID_to_PropIDX[propID];

    if ( count <= 0 )
        return NULL;


    /* read in the index pointers if not already loaded */
    if ( !pindex )
    {
        DB_ReadPropPositions_Native( indexf, fi, db);
        pindex = fi->prop_index;
    }


    if ( !pindex )
        progerr("Failed to call DB_ReadProperty_Native with seek positions");

    prop_loc = &pindex->prop_position[ propIDX ];

    seek_pos = pindex->prop_position[propIDX].seek;
    
    /* Any for this metaID? */
    if (!seek_pos )
    {
        *buf_len = 0;
        return NULL;
    }


    if (fseek(DB->prop, seek_pos, 0) == -1)
        progerrno("Failed to seek to properties located at %ld for file number %d : ", seek_pos, fi->filenum);

#ifdef DEBUG_PROP
    printf("Fetching filenum: %d propIDX: %d at seek: %ld\n", fi->filenum, propIDX, seek_pos);
#endif


    /* read compressed size (for use in zlib uncompression) */
    *buf_len = uncompress1( DB->prop, fgetc );

    /* Get the uncompressed size */
    saved_bytes = uncompress1( DB->prop, fgetc );
    /* If saved_bytes is 0 there was not any compression */
    if( !saved_bytes )             /* adjust *uncompressed_len */
        *uncompressed_len = 0;    /* This value means no compression */
    else
        *uncompressed_len = *buf_len + saved_bytes;


#ifdef DEBUG_PROP
    printf(" Fetched uncompressed length of %d (%ld bytes storage), now fetching %ld prop bytes from %ld\n",
             *uncompressed_len, ftell( DB->prop ) - seek_pos, *buf_len, ftell( DB->prop ) );
#endif


    /* allocate a read buffer */
    buffer = emalloc(*buf_len);


    if (fread(buffer, 1, *buf_len, DB->prop) != *buf_len)
        progerrno("Failed to read properties located at %ld for file number %d : ", seek_pos, fi->filenum);

    return buffer;
}


/****************************************************************
*  This routine closes the property file and reopens it as
*  readonly to improve seek times.
*  Note: It does not rename the property file.
*****************************************************************/

void    DB_Reopen_PropertiesForRead_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    int     no_rename = 0;
    char   *s = estrdup(DB->cur_prop_file);

    /* Close property file */
    DB_Close_File_Native(&DB->prop, &DB->cur_prop_file, &no_rename);


    if (!(DB->prop = openIndexFILEForRead(s)))
        progerrno("Couldn't open the property file \"%s\": ", s);

    DB->cur_prop_file = s;
}



#ifdef USE_BTREE


int    DB_WriteTotalWordsPerFile_Native(SWISH *sw, int idx, int wordcount, void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   ARRAY_Put(DB->totwords_array,idx,wordcount);

   return 0;
}


int     DB_ReadTotalWordsPerFile_Native(SWISH *sw, int index, int *value, void *db)
{
   struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

   *value = ARRAY_Get((ARRAY *)DB->totwords_array,index);
   return 0;
}


#endif
