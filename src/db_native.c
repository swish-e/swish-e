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
#include "db_native.h"


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

static void _DB_CheckHeader(struct Handle_DBNative *SW_DB)
{
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
}

static struct Handle_DBNative *newNativeDBHandle(SWISH *sw, char *dbname)
{
    struct Handle_DBNative *SW_DB;

    /* Allocate structure */
    SW_DB = (struct Handle_DBNative *) emalloc(sizeof(struct Handle_DBNative));
    memset( SW_DB, 0, sizeof( struct Handle_DBNative ));

    SW_DB->sw = sw;  /* for error messages */

    SW_DB->w_tell = sw_ftell;
    SW_DB->w_write = sw_fwrite;
    SW_DB->w_seek = sw_fseek;
    SW_DB->w_read = sw_fread;
    SW_DB->w_close = sw_fclose;
    SW_DB->w_putc = sw_fputc;
    SW_DB->w_getc = sw_fgetc;

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

/* Routine to compare packed longs - required to get BTREE duplicate entries
** sorted by wordID. wordID is a packed long
*/
int compare_packed_long(DB *dbp, const DBT *a, const DBT *b)
{
    return memcmp(a->data, b->data, sizeof(long)); 
}

DB * OpenBerkeleyFile(char *filename, DBTYPE db_type, u_int32_t db_flags, int dup)
{
    DB *dbp;
    int db_ret;
    if((db_ret = db_create(&dbp, NULL, 0)))
        progerrno("Couldn't create BERKELEY DB resource");
    if(dup)
    {
        if((db_ret = dbp->set_flags(dbp, DB_DUPSORT)))
            progerrno("Couldn't set DB_DUPSORT in DB Berkeley file \"%s\": ", filename);
        if((db_ret = dbp->set_dup_compare(dbp,compare_packed_long)))
            progerrno("Couldn't set DB_DUPSORT_ROUTINE in DB Berkeley file \"%s\": ", filename);
        if((db_ret = dbp->set_bt_compare(dbp,compare_packed_long)))
            progerrno("Couldn't set DB_BTSORT_ROUTINE in DB Berkeley file \"%s\": ", filename);
    }    
    if((db_ret = dbp->open(dbp,NULL,filename,NULL,db_type,db_flags,0)))
    {
        dbp->err(dbp,db_ret,"Database open failed: \"%s\"", filename);
        progerrno("Couldn't open the DB Berkeley file \"%s\": ", filename);
    }

    return dbp;
}

DB * CreateBerkeleyFile(char *filename,DBTYPE db_type, int dup)
{
    return OpenBerkeleyFile(filename, db_type, DB_CREATE | DB_TRUNCATE, dup);
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


void   *_DB_Create(SWISH *sw, char *dbname)
{
    long    swish_magic;
    char   *filename;
    FILE   *fp_tmp;
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

    /* Create Btree Index File */
    strcpy(filename, dbname);
    strcat(filename, BTREE_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_btree = 1;
#endif
    SW_DB->db_btree = CreateBerkeleyFile(filename,DB_BTREE,1);
    /* Allow sorted duplicate items */
    SW_DB->cur_btree_file = estrdup(filename);


    /* Create WordData File */
    strcpy(filename, dbname);
    strcat(filename, WORDDATA_EXTENSION);
#ifdef USE_TEMPFILE_EXTENSION
    strcat(filename, USE_TEMPFILE_EXTENSION);
    SW_DB->tmp_worddata = 1;
#endif
    SW_DB->db_worddata = CreateBerkeleyFile(filename,DB_RECNO,0);
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
    SW_DB->db_hashfile = CreateBerkeleyFile(filename,DB_HASH,0);
    SW_DB->cur_hashfile_file = estrdup(filename);

    efree(filename);

    return (void *) SW_DB;
}


/*******************************************************************
*   _DB_Open
*
*******************************************************************/

void   *_DB_Open(SWISH *sw, char *dbname,int mode)
{
    struct Handle_DBNative *SW_DB;
    FILE   *(*openRoutine)(char *) = NULL;
    char   *s;
    u_int32_t db_flags;

    switch(mode)
    {
    case DB_READ:
        openRoutine = openIndexFILEForRead;
        db_flags = DB_RDONLY;
        break;
    case DB_READWRITE:
        openRoutine = openIndexFILEForReadAndWrite;
        db_flags = 0;
        break;
    default:
        openRoutine = openIndexFILEForRead;
        db_flags = DB_RDONLY;
    }

    SW_DB = (struct Handle_DBNative *) newNativeDBHandle(sw, dbname);
    SW_DB->mode = mode;

    s = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + 1);

    strcpy(s, dbname);
    strcat(s, PROPFILE_EXTENSION);

    if (!(SW_DB->fp_prop = openRoutine(s)))
    {
        set_progerrno(INDEX_FILE_ERROR, SW_DB->sw, "Couldn't open the property file \"%s\": ", s);
        return (void *) SW_DB;
    }

    SW_DB->cur_prop_file = s;

    s = emalloc(strlen(dbname) + strlen(BTREE_EXTENSION) + 1);
    strcpy(s, dbname);
    strcat(s, BTREE_EXTENSION);
    SW_DB->db_btree = OpenBerkeleyFile(s, DB_BTREE, db_flags, 1);

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
    SW_DB->db_worddata = OpenBerkeleyFile(s, DB_RECNO, db_flags, 0);
    SW_DB->cur_worddata_file = s;

    s = emalloc(strlen(dbname) + strlen(HASHFILE_EXTENSION) + 1);
    strcpy(s, dbname);
    strcat(s, HASHFILE_EXTENSION);
    SW_DB->db_hashfile = OpenBerkeleyFile(s, DB_HASH, db_flags, 0);
    SW_DB->cur_hashfile_file = s;

    /* Validate index files */
    _DB_CheckHeader(SW_DB);
    if ( SW_DB->sw->lasterror )
        return (void *) SW_DB;

    /* Put the file pointer of props, propindex and totwords files 
    ** at the end of the files
    ** This is very important because if we are in update mode
    ** we must avoid the properties to be overwritten
    */
    sw_fseek(SW_DB->fp_prop,(sw_off_t)0,SEEK_END);
    sw_fseek(SW_DB->fp_propindex,(sw_off_t)0,SEEK_END);
    sw_fseek(SW_DB->fp_totwords,(sw_off_t)0,SEEK_END);

    return (void *) SW_DB;
}


/****************************************************************
* This closes a file, and will rename if flagged as such
*  Frees the associated current file name
*
*****************************************************************/

static void _DB_Close_File(FILE ** fp, char **filename, int *tempflag)
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




void    _DB_Close(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    /* Close (and rename) property file, if it's open */
    _DB_Close_File(&SW_DB->fp_prop, &SW_DB->cur_prop_file, &SW_DB->tmp_prop);

    /* Close (and rename) worddata file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_worddata, &SW_DB->cur_worddata_file, &SW_DB->tmp_worddata);

    /* Close (and rename) btree file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_btree, &SW_DB->cur_btree_file, &SW_DB->tmp_btree);

    /* Close (and rename) propindex file, if it's open */
    if(SW_DB->fp_propindex)
    {
        /* Close (and rename) property file, if it's open */
        _DB_Close_File(&SW_DB->fp_propindex, &SW_DB->cur_propindex_file, &SW_DB->tmp_propindex);
    }
    /* Close (and rename) totwords file, if it's open */
    if(SW_DB->fp_totwords)
    {
        /* Close (and rename) totwords file, if it's open */
        _DB_Close_File(&SW_DB->fp_totwords, &SW_DB->cur_totwords_file, &SW_DB->tmp_totwords);
    }
    /* Close (and rename) presorted index file, if it's open */
    if(SW_DB->fp_presorted)
    {
        _DB_Close_File(&SW_DB->fp_presorted, &SW_DB->cur_presorted_file, &SW_DB->tmp_presorted);
    }
    /* Close (and rename) header index file, if it's open */
    if(SW_DB->fp_header)
    {
        _DB_Close_File(&SW_DB->fp_header, &SW_DB->cur_header_file, &SW_DB->tmp_header);
    }

    /* Close (and rename) hash-file index file, if it's open */
    CloseBerkeleyFile(&SW_DB->db_hashfile, &SW_DB->cur_hashfile_file, &SW_DB->tmp_hashfile);

    if (SW_DB->dbname)
        efree(SW_DB->dbname);
    efree(SW_DB);
}

void    _DB_Remove(void *db)
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

int     _DB_InitWriteHeader(void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
long swish_magic;

    /* Jump over swish_magic ID (long number) */
    sw_fseek(SW_DB->fp_header, (sw_off_t)0, SEEK_SET);
    swish_magic = readlong(SW_DB->fp_header, sw_fread);

    return 0;
}


int     _DB_EndWriteHeader(void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE   *fp = SW_DB->fp_header;

    /* End of header delimiter */
    if ( putc(0, fp) == EOF )
        progerrno("putc() failed: ");

    return 0;
}

int     _DB_WriteHeaderData(int id, unsigned char *s, int len, void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE   *fp = SW_DB->fp_header;

    compress1(id, fp, sw_fputc);
    compress1(len, fp, sw_fputc);
    if ( sw_fwrite(s, len, sizeof(char), fp) != sizeof( char ) ) /* seems backward */
        progerrno("Error writing to device while trying to write %d bytes: ", len );

    return 0;
}


int     _DB_InitReadHeader(void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
long swish_magic;

    /* Jump over swish_magic ID (long number) */
    sw_fseek(SW_DB->fp_header, (sw_off_t)0, SEEK_SET);
    swish_magic = readlong(SW_DB->fp_header, sw_fread);

    return 0;
}

int     _DB_ReadHeaderData(int *id, unsigned char **s, int *len, void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE   *fp = SW_DB->fp_header;
int     tmp;

    tmp = uncompress1(fp, sw_fgetc);
    *id = tmp;
    if (tmp)
    {
        tmp = uncompress1(fp, sw_fgetc);
        *s = (unsigned char *) emalloc(tmp + 1);
        *len = tmp;
        int ret = sw_fread(*s, *len, sizeof(char), fp);
        if (ret != sizeof(char)) { 
            progerrno("Failed to read DB header data");
        }
    }
    else
    {
        len = 0;
        *s = NULL;
    }

    return 0;
}

int     _DB_EndReadHeader(void *db)
{
    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*                 Word Stuff                 */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     _DB_InitWriteWords(void *db)
{
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

int     _DB_EndWriteWords(void *db)
{
    return 0;
}

sw_off_t    _DB_GetWordID(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;

    return (sw_off_t) SW_DB->worddata_counter;
}

int     _DB_WriteWord(char *word, sw_off_t wordID, void *db)
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

    ret = SW_DB->db_btree->put(SW_DB->db_btree,NULL,&key,&data,0);

    if(ret != 0)
    {
        printf("ERROR %d inserting word \"%s\" in Berkeley DB BTREE\n",ret,word);
    }
    SW_DB->num_words++;

    return 0;
}

long    _DB_WriteWordData(sw_off_t wordID, unsigned char *worddata, int data_size, int saved_bytes, void *db)
{
    unsigned char stack_buffer[8192]; /* just to avoid emalloc,efree overhead */
    unsigned char *buf, *p;
    int buf_size;
    DBT key,data;
    db_recno_t recno;
    int ret;

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

    /* Write the worddata to disk */
    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = &SW_DB->worddata_counter;
    key.size = sizeof(SW_DB->worddata_counter);
    data.data = buf;
    data.size = buf_size;
    ret = SW_DB->db_worddata->put(SW_DB->db_worddata,NULL,&key,&data,DB_APPEND);

    if(ret == 0)
    {
        recno = *(db_recno_t *) key.data;
        SW_DB->worddata_counter = recno;
    }
    else
    {
        printf("ERROR %d inserting worddata in Berkeley DB BTREE\n",ret);
    }

    if(buf != stack_buffer)
        efree(buf);
    return 0;
}

int     _DB_InitReadWords(void *db)
{
    return 0;
}

int     _DB_EndReadWords(void *db)
{
    return 0;
}

int     _DB_ReadWord(char *word, DB_WORDID **wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    sw_off_t dummy = 0;
    DBT key,data;
    int ret, found = 0;
    DBC *dbcp;
    DB_WORDID *head = NULL, *last = NULL, *tmp = NULL;

    /* Acquire a cursor for the database. */
    if ((ret = SW_DB->db_btree->cursor(SW_DB->db_btree, NULL, &dbcp, 0)) != 0)
    {
        //dbp->err(SW_DB->db_btree, ret, "DB->cursor"); return (1);
        *wordID = (sw_off_t)0;
    }
    else
    {
        /* Initialize the key/data pair. */
        memset(&key, 0, sizeof(key));
        memset(&data, 0, sizeof(data));

        key.data = word;
        key.size = strlen(word); 
        data.data = &dummy;
        data.ulen = sizeof(dummy); 
        data.flags = DB_DBT_USERMEM;
   
        /* Walk through the database and print out the key/data pairs. */
        ret = dbcp->c_get(dbcp, &key, &data, DB_SET);
        if (ret == 0)
        {
            found = 1;
            while(ret == 0)
            {
                tmp = (DB_WORDID *)emalloc(sizeof(DB_WORDID));
                tmp->wordID = UNPACKLONG(dummy);
                tmp->next = NULL;
                if(!head) head = tmp;
                if(last) last->next = tmp;
                last = tmp;
                ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT_DUP);
            }
        }
        dbcp->c_close(dbcp);
    }
    *wordID = head;

    return 0;
}

int     _DB_ReadFirstWordInvertedIndex(char *word, char **resultword, DB_WORDID **wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBC *dbcp;
    DBT key, data;
    int ret; 
    sw_off_t dummy;
    DB_WORDID *tmp = NULL;

    /* Acquire a cursor for the database. */
    if ((ret = SW_DB->db_btree->cursor(SW_DB->db_btree, NULL, &dbcp, 0)) != 0) 
    {
        //dbp->err(SW_DB->db_btree, ret, "DB->cursor"); return (1); 
        *resultword = NULL;
        *wordID = NULL;
    }
    else
    {
        /* Initialize the key/data pair. */
        memset(&key, 0, sizeof(key));
        memset(&data, 0, sizeof(data));
        dummy = 0;
        key.data = word;
        key.size = strlen(word);
        key.flags = DB_DBT_MALLOC;
        data.data = &dummy;
        data.ulen = sizeof(dummy);
        data.flags = DB_DBT_USERMEM;
       
        /* Walk through the database and print out the key/data pairs. */ 
        ret = dbcp->c_get(dbcp, &key, &data, DB_SET_RANGE);
        if (ret == 0)
        {
            if((key.size < strlen(word)) || (strncmp(word,key.data,strlen(word))!=0))
            {
                *resultword = NULL;
                tmp = NULL;
                dbcp->c_close(dbcp);
                SW_DB->dbc_btree = NULL;
            }
            else
            {
                *resultword = emalloc(key.size + 1);
                memcpy(*resultword,key.data,key.size);
                (*resultword)[key.size]='\0';
                dummy = UNPACKLONG(dummy);
                tmp = (DB_WORDID *)emalloc(sizeof(DB_WORDID));
                tmp->wordID = dummy;
                tmp->next = NULL;
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
            tmp = NULL;
            dbcp->c_close(dbcp);
            SW_DB->dbc_btree = NULL;
        }
    }

    *wordID = tmp;

    return 0;
}

int     _DB_ReadNextWordInvertedIndex(char *word, char **resultword, DB_WORDID **wordID, void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    DBC *dbcp = SW_DB->dbc_btree;
    DBT key, data;
    int ret;
    sw_off_t dummy = 0;
    DB_WORDID *tmp = NULL;

    *resultword = NULL;
    *wordID = NULL;

    if(dbcp)
    {
        /* Initialize the key/data pair. */
        memset(&key, 0, sizeof(key));
        memset(&data, 0, sizeof(data));
        dummy = 0;
        key.flags = DB_DBT_MALLOC;
        data.data = &dummy;
        data.ulen = sizeof(dummy);
        data.flags = DB_DBT_USERMEM;

        /* Walk through the database and print out the key/data pairs. */
        ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT);

        if(ret == 0)
        {
            if((key.size < strlen(word)) || (strncmp(word,key.data,strlen(word))!=0))
            {
                *resultword = NULL;
                *wordID = NULL;
                dbcp->c_close(dbcp);
                SW_DB->dbc_btree = NULL;
            }
            else
            {
                *resultword = emalloc(key.size + 1);
                memcpy(*resultword,key.data,key.size);
                (*resultword)[key.size]='\0';
                dummy = UNPACKLONG(dummy);
                tmp = (DB_WORDID *)emalloc(sizeof(DB_WORDID));
                tmp->wordID = dummy;
                tmp->next = NULL;
                *wordID = tmp;
            }
            efree(key.data);
        }
        else if (ret == DB_NOTFOUND)
        {
            *resultword = NULL;
            *wordID = NULL;
            dbcp->c_close(dbcp);
            SW_DB->dbc_btree = NULL;
        }
    }

    return 0;
}

long    _DB_ReadWordData(sw_off_t wordID, unsigned char **worddata, int *data_size, int *saved_bytes, void *db)
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
    key.flags = 0;
    data.flags = DB_DBT_MALLOC;

    ret = SW_DB->db_worddata->get(SW_DB->db_worddata, NULL, &key, &data, 0);

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
        fprintf(stderr, "%s: %s\n", __FILE__, db_strerror(ret));
        progerrno("Unexpected error from Berkeley worddata file \"%s\": ", SW_DB->cur_worddata_file);
    }
    return 0;
}


/*--------------------------------------------
** 2002/12 Jose Ruiz
**     FilePath,FileNum  pairs
**     Auxiliar hash index
*/

/* Routine to write path,filenum */
int     _DB_WriteFileNum(int filenum, unsigned char *filedata, int sz_filedata, void *db)
{
unsigned long tmp = (unsigned long)filenum;
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
DBT key,data;
int ret;

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
    ret = SW_DB->db_hashfile->put(SW_DB->db_hashfile,NULL,&key,&data,0);
    if(ret != 0)
    {
        printf("ERROR %d inserting file \"%s\" in Berkeley DB HASH\n",ret,filedata);
    }

    return 0;
}

/* Routine to get filenum from path */
int     _DB_ReadFileNum(unsigned char *filedata, void *db)
{
unsigned long filenum;
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
DBT key,data;
int ret;

    /* Get it from the hash index */
    /*Berkeley DB stuff */
    memset(&key,0,sizeof(DBT));
    memset(&data,0,sizeof(DBT));
    key.data = filedata;
    key.size = strlen((char *)filedata);
    data.data = &filenum;
    data.ulen = sizeof(filenum);
    data.flags = DB_DBT_USERMEM;
    /* Read it from the hash index */
    ret = SW_DB->db_hashfile->get(SW_DB->db_hashfile,NULL,&key,&data,0);

    if(ret == 0)
    {
        filenum = UNPACKLONG(filenum);
    }
    else if (ret == DB_NOTFOUND)
    {
        filenum = 0;
    }
    else
    {
        //$$$ unexpected return code
        printf("Unexpected return code %d from Berkeley BD FILEHASH while searching for \"%s\"\n",ret,filedata);
        filenum = 0;
    }

    return (int)filenum;
}

/* Routine to test if filenum was deleted */
int     _DB_CheckFileNum(int filenum, void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
unsigned long totwords;

    /* swish_magic number is in position 0 (first_record) */
    /* filenum starts in 1 (first filenum is 1) */
    /* Go to totalwords position  and read it */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t) first_record + (sw_off_t)(filenum - 1) * sizeof(long), SEEK_SET);
    totwords = readlong(SW_DB->fp_totwords, sw_fread);
 
    return (int) totwords;
}


/* Routine to remove a filenum */
/* At this moment, to remove a filenum I am just puting 0 in the number of
** words */
int     _DB_RemoveFileNum(int filenum, void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
long    swish_magic;

    /* Jump swish_magic number */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t)0, SEEK_SET);
    swish_magic = readlong(SW_DB->fp_totwords, sw_fread);

    /* Go to totalwords position  and write a 0 */
    sw_fseek(SW_DB->fp_totwords, (sw_off_t)(filenum - 1) * sizeof(long), SEEK_CUR);
    printlong(SW_DB->fp_totwords, 0, sw_fwrite);

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
* _DB_InitWriteSortedIndex should probably write a null for the first
* record's "next table entry" and set next_ and last_ pointers.  Then
* _DB_EndWriteSortedIndex call would not be needed.
*
* Notes/Questions:
*   Seems like result_sort.c is the only place that loads this -- in LoadSortedProps.
*   LoadSortedProps is used by merge, proplimit, and result_sort.  LoadSortedProps
*   uncompresses the array (and pre_sort.c compresses).  Why isn't that compression
*   and uncompression done here?
*
*
********************************************************************************/


int     _DB_InitWriteSortedIndex(void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE *fp = SW_DB->fp_presorted;

   SW_DB->next_sortedindex = sw_ftell(fp);
   return 0;
}

/********************************************************************************
* _DB_WriteSortedIndex
*
* Input:
*   propID      property id of this table
*   *data       pointer to char array compressed table
*   sz_data     size of the char array in bytes
*   db          where to write
*
*********************************************************************************/

int     _DB_WriteSortedIndex(int propID, unsigned char *data, int sz_data,void *db)
{
sw_off_t tmp1,tmp2;
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE *fp = SW_DB->fp_presorted;

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

int     _DB_EndWriteSortedIndex(void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE *fp = SW_DB->fp_presorted;

   printfileoffset(fp,(sw_off_t)0,sw_fwrite);  /* No next table mark - Useful if no presorted indexes */
         /* NULL meta id- Only useful if no presorted indexes  */

    if ( putc(0, fp) == EOF )
        progerrno("putc() failed writing null: ");

   return 0;
}


/* Non Btree read functions */


int     _DB_InitReadSortedIndex(void *db)
{
   return 0;
}

/***********************************************************************************
*  _DB_ReadSortedIndex -
*
*  Searches through the sorted indexes looking for one that matches the propID
*  passed in.  If found then malloc's a table and reads it in.
*  The data is in compressed format and is not uncompressed here
*
***********************************************************************************/

int     _DB_ReadSortedIndex(int propID, unsigned char **data, int *sz_data,void *db)
{
sw_off_t next;
long id, tmp;
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
FILE *fp = SW_DB->fp_presorted;
unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */

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
           int ret = sw_fread(*data,*sz_data,1,fp);
           if (ret != 1) {
                progerrno("Failed to read sorted index");
           }
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

int     _DB_ReadSortedData(int *data,int index, int *value, void *db)
{
    *value = data[index];
    return 0;
}

int     _DB_EndReadSortedIndex(void *db)
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
*   Writing Properites
*
*   Properties are written sequentially to the .prop file.
*   Fixed length records of the seek position into the
*   property file are written sequentially to the main index (which is why
*   there's a separate .prop file).
*
*   _DB_InitWriteProperties is called first time a property is written
*   to save the offset of the property index table in the main index.
*   It's simply a ftell() of the current position in the index and that
*   seek position is stored in the main index "offsets" table.
*
*   _DB_WriteProperty writes a property.
*
*   _DB_WritePropPositions write the seek pointers to the main index and
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


int     _DB_InitWriteProperties(void *db)
{
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

void    _DB_WriteProperty( IndexFILE *indexf, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db)
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
void _DB_WritePropPositions(IndexFILE *indexf, FileRec *fi, void *db)
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
sw_off_t            seek_pos;

    /* Just in case there were no properties for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }

    /* now calculate index  (1 is for uniqueID )*/
    seek_pos = (sw_off_t)sizeof(long) * ((sw_off_t) 1 + (sw_off_t)(fi->filenum - 1) * (sw_off_t)count);
    sw_fseek(SW_DB->fp_propindex, seek_pos,SEEK_SET);

#ifdef DEBUG_PROP
    printf("Writing seek positions to index for file %d\n", fi->filenum );
#endif

    /* Write out the prop index */
    for ( i = 0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

        printlong(SW_DB->fp_propindex, prop_loc->seek, sw_fwrite);
    }

    efree( pindex );
    fi->prop_index = NULL;;
}

/****************************************************************************
*   Reads in the seek positions for the properties
*
*
*****************************************************************************/
void _DB_ReadPropPositions(IndexFILE *indexf, FileRec *fi, void *db)
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
char   *_DB_ReadProperty(IndexFILE *indexf, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db)
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
        _DB_ReadPropPositions( indexf, fi, db);
        pindex = fi->prop_index;
    }

    if ( !pindex )
        progerr("Failed to call _DB_ReadProperty with seek positions");

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

void    _DB_Reopen_PropertiesForRead(void *db)
{
    struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
    int     no_rename = 0;
    char   *s = estrdup(SW_DB->cur_prop_file);

    /* Close property file */
    _DB_Close_File(&SW_DB->fp_prop, &SW_DB->cur_prop_file, &no_rename);


    if (!(SW_DB->fp_prop = openIndexFILEForRead(s)))
        progerrno("Couldn't open the property file \"%s\": ", s);

    SW_DB->cur_prop_file = s;
}

int    _DB_WriteTotalWordsPerFile(SWISH *sw, int idx, int wordcount, void *db)
{
struct Handle_DBNative *SW_DB = (struct Handle_DBNative *) db;
unsigned long first_record = sizeof(unsigned long); /* jump swish magic number */
long seek_pos;

    /* now calculate index */
    seek_pos = (sw_off_t)first_record + (sw_off_t) sizeof(long) * (sw_off_t)idx;
    sw_fseek(SW_DB->fp_totwords, seek_pos,SEEK_SET);
    printlong(SW_DB->fp_totwords, (long)wordcount, sw_fwrite);

   return 0;
}


int     _DB_ReadTotalWordsPerFile(SWISH *sw, int index, int *value, void *db)
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


