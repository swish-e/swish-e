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


#define WRITE_WORDS_RAMDISK 1

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
    Db->DB_WriteWordHash = DB_WriteWordHash_Native;
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
    Db->DB_EndReadSortedIndex = DB_EndReadSortedIndex_Native;

    Db->DB_WriteProperty = DB_WriteProperty_Native;
    Db->DB_WritePropPositions = DB_WritePropPositions_Native;
    Db->DB_ReadProperty = DB_ReadProperty_Native;
    Db->DB_ReadPropPositions = DB_ReadPropPositions_Native;
    Db->DB_Reopen_PropertiesForRead = DB_Reopen_PropertiesForRead_Native;


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
        progerr("File \"%s\" has an unknown format.", DB->cur_index_file);

    {
        long    index,
                prop;

        index = readlong(DB->fp, fread);
        prop = readlong(DB->prop, fread);

        if (index != prop)
            progerr("Index file '%s' and property file '%s' are not related.", DB->cur_index_file, DB->cur_prop_file);
    }

}

struct Handle_DBNative *newNativeDBHandle(char *dbname)
{
    struct Handle_DBNative *DB;

    /* Allocate structure */
    DB = (struct Handle_DBNative *) emalloc(sizeof(struct Handle_DBNative));

    DB->offsetstart = 0;
    DB->hashstart = 0;
    DB->nextwordoffset = 0;
    DB->num_words = 0;
    DB->wordhash_counter = 0;
    DB->wordhashdata = NULL;
    DB->worddata_counter = 0;
    DB->mode = 1;
    DB->lastsortedindex = 0;
    DB->next_sortedindex = 0;
    DB->rd = NULL;
    DB->tmp_index = 0;          /* flags that the index is opened as create as a temporary file name */
    DB->tmp_prop = 0;
    DB->cur_index_file = NULL;
    DB->cur_prop_file = NULL;
    DB->fp = NULL;
    DB->prop = NULL;


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


void   *DB_Create_Native(char *dbname)
{
    int     i;
    long    swish_magic;
    FILE   *fp;
    char   *filename;
    long    unique_ID;          /* just because it's called that doesn't mean it is! */

    struct Handle_DBNative *DB;


    unique_ID = (long) time(NULL); /* Ok, so if more than one index is created the second... */

    /* Allocate structure */
    DB = (struct Handle_DBNative *) newNativeDBHandle(dbname);

#ifdef USE_TEMPFILE_EXTENSION
    filename = emalloc(strlen(dbname) + strlen(USE_TEMPFILE_EXTENSION) + strlen(PROPFILE_EXTENSION) + 1);
    strcpy(filename, dbname);
    strcat(filename, USE_TEMPFILE_EXTENSION);
    DB->tmp_index = 1;
#else
    filename = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + 1);
    strcpy(filename, dbname);
#endif


    /* Create index File */

    CreateEmptyFile(filename);
    if (!(DB->fp = openIndexFILEForReadAndWrite(filename)))
        progerrno("Couldn't create the index file \"%s\": ", filename);

    DB->cur_index_file = estrdup(filename);
    fp = DB->fp;


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

    efree(filename);


    for (i = 0; i < MAXCHARS; i++)
        DB->offsets[i] = 0L;
    for (i = 0; i < SEARCHHASHSIZE; i++)
        DB->hashoffsets[i] = 0L;
    for (i = 0; i < SEARCHHASHSIZE; i++)
        DB->lasthashval[i] = 0L;

    swish_magic = SWISH_MAGIC;
    printlong(DB->fp, swish_magic, fwrite);

    printlong(DB->fp, unique_ID, fwrite);
    printlong(DB->prop, unique_ID, fwrite);



    /* Reserve space for offset pointers */
    DB->offsetstart = ftell(fp);
    for (i = 0; i < MAXCHARS; i++)
        printlong(fp, (long) 0, fwrite);

    DB->hashstart = ftell(fp);
    for (i = 0; i < SEARCHHASHSIZE; i++)
        printlong(fp, (long) 0, fwrite);

    return (void *) DB;
}

/*******************************************************************
*   DB_Open_Native
*
*******************************************************************/

void   *DB_Open_Native(char *dbname)
{
    struct Handle_DBNative *DB;
    FILE   *fp;
    int     i;

    DB = (struct Handle_DBNative *) newNativeDBHandle(dbname);

    /* Create index File */
    if (!(DB->fp = openIndexFILEForRead(dbname)))
        progerrno("Could not open the index file '%s': ", dbname);

    DB->cur_index_file = estrdup(dbname);

    {
        char   *s = emalloc(strlen(dbname) + strlen(PROPFILE_EXTENSION) + 1);

        strcpy(s, dbname);
        strcat(s, PROPFILE_EXTENSION);

        if (!(DB->prop = openIndexFILEForRead(s)))
            progerrno("Couldn't open the property file \"%s\": ", s);

        DB->cur_prop_file = s;
    }


    /* Validate index files */
    DB_CheckHeader(DB);

    fp = DB->fp;

    /* Read offsets lookuptable */
    DB->offsetstart = ftell(fp);
    for (i = 0; i < MAXCHARS; i++)
        DB->offsets[i] = readlong(fp, fread);

    /* Read hashoffsets lookuptable */
    DB->hashstart = ftell(fp);
    for (i = 0; i < SEARCHHASHSIZE; i++)
        DB->hashoffsets[i] = readlong(fp, fread);

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
        progerr("Called close on non-opened file '%s'", *filename);

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

        *tempflag = 0;          /* no longer opened as a temporary file */
        efree(newname);
    }
#endif

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

    if (DB->mode)               /* If we are indexing update offsets to words and files */
    {
        /* Update internal pointers */

        fseek(fp, DB->offsetstart, 0);
        for (i = 0; i < MAXCHARS; i++)
            printlong(fp, DB->offsets[i], fwrite);

        fseek(fp, DB->hashstart, 0);
        for (i = 0; i < SEARCHHASHSIZE; i++)
            printlong(fp, DB->hashoffsets[i], fwrite);
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

    DB->offsets[HEADERPOS] = ftell(DB->fp);
    return 0;
}


int     DB_EndWriteHeader_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    /* End of header delimiter */
    fputc(0, fp);

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
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    DB->offsets[WORDPOS] = ftell(DB->fp);
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
    FILE   *fp = (FILE *) DB->fp;
    int     i,
            wordlen;
    long    wordID,
            f_hash_offset,
            f_offset,
            word_pos;

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
    /* Restore file pointer at the end of file */
    fseek(DB->fp, 0, SEEK_END);
    fputc(0, DB->fp);           /* End of words mark */
    return 0;
}

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


long    DB_WriteWordData_Native(long wordID, unsigned char *worddata, int lendata, void *db)
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
        progerrno("Internal db_native.c error in DB_WriteWordData_Native");
    DB->wordhashdata[3 * numhash->index + 2] = ftell(fp);

    DB->worddata_counter++;

    /* Write the worddata to disk */
    compress1(lendata, fp, fputc);
    fwrite(worddata, lendata, 1, fp);

    /* A NULL byte to indicate end of word data */
    fputc(0, fp);

    return 0;
}


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

    hashval = searchhash(word);

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
            progerrno("Internal db_native.c error in DB_WriteWordHash_Native");
        DB->wordhashdata[3 * numhash->index + 1] = (long) wordID;
    }
    DB->lasthashval[hashval] = wordID;

    return 0;
}

int     DB_InitReadWords_Native(void *db)
{
    return 0;
}

int     DB_EndReadWords_Native(void *db)
{
    return 0;
}


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
    hashval = searchhash(word);
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

    if (!DB->nextwordoffset)
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


long    DB_ReadWordData_Native(long wordID, unsigned char **worddata, int *lendata, void *db)
{
    int     len;
    unsigned char *buffer;
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    FILE   *fp = DB->fp;

    fseek(fp, wordID, 0);
    len = uncompress1(fp, fgetc);
    buffer = emalloc(len);
    fread(buffer, len, 1, fp);

    *worddata = buffer;
    *lendata = len;

    return 0;
}

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

int     DB_EndReadSortedIndex_Native(void *db)
{
   return 0;
}







/* 
** Jose Ruiz 04/00
** Store a portable long with just four bytes
*/
void    printlong(FILE * fp, unsigned long num, size_t(*f_write) (const void *, size_t, size_t, FILE *))
{
    num = PACKLONG(num);        /* Make the number portable */
    f_write(&num, MAXLONGLEN, 1, fp);
}

/* 
** Jose Ruiz 04/00
** Read a portable long (just four bytes)
*/
unsigned long readlong(FILE * fp, size_t(*f_read) (void *, size_t, size_t, FILE *))
{
    unsigned long num;

    f_read(&num, MAXLONGLEN, 1, fp);
    return UNPACKLONG(num);     /* Make the number readable */
}



/****************************************************************************
*   Init Writing files
*
*   Properties are written sequentially to the .prop file
*   Fixed length records of the file length and seek position into the
*   property file are written sequentially to the main index.
*
*   DB_InitWriteFiles is called first time a property is written
*   to save the offset of the property index.
*
*   DB_WriteProperty writes a property
*   DB_WritePropPositions write the seek pointers to the main index and
*   *must* be called after processing each file.
*   This is all done in WritePropertiesToDisk().
*
*   The index tables are all based on the count of properties in the index.
*
*
*****************************************************************************/


int     DB_InitWriteFiles_Native(void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;

    DB->offsets[FILELISTPOS] = ftell(DB->fp);
    return 0;
}


/****************************************************************************
*   Writes a property to the property file
*
*
*****************************************************************************/

void    DB_WriteProperty_Native( SWISH *sw, FileRec *fi, int propID, char *buffer, int buf_len, int uncompressed_len, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    int             written_bytes;
    PROP_INDEX      *pindex = fi->prop_index;
    PROP_LOCATION   *prop_loc;
    IndexFILE       *indexf = sw->indexlist;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             propIDX = header->metaID_to_PropIDX[propID];

    if ( count <= 0 )
        return;
    

    if (!DB->prop)
        progerr("Property database file not opened\n");


    /* Create place to store seek positions and lengths on first call for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }


    /* make an alias */
    prop_loc = &pindex->prop_position[ propIDX ];



    /* write the property to disk */

    if ((prop_loc->seek = ftell(DB->prop)) == -1)
        progerrno("O/S failed to tell me where I am - file number %d metaID %d : ", fi->filenum, propID);


    /* First write the uncompressed size */
   compress1( uncompressed_len+1, DB->prop, putc);


    if ((written_bytes = fwrite(buffer, 1, buf_len, DB->prop)) != buf_len) /* Write data */
        progerrno("Failed to write file number %d metaID %d to property file.  Tried to write %d, wrote %d : ", fi->filenum, propID, buf_len,
                  written_bytes);

    prop_loc->length = buf_len;     /* length of this prop */
}


/****************************************************************************
*   Writes out the seek positions for the properties
*
*   This writes out a fixed size records, one for each file.  Each
*   record is a list of <length>:<seek pos> entries, one for
*   each property defined.  Length is null if this file doesn't have a
*   property.
*
*   The advantage of the fixed width records is that the can be written
*   to disk after each file, saving RAM, and more importanly, all the
*   files don't need to be read when searhing.  Can just seek to the
*   file of interest, read the table, then read the property file.
*
*   This comes at a cost of disk space, since the much of the
*   data could be compressed.
*
*   An optional approach would be to only save the seek positions, plus
*   an extra seek position at the end (the next position after the last
*   property.  Then could calculate length by comparing the various start
*   positions.
*
*   For, say, five properties this would save 5 x 4(bytes/int) 20 bytes per
*   file.  But we need an extra position, so that's 20 - 4 = 16.  So, for
*   100,000 files that's only 1.6M of disk space.  Probably not worth the trouble.
*
*
*****************************************************************************/
void DB_WritePropPositions_Native(SWISH *sw, FileRec *fi, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    IndexFILE       *indexf = sw->indexlist;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    int             index_size;
    int             i;



    /* Just in case there were no properties for this file */
    if ( !pindex )
    {
        index_size = sizeof( PROP_INDEX ) + sizeof( PROP_LOCATION ) * (count - 1);
        pindex = fi->prop_index = emalloc( index_size );
        memset( pindex, 0, index_size );
    }


    /* Write out the prop index */
    for ( i = 0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

        /* Write in portable format */
        printlong( DB->fp, prop_loc->length, fwrite );
        printlong( DB->fp, prop_loc->seek, fwrite );
    }

    efree( pindex );
    fi->prop_index = NULL;;
}

/****************************************************************************
*   Reads in the seek positions for the properties
*
*
*****************************************************************************/
void DB_ReadPropPositions_Native(SWISH *sw, FileRec *fi, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    IndexFILE       *indexf = sw->indexlist;
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


    /* now calculate seek_pos */
    seek_pos = ((fi->filenum - 1) * index_size)  + DB->offsets[FILELISTPOS];


    /* and seek to table */
    if (fseek(DB->fp, seek_pos, 0) == -1)
        progerrno("Failed to seek to property index located at %ld for file number %d : ", seek_pos, fi->filenum);


    /* Read in the prop indexes */
    for ( i=0; i < count; i++ )
    {
        /* make an alias */
        PROP_LOCATION *prop_loc = &pindex->prop_position[ i ];

        prop_loc->length = readlong( DB->fp, fread );
        prop_loc->seek = readlong( DB->fp, fread );
    }

}

    

/****************************************************************************
*   Reads a property from the property file
*
*   Returns:
*       *char (buffer -- must be destoryed by caller)
*
*****************************************************************************/
char   *DB_ReadProperty_Native(SWISH *sw, FileRec *fi, int propID, int *buf_len, int *uncompressed_len, void *db)
{
    struct Handle_DBNative *DB = (struct Handle_DBNative *) db;
    PROP_INDEX      *pindex = fi->prop_index;
    IndexFILE       *indexf = sw->indexlist;
    INDEXDATAHEADER *header = &indexf->header;
    int             count = header->property_count;
    long            seek_pos;
    int             propIDX = header->metaID_to_PropIDX[propID];
    PROP_LOCATION   *prop_loc;
    char            *buffer;
    int             length;


    if ( count <= 0 )
        return NULL;


    /* read in the index pointers if not already loaded */
    if ( !pindex )
    {
        DB_ReadPropPositions_Native( sw, fi, db);
        pindex = fi->prop_index;
    }


    if ( !pindex )
        progerr("Failed to call DB_ReadProperty_Native with seek positions");

    prop_loc = &pindex->prop_position[ propIDX ];



    seek_pos = pindex->prop_position[propIDX].seek;
    length   = pindex->prop_position[propIDX].length;

    *buf_len = length;  /* pass the length back */


    /* Any for this metaID? */
    if (!length )
        return NULL;


    if (fseek(DB->prop, seek_pos, 0) == -1)
        progerrno("Failed to seek to properties located at %ld for file number %d : ", seek_pos, fi->filenum);


    /* read uncomprssed size (for use in zlib uncompression) */
    *uncompressed_len = uncompress1( DB->prop, fgetc ) - 1;


    /* allocate a read buffer */
    buffer = emalloc(length);


    if (fread(buffer, 1, length, DB->prop) != length)
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



