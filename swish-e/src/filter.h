/*
$ID: $
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
** 2001-02-28 rasc    own module started for filters
*/



struct filter *addfilter (struct filter *rp, char *FilterSuffix, char *FilterProg, char *FilterDir);
char *hasfilter (char *filename, struct filter *filterlist);
FILE *FilterOpen (FileProp *fprop);
int FilterClose (FILE *fp);


