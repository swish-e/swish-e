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
** Created June 4, 2001 - moseley
**
**/

#include "swish.h"
#include "modules.h"
#include "error.h"  // for progerr * progwarn
#include "mem.h"    // for emalloc & efree

/* Todo:
    1) way to change a callback's priority
    2) a way to remove your callback from a hook
    3) think about callback's calling the same callback hook - sub-request.
*/

   
        
/* Defines the structure to initialize a module */
typedef struct 
{
    INIT_FUNCTION init_function;
    char    *module_file;
    char    *module_name;
} MODULE_REGISTRATION;



/*********************** DECLARED MODULES *******************************
*
*  This section is used to register modules into Swish-e.
*
*  You must declare your init function, and
*  add it to the module_list array here.
*
*************************************************************************/


// void Module_NewMod();  This gives compiler warnings under Windows
//void Module_NewMod2();

extern void Module_NewMod( SWISH *sw, int selfID, void **data );
extern void Module_NewMod2( SWISH *sw, int selfID, void **data );

static MODULE_REGISTRATION module_list[] = {  
    { Module_NewMod, "newmod.c", "Test Module" },
    { Module_NewMod2, "newmod2.c", "Test Module2" },
};




/**************** PRIVATE FUNCTIONS **************************************/

#define numModuleList (sizeof(module_list)/sizeof(module_list[0]))


static int valid_hookID( HOOK hookID )
{
    return hookID > HOOK__FIRST && hookID < HOOK__LAST;
}

static int isregistered_moduleID( int ID )
{
    return sizeof(module_list) && ID >= 0 && ID < numModuleList;
}


/* Defines what each node in the callback list looks like */
typedef struct CALLBACK
{
    struct CALLBACK *next;
    CALLBACK_FUNCTION callback_function;
    int    module_id;
    int    priority;
} CALLBACK;




/**************** PUBLIC FUNCTIONS **************************************/



/*************************************************************************
*   Init_Modules()
*
*   This creates the array of callback modules in sw, and then calls
*   every module's init function.  Called at start of SwishNew().
*
*   Call with:
*       pointer to a SWISH structure
*
*   Returns:
*       void
*
*   Calls each registered function (as registered above in modules_list[]
*
*   Calls module's init function with:
*       pointer to a SWISH structure
*       the modules ID number (used for registering callback functions)
*       **pointer which is a place for the module to create a private data
*           storage area (such as a private structure).  Passed back to function
*           for every registered callback.  Module is responsible for cleanup.
*    
*
*
**************************************************************************/

void Init_Modules( SWISH *sw )
{
    int id;

    if ( sw->module_callbacks )
        progerr("Init_Modules called with non-NULL sw->module_hooks");

    if ( sw->module_private_data )
        progerr("Init_Modules called with non-NULL sw->module_private_data");


    /* Create an array to hold the various callback hooks, and initialize to NULLs */


    /* Create array in SWISH structure and zero array */
    sw->module_callbacks = emalloc( sizeof( CALLBACK * ) * (HOOK__LAST - HOOK__FIRST + 1) );

    for (id = HOOK__FIRST+1; id < HOOK__LAST; id++)
        sw->module_callbacks[id] = NULL;


    sw->module_private_data = NULL;


    /* Make sure there's modules to initialize */
    if ( !sizeof( module_list ) )
        return;


    /* Create an array to hold the various callback private data pointers and init to NULLs */
    sw->module_private_data = emalloc( sizeof( CALLBACK * ) * numModuleList );
        

    /* Now call each module's init functions */
    for (id = 0; id < numModuleList; id++)
    {
        sw->module_private_data[id] = NULL; /* init it's data to NULL */
        
        (module_list[id].init_function) ( sw, id, (void **) &(sw->module_private_data[id]) );
    }

}

/*************************************************************************
*   Free_Module_Hooks()
*
*   Function:
*       Calls the HOOK_SESSION_END callbacks
*       Cleans up all hooks and frees memory.
*
*   Should be called at end of SwishClose().
*
*   Call with:
*       pointer to a SWISH structure
*
*   Returns:
*       void
*
*
**************************************************************************/

void Free_Module_Hooks( SWISH *sw )
{
    int id;

    if ( !sw->module_callbacks )
        progerr("Free_Module_Hooks called with NULL sw->module_hooks");


    /* First call all SESSION_END hooks */
    Run_Hook( sw, HOOK_SESSION_END, NULL );



    /* Make sure all modules have freed up their data structures */

    if ( sizeof( module_list ) )
    {
        for (id = 0; id < numModuleList; id++)
            if ( sw->module_private_data[id] )
                progwarn("module '%s' (%s) did not release private memory",
                    module_list[id].module_name, module_list[id].module_file );

        /* Free up the array holding the sructures */
        efree( sw->module_private_data );
        sw->module_private_data = NULL;
    }



    /* Now free hood nodes and array in sw */
    for (id = HOOK__FIRST+1; id < HOOK__LAST; id++)
    {
        struct CALLBACK *node;
        struct CALLBACK *next;

        node = sw->module_callbacks[id];
        while ( node )
        {
            next = node->next;
            efree( node );
            node = next;
        }
    }

    efree( sw->module_callbacks );
    sw->module_callbacks = (CALLBACK **)NULL;
}



/*************************************************************************
*   Register_Hook()
*
*   This adds a callback to the given callback list
*
*   Call with:
*       pointer to a SWISH structure
*       A hook ID (enum in modules.h)
*       priority -- callback functions are called from low number to high number
*       ID number of module (passed to module when Init_Modules is run)
*           The ID number allows passing a private data structure back to the module.
*
*   Returns:
*       void
*
*
**************************************************************************/


void Register_Hook(SWISH *sw, HOOK hookID, int priority, int selfID, CALLBACK_FUNCTION callback )
{
    CALLBACK    *node;
    CALLBACK    *newentry;
    /* quite a warning under windows */
    CALLBACK    *first = (CALLBACK *)sw->module_callbacks[ hookID ];

    if ( !isregistered_moduleID( selfID ) )
        progerr("register_swish_hook called with non registered module ID '%d'", selfID );

    if ( !valid_hookID( hookID ) )
        progerr("register_swish_hook called by '%s' with invalid hook ID '%d'",
            module_list[selfID].module_file, hookID );

    /* Create the new entry */
    newentry = (CALLBACK *) emalloc( sizeof( CALLBACK ) );

    newentry->callback_function = callback;
    newentry->module_id = selfID;
    newentry->priority  = priority;
    newentry->next = NULL;


    /* check to see if it should be the first one */
    if ( !first || first->priority > priority )
    {
        /* set next to either NULL (first one) or next, if others exist */
        newentry->next = sw->module_callbacks[ hookID ];
        sw->module_callbacks[ hookID ] = newentry;
        return;
    }


    /* search for insert point */
    for ( node = sw->module_callbacks[ hookID ]; node->next && node->priority <= priority; node = node->next );

    newentry->next = node->next;
    node->next = newentry;
}




/*************************************************************************
*   Run_Hook()
*
*   This adds a callback to the given callback list
*
*   Call with:
*       pointer to a SWISH structure
*       the HOOK id (which hook to run)
*       pointer to whatever needs to be passed to module(s).
*
*   Returns:
*       void
*
*   Module's callback function is called with:
*       pointer to a SWISH structure
*       module's ID (so can register other callbacks)
*       a pointer to the modules private data structure
*       the pointer that was passed to run_swish_hook()
*
*   Module's callback function returns:
*       HOOK_STATUS
*           STATUS_OK - continue processing other hooks
*           STATUS_STOP - don't process any higher priority number hooks
*
* TODO:
*   Figure out a way to keep a callback in a module from calling this
*   routine again, causing a loop.  Maybe all that's needed is a way
*   to detect if you are the one that called the routine.  It might be
*   Handy to do "sub-requests" on hooks.
*
*   Also, should this check that HOOK_SESSION_END is only called from
*   Free_Module_Hooks?
*
*
**************************************************************************/


HOOK_STATUS Run_Hook(SWISH *sw, HOOK hookID, void *parameters )
{
    CALLBACK *node;
    HOOK_STATUS status = HOOK_NONE;  /* Default action if no hooks */
    
    
    if ( !valid_hookID( hookID ) )
        progerr("run_swish_hook called with invalid hook ID '%d'", hookID );

    node = sw->module_callbacks[ hookID ];


    while ( node )
    {
        /* Call module's function with sw, its id, and the pointer to it's data structure */
        status = (node->callback_function)( sw, node->module_id, (void **)&(sw->module_private_data[node->module_id]), parameters );


        if ( HOOK_SESSION_END == hookID && HOOK_STOP == status )
        {
            progwarn("module '%s' (%s) Requested HOOK_STOP - ignoring.",
                module_list[node->module_id].module_name, module_list[node->module_id].module_file );

            status = HOOK_OK;
        }


        if ( HOOK_OK != status && HOOK_STOP != status )
            progerr("Module '%s' returned invalid status '%d' when processing hook '%d'",
                module_list[node->module_id].module_file, (int)status, (int)hookID );


        if ( HOOK_STOP == status )
            return status;

        node = node->next;            

    }

    return status;
}

            

