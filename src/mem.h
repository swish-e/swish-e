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
** Author: Bill Meier, June 2001
**
*/

#ifndef MEM_H
#define MEM_H 1

/*
** The following settings control the memory allocator. Each setting is independent.
** They also affect the actual memory usage of the program, because (currently)
** turning on any of these settings increases the size of each allocation.
** MEM_STATISTICS allocates the least extra and MEM_DEBUG allocates the most extra per call. 
**
** In addition (currently) turning on any of these settings will map all
** realloc calls into a alloc and free for simplier implementation. However, this
** should be transparent to all programs!
*/

/*
** Normal settings (but not required):
**		If you turn on MEM_DEBUG, turn on MEM_STATISTICS
**		If you turn on MEM_TRACE, turn on MEM_STATISTICS
*/

#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MEM_DEBUG checks for memory consistency on alloc/free */
#define MEM_DEBUG 0

/* MEM_TRACE checks for unfreed memory, and where it is allocated */
#define MEM_TRACE 0

/* MEM_STATISTICS gives memory statistics (bytes allocated, calls, etc */
#define MEM_STATISTICS 0


typedef struct _mem_zone {
	struct _zone	*next;		/* link to free chunk */
	char			*name;		/* name of zone */
	size_t			size;		/* size to grow zone by */
	int				attributes;	/* attributes of zone (not used yet) */
	unsigned int	allocs;		/* count of allocations (for statistics) */
} MEM_ZONE;


/* The following are the basic malloc/realloc/free replacements */
#if MEM_TRACE
extern size_t memory_trace_counter;
void Mem_bp(int n);
#endif

#if MEM_DEBUG | MEM_TRACE | MEM_STATISTICS

#define emalloc(size) Mem_Alloc(size, __FILE__, __LINE__)
#define erealloc(ptr, size) Mem_Realloc(ptr, size, __FILE__, __LINE__)
#define efree(ptr) Mem_Free(ptr, __FILE__, __LINE__)

void *Mem_Alloc(size_t size, char *file, int line);
void *Mem_Realloc(void *ptr, size_t size, char *file, int line);
void Mem_Free(void *ptr, char *file, int line);

#else

void *emalloc(size_t size);
void *erealloc(void *ptr, size_t size);
void efree(void *ptr);

#endif

/* Hook to print out statistics if enabled */
void Mem_Summary(char *title, int final);

/* Memory zone routines */

/* create a zone -- size should be some reasonable number */
MEM_ZONE *Mem_ZoneCreate(char *name, size_t size, int attributes);

/* allocate memory from a zone (can use like malloc if you aren't going to realloc) */
void *Mem_ZoneAlloc(MEM_ZONE *head, size_t size);

/* free all memory in a zone */
void Mem_ZoneFree(MEM_ZONE **head); 

/* memory zone statistics */
#if MEM_STATISTICS
void Mem_ZoneStatistics(MEM_ZONE *head); 
#endif

/* make all memory in a zone reusable */
void Mem_ZoneReset(MEM_ZONE *head); 

/* Returns the allocated memory owned by a zone */
int Mem_ZoneSize(MEM_ZONE *head);

/* Don't let people use the regular C calls */
#define malloc $Please_use_emalloc
#define realloc $Please_use_erealloc
#define free $Please_use_efree


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* !MEM_H */
