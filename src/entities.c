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
** (c) Rainer.Scherg
**
**
** HTML entity routines (encoding, etc.):
**
** internally we are working with int/wchar_t to support unicode-16 for future
** enhancements of swish (rasc - Rainer Scherg).
**
** 2001-05-05  rasc   
**
*/




#include <stdlib.h>
#include "swish.h"
#include "mem.h"
#include "string.h"
#include "parse_conffile.h"
#include "entities.h"


/*
** ----------------------------------------------
** 
**  Private Module Data
**
** ----------------------------------------------
*/


#define  MAX_ENTITY_LEN  16           /* max chars after where we have to see the EOE */

/*
  -- Entity encoding/decoding structure 
  --- the macro IS_EOE (is_End_of_Entity) check for the end of an entity sequence.
  --- normally this is ';', but we may also allow whitespaces (and punctation)...
*/


/* #define  IS_EOE(a)  ((a)==';')                           --  W3C compliant */
/* #define  IS_EOE(a)  ((a)==';'||isspace((int)(a)))        --  tolerant behavior */
#define  IS_EOE(a)  (ispunct((int)(a))||isspace((int)(a)))  /*  oh god, accept everything */



typedef struct {
	char  *name;
	int   code;
} CEntity;


/*
  -- CEntity Quick Hash structure
  -- works like follow: Array of ASCII-7 start "positions" (1. char of entity name)
  -- each entry can have a chain of pointers
  -- e.g.  &quote; --> ['q']->ce(.name .code)
  --                        ->next (chains all &q...;)
  -- lots of slots in the array will be empty because only [A-Z] and [a-z]
  -- is needed. But this cost hardly any memory, and is convenient...  (rasc)
  -- The hash sequence list will be re-sequenced during(!) usage (dynamic re-chaining).
  -- This brings down compares to almost 1 strcmp on entity checks. 
  --
  -- Warning: don't change this (ce_hasharray,etc) unless you know how this really works!
  --
  --   2001-05-14  Rainer.Scherg@rexroth.de (rasc)
  --
 */

struct CEHE {             /* CharEntityHashEntry*/
     CEntity     *ce;
     struct CEHE *next;
};

static struct CEHE *ce_hasharray[128];



/*
 -- the following table is retrieved from HTML4.x / SGML definitions
 -- of the W3C  (did it automated 2001-05-05).
 --   http://www.w3.org/TR/html40/
 --   http://www.w3.org/TR/1999/REC-html401-19991224/sgml/entities.html
 -- 
 -- 2001-05-07 Rainer.Scherg
*/


static CEntity  entity_table[] = {
    {     "quot",  0x0022 },  /* quotation mark = APL quote, U+0022 ISOnum */
    {      "amp",  0x0026 },  /* ampersand, U+0026 ISOnum */
    {     "apos",  0x0027 },  /* single quote */
    {       "lt",  0x003C },  /* less-than sign, U+003C ISOnum */
    {       "gt",  0x003E },  /* greater-than sign, U+003E ISOnum */
      
      /*
       * A bunch still in the 128-255 range
       * Replacing them depend really on the charset used.
       */
    {     "nbsp",  0x00A0 },  /* no-break space = non-breaking space, U+00A0 ISOnum */
    {    "iexcl",  0x00A1 },  /* inverted exclamation mark, U+00A1 ISOnum */
    {     "cent",  0x00A2 },  /* cent sign, U+00A2 ISOnum */
    {    "pound",  0x00A3 },  /* pound sign, U+00A3 ISOnum */
    {   "curren",  0x00A4 },  /* currency sign, U+00A4 ISOnum */
    {      "yen",  0x00A5 },  /* yen sign = yuan sign, U+00A5 ISOnum */
    {   "brvbar",  0x00A6 },  /* broken bar = broken vertical bar, U+00A6 ISOnum */
    {     "sect",  0x00A7 },  /* section sign, U+00A7 ISOnum */
    {      "uml",  0x00A8 },  /* diaeresis = spacing diaeresis, U+00A8 ISOdia */
    {     "copy",  0x00A9 },  /* copyright sign, U+00A9 ISOnum */
    {     "ordf",  0x00AA },  /* feminine ordinal indicator, U+00AA ISOnum */
    {    "laquo",  0x00AB },  /* left-pointing double angle quotation mark = left pointing guillemet, U+00AB ISOnum */
    {      "not",  0x00AC },  /* not sign, U+00AC ISOnum */
    {      "shy",  0x00AD },  /* soft hyphen = discretionary hyphen, U+00AD ISOnum */
    {      "reg",  0x00AE },  /* registered sign = registered trade mark sign, U+00AE ISOnum */
    {     "macr",  0x00AF },  /* macron = spacing macron = overline = APL overbar, U+00AF ISOdia */
    {      "deg",  0x00B0 },  /* degree sign, U+00B0 ISOnum */
    {   "plusmn",  0x00B1 },  /* plus-minus sign = plus-or-minus sign, U+00B1 ISOnum */
    {     "sup2",  0x00B2 },  /* superscript two = superscript digit two = squared, U+00B2 ISOnum */
    {     "sup3",  0x00B3 },  /* superscript three = superscript digit three = cubed, U+00B3 ISOnum */
    {    "acute",  0x00B4 },  /* acute accent = spacing acute, U+00B4 ISOdia */
    {    "micro",  0x00B5 },  /* micro sign, U+00B5 ISOnum */
    {     "para",  0x00B6 },  /* pilcrow sign = paragraph sign, U+00B6 ISOnum */
    {   "middot",  0x00B7 },  /* middle dot = Georgian comma Greek middle dot, U+00B7 ISOnum */
    {    "cedil",  0x00B8 },  /* cedilla = spacing cedilla, U+00B8 ISOdia */
    {     "sup1",  0x00B9 },  /* superscript one = superscript digit one, U+00B9 ISOnum */
    {     "ordm",  0x00BA },  /* masculine ordinal indicator, U+00BA ISOnum */
    {    "raquo",  0x00BB },  /* right-pointing double angle quotation mark right pointing guillemet, U+00BB ISOnum */
    {   "frac14",  0x00BC },  /* vulgar fraction one quarter = fraction one quarter, U+00BC ISOnum */
    {   "frac12",  0x00BD },  /* vulgar fraction one half = fraction one half, U+00BD ISOnum */
    {   "frac34",  0x00BE },  /* vulgar fraction three quarters = fraction three quarters, U+00BE ISOnum */
    {   "iquest",  0x00BF },  /* inverted question mark = turned question mark, U+00BF ISOnum */
    {   "Agrave",  0x00C0 },  /* latin capital letter A with grave = latin capital letter A grave, U+00C0 ISOlat1 */
    {   "Aacute",  0x00C1 },  /* latin capital letter A with acute, U+00C1 ISOlat1 */
    {    "Acirc",  0x00C2 },  /* latin capital letter A with circumflex, U+00C2 ISOlat1 */
    {   "Atilde",  0x00C3 },  /* latin capital letter A with tilde, U+00C3 ISOlat1 */
    {     "Auml",  0x00C4 },  /* latin capital letter A with diaeresis, U+00C4 ISOlat1 */
    {    "Aring",  0x00C5 },  /* latin capital letter A with ring above = latin capital letter A ring, U+00C5 ISOlat1 */
    {    "AElig",  0x00C6 },  /* latin capital letter AE = latin capital ligature AE, U+00C6 ISOlat1 */
    {   "Ccedil",  0x00C7 },  /* latin capital letter C with cedilla, U+00C7 ISOlat1 */
    {   "Egrave",  0x00C8 },  /* latin capital letter E with grave, U+00C8 ISOlat1 */
    {   "Eacute",  0x00C9 },  /* latin capital letter E with acute, U+00C9 ISOlat1 */
    {    "Ecirc",  0x00CA },  /* latin capital letter E with circumflex, U+00CA ISOlat1 */
    {     "Euml",  0x00CB },  /* latin capital letter E with diaeresis, U+00CB ISOlat1 */
    {   "Igrave",  0x00CC },  /* latin capital letter I with grave, U+00CC ISOlat1 */
    {   "Iacute",  0x00CD },  /* latin capital letter I with acute, U+00CD ISOlat1 */
    {    "Icirc",  0x00CE },  /* latin capital letter I with circumflex, U+00CE ISOlat1 */
    {     "Iuml",  0x00CF },  /* latin capital letter I with diaeresis, U+00CF ISOlat1 */
    {      "ETH",  0x00D0 },  /* latin capital letter ETH, U+00D0 ISOlat1 */
    {   "Ntilde",  0x00D1 },  /* latin capital letter N with tilde, U+00D1 ISOlat1 */
    {   "Ograve",  0x00D2 },  /* latin capital letter O with grave, U+00D2 ISOlat1 */
    {   "Oacute",  0x00D3 },  /* latin capital letter O with acute, U+00D3 ISOlat1 */
    {    "Ocirc",  0x00D4 },  /* latin capital letter O with circumflex, U+00D4 ISOlat1 */
    {   "Otilde",  0x00D5 },  /* latin capital letter O with tilde, U+00D5 ISOlat1 */
    {     "Ouml",  0x00D6 },  /* latin capital letter O with diaeresis, U+00D6 ISOlat1 */
    {    "times",  0x00D7 },  /* multiplication sign, U+00D7 ISOnum */
    {   "Oslash",  0x00D8 },  /* latin capital letter O with stroke latin capital letter O slash, U+00D8 ISOlat1 */
    {   "Ugrave",  0x00D9 },  /* latin capital letter U with grave, U+00D9 ISOlat1 */
    {   "Uacute",  0x00DA },  /* latin capital letter U with acute, U+00DA ISOlat1 */
    {    "Ucirc",  0x00DB },  /* latin capital letter U with circumflex, U+00DB ISOlat1 */
    {     "Uuml",  0x00DC },  /* latin capital letter U with diaeresis, U+00DC ISOlat1 */
    {   "Yacute",  0x00DD },  /* latin capital letter Y with acute, U+00DD ISOlat1 */
    {    "THORN",  0x00DE },  /* latin capital letter THORN, U+00DE ISOlat1 */
    {    "szlig",  0x00DF },  /* latin small letter sharp s = ess-zed, U+00DF ISOlat1 */
    {   "agrave",  0x00E0 },  /* latin small letter a with grave = latin small letter a grave, U+00E0 ISOlat1 */
    {   "aacute",  0x00E1 },  /* latin small letter a with acute, U+00E1 ISOlat1 */
    {    "acirc",  0x00E2 },  /* latin small letter a with circumflex, U+00E2 ISOlat1 */
    {   "atilde",  0x00E3 },  /* latin small letter a with tilde, U+00E3 ISOlat1 */
    {     "auml",  0x00E4 },  /* latin small letter a with diaeresis, U+00E4 ISOlat1 */
    {    "aring",  0x00E5 },  /* latin small letter a with ring above = latin small letter a ring, U+00E5 ISOlat1 */
    {    "aelig",  0x00E6 },  /* latin small letter ae = latin small ligature ae, U+00E6 ISOlat1 */
    {   "ccedil",  0x00E7 },  /* latin small letter c with cedilla, U+00E7 ISOlat1 */
    {   "egrave",  0x00E8 },  /* latin small letter e with grave, U+00E8 ISOlat1 */
    {   "eacute",  0x00E9 },  /* latin small letter e with acute, U+00E9 ISOlat1 */
    {    "ecirc",  0x00EA },  /* latin small letter e with circumflex, U+00EA ISOlat1 */
    {     "euml",  0x00EB },  /* latin small letter e with diaeresis, U+00EB ISOlat1 */
    {   "igrave",  0x00EC },  /* latin small letter i with grave, U+00EC ISOlat1 */
    {   "iacute",  0x00ED },  /* latin small letter i with acute, U+00ED ISOlat1 */
    {    "icirc",  0x00EE },  /* latin small letter i with circumflex, U+00EE ISOlat1 */
    {     "iuml",  0x00EF },  /* latin small letter i with diaeresis, U+00EF ISOlat1 */
    {      "eth",  0x00F0 },  /* latin small letter eth, U+00F0 ISOlat1 */
    {   "ntilde",  0x00F1 },  /* latin small letter n with tilde, U+00F1 ISOlat1 */
    {   "ograve",  0x00F2 },  /* latin small letter o with grave, U+00F2 ISOlat1 */
    {   "oacute",  0x00F3 },  /* latin small letter o with acute, U+00F3 ISOlat1 */
    {    "ocirc",  0x00F4 },  /* latin small letter o with circumflex, U+00F4 ISOlat1 */
    {   "otilde",  0x00F5 },  /* latin small letter o with tilde, U+00F5 ISOlat1 */
    {     "ouml",  0x00F6 },  /* latin small letter o with diaeresis, U+00F6 ISOlat1 */
    {   "divide",  0x00F7 },  /* division sign, U+00F7 ISOnum */
    {   "oslash",  0x00F8 },  /* latin small letter o with stroke, = latin small letter o slash, U+00F8 ISOlat1 */
    {   "ugrave",  0x00F9 },  /* latin small letter u with grave, U+00F9 ISOlat1 */
    {   "uacute",  0x00FA },  /* latin small letter u with acute, U+00FA ISOlat1 */
    {    "ucirc",  0x00FB },  /* latin small letter u with circumflex, U+00FB ISOlat1 */
    {     "uuml",  0x00FC },  /* latin small letter u with diaeresis, U+00FC ISOlat1 */
    {   "yacute",  0x00FD },  /* latin small letter y with acute, U+00FD ISOlat1 */
    {    "thorn",  0x00FE },  /* latin small letter thorn with, U+00FE ISOlat1 */
    {     "yuml",  0x00FF },  /* latin small letter y with diaeresis, U+00FF ISOlat1 */
      
    {    "OElig",  0x0152 },  /* latin capital ligature OE, U+0152 ISOlat2 */
    {    "oelig",  0x0153 },  /* latin small ligature oe, U+0153 ISOlat2 */
    {   "Scaron",  0x0160 },  /* latin capital letter S with caron, U+0160 ISOlat2 */
    {   "scaron",  0x0161 },  /* latin small letter s with caron, U+0161 ISOlat2 */
    {     "Yuml",  0x0178 },  /* latin capital letter Y with diaeresis, U+0178 ISOlat2 */
      
      /*
       * Anything below should really be kept as entities references
       */

      /*
       -- Latin Extended-B
      */
    {     "fnof",  0x0192 },  /* latin small f with hook = function = florin, U+0192 ISOtech */
      
    {     "circ",  0x02C6 },  /* modifier letter circumflex accent, U+02C6 ISOpub */
    {    "tilde",  0x02DC },  /* small tilde, U+02DC ISOdia */

     /*
       -- Greek symbols
      */
    {    "Alpha",  0x0391 },  /* greek capital letter alpha, U+0391 */
    {     "Beta",  0x0392 },  /* greek capital letter beta, U+0392 */
    {    "Gamma",  0x0393 },  /* greek capital letter gamma, U+0393 ISOgrk3 */
    {    "Delta",  0x0394 },  /* greek capital letter delta, U+0394 ISOgrk3 */
    {  "Epsilon",  0x0395 },  /* greek capital letter epsilon, U+0395 */
    {     "Zeta",  0x0396 },  /* greek capital letter zeta, U+0396 */
    {      "Eta",  0x0397 },  /* greek capital letter eta, U+0397 */
    {    "Theta",  0x0398 },  /* greek capital letter theta, U+0398 ISOgrk3 */
    {     "Iota",  0x0399 },  /* greek capital letter iota, U+0399 */
    {    "Kappa",  0x039A },  /* greek capital letter kappa, U+039A */
    {   "Lambda",  0x039B },  /* greek capital letter lambda, U+039B ISOgrk3 */
    {       "Mu",  0x039C },  /* greek capital letter mu, U+039C */
    {       "Nu",  0x039D },  /* greek capital letter nu, U+039D */
    {       "Xi",  0x039E },  /* greek capital letter xi, U+039E ISOgrk3 */
    {  "Omicron",  0x039F },  /* greek capital letter omicron, U+039F */
    {       "Pi",  0x03A0 },  /* greek capital letter pi, U+03A0 ISOgrk3 */
    {      "Rho",  0x03A1 },  /* greek capital letter rho, U+03A1 */
       /* -- there is no Sigmaf, and no U+03A2 character either */
    {    "Sigma",  0x03A3 },  /* greek capital letter sigma, U+03A3 ISOgrk3 */
    {      "Tau",  0x03A4 },  /* greek capital letter tau, U+03A4 */
    {  "Upsilon",  0x03A5 },  /* greek capital letter upsilon, U+03A5 ISOgrk3 */
    {      "Phi",  0x03A6 },  /* greek capital letter phi, U+03A6 ISOgrk3 */
    {      "Chi",  0x03A7 },  /* greek capital letter chi, U+03A7 */
    {      "Psi",  0x03A8 },  /* greek capital letter psi, U+03A8 ISOgrk3 */
    {    "Omega",  0x03A9 },  /* greek capital letter omega, U+03A9 ISOgrk3 */
      
    {    "alpha",  0x03B1 },  /* greek small letter alpha, U+03B1 ISOgrk3 */
    {     "beta",  0x03B2 },  /* greek small letter beta, U+03B2 ISOgrk3 */
    {    "gamma",  0x03B3 },  /* greek small letter gamma, U+03B3 ISOgrk3 */
    {    "delta",  0x03B4 },  /* greek small letter delta, U+03B4 ISOgrk3 */
    {  "epsilon",  0x03B5 },  /* greek small letter epsilon, U+03B5 ISOgrk3 */
    {     "zeta",  0x03B6 },  /* greek small letter zeta, U+03B6 ISOgrk3 */
    {      "eta",  0x03B7 },  /* greek small letter eta, U+03B7 ISOgrk3 */
    {    "theta",  0x03B8 },  /* greek small letter theta, U+03B8 ISOgrk3 */
    {     "iota",  0x03B9 },  /* greek small letter iota, U+03B9 ISOgrk3 */
    {    "kappa",  0x03BA },  /* greek small letter kappa, U+03BA ISOgrk3 */
    {   "lambda",  0x03BB },  /* greek small letter lambda, U+03BB ISOgrk3 */
    {       "mu",  0x03BC },  /* greek small letter mu, U+03BC ISOgrk3 */
    {       "nu",  0x03BD },  /* greek small letter nu, U+03BD ISOgrk3 */
    {       "xi",  0x03BE },  /* greek small letter xi, U+03BE ISOgrk3 */
    {  "omicron",  0x03BF },  /* greek small letter omicron, U+03BF NEW */
    {       "pi",  0x03C0 },  /* greek small letter pi, U+03C0 ISOgrk3 */
    {      "rho",  0x03C1 },  /* greek small letter rho, U+03C1 ISOgrk3 */
    {   "sigmaf",  0x03C2 },  /* greek small letter final sigma, U+03C2 ISOgrk3 */
    {    "sigma",  0x03C3 },  /* greek small letter sigma, U+03C3 ISOgrk3 */
    {      "tau",  0x03C4 },  /* greek small letter tau, U+03C4 ISOgrk3 */
    {  "upsilon",  0x03C5 },  /* greek small letter upsilon, U+03C5 ISOgrk3 */
    {      "phi",  0x03C6 },  /* greek small letter phi, U+03C6 ISOgrk3 */
    {      "chi",  0x03C7 },  /* greek small letter chi, U+03C7 ISOgrk3 */
    {      "psi",  0x03C8 },  /* greek small letter psi, U+03C8 ISOgrk3 */
    {    "omega",  0x03C9 },  /* greek small letter omega, U+03C9 ISOgrk3 */
    { "thetasym",  0x03D1 },  /* greek small letter theta symbol, U+03D1 NEW */
    {    "upsih",  0x03D2 },  /* greek upsilon with hook symbol, U+03D2 NEW */
    {      "piv",  0x03D6 },  /* greek pi symbol, U+03D6 ISOgrk3 */
      
    {     "ensp",  0x2002 },  /* en space, U+2002 ISOpub */
    {     "emsp",  0x2003 },  /* em space, U+2003 ISOpub */
    {   "thinsp",  0x2009 },  /* thin space, U+2009 ISOpub */
    {     "zwnj",  0x200C },  /* zero width non-joiner, U+200C NEW RFC 2070 */
    {      "zwj",  0x200D },  /* zero width joiner, U+200D NEW RFC 2070 */
    {      "lrm",  0x200E },  /* left-to-right mark, U+200E NEW RFC 2070 */
    {      "rlm",  0x200F },  /* right-to-left mark, U+200F NEW RFC 2070 */
    {    "ndash",  0x2013 },  /* en dash, U+2013 ISOpub */
    {    "mdash",  0x2014 },  /* em dash, U+2014 ISOpub */
    {    "lsquo",  0x2018 },  /* left single quotation mark, U+2018 ISOnum */
    {    "rsquo",  0x2019 },  /* right single quotation mark, U+2019 ISOnum */
    {    "sbquo",  0x201A },  /* single low-9 quotation mark, U+201A NEW */
    {    "ldquo",  0x201C },  /* left double quotation mark, U+201C ISOnum */
    {    "rdquo",  0x201D },  /* right double quotation mark, U+201D ISOnum */
    {    "bdquo",  0x201E },  /* double low-9 quotation mark, U+201E NEW */
    {   "dagger",  0x2020 },  /* dagger, U+2020 ISOpub */
    {   "Dagger",  0x2021 },  /* double dagger, U+2021 ISOpub */
      
    {     "bull",  0x2022 },  /* bullet = black small circle, U+2022 ISOpub */
    {   "hellip",  0x2026 },  /* horizontal ellipsis = three dot leader, U+2026 ISOpub */
      
    {   "permil",  0x2030 },  /* per mille sign, U+2030 ISOtech */
      
    {    "prime",  0x2032 },  /* prime = minutes = feet, U+2032 ISOtech */
    {    "Prime",  0x2033 },  /* double prime = seconds = inches, U+2033 ISOtech */
      
    {   "lsaquo",  0x2039 },  /* single left-pointing angle quotation mark, U+2039 ISO proposed */
    {   "rsaquo",  0x203A },  /* single right-pointing angle quotation mark, U+203A ISO proposed */
      
    {    "oline",  0x203E },  /* overline = spacing overscore, U+203E NEW */
    {    "frasl",  0x2044 },  /* fraction slash, U+2044 NEW */
      
    {     "euro",  0x20AC },  /* euro sign, U+20AC NEW */
      
       /* -- Letterlike Symbols  */
    {    "image",  0x2111 },  /* blackletter capital I = imaginary part, U+2111 ISOamso */
    {   "weierp",  0x2118 },  /* script capital P = power set = Weierstrass p, U+2118 ISOamso */
    {     "real",  0x211C },  /* blackletter capital R = real part symbol, U+211C ISOamso */
    {    "trade",  0x2122 },  /* trade mark sign, U+2122 ISOnum */

       /* -- alef symbol is NOT the same as hebrew letter alef, U+05D0 */
    {  "alefsym",  0x2135 },  /* alef symbol = first transfinite cardinal, U+2135 NEW */

       /* -- Arrow Symbols  */
    {     "larr",  0x2190 },  /* leftwards arrow, U+2190 ISOnum */
    {     "uarr",  0x2191 },  /* upwards arrow, U+2191 ISOnum */
    {     "rarr",  0x2192 },  /* rightwards arrow, U+2192 ISOnum */
    {     "darr",  0x2193 },  /* downwards arrow, U+2193 ISOnum */
    {     "harr",  0x2194 },  /* left right arrow, U+2194 ISOamsa */
    {    "crarr",  0x21B5 },  /* downwards arrow with corner leftwards = carriage return, U+21B5 NEW */
    {     "lArr",  0x21D0 },  /* leftwards double arrow, U+21D0 ISOtech */
    {     "uArr",  0x21D1 },  /* upwards double arrow, U+21D1 ISOamsa */
    {     "rArr",  0x21D2 },  /* rightwards double arrow, U+21D2 ISOtech */
    {     "dArr",  0x21D3 },  /* downwards double arrow, U+21D3 ISOamsa */
    {     "hArr",  0x21D4 },  /* left right double arrow, U+21D4 ISOamsa */
      
       /* -- Mathematical Operators */
    {   "forall",  0x2200 },  /* for all, U+2200 ISOtech */
    {     "part",  0x2202 },  /* partial differential, U+2202 ISOtech */
    {    "exist",  0x2203 },  /* there exists, U+2203 ISOtech */
    {    "empty",  0x2205 },  /* empty set = null set = diameter, U+2205 ISOamso */
    {    "nabla",  0x2207 },  /* nabla = backward difference, U+2207 ISOtech */
    {     "isin",  0x2208 },  /* element of, U+2208 ISOtech */
    {    "notin",  0x2209 },  /* not an element of, U+2209 ISOtech */
    {       "ni",  0x220B },  /* contains as member, U+220B ISOtech */
    {     "prod",  0x220F },  /* n-ary product = product sign, U+220F ISOamsb */
    {      "sum",  0x2211 },  /* n-ary sumation, U+2211 ISOamsb */
    {    "minus",  0x2212 },  /* minus sign, U+2212 ISOtech */
    {   "lowast",  0x2217 },  /* asterisk operator, U+2217 ISOtech */
    {    "radic",  0x221A },  /* square root = radical sign, U+221A ISOtech */
    {     "prop",  0x221D },  /* proportional to, U+221D ISOtech */
    {    "infin",  0x221E },  /* infinity, U+221E ISOtech */
    {      "ang",  0x2220 },  /* angle, U+2220 ISOamso */
    {      "and",  0x2227 },  /* logical and = wedge, U+2227 ISOtech */
    {       "or",  0x2228 },  /* logical or = vee, U+2228 ISOtech */
    {      "cap",  0x2229 },  /* intersection = cap, U+2229 ISOtech */
    {      "cup",  0x222A },  /* union = cup, U+222A ISOtech */
    {      "int",  0x222B },  /* integral, U+222B ISOtech */
    {   "there4",  0x2234 },  /* therefore, U+2234 ISOtech */
    {      "sim",  0x223C },  /* tilde operator = varies with = similar to, U+223C ISOtech */
    {     "cong",  0x2245 },  /* approximately equal to, U+2245 ISOtech */
    {    "asymp",  0x2248 },  /* almost equal to = asymptotic to, U+2248 ISOamsr */
    {       "ne",  0x2260 },  /* not equal to, U+2260 ISOtech */
    {    "equiv",  0x2261 },  /* identical to, U+2261 ISOtech */
    {       "le",  0x2264 },  /* less-than or equal to, U+2264 ISOtech */
    {       "ge",  0x2265 },  /* greater-than or equal to, U+2265 ISOtech */
    {      "sub",  0x2282 },  /* subset of, U+2282 ISOtech */
    {      "sup",  0x2283 },  /* superset of, U+2283 ISOtech */
    {     "nsub",  0x2284 },  /* not a subset of, U+2284 ISOamsn */
    {     "sube",  0x2286 },  /* subset of or equal to, U+2286 ISOtech */
    {     "supe",  0x2287 },  /* superset of or equal to, U+2287 ISOtech */
    {    "oplus",  0x2295 },  /* circled plus = direct sum, U+2295 ISOamsb */
    {   "otimes",  0x2297 },  /* circled times = vector product, U+2297 ISOamsb */
    {     "perp",  0x22A5 },  /* up tack = orthogonal to = perpendicular, U+22A5 ISOtech */
    {     "sdot",  0x22C5 },  /* dot operator, U+22C5 ISOamsb */
    {    "lceil",  0x2308 },  /* left ceiling = apl upstile, U+2308 ISOamsc */
    {    "rceil",  0x2309 },  /* right ceiling, U+2309 ISOamsc */
    {   "lfloor",  0x230A },  /* left floor = apl downstile, U+230A ISOamsc */
    {   "rfloor",  0x230B },  /* right floor, U+230B ISOamsc */
    {     "lang",  0x2329 },  /* left-pointing angle bracket = bra, U+2329 ISOtech */
    {     "rang",  0x232A },  /* right-pointing angle bracket = ket, U+232A ISOtech */
    {      "loz",  0x25CA },  /* lozenge, U+25CA ISOpub */

      /* -- Miscellaneous Symbols */
    {   "spades",  0x2660 },  /* black spade suit, U+2660 ISOpub */
    {    "clubs",  0x2663 },  /* black club suit = shamrock, U+2663 ISOpub */
    {   "hearts",  0x2665 },  /* black heart suit = valentine, U+2665 ISOpub */
    {    "diams",  0x2666 },  /* black diamond suit, U+2666 ISOpub */

};


/*
** ----------------------------------------------
** 
**  Module management code starts here
**
** ----------------------------------------------
*/

/* 
  -- init structures for Entities
*/

void initModule_Entities (SWISH  *sw)

{
   struct MOD_Entities *md;

   
      md = (struct MOD_Entities *) emalloc(sizeof(struct MOD_Entities));
      sw->Entities = md;

      md->convertEntities = 1;	/* default = YES convert entities! */


      /* 
        -- init entity hash 
        -- this is module local only
      */
      {
       int          i, tab_len;
       CEntity      *ce_p;
       struct CEHE  **hash_pp, *tmp_p;
       
        /* empty positions */
        for (i=0; i< sizeof(ce_hasharray)/sizeof(ce_hasharray[0]); i++)
            ce_hasharray[i] = (struct CEHE *)NULL;
        /* 
           -- fill entity table into hash
           --  process from end to start of entity_table, because most used
           --  entities are at the beginning (iso)
           --  this is due to "insert in hash sequence" behavior in hashtab (performance!)
           --  The improvement is minimal, because the hash table re-chains during usage.
         */
        tab_len = sizeof(entity_table)/sizeof(entity_table[0]);
/*        for (i=0; i<tab_len; i++) {      */
        for (i=tab_len-1; i>=0; i--) {
               ce_p = &entity_table[i];
               hash_pp = &ce_hasharray[(int)*(ce_p->name) & 0x7F];
               /* insert entity-ptr at start of ptr sequence in hash */
               tmp_p = *hash_pp;
		   *hash_pp = (struct CEHE *)emalloc(sizeof(struct CEHE));
               (*hash_pp)->ce = ce_p;
               (*hash_pp)->next = tmp_p;
        }  

      } /* end init hash block */


//$$$ debugModule_Entities ();
}



/* 
  -- release structures for Entities
  -- release all wired memory
*/

void freeModule_Entities (SWISH *sw)

{

      /* free module data structure */

      efree (sw->Entities);
      sw->Entities = NULL;


      /* 
        -- free local entity hash table 
      */
      {
       int          i;
       struct CEHE  *hash_p, *tmp_p;
       
        /* free ptr "chains" in array */
        for (i=0; i< sizeof(ce_hasharray)/sizeof(ce_hasharray[0]); i++) {
            hash_p = ce_hasharray[i];
            while (hash_p) {
                 tmp_p = hash_p->next;
                 efree (hash_p);
                 hash_p = tmp_p;
            }
            ce_hasharray[i] = (struct CEHE *)NULL;
        }

      } /* end free hash block */

}



/*
** ----------------------------------------------
** 
**  Module config code starts here
**
** ----------------------------------------------
*/


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_Entities (SWISH *sw, StringList *sl)

{
  struct MOD_Entities *md = sw->Entities;
  char *w0;
  int  retval;


  w0 = sl->word[0];
  retval = 1;


  if (strcasecmp(w0, "ConvertHTMLEntities") == 0) {
      md->convertEntities = getYesNoOrAbort(sl, 1, 1);
  } else {
      retval = 0;	            /* not a Entities directive */
  }


  return retval;
}






/*
** ----------------------------------------------
** 
**  Module code starts here
**
** ----------------------------------------------
*/


/*
  -- convert a string containing HTML/XML entities
  -- conversion is done on the string itsself.
  -- conversion is only done, if config directive is set to "YES"
  -- return ptr to converted string.
*/

unsigned char *sw_ConvHTMLEntities2ISO(SWISH *sw, unsigned char *s)

{
  return (sw->Entities->convertEntities) ? strConvHTMLEntities2ISO (s): s;
}



/*
  -- convert a string containing HTML/XML entities
  -- conversion is done on the string itsself.
  -- return ptr to converted string.
*/

unsigned char *strConvHTMLEntities2ISO (unsigned char *buf)

{
  unsigned char *s,*t;
  unsigned char *d;
  int	          code;


  s = d = buf;

  while (*s) {

     /* if not entity start, next */
     if (*s != '&') {
        *d++ = *s++;
     } else {
        /* entity found, identify and decode */
        /* ignore zero entities and UNICODE ! */
        code = charEntityDecode (s, &t);
        if (code && (code < 256)) *d++ = (unsigned char)code;
        s = t;
     }
  }
  *d = '\0';
fprintf (stderr,"DBG: buffer after entity decoding: _%s_\n",buf); //$$$ DEBUG

  return buf;
}



/* 
 -- decode entity string to character code:
 --  &#dec;   &#xhex;   &#Xhex;   &named;
 -- Decoding is hash optimized with dynamic re-chaining for
 -- performance improvement...
 -- return: entity character (decoded)
 --    position "end" (if != NULL) past "entity" or behind ret. char
 --    on illegal entities, just return the char...
*/

int charEntityDecode (unsigned char *s, unsigned char **end)

{
  unsigned char *s1,*t;
  unsigned char *e_end;
  unsigned char s_cmp[MAX_ENTITY_LEN+1];
  int len;
  int code;


  /*
   -- no entity ctrl start char?, err: return char 
   */
  if (*s != '&') {
    if (end) *end = s+1;
    return (int) *s;
  }



  /* ok, seems valid entity starting char */
  code = 0;
  e_end = NULL;

  if (*(s+1) == '#') {   /* numeric entity  "&#" */

      s+=2;               /* after "&#" */
      switch (*s) {
         case 'x':
         case 'X':
              ++s;     /* skip x */
              code = (int) strtoul (s,(char **)&s,(int)16);
              break;
         default:
              code = (int) strtoul (s,(char **)&s,(int)10); 
              break;
      }
      e_end = s;

  } else {

      /* 
        -- ok, seems to be a named entity, find terminating char 
        -- t = NULL if  not found...
        -- if no char found: return '&' (illegal entity) 
       */
  
      len = 0;
      t = NULL;
      s1 = s;
      while (*(++s1) && (len < MAX_ENTITY_LEN)) {
         s_cmp[len] = *s1;
         if (IS_EOE(*s1)) {
             t = s1;      /* End of named entity */
             break;
         }
         len ++;
      }
      s_cmp[len] = '\0';

      /*
         -- hash search block
         -- case sensitiv search  (hashvalue = 1 entity name char)
         -- (& 0x7F to prevent hashtable mem coredumps by illegal chars)
         -- improve performance, by rechaining found elements
       */

      if (t) {
         struct CEHE  *hash_p;
         struct CEHE  **hash_pp, *last_p;

         hash_pp = & ce_hasharray[*(s+1) & 0x7F];
         last_p   = NULL;
         hash_p  = *hash_pp;
fprintf (stderr," DBG: entity: _%s_ --> \n",s_cmp); //$$$$ Debug
         while (hash_p) {
fprintf (stderr," DBG decoding:     --> _%s_\n",hash_p->ce->name); //$$$$ Debug 
             if (!strcmp (hash_p->ce->name,s_cmp)) {
                 code = hash_p->ce->code;
                 if (last_p) {  /* rechain hash sequence list (last found = first) */
                    last_p->next = hash_p->next;   /* take elem out of seq */
                    hash_p->next = *hash_pp;       /* old 1. = 2.          */
                    *hash_pp     = hash_p;         /* found = 1st          */
                 }
                 e_end = t;                        /* found -> set end     */
                 break;
             }
             last_p = hash_p;
             hash_p = hash_p->next;
         }

      }
  } /* end if */


  if (!(e_end && IS_EOE(*e_end))) code = *s;
  if (! e_end) {
     e_end = s+1;
  } else {
     if (*e_end == ';') e_end++;   /* W3C  EndOfEntity */
  } 


  if (end) *end = e_end;
  return code;
}




/*
$$$
 --- debug routine
 --- display entity hash
*/


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

void debugModule_Entities (void)

{

      /* 
        -- debug local entity hash table 
      */
      {
       int          i;
       struct CEHE  *hash_p;
       
        /* free ptr "chains" in array */
        for (i=0; i< sizeof(ce_hasharray)/sizeof(ce_hasharray[0]); i++) {
            hash_p =  ce_hasharray[i];
            fprintf (stderr,"Hash-Entry: %d ('%c'):\n",i,(char)i);
            while (hash_p) {
                 fprintf (stderr,"--> '%s' (%d)\n",hash_p->ce->name,hash_p->ce->code);
                 hash_p = hash_p->next;
            }
        }

      } /* end debug hash block */


//----- $$$ manual test routine...

{
 unsigned char *s,*t;
 char buf[1024];
 int  i;

 
  while (gets(buf)) {
     printf ("in: __%s__\n",buf);
     s = buf;

     i = charEntityDecode (s, &t );
     printf ("out: Hex: %04X (%d) _%c_ --> Rest: _%s_\n", i,i, (char)i ,t);

     printf ("converted: _%s_\n\n", strConvHTMLEntities2ISO(buf));
  }

 }
 
//--------





  return;
}
 




