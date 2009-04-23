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
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
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
typedef void (*INIT_FUNCTION)( SWISH *sw, SWINT_T selfID, void **moduledata );
typedef HOOK_STATUS (*CALLBACK_FUNCTION)( SWISH *sw, SWINT_T selfID, void **moduleData, void *parameters );

void Init_Modules( SWISH *sw );
void Free_Module_Hooks( SWISH *sw );
void Register_Hook(SWISH *sw, HOOK hookID, SWINT_T priority, SWINT_T selfID, CALLBACK_FUNCTION callback );
HOOK_STATUS Run_Hook(SWISH *sw, HOOK hookID, void *parameters );



