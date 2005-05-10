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

*/

/* Jose Ruiz 2000/01 Definitions for MetaNames/Fields */

/* META_INDEX and META_PROP could now share the same bit, since props and metas are separated entries */
#define META_INDEX    (1<<0)      /* bynary 00000001 */  /* Meta is indexed */
#define META_PROP     (1<<1)      /* bynary 00000010 */ /* Also stored as property */
#define META_STRING   (1<<2)      /* String type of property */
#define META_NUMBER   (1<<3)      /* Data is binary number */
#define META_DATE     (1<<4)      /* Data is binary date */
#define META_INTERNAL (1<<5)      /* flag saying this is an internal metaname */
#define META_IGNORE_CASE (1<<6)   /* flag to say ignore case when comparing/sorting */
#define META_NOSTRIP  (1<<7)      /* Do not strip low ascii chars when indexing */
#define META_USE_STRCOLL (1<<8)  /* Use strcoll for sorting string properties */

/* Macros to test the type of a MetaName */
#define is_meta_internal(x)     ((x)->metaType & META_INTERNAL)
#define is_meta_index(x)        ((x)->metaType & META_INDEX)
#define is_meta_property(x)     ((x)->metaType & META_PROP)
#define is_meta_number(x)       ((x)->metaType & META_NUMBER)
#define is_meta_date(x)         ((x)->metaType & META_DATE)
#define is_meta_string(x)       ((x)->metaType & META_STRING)
#define is_meta_ignore_case(x)  ((x)->metaType & META_IGNORE_CASE)
#define is_meta_nostrip(x)      ((x)->metaType & META_NOSTRIP)
#define is_meta_use_strcoll(x)  ((x)->metaType & META_USE_STRCOLL)

int properties_compatible( struct metaEntry *m1, struct metaEntry *m2 );

void add_default_metanames(IndexFILE *);

struct metaEntry * getMetaNameByNameNoAlias(INDEXDATAHEADER * header, char *word);
struct metaEntry * getMetaNameByName(INDEXDATAHEADER *, char *);
struct metaEntry * getMetaNameByID(INDEXDATAHEADER *, int);

struct metaEntry * getPropNameByNameNoAlias(INDEXDATAHEADER * header, char *word);
struct metaEntry * getPropNameByName(INDEXDATAHEADER *, char *);
struct metaEntry * getPropNameByID(INDEXDATAHEADER *, int);


struct metaEntry * addMetaEntry(INDEXDATAHEADER *header, char *metaname, int metaType, int metaID);
struct metaEntry * addNewMetaEntry(INDEXDATAHEADER *header, char *metaWord, int metaType, int metaID);
struct metaEntry * cloneMetaEntry(INDEXDATAHEADER *header, struct metaEntry *meta );

void freeMetaEntries( INDEXDATAHEADER * );
int isDontBumpMetaName(struct swline *,char *tag);
int is_meta_entry( struct metaEntry *meta_entry, char *name );
void ClearInMetaFlags(INDEXDATAHEADER * header);

void init_property_list(INDEXDATAHEADER *header);
