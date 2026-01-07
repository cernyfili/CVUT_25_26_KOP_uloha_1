#include <stdio.h>
#include "ctrlc_handler.h"
#ifdef _MSC_VER
#include <windows.h>            /* ctrl c handler */
#undef max
#undef min
#else
#include <unistd.h>             /* getopt */
#include <limits.h>
#include <float.h>
#include <signal.h>
#endif

int cont=1;                     /* do continue iterating */

/*-----------------------------------------------------------------------------*/
/*  CTRL-C handler                                                             */
/*-----------------------------------------------------------------------------*/
#ifdef _MSC_VER
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        
    case CTRL_CLOSE_EVENT:  /* CTRL-CLOSE: confirm that the user wants to exit. */
    case CTRL_C_EVENT:      /* Handle the CTRL-C signal. */
        cont = 0;
        return TRUE;
                
    case CTRL_BREAK_EVENT:  /* Pass other signals to the next handler. */
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        cont = 0;
        return FALSE;
    default:
        return FALSE;
    }
}
#else
void handler (int signo) {
    cont = 0;
}
#endif

/*-----------------------------------------------------------------------------*/
int* establish_handler (const char* prog) {
#ifdef _MSC_VER
    if (! SetConsoleCtrlHandler(CtrlHandler, TRUE)) { 
        fprintf (stderr, "%s: cannot establish a signal handler\n", prog); 
        return NULL; 
    }
#else
    struct sigaction act = { 0 };

    act.sa_flags = SA_RESTART;
    act.sa_handler = &handler;

    if (sigaction(SIGINT,&act,NULL) != 0) {
        fprintf (stderr, "%s: cannot establish a signal handler\n", prog); 
        return NULL; 
    }
#endif
    return &cont;
}


