
/*
   -- This module does result output for swish-e
   -- License: GPL

   -- 2001-01  R. Scherg  (rasc)   initial coding

*/


#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "swish.h"
#include "string.h"
#include "merge.h"
#include "docprop.h"
#include "result_output.h"





/* Prints the final results of a search.
   2001-01-01  rasc  Standard is swish 1.x default output

   if option extended format string is set, an alternate
   userdefined result output is possible (format like strftime or printf)
   in this case -d (delimiter is obsolete)
     e.g. : -x "result: COUNT:%c \t URL:%u\n"
*/


/* $$$ Remark / ToDO:
   -- The code is a prototype and needs optimizing:
   -- format control string is parsed on each result entry. (very bad!)
   -- ToDO: build an "action array" from an initial parsing of fmt
   --       ctrl string.
   --       on each entry step thru this action output list
   --       seems to be simple, but has to be done.
   -- but for now: get this stuff running on the easy way. 
   -- (rasc)
   $$$ 
*/





/*
  -- Output search result 
  -- do sorting according to cmd opt  ($todo...)
  -- print header informations  ($$todo, move from search.c )
*/

void printResultOutput (SWISH *sw) {

   printsortedresults(sw);
}



/*
  -- Output the resuult entries in the given order
  -- outputformat depends on some cmd opt settings
*/

void printsortedresults(SWISH *sw)

{
RESULT *r;
int    resultmaxhits;
int    counter;
char   *delimiter;


  resultmaxhits=sw->maxhits;
  counter = 0;
  delimiter = (sw->useCustomOutputDelimiter) ? sw->customOutputDelimiter : " ";

  while ((r=SwishNext(sw))) {
     r->count = ++counter;		/* set rec. counter for output */

     if (!sw->beginhits) {
	if (sw->maxhits) {
	   if (sw->opt.extendedformat)
	      print_ext_resultentry (sw, NULL, sw->opt.extendedformat, r);
	   else {
					 /* old std output */
	      printf("%d%s%s%s%s%s%d",
			r->rank,  delimiter, r->filename,
			delimiter, r->title, delimiter, r->size);
	      printSearchResultProperties(sw, r->Prop);
	      printf("\n");
	   }
				
	   if (resultmaxhits > 0) resultmaxhits--; /* Modified DN 08/29/99  */
	}
     }

     if(sw->beginhits) sw->beginhits--;
     r = r->nextsort;
  }

}





/*
   -- print a result entry in extented output format
   -- Format characters: see switch cases...
   -- f_out == NULL, use STDOUT
   -- fmt = output format
   -- count = current result record counter
   2001-01-01   rasc
*/

void print_ext_resultentry (SWISH *sw, FILE *f_out, char *fmt, RESULT *r)

{
  FILE   *f;
  char   *propname;
  char   *subfmt;
  

  f = (f_out) ? f_out : stdout;

   while (*fmt) {			/* loop fmt string */

     switch (*fmt) {

	case '%':			/* swish abbrevation controls */
		fmt = printTagAbbrevControl (sw, f, fmt, r);
		break;

	case '<':
		/* Property - Control: read and print Property Tag  <name> */
        	fmt = parsePropertyResultControl (fmt, &propname, &subfmt);
		printPropertyResultControl (sw, f, propname, subfmt, r);
		efree (subfmt);
		efree (propname);
		break;

	case '\\':			/* print format controls */
            	fmt = printResultControlChar (f, fmt);
		break;


	default:		/* just output the character in fmt string */
		fputc (*(fmt++),f);
		break;
     }

   }


}









/*  -- parse print control and print it
    --  output on file <f>
    --  *s = "\....."
    -- return: string ptr to char after control sequence.
*/

char *printResultControlChar (FILE *f, char *s) 

{
  char c, *se;
  long l;

  if (*s != '\\') return s;

  switch (*(++s)) {			   /* can be optimized ... */
	case 'a':   c = '\a';  break;
	case 'b':   c = '\b';  break;
	case 'f':   c = '\f';  break;
	case 'n':   c = '\n';  break;
	case 'r':   c = '\r';  break;
	case 't':   c = '\t';  break;
	case 'v':   c = '\v';  break;

	case 'x':				  /* Hex  \xff  */
		c = (char) strtoul (++s, &se, 16);
		s = se;
		break;

	case '0':				/* Oct  \0,  \012 */
		c = (char) strtoul (s, &se, 8);
		s = se;
		break;

	default: 
		c =  *s;		/* print the escaped character */
		break;
 }

 s++;
 fputc (c,f);
 return s;
}





/*  -- parse % control and print it
    --  output on file <f>
    --  in fact expand shortcut to fullnamed autoproperty tag
    --  *s = "%.....
    -- return: string ptr to char after control sequence.
*/

char *printTagAbbrevControl (SWISH *sw, FILE *f, char *s, RESULT *r) 

{
  char *t;
  char buf[MAXWORDLEN];

  if (*s != '%') return s;
  t = NULL;

  switch (*(++s)) {
	case 'c':  t=AUTOPROPERTY_REC_COUNT; break;
	case 'd':  t=AUTOPROPERTY_SUMMARY; break;
	case 'D':  t=AUTOPROPERTY_LASTMODIFIED; break;
	case 'I':  t=AUTOPROPERTY_INDEXFILE; break;
	case 'p':  t=AUTOPROPERTY_DOCPATH; break;
	case 'r':  t=AUTOPROPERTY_RESULT_RANK; break;
	case 'l':  t=AUTOPROPERTY_DOCSIZE; break;
	case 'S':  t=AUTOPROPERTY_STARTPOS; break;
	case 't':  t=AUTOPROPERTY_TITLE; break;

	case '%':  fputc ('%',f); break;
	default:   fprintf (f,"err-unkown-abbrev '%%%c'",*s); break;

 }

 if (t) {
    sprintf (buf, "<%s>", t);			/* create <...> tag */
    print_ext_resultentry  (sw, f, buf, r);
 }
 return ++s;
}




/*  -- parse <tag fmt="..."> control
    --  *s = "<....." format control string
    --       possible subformat:  fmt="...", fmt=/..../, etc. 
    -- return: string ptr to char after control sequence.
    --         **propertyname = Tagname  (or NULL)
    --         **subfmt = NULL or subformat
*/

char *parsePropertyResultControl (char *s, char **propertyname, char **subfmt)

{
  char *s1;
  char c;
  int  len;


  *propertyname = NULL;
  *subfmt = NULL;

  s = skip_ws (s);
  if (*s != '<') return s;
  s = skip_ws (++s);


  /* parse propertyname */

  s1 = s;
  while (*s) {				/* read to end of propertyname */
    if ((*s=='>')|| isspace(*s)) {  	/* delim > or whitespace ? */
	break;  			/* break on delim */
    }
    s++;
  }
  len = s-s1;
  *propertyname = (char *) emalloc(len+1);	
  strncpy (*propertyname, s1, len);
  *(*propertyname+len) = '\0';


  if (*s == '>') return ++s;		/* no fmt, return */
  s = skip_ws (s);

  
  /* parse optional fmt=<c>...<c>  e.g. fmt="..." */

  if (! strncmp (s,"fmt=",4)) {
      s += 4;			/* skip "fmt="  */
      c = *(s++);		/* string delimiter */ 
      s1 = s;
      while (*s) {		/* read to end of delim. char */
	if (*s == c) {  	/* c or \c */
	   if (*(s-1) != '\\') break;  /* break on delim c */
	}
	s++;
      }

      len = s-s1;
      *subfmt = (char *) emalloc(len + 1);	
      strncpy (*subfmt, s1, len);
      *(*subfmt+len) = '\0';
  }


  /* stupid "find end of tag" */

  while (*s && *s != '>') s++;
  if (*s == '>') s++;

  return s;
}



/* Skip white spaces...
*/

char *skip_ws (char *s)

{
   while (isspace(*s)) s++;
   return s;
}




/*
  -- Print the result value of propertytag <name> on file <f>
  -- if a format is given use it (data type dependend)
  -- string and numeric types are using printfcontrols formatstrings
  -- date formats are using strftime fromat strings.
*/


void printPropertyResultControl (SWISH *sw, FILE *f, char *propname,
				 char *subfmt, RESULT *r)

{
  char      *fmt;
  PropValue *pv;
  char      *s;
  int       n;
  
// fprintf (stdout,"!out: propname: _%s_ subfmt=_%s_!",propname,(subfmt)?subfmt:"NULL");

  pv = getResultPropertyByName (sw, propname, r);
  if (! pv) {
	fprintf (f, "(NULL)");		/* Null value, no propname */
        return;
  }

  switch (pv->datatype) {		/* property data type */
					/* use passed or default fmt */
	case INTEGER:
		fmt = (subfmt) ? subfmt: "%d";
		fprintf (f,fmt,pv->value.v_int); 
		break;

	case STRING:
		fmt = (subfmt) ? subfmt: "%s";
		fprintf (f,fmt,(char *)pv->value.v_str); 
		break;

	case DATE:
		fmt = (subfmt) ? subfmt: "%Y-%m-%d %H:%M:%S";
		if (!strcmp (fmt,"%d")) {
			/* special: Print date as serial int (for Bill) */
		   fprintf (f,fmt, (int) pv->value.v_date);
		} else {
			/* fmt is strftime format control! */
		   s = (char *) emalloc(MAXWORDLEN + 1);
		   n = strftime (s,(size_t) MAXWORDLEN, fmt,
			   localtime(&(pv->value.v_date)));
		   if (n) fprintf (f,s);
		   efree (s);
		}
		break;

	case FLOAT:
		fmt = (subfmt) ? subfmt: "%f";
		fprintf (f,fmt,(double) pv->value.v_float); 
		break;

	default:
		fprintf (stderr,"(unknown datatype <%s>)\n",propname);
		break;


  }
  efree (pv); 

}



/*
  -------------------------------------------
*/

