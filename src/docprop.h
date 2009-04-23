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

unsigned char *storeDocProperties (docProperties *, SWINT_T *);

unsigned char *allocatePropIOBuffer(SWISH *sw, SWUINT_T buf_needed );

propEntry *getDocProperty( RESULT *result, struct metaEntry **meta_entry, SWINT_T metaID, SWINT_T max_size );
propEntry *CreateProperty(struct metaEntry *meta_entry, unsigned char *propValue, SWINT_T propLen, SWINT_T preEncoded, SWINT_T *error_flag );
void addDocProperties( INDEXDATAHEADER *header, docProperties **docProperties, unsigned char *propValue, SWINT_T propLen, char *filename );
SWINT_T addDocProperty (docProperties **, struct metaEntry * , unsigned char* ,SWINT_T, SWINT_T );
SWINT_T Compare_Properties( struct metaEntry *meta_entry, propEntry *p1, propEntry *p2 );

unsigned char *fetchDocProperties ( FileRec *, char * );


void swapDocPropertyMetaNames (docProperties **, struct metaMergeEntry *);

char *getResultPropAsString(RESULT *, SWINT_T);
char *DecodeDocProperty( struct metaEntry *meta_entry, propEntry *prop );
void getSwishInternalProperties(FileRec *, IndexFILE *);


PropValue *getResultPropValue (RESULT *r, char *name, SWINT_T ID);
void    freeResultPropValue(PropValue *pv);



void     WritePropertiesToDisk( SWISH *sw , FileRec *fi);
propEntry *ReadSingleDocPropertiesFromDisk( IndexFILE *indexf, FileRec *fi, SWINT_T metaID, SWINT_T max_size );
docProperties *ReadAllDocPropertiesFromDisk( IndexFILE *indexf, SWINT_T filenum );



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

