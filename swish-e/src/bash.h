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

*/


#define xmalloc emalloc
#define xfree efree
#define xrealloc erealloc

#define FS_EXISTS		0x1
#if defined(_WIN32) && !defined(__CYGWIN__)
#define FS_EXECABLE		0x1
#else
#define FS_EXECABLE		0x2
#endif

/* horrible Win32 hack */
#if defined _WIN32 || defined(__VMS)
/* Fake group functions... */
#define GETGROUPS_T int
#define getegid() 0
#define geteuid() 0
#define getgid()  0
#endif

#define savestring(x) (char *)strcpy((char *)xmalloc(1 + strlen (x)), (x))

extern int file_status(const char *name);
extern int absolute_program(const char *string);
extern char *get_next_path_element(const char *path_list, int *path_index_pointer);
extern char *make_full_pathname(const char *path, const char *name, int name_len);
