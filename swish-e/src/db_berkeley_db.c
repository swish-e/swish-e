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
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.
    
** Mon May  9 15:51:39 CDT 2005
** added GPL


**
** db_berkeley_db module
**
** 2001-07 jose ruiz initial coding
**
** Tested and Built with db-3.29 (See www.sleepycat.com)
**
** Must be linked with the correspondant libdb.a
**
*/

#include "swish.h"
#include "file.h"
#include "error.h"
#include "swstring.h"
#include "mem.h"
#include "compress.h"
#include "hash.h"
#include "db.h"
#include "db_berkeley_db.h"


/*
  -- init structures for this module
*/

void    initModule_DB_db(SWISH * sw)
{
    struct MOD_DB *Db;

    Db = (struct MOD_DB *) emalloc(sizeof(struct MOD_DB));

    Db->DB_name = (char *) estrdup("gdbm");

    Db->DB_Create = DB_Create_db;
    Db->DB_Open = DB_Open_db;
    Db->DB_Close = DB_Close_db;
    Db->DB_Remove = DB_Remove_db;

    Db->DB_InitWriteHeader = DB_InitWriteHeader_db;
    Db->DB_WriteHeaderData = DB_WriteHeaderData_db;
    Db->DB_EndWriteHeader = DB_EndWriteHeader_db;

    Db->DB_InitReadHeader = DB_InitReadHeader_db;
    Db->DB_ReadHeaderData = DB_ReadHeaderData_db;
    Db->DB_EndReadHeader = DB_EndReadHeader_db;

    Db->DB_InitWriteWords = DB_InitWriteWords_db;
    Db->DB_GetWordID = DB_GetWordID_db;
    Db->DB_WriteWord = DB_WriteWord_db;
    Db->DB_WriteWordHash = DB_WriteWordHash_db;
    Db->DB_WriteWordData = DB_WriteWordData_db;
    Db->DB_EndWriteWords = DB_EndWriteWords_db;

    Db->DB_InitReadWords = DB_InitReadWords_db;
    Db->DB_ReadWordHash = DB_ReadWordHash_db;
    Db->DB_ReadFirstWordInvertedIndex = DB_ReadFirstWordInvertedIndex_db;
    Db->DB_ReadNextWordInvertedIndex = DB_ReadNextWordInvertedIndex_db;
    Db->DB_ReadWordData = DB_ReadWordData_db;
    Db->DB_EndReadWords = DB_EndReadWords_db;

    Db->DB_InitWriteFiles = DB_InitWriteFiles_db;
    Db->DB_WriteFile = DB_WriteFile_db;
    Db->DB_EndWriteFiles = DB_EndWriteFiles_db;

    Db->DB_InitReadFiles = DB_InitReadFiles_db;
    Db->DB_ReadFile = DB_ReadFile_db;
    Db->DB_EndReadFiles = DB_EndReadFiles_db;

    Db->DB_InitWriteSortedIndex = DB_InitWriteSortedIndex_db;
    Db->DB_WriteSortedIndex = DB_WriteSortedIndex_db;
    Db->DB_EndWriteSortedIndex = DB_EndWriteSortedIndex_db;

    Db->DB_InitReadSortedIndex = DB_InitReadSortedIndex_db;
    Db->DB_ReadSortedIndex = DB_ReadSortedIndex_db;
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_db;


#ifdef PROPFILE
    Db->DB_WriteProperty = DB_WriteProperty_db;
    Db->DB_ReadProperty = DB_ReadProperty_db;
    Db->DB_Reopen_PropertiesForRead = DB_Reopen_PropertiesForRead_db;
#endif

    sw->Db = Db;

    return;
}


/*
  -- release all wired memory for this module
*/

void    freeModule_DB_db(SWISH * sw)
{
    efree(sw->Db->DB_name);
    efree(sw->Db);
    return;
}



/* ---------------------------------------------- */



/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int     configModule_DB_db(SWISH * sw, StringList * sl)
{
    // struct MOD_DB_db *md = sw->DB_db;
    // char *w0    = sl->word[0];
    int     retval = 1;


    retval = 0;                 // tmp due to empty routine

    return retval;
}



struct Handle_DB_db *open_db_files(char *dbname, u_int32_t flags)
{
    char   *tmp;
    DB     *dbp;
    struct Handle_DB_db *DB;
    int     ret;

    /* Allocate structure */
    DB = (struct Handle_DB_db *) emalloc(sizeof(struct Handle_DB_db));

    DB->wordcounter = 0;

    tmp = emalloc(strlen(dbname) + 7 + 1);


    /* Create index DB */

    /* db file to store header */
    /* we will use a hash for the pair values */

    sprintf(tmp, "%s_hdr.db", dbname);


    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }

    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }
    DB->dbf_header = dbp;



    /* db file to store words */

    sprintf(tmp, "%s_wds.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }

    DB->dbf_words = dbp;



    /* db file to store properties */

    sprintf(tmp, "%s_prp.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }

    DB->dbf_properties = dbp;



    /* db file to store word's data */

    sprintf(tmp, "%s_wdd.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }
    DB->dbf_worddata = dbp;



    /* db file to store word's inverted index */

    sprintf(tmp, "%s_wii.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_BTREE, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }
    DB->dbf_invertedindex = dbp;



    /* db file to store docs's data */

    sprintf(tmp, "%s_doc.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }
    DB->dbf_docs = dbp;



    /* db file to store sorted indexes */

    sprintf(tmp, "%s_srt.db", dbname);
    if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db_create: %s\n", db_strerror(ret));
    }
    if ((ret = dbp->open(dbp, tmp, NULL, DB_HASH, flags, 0664)) != 0)
    {
        dbp->err(dbp, ret, "%s", tmp);
        progerr("db_berkeley_db.c error in db->open: %s\n", db_strerror(ret));
    }
    DB->dbf_sorted_indexes = dbp;

    efree(tmp);

    return DB;
}

void   *DB_Create_db(SWISH *sw, char *dbname)
{
    return (void *) open_db_files(dbname, DB_CREATE);
}

void   *DB_Open_db(SWISH *sw, char *dbname)
{
    return (void *) open_db_files(dbname, DB_RDONLY);
}

void    DB_Close_db(void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;
    int     ret;

    dbp = DB->dbf_header;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }

    dbp = DB->dbf_words;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }

    dbp = DB->dbf_worddata;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }

    dbp = DB->dbf_invertedindex;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }

    dbp = DB->dbf_docs;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }

    dbp = DB->dbf_sorted_indexes;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }


    dbp = DB->dbf_properties;
    if ((ret = dbp->close(dbp, 0)) != 0)
    {
        progerr("db_berkeley_db.c error in db->close\n");
    }


    efree(DB);
}

void    DB_Remove_db(void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    dbp = DB->dbf_header;
    dbp = DB->dbf_words;
    dbp = DB->dbf_worddata;
    dbp = DB->dbf_invertedindex;
    dbp = DB->dbf_docs;
    dbp = DB->dbf_sorted_indexes;
    dbp = DB->dbf_properties;

    efree(DB);
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Header stuff                  */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteHeader_db(void *db)
{
    return 0;
}


int     DB_EndWriteHeader_db(void *db)
{
    return 0;
}

int     DB_WriteHeaderData_db(int id, unsigned char *s, int len, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(int);

    key.data = (char *) &id;

    content.size = len;
    content.data = s;

    dbp = DB->dbf_header;
    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("Berkeley DB error writing header");

    return 0;
}


int     DB_InitReadHeader_db(void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    int     ret;

    dbp = DB->dbf_header;

    /* Acquire a cursor for the database. */
    if ((ret = dbp->cursor(dbp, NULL, &DB->cursor_header, 0)) != 0)
    {
        DB->cursor_header = NULL;
    }

    return 0;
}

int     DB_ReadHeaderData_db(int *id, unsigned char **s, int *len, void *db)
{
    DB     *dbp;
    DBC    *dbcp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;
    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_header;
    dbcp = DB->cursor_header;


    if (!dbcp)
    {
        *id = 0;
        *len = 0;
        *s = NULL;
    }
    else
    {
        /* Initialize the key/data pair so the flags aren't set. */
        memset(&key, 0, sizeof(key));
        memset(&content, 0, sizeof(content));

        ret = dbcp->c_get(dbcp, &key, &content, DB_NEXT);
        if (!ret)
        {
            *id = *(int *) key.data;
            *len = content.size;
            *s = emalloc(content.size);
            memcpy(*s, content.data, content.size);
            //*s = content.data;
        }
        else
        {
            *id = 0;
            *len = 0;
            *s = NULL;
        }
    }
    return 0;
}


int     DB_EndReadHeader_db(void *db)
{
    DB     *dbp;
    DBC    *dbcp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    dbp = DB->dbf_header;
    dbcp = DB->cursor_header;

    if (dbcp)
        dbcp->c_close(dbcp);

    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*                 Word Stuff                 */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteWords_db(void *db)
{
    return 0;
}


int     DB_EndWriteWords_db(void *db)
{
    return 0;
}

long    DB_GetWordID_db(void *db)
{
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    return (long) ++(DB->wordcounter);
}

int     DB_WriteWord_db(char *word, long wordID, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_invertedindex;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = strlen(word) + 1;
    key.data = (char *) word;

    content.size = sizeof(int);

    content.data = (char *) &wordID;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("DB error writing word");

    return 0;
}


long    DB_WriteWordData_db(long wordID, unsigned char *worddata, int lendata, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_worddata;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(long);

    key.data = (char *) &wordID;

    content.size = lendata;
    content.data = worddata;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("DB error writing word's data");

    return 0;
}



int     DB_WriteWordHash_db(char *word, long wordID, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_words;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = strlen(word) + 1;
    key.data = (char *) word;

    content.size = sizeof(int);

    content.data = (char *) &wordID;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("DB error writing word");

    return 0;
}

int     DB_InitReadWords_db(void *db)
{
    return 0;
}

int     DB_EndReadWords_db(void *db)
{
    return 0;
}


int     DB_ReadWordHash_db(char *word, long *wordID, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_words;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = strlen(word) + 1;
    key.data = (char *) word;

    ret = dbp->get(dbp, NULL, &key, &content, 0);

    if (ret)
    {
        *wordID = 0;
    }
    else
    {
        *wordID = (long) *(int *) content.data;
    }

    return 0;
}



int     DB_ReadFirstWordInvertedIndex_db(char *word, char **resultword, long *wordID, void *db)
{
    DB     *dbp;
    DBC    *dbcp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;
    DBT     key,
            content;
    int     ret;
    int     len;

    len = strlen(word);

    dbp = DB->dbf_invertedindex;
    dbcp = NULL;

    DB->cursor_inverted = NULL;

    /* Acquire a cursor for the database. */
    if ((ret = dbp->cursor(dbp, NULL, &DB->cursor_inverted, 0)) != 0)
    {
        DB->cursor_inverted = NULL;
    }
    dbcp = DB->cursor_inverted;

    /* Initialize the key/data pair so the flags aren't set. */
    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = strlen(word);
    key.data = word;

    ret = dbcp->c_get(dbcp, &key, &content, DB_SET_RANGE);
    if (!ret)
    {
        if (strncmp(word, key.data, len) == 0)
        {
            *resultword = emalloc(key.size + 1);
            memcpy(*resultword, key.data, key.size);
            (*resultword)[key.size] = '\0';
            *wordID = *(long *) content.data;
            return 0;
        }
    }
    *resultword = NULL;
    *wordID = 0;
    dbcp->c_close(dbcp);
    DB->cursor_inverted = NULL;

    return 0;
}

int     DB_ReadNextWordInvertedIndex_db(char *word, char **resultword, long *wordID, void *db)
{
    DB     *dbp;
    DBC    *dbcp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;
    DBT     key,
            content;
    int     ret;
    int     len;

    len = strlen(word);

    dbp = DB->dbf_invertedindex;
    dbcp = DB->cursor_inverted;


    if (!dbcp)
    {
        *resultword = NULL;
        *wordID = 0;
    }
    else
    {
        /* Initialize the key/data pair so the flags aren't set. */
        memset(&key, 0, sizeof(key));
        memset(&content, 0, sizeof(content));

        ret = dbcp->c_get(dbcp, &key, &content, DB_NEXT);
        if (!ret)
        {
            if (strncmp(word, key.data, len) == 0)
            {

                *resultword = emalloc(key.size + 1);
                memcpy(*resultword, key.data, key.size);
                (*resultword)[key.size] = '\0';
                *wordID = *(long *) content.data;
                return 0;
            }
        }
        *resultword = NULL;
        *wordID = 0;
        dbcp->c_close(dbcp);
        DB->cursor_inverted = NULL;
    }
    return 0;
}


long    DB_ReadWordData_db(long wordID, unsigned char **worddata, int *lendata, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_worddata;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(long);

    key.data = (char *) &wordID;

    ret = dbp->get(dbp, NULL, &key, &content, 0);

    if (ret)
    {
        *lendata = 0;
        *worddata = NULL;
    }
    else
    {
        *lendata = content.size;
        *worddata = emalloc(content.size);
        memcpy(*worddata, content.data, content.size);
        //*worddata = content.data;
    }

    return 0;
}

/*--------------------------------------------*/
/*--------------------------------------------*/
/*              FileList  Stuff               */
/*--------------------------------------------*/
/*--------------------------------------------*/

int     DB_InitWriteFiles_db(void *db)
{
    return 0;
}


int     DB_EndWriteFiles_db(void *db)
{
    return 0;
}

int     DB_WriteFile_db(int filenum, unsigned char *filedata, int sz_filedata, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_docs;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(int);

    key.data = (char *) &filenum;

    content.size = sz_filedata;
    content.data = (char *) filedata;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("DB error writing docs");

    return 0;
}

int     DB_InitReadFiles_db(void *db)
{
    return 0;
}

int     DB_ReadFile_db(int filenum, unsigned char **filedata, int *sz_filedata, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_docs;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    // This needs to be fixed --> The old compress problem with zeroes
    filenum--;

    key.size = sizeof(int);

    key.data = (char *) &filenum;

    ret = dbp->get(dbp, NULL, &key, &content, 0);

    if (ret)
    {
        *sz_filedata = 0;
        *filedata = NULL;
    }
    else
    {
        *sz_filedata = content.size;
        *filedata = emalloc(content.size);
        memcpy(*filedata, content.data, content.size);
        //*filedata = content.data;
    }
    return 0;
}


int     DB_EndReadFiles_db(void *db)
{
    return 0;
}


/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Sorted data Stuff             */
/*--------------------------------------------*/
/*--------------------------------------------*/





int     DB_InitWriteSortedIndex_db(void *db)
{
    return 0;
}

int     DB_WriteSortedIndex_db(int propID, unsigned char *data, int sz_data, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_sorted_indexes;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(int);

    key.data = (char *) &propID;

    content.size = sz_data;
    content.data = (char *) data;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    if (ret)
        progerr("DB error writing sorted indexes");

    return 0;
}

int     DB_EndWriteSortedIndex_db(void *db)
{
    return 0;
}


int     DB_InitReadSortedIndex_db(void *db)
{
    return 0;
}

int     DB_ReadSortedIndex_db(int propID, unsigned char **data, int *sz_data, void *db)
{
    DB     *dbp;
    struct Handle_DB_db *DB = (struct Handle_DB_db *) db;

    DBT     key,
            content;
    int     ret;

    dbp = DB->dbf_sorted_indexes;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    key.size = sizeof(int);

    key.data = (char *) &propID;

    ret = dbp->get(dbp, NULL, &key, &content, 0);

    if (ret)
    {
        *sz_data = 0;
        *data = NULL;
    }
    else
    {
        *sz_data = content.size;
        *data = emalloc(content.size);
        memcpy(*data, content.data, content.size);
        //*data = content.data;
    }

    return 0;
}

int     DB_EndReadSortedIndex_db(void *db)
{
    return 0;
}


#ifdef PROPFILE
/*--------------------------------------------*/
/*--------------------------------------------*/
/*              Properties                    */
/*--------------------------------------------*/
/*--------------------------------------------*/


typedef union
{
    char   skey;
    struct {
        int     filenum;
        int     propID;
    } key;
} PropKEY;


void    DB_WriteProperty_db( FileRec *fi, int propID, char *buffer, int datalen, void *db )
{
    DB     *dbp;
    struct  Handle_DB_db *DB = (struct Handle_DB_db *) db;
    DBT     key,
            content;
    PropKEY propkey;
    int     ret;


    /*** Just until I can get a flag to see if this data is needed ***/
    /*** create empty space using arrays! ***/

    /* Create places to store the seek positions and lengths if first time */
    if ( !fi->propSize )
    {
        int i;

        fi->propLocationsCount = fi->docProperties->n;
        fi->propLocations = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );
        fi->propSize = (long *) emalloc( fi->propLocationsCount * sizeof( int *) );

        /* Zero array */
        for( i = 0; i < fi->propLocationsCount; i++ )
        {
            fi->propSize[ i ] = 0;         // here's the flag!
            fi->propLocations[ i ] = 0;    // not here!
        }
    }
    


    dbp = DB->dbf_properties;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    propkey.key.filenum = fi->filenum;
    propkey.key.propID  = propID;
    key.size = sizeof(propkey);
    key.data = (char *) &propkey;

    content.size = datalen;
    content.data = (char *) buffer;

    ret = dbp->put(dbp, NULL, &key, &content, 0);

    /* error message? */
    if (ret)
        progerr("DB error writing properties filenum: %d property %d", fi->filenum, propID);
}


char  * DB_ReadProperty_db( FileRec *fi, int propID, void *db )
{
    DB     *dbp;
    struct  Handle_DB_db *DB = (struct Handle_DB_db *) db;
    DBT     key,
            content;
    PropKEY propkey;
    char   *buffer;
    int     ret;


    dbp = DB->dbf_properties;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    propkey.key.filenum = fi->filenum;
    propkey.key.propID  = propID;
    key.size = sizeof(propkey);
    key.data = (char *) &propkey;

    ret = dbp->get(dbp, NULL, &key, &content, 0);

    /* No property for this file:id? */
    if (ret)
        return NULL;

    buffer = (char *)emalloc(content.size);
    memcpy(buffer, content.data, content.size);

    return buffer;
}




void    DB_Reopen_PropertiesForRead_db(void *db)
{
    return;
}
#endif
