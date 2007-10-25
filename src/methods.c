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
    
** Mon May  9 14:58:45 CDT 2005
** added GPL

*/

#include "swish.h"

#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
extern struct _indexing_data_source_def FileSystemIndexingDataSource;
#endif

#ifdef ALLOW_EXTERNAL_PROGRAM_DATA_SOURCE
extern struct _indexing_data_source_def ExternalProgramDataSource;
#endif    


struct _indexing_data_source_def *data_sources[] = {

#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
    &FileSystemIndexingDataSource,
#endif


#ifdef ALLOW_EXTERNAL_PROGRAM_DATA_SOURCE
    &ExternalProgramDataSource,
#endif    

    0
};
