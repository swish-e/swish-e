/* Defines the available hooks in swish */
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


*/

typedef enum {
    HOOK__FIRST = -1,

    /* Add new hooks to this list */
    HOOK_SESSION_END,       // Called when ending a session
    HOOK_CONFIG_FILE,       // Processes config file line-by-line
    HOOK_INPUT_METHOD,
    HOOK_FILTER_SWISH_WORD,

    HOOK__LAST
} HOOK;

/* Defines what a modules function can return */
typedef enum {
    HOOK_OK,                // Continue processing more hooks, if any
    HOOK_STOP,              // Stop processing any more hooks
    HOOK_NONE               // Returned from run_swish_hook when no hooks are assigned		
} HOOK_STATUS;


/* Define what the init and callback functions looks like */
typedef void (*INIT_FUNCTION)( SWISH *sw, int selfID, void **moduledata );
typedef HOOK_STATUS (*CALLBACK_FUNCTION)( SWISH *sw, int selfID, void **moduleData, void *parameters );

void Init_Modules( SWISH *sw );
void Free_Module_Hooks( SWISH *sw );
void Register_Hook(SWISH *sw, HOOK hookID, int priority, int selfID, CALLBACK_FUNCTION callback );
HOOK_STATUS Run_Hook(SWISH *sw, HOOK hookID, void *parameters );



