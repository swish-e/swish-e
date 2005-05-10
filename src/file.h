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


**
**
** added buffer size arg to grabStringValue prototype - core dumping from overrun
** SRE 2/22/00
*/


#ifdef _WIN32
void make_windows_path( char *path );
#endif
char *get_libexec(void);

void normalize_path(char *path);

int isdirectory(char *);
int isfile(char *);
int islink(char *);
int getsize(char *);

void indexpath(SWISH *, char *);

char *read_stream(SWISH *, FileProp *fprop, int is_text);
void flush_stream( FileProp *fprop );


/* Get/eval properties for file  (2000-11 rasc) */
FileProp *file_properties (char *real_path, char *work_path, SWISH *sw);
FileProp *init_file_properties (void);
void init_file_prop_settings( SWISH *sw, FileProp *fprop );
void     free_file_properties (FileProp *fprop);

 
/*
 * Some handy routines for parsing the Configuration File
 */

int grabCmdOptionsIndexFILE(char* line, char* commandTag, IndexFILE **listOfWords, int* gotAny, int dontToIt);

FILE *create_tempfile(SWISH *sw, const char *mode, char *prefix, char **file_name_buffer, int remove_file_name );

