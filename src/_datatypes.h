/*
$Id$
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
** 2001-05-25 rasc    new module
**
*/


#ifndef __HasSeenModule_DataTypes
#define __HasSeenModule_DataTypes	1


/* Global module data definition */



/* -- Swish Data Types 
   -- used for Properties and MetaNames
   -- Result handling structures, (types storage, values)
   -- Warnung! Changing types inflicts output routines, etc. 
   -- moved from swish.h and adapted, replaces former PropValue
   -- 2001-01  rasc
*/

typedef enum
{                               /* Property/MetaNames Datatypes */
    UNDEFINED = -1, UNKNOWN = 0, STRING, INTEGER, FLOAT, DATE
}
SwDataType;

typedef union
{                               /* storage of the DataValue */
    char   *v_str;              /* strings */
    int     v_int;              /* Integer */
    time_t  v_date;             /* Date    */
    double  v_float;            /* Double Float */
}
u_SwDataValue1;

typedef struct
{                               /* DataValue with type info */
    SwDataType     datatype;
    u_SwDataValue1 value;
}
SwDataValue;




/* Public prototypes */

char *datavalue2str (SwDataValue *v);
char *datavalue2strfmt (SwDataValue *v, char *altfmt);
SwDataValue *str2var (char *s, SwDataType dt);
int datavaluecmp (SwDataValue *a, SwDataValue *b);


#endif


