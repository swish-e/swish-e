/* Copyright (C) 1991, 1992, 1996, 1998, 2001 Free Software Foundation, Inc.
   This file is derived from mkstemps.c from the GNU Libiberty Library
   which in turn is derived from the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA. 

*/

#if defined _WIN32
# include <windows.h>
#endif

/*********************************************************
 *  Replacement setenv/unsetenv for Windows 
 *********************************************************
 *  (excerpt from the Linux Programmer’s Manual)
 *  DESCRIPTION
 *     The  setenv()  function adds the variable name to the environment with the value value, if name does not
 *     already exist.  If name does exist in the environment, then its value is changed to value  if  overwrite
 *     is non-zero; if overwrite is zero, then the value of name is not changed.
 *     The unsetenv() function deletes the variable name from the environment.
 *
 *  RETURN VALUE
 *     The setenv() function returns zero on success, or -1 if there was insufficient space in the environment.
 *     The unsetenv() function returns zero on success, or -1 on error, with errno set to indicate the cause of
 *     the error.
 *
 *  CONFORMING TO
 *     4.3BSD, POSIX.1-2001.
 *********************************************************/

int setenv(char * lpname, char * lpvalue, int overwrite)
{
    if( SetEnvironmentVariableA(lpname, lpvalue) == 0 ){
	return(-1);
    }
    else {
        return(0);
    }
}


int unsetenv(char * lpname)
{
    if( SetEnvironmentVariableA(lpname, "") == 0 ){
	return(-1);
    }
    else {
        return(0);
    }
}

