
... Hard hat area ...
... work in process ...
... module not yet active and does not compile...

This module will affect currently
  - metanames.c 
  - result_output.c
  - swish.h


- rasc


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
**
** This is the place for handling datatypes in swish
** for properties and metanames.
** (ascii -> binary, binary --> ascii, compare)
**
** 2001-05-24  rasc
**
*/

#include <string.h>
#include "swish.h"
#include "datatypes.h"



/* 
  -- module private definitions and prototypes
*/

typedef LongestType   double;     /* longest module internal type needed  */
                                  /* maybe you can use also "long double" */


static LongestType  cast_numeric (SwDataValue v);






/*
  ------------------------------------------------------
  -- routines:  value --> string
  --            string --> value and type
 */




/* 
 -- convert variables to string
 -- using standard conversion format
 -- return: ptr. to allocated buffer
*/

char *datavalue2str (SwDataValue *v)
{
  return var2strfmt (v, (char *)NULL);
}



/* 
 -- convert variables to string
 -- using standard conversion format (NULL)
 -- or given conversion fmt string
 -- return: ptr. to allocated buffer (char *)
 --         NULL: illegal type or operation
*/


char *datavalue2strfmt (SwDataValue *v, char *altfmt)

{
 char  *fmt;
 char  wordbuf[65500];              /* should be enough */
 int   n;


  switch (v->datatype) {		/* data value type */
					/* use passed or default fmt */
	case INTEGER:
		fmt = (altfmt) ? altfmt: "%d";
		n = sprintf (wordbuf,fmt,v->value.v_int); 
		break;

	case STRING:
		fmt = (altfmt) ? altfmt: "%s";
		/* -- get rid of \n\r in string! */
		for (s=v->value.v_str; *s; s++) {
		   if (isspace((unsigned char) *s)) *s=' ';
		}
		/* $$$ ToDo: escaping of delimiter characters  $$$ */
		n = sprintf (wordbuf,fmt,(char *)v->value.v_str); 
		break;

	case DATE:
		fmt = (altfmt) ? altfmt: "%Y-%m-%d %H:%M:%S";
		if (!strcmp (fmt,"%ld")) {
			/* special: Print date as serial int (for Bill) */
		   n = sprintf (wordbuf,fmt, (long) v->value.v_date);
		} else {
			/* fmt is strftime format control! */
		   n = strftime (wordbuf,sizeof(wordbuf), fmt,
			   localtime(&(v->value.v_date)));
		}
		break;

	case FLOAT:
		fmt = (altfmt) ? altfmt: "%f";
		sprintf (wordbuf,fmt,(double) v->value.v_float); 
		break;

	default:        /* fail, unknown data type */
		return (char *)NULL;
		break;

  }

  return estrdup (wordbuf);
}





/* 
 -- string to binary datavalue
 -- return: ptr. to allocated datavalue
 --         strings are copied and have to be freed(!)
*/


SwDataValue *str2var (char *s, SwDataType dt)

{
  SwDataValue *v;
  char        *se;


  v = emalloc (sizeof (SwDataValue));
  v->datatype = dt;

  switch (dt) {		/* data value type */
	case INTEGER:
            v->value.v_int = strtol(s,&se);
		break;

	case STRING:
		v->value.v_str = estrdup(s);
		break;

	case DATE:
		v->value.v_date = str2date(s, &se);
		break;

	case FLOAT:
		v->value.v_float = strtof(s,&se);
		break;

	default:        /* fail, unknown data type */
		efree (v);
		return (SwDataValue *) NULL;
		break;
  }

  return v;
}





/*
  -- this routine compares two "values  a-b"
  -- if the datatypes don't match they will internally be converted
  -- return: like strcmp
*/

int datavaluecmp (SwDataValue *a, SwDataValue *b)

{
   int          cmp = 0;
   LongestType  d;
   char         *sa,*sb;


   /* 
     -- cmp same data type, should be easy
     -- cmp on different data types is harder, in this case
     -- cast numeric types to largest type...  
    */


   if (a->datatype == b->datatype) {
         /*
          -- same data types, direct compare
         */
       switch (a->datatype) {
         case STRING:
             cmp = strcmp (a->value.v_str,b->value.v_str);
             break;

         case DATE:
             cmp = (a->value.v_date - b->value.v_date);
             break;

         case INTEGER:
             cmp = (a->value.v_int - b->value.v_int);
             break;

         case FLOAT:
             d = (a->value.v_float - b->value.v_float);
             if (d > 0.0) cmp = 1;
             if (d < 0.0) cmp = -1;
             /* check 0 on floats is nuts */
             break;
       }

   } else {

       if (a->datatype == STRING || b->datatype == STRING) {
         /*
          -- Outch, one is string, bad thing: convert to string!
          -- This will hardly make sense, because it's not
          -- clear how to convert correctly for compare
         */

         sa = datavalue2str (a);
         sb = datavalue2str (b);
         cmp = strcmp (a,b);
         efree (sa);
         efree (sb);

       } else {

         /*
          -- numeric data types, cast to largest type
          -- and compare
         */
         d = cast_numeric(a) - cast_numeric(b);
         if (d > 0.0) cmp = 1;
         if (d < 0.0) cmp = -1;
         /* check 0 on floats is nuts */

       }
   }

   return cmp;
}



/*
  -- simply cast datavalue to a uniq large type
  -- return: casted value
*/

static LongestType  cast_numeric (SwDataValue v)

{
 LongestType  r;

 switch (v->datatype) {
    case DATE:
             r = v->value.v_date;
             break;

    case INTEGER:
             r = v->value.v_int;
             break;

    case FLOAT:
             r = v->value.v_float;
             break;
 }

 return r;
}







/*
  -- date string to binary
  -- this is somewhat complicated, because
  -- this routine has to recognize different
  -- date formats.
  -- so this should be the mastermind routine for dates
  -- return: time_t
*/

static time_t str2date (char *s, char **se)

{


}

