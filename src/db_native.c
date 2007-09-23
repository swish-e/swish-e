/*

$Id$

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


**
** 2001-05-07 jmruiz init coding
**
*/

#include <time.h>
#include "swish.h"
#include "mem.h"
#include "file.h"
#include "error.h"
#include "swstring.h"
#include "compress.h"
#include "hash.h"
#include "sw_db.h"
#include "swish_qsort.h"
#include "ramdisk.h"
#include "db_native.h"

#ifdef USE_BTREE
#define WRITE_WORDS_RAMDISK 0
#else
#define WRITE_WORDS_RAMDISK 1
#endif

/* MAX_PATH used by Herman's NEAR feature but it seems to be a Windoze thing 
 * so karman just made this value up so it will compile on *nix
 */
#if !defined(_WIN32)
#define MAX_PATH    255
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

    Db->DB_WriteFileNum = DB_WriteFileNum_Native;
    Db->DB_ReadFileNum = DB_ReadFileNum_Native;
    Db->DB_CheckFileNum = DB_CheckFileNum_Native;
    Db->DB_RemoveFileNum = DB_RemoveFileNum_Native;

    Db->DB_InitWriteSortedIndex = DB_InitWriteSortedIndex_Native;
    Db->DB_WriteSortedIndex = DB_WriteSortedIndex_Native;
    Db->DB_EndWriteSortedIndex = DB_EndWriteSortedIndex_Native;

    Db->DB_InitReadSortedIndex = DB_InitReadSortedIndex_Native;
    Db->DB_ReadSortedIndex = DB_ReadSortedIndex_Native;
    Db->DB_ReadSortedData = DB_ReadSortedData_Native;
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_Native;

    Db->DB_InitWriteProperties = DB_InitWriteProperties_Native;
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





/* Does an index file have a readable format?
*/

static void DB_CheckHeader(struct Handle_DBNative *SW_DB)
{
#ifndef USE_BTREE
    long    swish_magic;
    long prop;

    sw_fseek(SW_DB->fp, (sw_off_t)0, SEEK_SET);
    swish_magic = readlong(SW_DB->fp, sw_fread);

    if (swish_magic != SWISH_MAGIC)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "File \"%s\" has an unknown format.", SW_DB->cur_index_file);
        return;
    }

    SW_DB->unique_ID = readlong(SW_DB->fp, sw_fread);
    prop = readlong(SW_DB->fp_prop, sw_fread);

    if (SW_DB->unique_ID != prop)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "Index file '%s' and property file '%s' are not related.", SW_DB->cur_index_file, SW_DB->cur_prop_file);
        return;
    }
#else
    long propindex, totwords, presorted, header;
    SW_DB->unique_ID = readlong(SW_DB->fp_prop, sw_fread);

    propindex = readlong(SW_DB->fp_propindex, sw_fread);
    if (SW_DB->unique_ID != propindex)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "Properties file '%s' and propindex file '%s' are not related.", SW_DB->cur_prop_file, SW_DB->cur_propindex_file);
        return;
    }

    totwords = readlong(SW_DB->fp_totwords, sw_fread);
    if (SW_DB->unique_ID != totwords)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "Properties file '%s' and totwords file '%s' are not related.", SW_DB->cur_prop_file, SW_DB->cur_totwords_file);
        return;
    }

    presorted = readlong(SW_DB->fp_presorted, sw_fread);

    if (SW_DB->unique_ID != presorted)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "Properties file '%s' and presorted index file '%s' are not related.", SW_DB->cur_prop_file, SW_DB->cur_presorted_file);
        return;
    }

    header = readlong(SW_DB->fp_header, sw_fread);

    if (SW_DB->unique_ID != header)
    {
        set_progerr(INDEX_FILE_ERROR, SW_DB->sw, "Properties file '%s' and header index file '%s' are not related.", SW_DB->cur_prop_file, SW_DB->cur_header_file);
        return;
    }
#endif
}

static struct Handle_DBNative *newNativeDBHandle(SWISH *sw, char *dbname)
{
    struct Handle_DBNative *SW_DB;

    /* Allocate structure */
    SW_DB = (struct Handle_DBNative *) emalloc(sizeof(struct Handle_DBNative));
    memset( SW_DB, 0, sizeof( struct Handle_DBNative ));

    SW_DB->sw = sw;  /* for error messages */

    if (WRITE_WORDS_RAMDISK)
    {
        SW_DB->w_tell = ramdisk_tell;
        SW_DB->w_write = ramdisk_write;
        SW_DB->w_seek = ramdisk_seek;
        SW_DB->w_read = ramdisk_read;
        SW_DB->w_close = ramdisk_close;
        SW_DB->w_putc = ramdisk_putc;
        SW_DB->w_getc = ramdisk_getc;
    }
    else
    {
        SW_DB->w_tell = sw_ftell;
        SW_DB->w_write = sw_fwrite;
        SW_DB->w_seek = sw_fseek;
        SW_DB->w_read = sw_fread;
        SW_DB->w_close = sw_fclose;
        SW_DB->w_putc = sw_fputc;
        SW_DB->w_getc = sw_fgetc;
    }

    SW_DB->dbname = estrdup(dbname);

    return SW_DB;
}


/* Open files */


static FILE   *openIndexFILEForRead(char *filename)
{
    return sw_fopen(filename, F_READ_BINARY);
}

static FILE   *openIndexFILEForReadAndWrite(char *filename)
{
    return sw_fopen(filename, F_READWRITE_BINARY);
}


static FILE   *openIndexFILEForWrite(char *filename)
{
    return sw_fopen(filename, F_WRITE_BINARY);
}

static void    CreateEmptyFile(char *filename)
{
    FILE   *fp;

    if (!(fp = openIndexFILEForWrite(filename)))
    {
        progerrno("Couldn't write the file \"%s\": ", filename);
    }
    sw_fclose(fp);
}

static int  is_directory(char *path)
{
    struct stat stbuf;

    if (stat(path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

#ifdef USE_BTREE
DB * OpenBerkeleyFile(char *filename, DBTYPE db_type, u_int32_t db_flags)
{
DB *dbp;
int db_ret;
    if((db_ret = db_create(&dbp, NULL, 0)))
        progerrno("Couldn't create BERKELEY DB resource");
    if((db_ret = dbp->open(dbp,NULL,filename,NULL,db_type,db_flags,0)))
    {
        dbp->err(dbp,db_ret,"Database open failed: \"%s\"", filename);
        progerrno("Couldn't open the DB Berkeley file \"%s\": ", filename);
    }

    return dbp;
}

DB * CreateBerkeleyFile(char *filename,DBTYPE db_type)
{
    return OpenBerkeleyFile(filename, db_type, DB_CREATE | DB_TRUNCATE);
}

void CloseBerkeleyFile(DB **dbp, char **filename, int *tempflag)
{
int     ret;
char   *newname;
DB     *temp_dbp;
    if(*dbp)
    {
        ret = (*dbp)->close(*dbp,0);
        *dbp = NULL;
        if(*tempflag)
        {
            newname = estrdup(*filename);
            if((ret = db_create(&temp_dbp, NULL, 0)))
                progerrno("Couldn't create BERKELEY DB resource");

            newname[strlen(newname) - strlen(USE_TEMPFILE_EXTENSION)] = '\0';
            remove(newname);
            ret = temp_dbp->rename(temp_dbp,*filename,NULL,newname,0);
            if (ret != 0) 
            {
                progerrno("Couldn't rename %s to %s. Berkeley DB Error code: %d\n",*filename,newname,ret);
            }
            efree(newname);
            *tempflag = 0;          /* no longer opened as a temporary file */
        }
        efree(*filename);
        *filename = NULL;
    }
}
#endif

/**********************/



void   *DB_Create_Native(SWISH *sw, char *dbname)
{
    long    swish_magic;
    char   *filename;
#ifdef USE_BTREE
    FILE   *fp_tmp;
#else
    int     i;
#endif
    struct Handle_DBNative *SW_DB;

    if ( is_directory( dbname ) )
        progerr( "Index file '%s' is a directory", dbname );


    swish_magic = SWISH_MAGIC;
   /* Allocate structure */
    SW_DB = (struct Handle_DBNative *) newNativeDBHandle(sw, dbname);
    SW_DB->mode = DB_CREATE;
    SW_DB->unique_ID = (long) time(NULL); /* Ok, so if more than one index is created the second... */

#ifdef USE_TEMPFILE_EXTENSION
    filename = emalloc(strlen(dbname) + strlen(USE_TEMPFILE_EXTENSION) + strlen(PROPFILE_EXTENSION) + strlen(PROPINDEX_EXTENSION) + strlen(BTREE_EXTENSION) + strlen(WORDDATA_EXTENSION) + strlen(TOTWORDS_EXTENSION) + strlen(PRESORTED_EXTENSION) + strlen(HASHFILE_EXTENSION) + strlen(HEADER_EXTENSION) + 1);
    strcpy(filename, dbname);
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_index = 1;
#else
    filename = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + strlen(PROPINDEX_EXTENSION) + strlen(BTREE_EXTENSION) + strlen(WORDDATA_EXTENSION) + strlen(TOTWORDS_EXTENSION) + strlen(PRESORTED_EXTENSION) + strlen(HASHFILE_EXTENSION) + strlen(HEADER_EXTENSION) + 1);
    strcpy(filename, dbname);
#endif


    /* Create index File */
#ifndef USE_BTREE
    CreateEmptyFile(filename);
    if (!(SW_DB->fp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the index file \"%s\": ", filename);

    SW_DB->cur_index_file = estrdup(filename);
    printlong(SW_DB->fp, swish_magic, sw_fwrite);
    printlong(SW_DB->fp, SW_DB->unique_ID, sw_fwrite);
#endif

    /* Create property File */
    strcpy(filename, dbname);
    strcat(filename, PROPFILE_EXTENSION);

#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_prop = 1;
#endif

    CreateEmptyFile(filename);
    if (!(SW_DB->fp_prop = openIndexFILEForWrite(filename)))
        progerrno("Couldn't create the property file \"%s\": ", filename);

    SW_DB->cur_prop_file = estrdup(filename);
    printlong(SW_DB->fp_prop, SW_DB->unique_ID, sw_fwrite);


#ifdef USE_BTREE
    /* Create Btree File */
    strcpy(filename, dbname);
    strcat(filename, BTREE_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_btree = 1;
#endif
    SW_DB->db_btree = CreateBerkeleyFile(filename,DB_BTREE);
    SW_DB->cur_btree_file = estrdup(filename);


    /* Create WordData File */
    strcpy(filename, dbname);
    strcat(filename, WORDDATA_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_worddata = 1;
#endif
    SW_DB->db_worddata = CreateBerkeleyFile(filename,DB_RECNO);
    SW_DB->cur_worddata_file = estrdup(filename);

    /* Create totwords File */
    strcpy(filename, dbname);
    strcat(filename, TOTWORDS_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_totwords = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the totwords file \"%s\": ", filename);
    printlong(fp_tmp, SW_DB->unique_ID, sw_fwrite);
    SW_DB->cur_totwords_file = estrdup(filename);
    SW_DB->fp_totwords = fp_tmp;

    /* Create propindex File */
    strcpy(filename, dbname);
    strcat(filename, PROPINDEX_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_propindex = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the propindex file \"%s\": ", filename);
    printlong(fp_tmp, SW_DB->unique_ID, sw_fwrite);
    SW_DB->cur_propindex_file = estrdup(filename);
    SW_DB->fp_propindex = fp_tmp;

    /* Create PreSorted Index File */
    strcpy(filename, dbname);
    strcat(filename, PRESORTED_EXTENSION);

#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_presorted = 1;
#endif

    CreateEmptyFile(filename);
    if (!(SW_DB->fp_presorted = openIndexFILEForWrite(filename)))
        progerrno("Couldn't create the presorted index file \"%s\": ", filename);

    SW_DB->cur_presorted_file = estrdup(filename);
    printlong(SW_DB->fp_presorted, SW_DB->unique_ID, sw_fwrite);

    /* Create header File */
    strcpy(filename, dbname);
    strcat(filename, HEADER_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_header = 1;
#endif
    CreateEmptyFile(filename);
    if (!(fp_tmp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the header file \"%s\": ", filename);
    printlong(fp_tmp, SW_DB->unique_ID, sw_fwrite);
    SW_DB->cur_header_file = estrdup(filename);
    SW_DB->fp_header = fp_tmp;


    /* Create HashFileIndex File */
    strcpy(filename, dbname);
    strcat(filename, HASHFILE_EXTENSION);

#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_hashfile = 1;
#endif
    SW_DB->db_hashfile = CreateBerkeleyFile(filename,DB_HASH);
    SW_DB->cur_hashfile_file = estrdup(filename);


#endif

    efree(filename);

#ifndef USE_BTREE
    for (i = 0; i < MAXCHARS; i++)
        SW_DB->offsets[i] = (sw_off_t)0;
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        SW_DB->hashoffsets[i] = (sw_off_t)0;
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        SW_DB->lasthashval[i] = (sw_off_t)0;

    /* Reserve space for offset pointers */
    SW_DB->offsetstart = sw_ftell(SW_DB->fp);
    for (i = 0; i < MAXCHARS; i++)
        printfileoffset(SW_DB->fp, (sw_off_t) 0, sw_fwrite);

    SW_DB->hashstart = sw_ftell(SW_DB->fp);
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        printfileoffset(SW_DB->fp, (sw_off_t) 0, sw_fwrite);
#endif

    return (void *) SW_DB;
}


/*******************************************************************
*   DB_Open_Native
*
*******************************************************************/

void   *DB_Open_Native(SWISH *sw, char *dbname,int mode)
{
    struct Handle_DBNative *SW_DB;
    FILE   *(*openRoutine)(char *) = NULL;
    char   *s;
#ifdef USE_BTREE
    u_int32_t db_flags;
#else
    int     i;
#endif

    switch(mode)
    {
    case DB_READ:
        openRoutine = openIndexFILEForRead;
#ifdef USE_BTREE
        db_flags = DB_RDONLY;
#endif
        break;
    case DB_READWRITE:
        openRoutine = openIndexFILEForReadAndWrite;
#ifdef USE_BTREE
        db_flags = 0;
#endif
        break;
    default:
        openRoutine = openIndexFILEForRead;
#ifdef USE_BTREE
        db_flags = DB_RDONLY;
#endif
    }

    SW_DB = (struct Handle_DBNative *) newNativeDBHandle(sw, dbname);
    SW_DB->mode = mode;

#ifndef USE_BTREE
    /* Open index File */
    if (!(SW_DB->fp = openRoutine(dbname)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Could not open the index file '%s': ", dbname);
        return (void *) SW_DB;
    }

    SW_DB->cur_index_file = estrdup(dbname);
#endif

    s = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PROPFILE_EXTENSION);

    if (!(SW_DB->fp_prop = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the property file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_prop_file = s;

#ifdef USE_BTREE

    s = emalloc(strlen(dbname) + strlen(BTREE_EXTENSION) + 1);
    strcpy(s, dbname);
    strcat(s, BTREE_EXTENSION);
    SW_DB->db_btree = OpenBerkeleyFile(s, DB_BTREE, db_flags);

    SW_DB->cur_btree_file = s;

    s = emalloc(strlen(dbname) + strlen(PRESORTED_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PRESORTED_EXTENSION);

    if (!(SW_DB->fp_presorted = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the presorted index file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_presorted_file = s;

    s = emalloc(strlen(dbname) + strlen(PROPINDEX_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PROPINDEX_EXTENSION);

    if (!(SW_DB->fp_propindex = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the propindex index file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_propindex_file = s;

    s = emalloc(strlen(dbname) + strlen(TOTWORDS_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, TOTWORDS_EXTENSION);

    if (!(SW_DB->fp_totwords = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the totwords index file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_totwords_file = s;

    s = emalloc(strlen(dbname) + strlen(HEADER_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, HEADER_EXTENSION);

    if (!(SW_DB->fp_header = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the header index file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_header_file = s;

    s = emalloc(strlen(dbname) + strlen(WORDDATA_EXTENSION) + 1);
    strcpy(s, dbname);
    strcat(s, WORDDATA_EXTENSION);
    SW_DB->db_worddata = OpenBerkeleyFile(s, DB_RECNO, db_flags);
    SW_DB->cur_worddata_file = s;

    s = emalloc(strlen(dbname) + strlen(HASHFILE_EXTENSION) + 1);
    strcpy(s, dbname);
    strcat(s, HASHFILE_EXTENSION);
    SW_DB->db_hashfile = OpenBerkeleyFile(s, DB_HASH, db_flags);
    SW_DB->cur_hashfile_file = s;


#endif

    /* Validate index files */
    DB_CheckHeader(SW_DB);
    if ( SW_DB->sw->lasterror )
        return (void *) SW_DB;

#ifndef USE_BTREE
    /* Read offsets lookuptable */
    SW_DB->offsetstart = sw_ftell(SW_DB->fp);
    for (i = 0; i < MAXCHARS; i++)
        SW_DB->offsets[i] = readfileoffset(SW_DB->fp, sw_fread);

    /* Read hashoffsets lookuptable */
    SW_DB->hashstart = sw_ftell(SW_DB->fp);
    for (i = 0; i < VERYBIGHASHSIZE; i++)
        SW_DB->hashoffsets[i] = readfileoffset(SW_DB->fp, sw_fread);
#else

    /* Put the file pointer of props, propindex and totwords files 
    ** at the end of the files
    ** This is very important because if we are in update mode
    ** we must avoid the properties to be overwritten
    */
    sw_fseek(SW_DB->fp_prop,(sw_off_t)0,SEEK_END);
    sw_fseek(SW_DB->fp_propindex,(sw_off_t)0,SEEK_END);
    sw_fseek(SW_DB->fp_totwords,(sw_off_t)0,SEEK_END);
#endif

    return (void *) SW_DB;
}


/****************************************************************
* This closes a file, and will rename if flagged as such
*  Frees the associated current file name
*
*****************************************************************/

static void DB_Close_File_Native(FILE ** fp, char **filename, int *tempflag)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
struct stat stbuf;
#endif
    if (!*fp)
        return;

    if (sw_fclose(*fp))
        progerrno("Failed to close file '%s': ", *filename);

    *fp = NULL;

#ifdef USE_TEMPFILE_EXTENSION
    if (*tempflag)
    {
        char   *newname = estrdup(*filename);

        newname[strlen(newname) - strlen(USE_TEMPFILE_EXTENSION)] = '\0';

#if defined(_WIN32) && !defined(__CYGWIN__)
        if(!stat(newname, &stbuf) && ((stbuf.st_mode & S_IFMT) == S_IFREG))
        /* if(isfile(newname)) FIXME: file.c shouldn't rely on indexing structures */
            if (remove(newname))
                progerrno("Failed to unlink '%s' before renaming. : ", newname);
#endif

        if (rename(*filename, newname))
            progerrno("Failed to rename '%s' to '%s' : ", *filename, newname);


#ifdef INDEXPERMS
        chmod(newname, INDEXPERMS);
#endif

        efree(newname);

        *tempflag = 0;          /* no longer opened as a temporary file */
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    int     i;
    FILE   *fp = SW_DB->fp;
#endif


    /* Close (and rename) property file, if it's open */
    DB_Close_File_Native(&SW_DB->fp_prop, &SW_DB->cur_prop_file, &SW_DB->tmp_prop);

#ifndef USE_BTREE

    if (SW_DB->mode == DB_CREATE || SW_DB->mode == DB_READWRITE)     /* If we are indexing update offsets to words and files */
    {
        /* Update internal pointers */

        sw_fseek(fp, SW_DB->offsetstart, SEEK_SET);
        for (i = 0; i < MAXCHARS; i++)
            printfileoffset(fp, SW_DB->offsets[i], sw_fwrite);

        sw_fseek(fp, SW_DB->hashstart, SEEK_SET);
        for (i = 0; i < VERYBIGHASHSIZE; i++)
            printfileoffset(fp, SW_DB->hashoffsets[i], sw_fwrite);
    }

    /* Close (and rename) the index file */
    DB_Close_File_Native(&SW_DB->fp, &SW_DB->cur_index_file, &SW_DB->tmp_index);

#else

    /* Close (and rename) worddata file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_worddata, &SW_DB->cur_worddata_file, &SW_DB->tmp_worddata);

    /* Close (and rename) btree file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_btree, &SW_DB->cur_btree_file, &SW_DB->tmp_btree);

    /* Close (and rename) propindex file, if it's open */
    if(SW_DB->fp_propindex)
    {
        /* Close (and rename) property file, if it's open */
        DB_Close_File_Native(&SW_DB->fp_propindex, &SW_DB->cur_propindex_file, &SW_DB->tmp_propindex);
    }
    /* Close (and rename) totwords file, if it's open */
    if(SW_DB->fp_totwords)
    {
        /* Close (and rename) totwords file, if it's open */
        DB_Close_File_Native(&SW_DB->fp_totwords, &SW_DB->cur_totwords_file, &SW_DB->tmp_totwords);
    }
    /* Close (and rename) presorted index file, if it's open */
    if(SW_DB->fp_presorted)
    {
        DB_Close_File_Native(&SW_DB->fp_presorted, &SW_DB->cur_presorted_file, &SW_DB->tmp_presorted);
    }
    /* Close (and rename) header index file, if it's open */
    if(SW_DB->fp_header)
    {
        DB_Close_File_Native(&SW_DB->fp_header, &SW_DB->cur_header_file, &SW_DB->tmp_header);
    }

    /* Close (and rename) hash-file index file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_hashfile, &SW_DB->cur_hashfile_file, &SW_DB->tmp_hashfile);
#endif


    if (SW_DB->dbname)
        efree(SW_DB->dbname);
    efree(SW_DB);
}

void    DB_Remove_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;


    /* this is currently not used */
    /* $$$ remove the prop file too */
/*
    sw_fclose(SW_DB->fp);
    remove(SW_DB->dbname);
*/
    efree(SW_DB->dbname);
    efree(SW_DB);
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Header stuff                  */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteHeader_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
        /* The index file is being created. So put the header in the
        ** current file position (coincides with the end of the file
        */
        SW_DB->offsets[HEADERPOS] = sw_ftell(SW_DB->fp);
#else
    long swish_magic;
        /* Jump over swish_magic ID (long number) */
        sw_fseek(SW_DB->fp_header, (sw_off_t)0, SEEK_SET);
        swish_magic = readlong(SW_DB->fp_header, sw_fread);
#endif

    return 0;
}


int     DB_EndWriteHeader_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    FILE   *fp = SW_DB->fp;
#else
    FILE   *fp = SW_DB->fp_header;
#endif

    /* End of header delimiter */
    if ( putc(0, fp) == EOF )
        progerrno("putc() failed: ");

    return 0;
}

int     DB_WriteHeaderData_Native(int id, unsigned char *s, int len, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    FILE   *fp = SW_DB->fp;
#else
    FILE   *fp = SW_DB->fp_header;
#endif

    compress1(id, fp, sw_fputc);
    compress1(len, fp, sw_fputc);
    if ( sw_fwrite(s, len, sizeof(char), fp) != sizeof( char ) ) /* seems backward */
        progerrno("Error writing to device while trying to write %d bytes: ", len );

    return 0;
}


int     DB_InitReadHeader_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    sw_fseek(SW_DB->fp, SW_DB->offsets[HEADERPOS], SEEK_SET);
#else
    long swish_magic;
        /* Jump over swish_magic ID (long number) */
        sw_fseek(SW_DB->fp_header, (sw_off_t)0, SEEK_SET);
        swish_magic = readlong(SW_DB->fp_header, sw_fread);
#endif
    return 0;
}

int     DB_ReadHeaderData_Native(int *id, unsigned char **s, int *len, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifndef USE_BTREE
    FILE   *fp = SW_DB->fp;
#else
    FILE   *fp = SW_DB->fp_header;
#endif
    int     tmp;

    tmp = uncompress1(fp, sw_fgetc);
    *id = tmp;
    if (tmp)
    {
        tmp = uncompress1(fp, sw_fgetc);
        *s = (unsigned char *) emalloc(tmp + 1);
        *len = tmp;
        sw_fread(*s, *len, sizeof(char), fp);
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    SW_DB->offsets[WORDPOS] = sw_ftell(SW_DB->fp);
#endif

    return 0;
}

int     cmp_wordhashdata(const void *s1, const void *s2)
{
    sw_off_t    *i = (sw_off_t *) s1;
    sw_off_t    *j = (sw_off_t *) s2;
    sw_off_t     d = (*i - *j);

    if(d == (sw_off_t)0) return 0;
    else if(d > (sw_off_t)0) return 1;
    else return -1;
}

int     DB_EndWriteWords_Native(void *db)
{
#ifndef USE_BTREE
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = (FILE *) SW_DB->fp;
    int     i,
            wordlen;
    sw_off_t wordID, word_pos;
    sw_off_t f_hash_offset, f_offset;

    /* Free hash zone */
    Mem_ZoneFree(&SW_DB->hashzone);

    /* Now update word's data offset into the list of words */
    /* Simple check  words and worddata must match */

    if (! SW_DB->num_words)
        progerr("No unique words indexed");

    if (SW_DB->num_words != SW_DB->wordhash_counter)
        progerrno("Internal DB_native error - SW_DB->num_words != SW_DB->wordhash_counter: ");

    if (SW_DB->num_words != SW_DB->worddata_counter)
        progerrno("Internal DB_native error - SW_DB->num_words != SW_DB->worddata_counter: ");

    /* Sort wordhashdata to be written to allow sequential writes */
    swish_qsort(SW_DB->wordhashdata, SW_DB->num_words, 3 * sizeof(sw_off_t), cmp_wordhashdata);

    if (WRITE_WORDS_RAMDISK)
    {
        fp = (FILE *) SW_DB->rd;
    }
    for (i = 0; i < SW_DB->num_words; i++)
    {
        wordID = SW_DB->wordhashdata[3 * i];
        f_hash_offset = SW_DB->wordhashdata[3 * i + 1];
        f_offset = SW_DB->wordhashdata[3 * i + 2];

        word_pos = wordID;
        if (WRITE_WORDS_RAMDISK)
        {
            word_pos -= SW_DB->offsets[WORDPOS];
        }
        /* Position file pointer in word */
        SW_DB->w_seek(fp, word_pos, SEEK_SET);
        /* Jump over word length and word */
        wordlen = uncompress1(fp, SW_DB->w_getc); /* Get Word length */
        SW_DB->w_seek(fp, (sw_off_t) wordlen, SEEK_CUR); /* Jump Word */
        /* Write offset to next chain */
        printfileoffset(fp, f_hash_offset, SW_DB->w_write);
        /* Write offset to word data */
        printfileoffset(fp, f_offset, SW_DB->w_write);
    }

    efree(SW_DB->wordhashdata);
    SW_DB->wordhashdata = NULL;
    SW_DB->worddata_counter = 0;
    SW_DB->wordhash_counter = 0;

    if (WRITE_WORDS_RAMDISK)
    {
        unsigned char buffer[4096];
        sw_off_t    ramdisk_size;
        long    read = 0;

        ramdisk_seek((FILE *) SW_DB->rd, (sw_off_t)0, SEEK_END);
        ramdisk_size = ramdisk_tell((FILE *) SW_DB->rd);
        /* Write ramdisk to fp end free it */
        sw_fseek((FILE *) SW_DB->fp, SW_DB->offsets[WORDPOS], SEEK_SET);
        ramdisk_seek((FILE *) SW_DB->rd, (sw_off_t)0, SEEK_SET);
        while (ramdisk_size)
        {
            read = ramdisk_read(buffer, 4096, 1, (FILE *) SW_DB->rd);
            if ( sw_fwrite(buffer, read, 1, SW_DB->fp) != 1 )
                progerrno("Error while flushing ramdisk to disk:");

            ramdisk_size -= (sw_off_t)read;
        }
        ramdisk_close((FILE *) SW_DB->rd);
    }
    /* Get last word file offset - For the last word, this will be
    ** used to delimite the last word in the index file
    ** In other words. This is the file offset where no more words
    ** are added.
    */
    SW_DB->offsets[ENDWORDPOS] = sw_ftell(SW_DB->fp);

    /* Restore file pointer at the end of file */
    sw_fseek(SW_DB->fp, (sw_off_t)0, SEEK_END);
    if ( sw_fputc(0, SW_DB->fp) == EOF )           /* End of words mark */
        progerrno("sw_fputc() failed writing null: ");

#endif

    return 0;
}

#ifndef USE_BTREE
sw_off_t    DB_GetWordID_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;
    sw_off_t    pos = (sw_off_t)0;

    if (WRITE_WORDS_RAMDISK)
    {
        if (!SW_DB->rd)
        {
            /* ramdisk size as suggested by Bill Meier */
            SW_DB->rd = ramdisk_create("RAM Disk: write words", 32 * 4096);
        }
        pos = SW_DB->offsets[WORDPOS];
        fp = (FILE *) SW_DB->rd;
    }
    pos += SW_DB->w_tell(fp);

    return pos;                 /* Native database uses position as a Word ID */
}

int     DB_WriteWord_Native(char *word, sw_off_t wordID, void *db)
{
    int     i,
            wordlen;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    FILE   *fp = SW_DB->fp;

    i = (int) ((unsigned char) word[0]);

    if (!SW_DB->offsets[i])
        SW_DB->offsets[i] = wordID;


    /* Write word length, word and a NULL offset */
    wordlen = strlen(word);

    if (WRITE_WORDS_RAMDISK)
    {
        fp = (FILE *) SW_DB->rd;
    }
    compress1(wordlen, fp, SW_DB->w_putc);
    SW_DB->w_write(word, wordlen, sizeof(char), fp);

    printfileoffset(fp, (sw_off_t) 0, SW_DB->w_write); /* hash chain */
    printfileoffset(fp, (sw_off_t) 0, SW_DB->w_write); /* word's data pointer */

    SW_DB->num_words++;

    return 0;
}

int offsethash(sw_off_t offset)
{
    return (int)(offset % (sw_off_t) BIGHASHSIZE);
}

long    DB_WriteWordData_Native(sw_off_t wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;
    struct numhash *numhash;
    int     numhashval;

    /* We must be at the end of the file */

    if (!SW_DB->worddata_counter)
    {
        /* We are starting writing worddata */
        /* If inside a ramdisk we must preserve its space */
        if (WRITE_WORDS_RAMDISK)
        {
            sw_off_t    ramdisk_size;

            ramdisk_seek((FILE *) SW_DB->rd, (sw_off_t)0, SEEK_END);
            ramdisk_size = ramdisk_tell((FILE *) SW_DB->rd);
            /* Preserve ramdisk size in DB file  */
            /* it will be written later */
            sw_fseek((FILE *) SW_DB->fp, ramdisk_size, SEEK_END);
        }
    }
    /* Search for word's ID */
    numhashval = offsethash(wordID);
    for (numhash = SW_DB->hash[numhashval]; numhash; numhash = numhash->next)
        if (SW_DB->wordhashdata[3 * numhash->index] == wordID)
            break;
    if (!numhash)
        progerrno("Internal db_native.c error in DB_WriteWordData_Native: ");
    SW_DB->wordhashdata[3 * numhash->index + 2] = sw_ftell(fp);

    SW_DB->worddata_counter++;

    /* Write the worddata to disk */
    /* Write in the form:  <data_size><saved_bytes><worddata> */
    /* If there is not any compression then saved_bytes is 0 */
    compress1(data_size, fp, sw_fputc);
    compress1(saved_bytes, fp, sw_fputc);
    if ( sw_fwrite(worddata, data_size, 1, fp) != 1 )
        progerrno("Error writing to device while trying to write %d bytes: ", data_size );


    /* A NULL byte to indicate end of word data */
    if ( sw_fputc(0, fp) == EOF )
        progerrno( "sw_fputc() returned error writing null: ");



    return 0;
}

#else

sw_off_t    DB_GetWordID_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    return (sw_off_t) SW_DB->worddata_counter;
}

int     DB_WriteWord_Native(char *word, sw_off_t wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    DBT key,data;
    int ret;

    /*Berkeley DB stuff */
    wordID = PACKLONG(wordID);
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = word;
    key.size = strlen(word);
    data.data = (void *)&wordID;
    data.size = sizeof(wordID);

    ret = SW_DB->db_btree->put(SW_DB->db_btree,NULL,&key,&data,DB_NOOVERWRITE);

    if(ret != 0)
    {
        printf("ERROR %d inserting word \"%s\" in Berkeley DB BTREE\n",ret,word);
    }
    SW_DB->num_words++;

    return 0;
}

int     DB_UpdateWordID_Native(char *word, sw_off_t new_wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    DBT key,data;

    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = word;
    key.size = strlen(word);
    data.data = (void *)&new_wordID;
    data.size = sizeof(new_wordID);
    SW_DB->db_btree->del(SW_DB->db_btree,NULL,&key,0);
    SW_DB->db_btree->put(SW_DB->db_btree,NULL,&key,&data,DB_NOOVERWRITE);

    return 0;
}

int     DB_DeleteWordData_Native(sw_off_t wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    DBT key;

    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    key.data = (void *)&wordID;
    key.size = sizeof(wordID);
    SW_DB->db_worddata->del(SW_DB->db_worddata,NULL,&key,0);

    return 0;
}

long    DB_WriteWordData_Native(sw_off_t wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db)
{
    unsigned char stack_buffer[8192]; /* just to avoid emalloc,efree overhead */
    unsigned char *buf, *p;
    int buf_size;
    DBT key,data;
    db_recno_t recno;

    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

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

    SW_DB->worddata_counter++;

    /* Write the worddata to disk */
    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = &SW_DB->worddata_counter;
    key.size = sizeof(SW_DB->worddata_counter);
    data.data = buf;
    data.size = buf_size;
    SW_DB->db_worddata->put(SW_DB->db_worddata,NULL,&key,&data,DB_APPEND);
    recno = *(db_recno_t *) key.data;

    if(buf != stack_buffer)
        efree(buf);
    return 0;
}

#endif

#ifndef USE_BTREE
int     DB_WriteWordHash_Native(char *word, sw_off_t wordID, void *db)
{
    int     i,
            hashval,
            numhashval;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    struct numhash *numhash;

    if (!SW_DB->wordhash_counter)
    {
        /* Init hash array */
        for (i = 0; i < BIGHASHSIZE; i++)
            SW_DB->hash[i] = NULL;
        SW_DB->hashzone = Mem_ZoneCreate("WriteWordHash", SW_DB->num_words * sizeof(struct numhash), 0);

        /* If we are here we have finished WriteWord_Native */
        /* If using ramdisk - Reserve space upto the size of the ramdisk */
        if (WRITE_WORDS_RAMDISK)
        {
            sw_off_t    ram_size = (sw_off_t) (SW_DB->w_seek((FILE *) SW_DB->rd, 0, SEEK_END));

            sw_fseek(SW_DB->fp, ram_size, SEEK_SET);
        }

        SW_DB->wordhashdata = emalloc(3 * SW_DB->num_words * sizeof(sw_off_t));
    }

    hashval = verybighash(word);

    if (!SW_DB->hashoffsets[hashval])
    {
        SW_DB->hashoffsets[hashval] = wordID;
    }

    SW_DB->wordhashdata[3 * SW_DB->wordhash_counter] = wordID;
    SW_DB->wordhashdata[3 * SW_DB->wordhash_counter + 1] = (sw_off_t) 0;


    /* Add to the hash */
    numhash = (struct numhash *) Mem_ZoneAlloc(SW_DB->hashzone, sizeof(struct numhash));

    numhashval = offsethash(wordID);
    numhash->index = SW_DB->wordhash_counter;
    numhash->next = SW_DB->hash[numhashval];
    SW_DB->hash[numhashval] = numhash;

    SW_DB->wordhash_counter++;

    /* Update previous word in hashlist */
    if (SW_DB->lasthashval[hashval])
    {
        /* Search for SW_DB->lasthashval[hashval] */
        numhashval = offsethash(SW_DB->lasthashval[hashval]);
        for (numhash = SW_DB->hash[numhashval]; numhash; numhash = numhash->next)
            if (SW_DB->wordhashdata[3 * numhash->index] == SW_DB->lasthashval[hashval])
                break;
        if (!numhash)
            progerrno("Internal db_native.c error in DB_WriteWordHash_Native: ");
        SW_DB->wordhashdata[3 * numhash->index + 1] = wordID;
    }
    SW_DB->lasthashval[hashval] = wordID;

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
int     DB_ReadWordHash_Native(char *word, sw_off_t *wordID, void *db)
{
    int     wordlen,
            res,
            hashval;
    sw_off_t    offset, dataoffset;
    char   *fileword = NULL;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;


    /* If there is not a star use the hash approach ... */
    res = 1;

    /* Get hash file offset */
    hashval = verybighash(word);
    if (!(offset = SW_DB->hashoffsets[hashval]))
    {
        *wordID = (sw_off_t)0;
        return 0;
    }
    /* Search for word */
    while (res)
    {
        /* Position in file */
        sw_fseek(fp, offset, SEEK_SET);
        /* Get word */
        wordlen = uncompress1(fp, sw_fgetc);
        fileword = emalloc(wordlen + 1);
        sw_fread(fileword, 1, wordlen, fp);
        fileword[wordlen] = '\0';
        offset = readfileoffset(fp, sw_fread); /* Next hash */
        dataoffset = readfileoffset(fp, sw_fread); /* Offset to Word data */

        res = strcmp(word, fileword);
        efree(fileword);

        if (!res)
            break;              /* Found !! */
        else if (!offset)
        {
            dataoffset = (sw_off_t)0;
            break;
        }
    }
    *wordID = (sw_off_t)dataoffset;
    return 0;
}

int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db)
{
    int     wordlen,
            i,
            res,
            len,
            found;
    sw_off_t    dataoffset = 0;
    char   *fileword = NULL;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;


    len = strlen(word);

    i = (int) ((unsigned char) word[0]);

    if (!SW_DB->offsets[i])
    {
        *resultword = NULL;
        *wordID = (sw_off_t)0;
        return 0;
    }
    found = 1;
    sw_fseek(fp, SW_DB->offsets[i], SEEK_SET);

    /* Look for first occurrence */
    wordlen = uncompress1(fp, sw_fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    while (wordlen)
    {
        int bytes_read = (int)sw_fread(fileword, 1, wordlen, fp);
        if ( bytes_read != wordlen )
            progerr("Read %d bytes, expected %d in DB_ReadFirstWordInvertedIndex_Native", bytes_read, wordlen);

        fileword[wordlen] = '\0';
        readfileoffset(fp, sw_fread);    /* jump hash offset */
        dataoffset = readfileoffset(fp, sw_fread); /* Get offset to word's data */

        if (!(res = strncmp(word, fileword, len))) /*Found!! */
        {
            SW_DB->nextwordoffset = sw_ftell(fp); /* preserve next word pos */
            break;
        }

        /* check if past current word or at end */
        if (res < 0 || sw_ftell(fp) ==  SW_DB->offsets[ENDWORDPOS] )
        {
            dataoffset = 0;
            break;
        }

        /* Go to next value */
        wordlen = uncompress1(fp, sw_fgetc); /* Next word */
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

int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db)
{
    int     len,
            wordlen;
    sw_off_t    dataoffset;
    char   *fileword;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;

    /* Check for end of words */
    if (!SW_DB->nextwordoffset || SW_DB->nextwordoffset == SW_DB->offsets[ENDWORDPOS])
    {
        *resultword = NULL;
        *wordID = (sw_off_t)0;
        return 0;
    }

    len = strlen(word);


    sw_fseek(fp, SW_DB->nextwordoffset, SEEK_SET);

    wordlen = uncompress1(fp, sw_fgetc);
    fileword = (char *) emalloc(wordlen + 1);

    sw_fread(fileword, 1, wordlen, fp);
    fileword[wordlen] = '\0';
    if (strncmp(word, fileword, len))
    {
        efree(fileword);
        fileword = NULL;
        dataoffset = (sw_off_t)0;         /* No more data */
        SW_DB->nextwordoffset = (sw_off_t)0;
    }
    else
    {
        readfileoffset(fp, sw_fread);    /* jump hash offset */
        dataoffset = readfileoffset(fp, sw_fread); /* Get data offset */
        SW_DB->nextwordoffset = sw_ftell(fp);
    }
    *resultword = fileword;
    *wordID = dataoffset;

    return 0;

}


long    DB_ReadWordData_Native(sw_off_t wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db)
{
    unsigned char *buffer;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    FILE   *fp = SW_DB->fp;

    sw_fseek(fp, wordID, SEEK_SET);
    *data_size = uncompress1(fp, sw_fgetc);
    *saved_bytes = uncompress1(fp, sw_fgetc);
    buffer = emalloc(*data_size);
    sw_fread(buffer, *data_size, 1, fp);

    *worddata = buffer;

    return 0;
}


#else
int     DB_ReadWordHash_Native(char *word, sw_off_t *wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    sw_off_t dummy = 0;
    DBT key,data;
    int ret;

    /* Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));

    key.data = word;
    key.size = strlen(word); 
    data.data = &dummy;
    data.ulen = sizeof(dummy); 
    data.flags = DB_DBT_USERMEM;
   
    ret = SW_DB->db_btree->get(SW_DB->db_btree,NULL,&key,&data,0);

    if(ret == 0)
    {
        *wordID = UNPACKLONG(dummy);
    }
    else if (ret == DB_NOTFOUND)
    {
        *wordID = (sw_off_t)0;
    }
    else
    {
        //$$$ unexpected return code
        printf("Unexpected return code %d from Berkeley BD BTREE while searching for \"%s\"\n",ret,word);
        *wordID = (sw_off_t)0;
    }

    return 0;
}

int     DB_ReadFirstWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBC *dbcp;
    DBT key, data;
    int ret;

    /* Acquire a cursor for the database. */
    if ((ret = SW_DB->db_btree->cursor(SW_DB->db_btree, NULL, &dbcp, 0)) != 0) 
    {
        //dbp->err(SW_DB->db_btree, ret, "DB->cursor"); return (1); 
        *resultword = NULL;
        *wordID = (sw_off_t)0;
    }
    else
    {
        /* Initialize the key/data pair. */
        memset(&key, 0, sizeof(key));
        memset(&data, 0, sizeof(data));
        *wordID = 0;
        key.data = word;
        key.size = strlen(word);
        key.flags = DB_DBT_MALLOC;
        data.data = wordID;
        data.ulen = sizeof(*wordID);
        data.flags = DB_DBT_USERMEM;
       
        /* Walk through the database and print out the key/data pairs. */ 
        ret = dbcp->c_get(dbcp, &key, &data, DB_SET_RANGE);
        if (ret == 0)
        {
            if((key.size < strlen(word)) || (strncmp(word,key.data,strlen(word))!=0))
            {
                *resultword = NULL;
                *wordID = (sw_off_t)0;
                dbcp->c_close(dbcp);
                SW_DB->dbc_btree = NULL;
            }
            else
            {
                *resultword = emalloc(key.size + 1);
                memcpy(*resultword,key.data,key.size);
                (*resultword)[key.size]='\0';
                *wordID = UNPACKLONG(*wordID);
                /* Preserve  cursor */
                SW_DB->dbc_btree = dbcp;
            }
            /* Perhaps we have to use free instead of efree because Berkeley
            ** used malloc internally */
            efree(key.data);
        }
        else if (ret == DB_NOTFOUND)
        {
            *resultword = NULL;
            *wordID = (sw_off_t)0;
            dbcp->c_close(dbcp);
            SW_DB->dbc_btree = NULL;
        }
    }

    return 0;
}

int     DB_ReadNextWordInvertedIndex_Native(char *word, char **resultword, sw_off_t *wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBC *dbcp = SW_DB->dbc_btree;
    DBT key, data;
    int ret;

    if(dbcp)
    {
        /* Initialize the key/data pair. */
        memset(&key, 0, sizeof(key));
        memset(&data, 0, sizeof(data));
        *wordID = 0;
        key.flags = DB_DBT_MALLOC;
        data.data = wordID;
        data.ulen = sizeof(*wordID);
        data.flags = DB_DBT_USERMEM;

        /* Walk through the database and print out the key/data pairs. */
        ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT);

        if(ret == 0)
        {
            if((key.size < strlen(word)) || (strncmp(word,key.data,strlen(word))!=0))
            {
                *resultword = NULL;
                *wordID = (sw_off_t)0;
                dbcp->c_close(dbcp);
                SW_DB->dbc_btree = NULL;
            }
            else
            {
                *resultword = emalloc(key.size + 1);
                memcpy(*resultword,key.data,key.size);
                (*resultword)[key.size]='\0';
                *wordID = UNPACKLONG(*wordID);
            }
            efree(key.data);
        }
        else if (ret == DB_NOTFOUND)
        {
            *resultword = NULL;
            *wordID = (sw_off_t)0;
            dbcp->c_close(dbcp);
            SW_DB->dbc_btree = NULL;
        }
    }
    return 0;
}

long    DB_ReadWordData_Native(sw_off_t wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db)
{
    unsigned char *buf;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBT key, data;
    int ret;

    /* Initialize the key/data pair. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = &wordID;
    key.size = sizeof(wordID);
    data.flags = DB_DBT_MALLOC;

    ret = SW_DB->db_worddata->get(SW_DB->db_worddata,NULL,&key,&data,0);

    if(ret == 0)
    {
        *data_size = (int) data.size;
        buf = data.data;

        /* Get saved_bytes and adjust data_size */
        *saved_bytes = uncompress2(&buf);
        *data_size -= ((char *)buf - (char *)data.data);
        *worddata = emalloc(*data_size);
        memcpy(*worddata,buf, *data_size);
        /* Free data.data allocated by Berkeley library */
        efree(data.data);
    }
    else
    {
        progerrno("Unexpected error from Berkeley worddata file \"%s\": ", SW_DB->cur_worddata_file);
    }
    return 0;
}

#endif


/*--------------------------------------------
** 2002/12 Jose Ruiz
**     FilePath,FileNum  pairs
**     Auxiliar hash index
*/

/* Routine to write path,filenum */
int     DB_WriteFileNum_Native(int filenum, unsigned char *filedata, int sz_filedata, void *db)
{
#ifdef USE_BTREE
    unsigned long tmp = (unsigned long)filenum;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBT key,data;

    /* Pack tmp */
    tmp = PACKLONG(tmp);

    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = filedata;
    key.size = sz_filedata;
    data.data = (void *)&tmp;
    data.size = sizeof(tmp);
    /* Write it to the hash index */
    SW_DB->db_hashfile->put(SW_DB->db_hashfile,NULL,&key,&data,0);
#endif

    return 0;
}

/* Routine to get filenum from path */
int     DB_ReadFileNum_Native(unsigned char *filedata, void *db)
{
#ifdef USE_BTREE
    unsigned long tmp;
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBT key,data;

    /* Get it from the hash index */
    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = filedata;
    key.size = strlen((char *)filedata);
    data.data = &tmp;
    data.ulen = sizeof(tmp);
    data.flags = DB_DBT_USERMEM;
    /* Read it from the hash index */
    SW_DB->db_hashfile->get(SW_DB->db_hashfile,NULL,&key,&data,0);

    /* UnPack tmp */
    tmp = UNPACKLONG(tmp);

    return (int)tmp;
#endif
    return 0;
}

/* Routine to test if filenum was deleted */
int     DB_CheckFileNum_Native(int filenum, void *db)
{
#ifdef USE_BTREE
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
    unsigned long totwords;

    /* swish_magic number is in position 0 (first_record) */
    /* filenum starts in 1 (first filenum is 1) */
    /* Go to totalwords position  and read it */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t) first_record + (sw_off_t)(filenum - 1) * sizeof(long), SEEK_SET);
    totwords = readlong(SW_DB->fp_totwords, sw_fread);
 
    return (int) totwords;
#endif
    return 1;
}


/* Routine to remove a filenum */
/* At this moment, to remove a filenum I am just puting 0 in the number of
** words */
int     DB_RemoveFileNum_Native(int filenum, void *db)
{
#ifdef USE_BTREE
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    long    swish_magic;

    /* Jump swish_magic number */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t)0, SEEK_SET);
    swish_magic = readlong(SW_DB->fp_totwords, sw_fread);

    /* Go to totalwords position  and write a 0 */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t)(filenum - 1) * sizeof(long), SEEK_CUR);
    printlong(SW_DB->fp_totwords, 0, sw_fwrite);

#endif

    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Sorted data Stuff             */
/*--------------------------------------------*/
/*--------------------------------------------*/

/********************************************************************************
*
*   SW_DB->offsets[SORTEDINDEX] => first record in table There's a record for each
*   property that was sorted (not all are sorted) The data passed in has
*   already been compressed
*
*                 +------------------------------+
*                 | Pointer to next table entry  | <<-- SW_DB->last_sortedindex
*                 |   (initially zero)           |
*                 +------------------------------+
*                 | PropID                       |
*                 +------------------------------+
*                 | Data Length in bytes         |
*                 +------------------------------+
*                 | Data [....]                  |
*                 +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
*                 |                              | <<-- DB_next_sortedindex
*                 +------------------------------+
*
*   Again, SW_DB->offsets[SORTEDINDEX] points to the very first record in the
*   index While writing, SW_DB->next_sortedindex points to the next place to start
*   writing.  SW_DB->last_sortedindex points to the (initially zero) point to the
*   next table.  So, when it is not null then last_sortedinex is used to update
*   the pointer in the previous table to point to the next table.  A zero entry
*   indicates that there are no more records.
*
* DB_InitWriteSortedIndex_Native should probably write a null for the first
* record's "next table entry" and set next_ and last_ pointers.  Then
* DB_EndWriteSortedIndex_Native call would not be needed.
*
* Notes/Questions:
*   Seems like result_sort.c is the only place that loads this -- in LoadSortedProps.
*   LoadSortedProps is used by merge, proplimit, and result_sort.  LoadSortedProps
*   uncompresses the array (and pre_sort.c compresses).  Why isn't that compression
*   and uncompression done here?
*
*
********************************************************************************/


int     DB_InitWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifdef USE_BTREE
   FILE *fp = SW_DB->fp_presorted;
#else
   FILE *fp = SW_DB->fp;

   SW_DB->offsets[SORTEDINDEX] = sw_ftell(fp);
#endif

   SW_DB->next_sortedindex = sw_ftell(fp);
   return 0;
}

/********************************************************************************
* DB_WriteSortedIndex
*
* Input:
*   propID      property id of this table
*   *data       pointer to char array compressed table
*   sz_data     size of the char array in bytes
*   db          where to write
*
*********************************************************************************/

int     DB_WriteSortedIndex_Native(int propID, unsigned char *data, int sz_data,void *db)
{
   sw_off_t tmp1,tmp2;
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifdef USE_BTREE
   FILE *fp = SW_DB->fp_presorted;
#else
   FILE *fp = SW_DB->fp;
#endif

   sw_fseek(fp, SW_DB->next_sortedindex, SEEK_SET);

    /* save start of this record so can update previous record's "next record" pointer */
   tmp1 = sw_ftell(fp);

   printfileoffset(fp,(sw_off_t)0,sw_fwrite);  /* Pointer to next table if any */

    /* Write ID */
   compress1(propID,fp,sw_fputc);

   /* Write len of data */
   compress1(sz_data,fp,putc);

   /* Write data */
   if ( sw_fwrite(data,sz_data,1,fp) != 1 )
        progerrno("Error writing to device while trying to write %d bytes: ", sz_data );


   SW_DB->next_sortedindex = tmp2 = sw_ftell(fp);


   if(SW_DB->last_sortedindex)
   {
       sw_fseek(fp,SW_DB->last_sortedindex,SEEK_SET);
       printfileoffset(fp,tmp1,sw_fwrite);
       sw_fseek(fp,tmp2,SEEK_SET);
   }
   SW_DB->last_sortedindex = tmp1;
   return 0;
}

int     DB_EndWriteSortedIndex_Native(void *db)
{
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifdef USE_BTREE
   FILE *fp = SW_DB->fp_presorted;
#else
   FILE *fp = SW_DB->fp;
#endif

   printfileoffset(fp,(sw_off_t)0,sw_fwrite);  /* No next table mark - Useful if no presorted indexes */
         /* NULL meta id- Only useful if no presorted indexes  */

    if ( putc(0, fp) == EOF )
        progerrno("putc() failed writing null: ");

   return 0;
}


/* Non Btree read functions */


int     DB_InitReadSortedIndex_Native(void *db)
{
   return 0;
}

/***********************************************************************************
*  DB_ReadSortedIndex_Native -
*
*  Searches through the sorted indexes looking for one that matches the propID
*  passed in.  If found then malloc's a table and reads it in.
*  The data is in compressed format and is not uncompressed here
*
***********************************************************************************/

int     DB_ReadSortedIndex_Native(int propID, unsigned char **data, int *sz_data,void *db)
{
   sw_off_t next;
   long id, tmp;
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
#ifdef USE_BTREE
   FILE *fp = SW_DB->fp_presorted;
   unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
#else
   FILE *fp = SW_DB->fp;
   unsigned long first_record = SW_DB->offsets[SORTEDINDEX];
#endif


   /* seek to the first record */
   sw_fseek(fp,first_record,SEEK_SET);


   /* get seek position of the next record, if needed */
   next = readfileoffset(fp,sw_fread);

   /* read propID for this record */
   id = uncompress1(fp,sw_fgetc);


   while(1)
   {
       if(id == propID)  /* this is the property we are looking for */
       {
           tmp = uncompress1(fp,sw_fgetc);
           *sz_data = tmp;

           *data = emalloc(*sz_data);
           sw_fread(*data,*sz_data,1,fp);
           return 0;
       }
       if(next)
       {
           sw_fseek(fp,next,SEEK_SET);
           next = readfileoffset(fp,sw_fread);
           id = uncompress1(fp,sw_fgetc);
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


/*
** Jose Ruiz 04/00
** Store a portable long in a portable format
*/
void    printlong(FILE * fp, unsigned long num, size_t(*f_write) (const void *, size_t, size_t, FILE *))
{
    size_t written;

    num = PACKLONG(num);        /* Make the number portable */
    if ( (written = f_write(&num, sizeof(long), 1, fp)) != 1 )
        progerrno("Error writing %d of %d bytes: ", sizeof(long), written );
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

/*
** 2003/10 Jose Ruiz
** Store a file offset in a portable format
*/
void    printfileoffset(FILE * fp, sw_off_t num, size_t(*f_write) (const void *, size_t, size_t, FILE *))
{
    size_t written;

    num = PACKFILEOFFSET(num);        /* Make the number portable */
    if ( (written = f_write(&num, sizeof(num), 1, fp)) != 1 )
        progerrno("Error writing %d of %d bytes: ", sizeof(num), written );
}

/*
** 2003/10Jose Ruiz
** Read a file offset from a portable format
*/
sw_off_t readfileoffset(FILE * fp, size_t(*f_read) (void *, size_t, size_t, FILE *))
{
    sw_off_t num;

    f_read(&num, sizeof(num), 1, fp);
    return UNPACKFILEOFFSET(num);     /* Make the number readable */
}



/****************************************************************************
*   Writing Properites  (not for USE_BTREE)
*
*   Properties are written sequentially to the .prop file.
*   Fixed length records of the seek position into the
*   property file are written sequentially to the main index (which is why
*   there's a separate .prop file).
*
*   DB_InitWriteProperties is called first time a property is written
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


int     DB_InitWriteProperties_Native(void *db)
{
#ifndef USE_BTREE
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    SW_DB->offsets[FILELISTPOS] = sw_ftell(SW_DB->fp);

#ifdef DEBUG_PROP
    printf("InitWriteProperties: Start of property table in main index at offset: %ld\n", SW_DB->offsets[FILELISTPOS] );
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    size_t             written_bytes;
    PROP_INDEX      *pindex = fi->prop_index;
    PROP_LOCATION   *prop_loc;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             propIDX = header->metaID_to_PropIDX[propID];
#ifdef DEBUG_PROP
    sw_off_t            prop_start_pos;
#endif
    int             saved_bytes;

    if ( count <= 0 )
        return;


    if (!SW_DB->fp_prop)
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
        prop_loc->seek = (sw_off_t)0;
        return;
    }

    /* write the property to disk */

    if ((prop_loc->seek = sw_ftell(SW_DB->fp_prop)) == (sw_off_t) -1)
        progerrno("O/S failed to tell me where I am - file number %d metaID %d : ", fi->filenum, propID);


    /* First write the compressed size */
    /* should be smaller than uncompressed_len if any */
    /* NOTE: uncompressed_len is 0 if no compression was made */
    compress1( buf_len, SW_DB->fp_prop, putc);

    /* Second write the number of saved bytes */
    if( !uncompressed_len )   /* No compression */
        saved_bytes = 0;
    else
        saved_bytes = uncompressed_len - buf_len;
    /* Write them */
    compress1( saved_bytes, SW_DB->fp_prop, putc);

#ifdef DEBUG_PROP
    prop_start_pos = sw_ftell(SW_DB->fp_prop);
#endif



    if ( (int)(written_bytes = sw_fwrite(buffer, 1, buf_len, SW_DB->fp_prop)) != buf_len) /* Write data */
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             i;
#ifdef DEBUG_PROP
    sw_off_t            start_seek;
#endif
#ifdef USE_BTREE
    sw_off_t            seek_pos;
#endif



    /* Just in case there were no properties for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }

#ifdef USE_BTREE
    /* now calculate index  (1 is for uniqueID )*/
    seek_pos = (sw_off_t)sizeof(long) * ((sw_off_t) 1 + (sw_off_t)(fi->filenum - 1) * (sw_off_t)count);
    sw_fseek(SW_DB->fp_propindex, seek_pos,SEEK_SET);
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
        start_seek = sw_ftell( SW_DB->fp );
#endif

        /* Write in portable format */
        printfileoffset( SW_DB->fp, prop_loc->seek, sw_fwrite );

#ifdef DEBUG_PROP
        printf("  PropIDX: %d  data=[seek: %ld]  main index location: %ld for %ld bytes (one print long)\n",
                 i,  prop_loc->seek, start_seek, sw_ftell( SW_DB->fp ) - start_seek );
#endif


#else
        printlong(SW_DB->fp_propindex, prop_loc->seek, sw_fwrite);
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    sw_off_t        seek_pos;
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
    seek_pos = (sw_off_t)(((sw_off_t)(fi->filenum - 1)) * ((sw_off_t)sizeof(sw_off_t)) * ((sw_off_t)(count))  + (sw_off_t)SW_DB->offsets[FILELISTPOS]);


    /* and seek to table */
    if (sw_fseek(SW_DB->fp, seek_pos, SEEK_SET) == -1)
        progerrno("Failed to seek to property index located at %ld for file number %d : ", seek_pos, fi->filenum);


#ifdef DEBUG_PROP
        printf("\nFetching seek positions for file %d\n", fi->filenum );
        printf(" property index table at %ld, this file at %ld\n", SW_DB->offsets[FILELISTPOS], seek_pos );
#endif


    /* Read in the prop indexes */
    for ( i=0; i < count; i++ )
    {
#ifdef DEBUG_PROP
        sw_off_t    seek_start = sw_ftell( SW_DB->fp );
#endif

        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

        prop_loc->seek = readfileoffset( SW_DB->fp, sw_fread );

#ifdef DEBUG_PROP
        printf("   PropIDX: %d  data[Seek: %ld] at seek %ld read %ld bytes (one readlong)\n", i, prop_loc->seek, seek_start, sw_ftell( SW_DB->fp ) - seek_start  );
#endif


    }
#else

    /* now calculate index  (1 is for uniqueID )*/
    seek_pos = (sw_off_t)sizeof(long) * ((sw_off_t) 1 + (sw_off_t)(fi->filenum - 1) * (sw_off_t)count);
    sw_fseek(SW_DB->fp_propindex, seek_pos,SEEK_SET);


    /* Read in the prop indexes */
    for ( i=0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];
        prop_loc->seek = readlong(SW_DB->fp_propindex, sw_fread);;
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
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    sw_off_t        seek_pos, prev_seek_pos;
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

    /* Preserve seek_pos */
    prev_seek_pos = sw_ftell(SW_DB->fp_prop);

    if (sw_fseek(SW_DB->fp_prop, seek_pos, SEEK_SET) == -1)
        progerrno("Failed to seek to properties located at %ld for file number %d : ", seek_pos, fi->filenum);

#ifdef DEBUG_PROP
    printf("Fetching filenum: %d propIDX: %d at seek: %ld\n", fi->filenum, propIDX, seek_pos);
#endif


    /* read compressed size (for use in zlib uncompression) */
    *buf_len = uncompress1( SW_DB->fp_prop, sw_fgetc );

    /* Get the uncompressed size */
    saved_bytes = uncompress1( SW_DB->fp_prop, sw_fgetc );

    /* If saved_bytes is 0 there was not any compression */
    if( !saved_bytes )             /* adjust *uncompressed_len */
        *uncompressed_len = 0;    /* This value means no compression */
    else
        *uncompressed_len = *buf_len + saved_bytes;


#ifdef DEBUG_PROP
    printf(" Fetched uncompressed length of %d (%ld bytes storage), now fetching %ld prop bytes from %ld\n",
             *uncompressed_len, sw_ftell( SW_DB->fp_prop ) - seek_pos, *buf_len, sw_ftell( SW_DB->fp_prop ) );
#endif


    /* allocate a read buffer */
    buffer = emalloc(*buf_len);


    if ( (int)sw_fread(buffer, 1, *buf_len, SW_DB->fp_prop) != *buf_len)
        progerrno("Failed to read properties located at %ld for file number %d : ", seek_pos, fi->filenum);

    /* Restore previous seek_pos */
    sw_fseek(SW_DB->fp_prop, prev_seek_pos,SEEK_SET);

    return buffer;
}


/****************************************************************
*  This routine closes the property file and reopens it as
*  readonly to improve seek times.
*  Note: It does not rename the property file.
*****************************************************************/

void    DB_Reopen_PropertiesForRead_Native(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    int     no_rename = 0;
    char   *s = estrdup(SW_DB->cur_prop_file);

    /* Close property file */
    DB_Close_File_Native(&SW_DB->fp_prop, &SW_DB->cur_prop_file, &no_rename);


    if (!(SW_DB->fp_prop = openIndexFILEForRead(s)))
        progerrno("Couldn't open the property file \"%s\": ", s);

    SW_DB->cur_prop_file = s;
}



#ifdef USE_BTREE


int    DB_WriteTotalWordsPerFile_Native(SWISH *sw, int idx, int wordcount, void *db)
{
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
   unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
   long seek_pos;

    /* now calculate index */
    seek_pos = (sw_off_t)first_record + (sw_off_t) sizeof(long) * (sw_off_t)index;
    sw_fseek(SW_DB->fp_totwords, seek_pos,SEEK_SET);
    printlong(SW_DB->fp_totwords, (long)wordcount, sw_fwrite);

   return 0;
}


int     DB_ReadTotalWordsPerFile_Native(SWISH *sw, int index, int *value, void *db)
{
   struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
   unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
   long seek_pos;

    /* now calculate index */
    seek_pos = (sw_off_t)first_record + (sw_off_t) sizeof(long) * (sw_off_t)index;
    sw_fseek(SW_DB->fp_totwords, seek_pos,SEEK_SET);
    *value =  readlong(SW_DB->fp_totwords, sw_fread);

   return 0;
}


#endif
