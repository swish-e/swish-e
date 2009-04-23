/*
$Id$
**


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


**
**
** added buffer size arg to grabStringValue prototype - core dumping from overrun
** SRE 2/22/00
*/


#if defined(_WIN32) && !defined(__CYGWIN__)
void make_windows_path( char *path );
#endif
char *get_libexec(void);

void normalize_path(char *path);

SWINT_T isdirectory(char *);
SWINT_T isfile(char *);
SWINT_T islink(char *);
SWINT_T getsize(char *);

void indexpath(SWISH *, char *);

char *read_stream(SWISH *, FileProp *fprop, SWINT_T is_text);
void flush_stream( FileProp *fprop );


/* Get/eval properties for file  (2000-11 rasc) */
FileProp *file_properties (char *real_path, char *work_path, SWISH *sw);
FileProp *init_file_properties (void);
void init_file_prop_settings( SWISH *sw, FileProp *fprop );
void     free_file_properties (FileProp *fprop);

 
/*
 * Some handy routines for parsing the Configuration File
 */

SWINT_T grabCmdOptionsIndexFILE(char* line, char* commandTag, IndexFILE **listOfWords, SWINT_T* gotAny, SWINT_T dontToIt);

FILE *create_tempfile(SWISH *sw, const char *mode, char *prefix, char **file_name_buffer, SWINT_T remove_file_name );

