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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


************************************************************************************
 * DocProperties.c, DocProperties.h
 *
 * Functions to manage the index's Document Properties 
 *
 * File Created.
 * M. Gaulin 8/10/98
 * Jose Ruiz 2000/10 many modifications
 * Jose Ruiz 2001/01 many modifications
 *
 * 2001-01-26  rasc  getPropertyByname changed
 * 2001-02-09  rasc  printSearchResultProperties changed
 */

#ifdef __cplusplus
extern "C" {
#endif

void freeProperty( propEntry *prop );
void freeDocProperties (docProperties *);
void freefileinfo(FileRec *);

unsigned char *storeDocProperties (docProperties *, int *);

unsigned char *allocatePropIOBuffer(SWISH *sw, unsigned long buf_needed );

propEntry *getDocProperty( RESULT *result, struct metaEntry **meta_entry, int metaID, int max_size );
propEntry *CreateProperty(struct metaEntry *meta_entry, unsigned char *propValue, int propLen, int preEncoded, int *error_flag );
void addDocProperties( INDEXDATAHEADER *header, docProperties **docProperties, unsigned char *propValue, int propLen, char *filename );
int addDocProperty (docProperties **, struct metaEntry * , unsigned char* ,int, int );
int Compare_Properties( struct metaEntry *meta_entry, propEntry *p1, propEntry *p2 );

unsigned char *fetchDocProperties ( FileRec *, char * );


void swapDocPropertyMetaNames (docProperties **, struct metaMergeEntry *);

char *getResultPropAsString(RESULT *, int);
char *DecodeDocProperty( struct metaEntry *meta_entry, propEntry *prop );
void getSwishInternalProperties(FileRec *, IndexFILE *);


PropValue *getResultPropValue (RESULT *r, char *name, int ID);
void    freeResultPropValue(PropValue *pv);



void     WritePropertiesToDisk( SWISH *sw , FileRec *fi);
propEntry *ReadSingleDocPropertiesFromDisk( IndexFILE *indexf, FileRec *fi, int metaID, int max_size );
docProperties *ReadAllDocPropertiesFromDisk( IndexFILE *indexf, int filenum );



/*
   -- Mapping AutoProperties <-> METANAMES  
   -- should be the same
*/

/* all AutoPropteries start with this string ! */


#define AUTOPROPERTY_DEFAULT      "swishdefault"
#define AUTOPROPERTY_REC_COUNT    "swishreccount"
#define AUTOPROPERTY_RESULT_RANK  "swishrank"
#define AUTOPROPERTY_FILENUM      "swishfilenum"
#define AUTOPROPERTY_INDEXFILE    "swishdbfile"

#define AUTOPROPERTY_DOCPATH      "swishdocpath"
#define AUTOPROPERTY_TITLE        "swishtitle"
#define AUTOPROPERTY_DOCSIZE      "swishdocsize"
#define AUTOPROPERTY_LASTMODIFIED "swishlastmodified"
#define AUTOPROPERTY_SUMMARY      "swishdescription"
#define AUTOPROPERTY_STARTPOS     "swishstartpos"

#ifdef __cplusplus
}
#endif /* __cplusplus */

