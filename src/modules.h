
/* Defines the available hooks in swish */
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



