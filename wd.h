#ifndef __WD_H__
#define __WD_H__

#include <stddef.h>     /* for size_t type */
#include <time.h>       /* for time_t type */

/* -----------------------------------------------------------------------------

    Watch-Dog 

    Description - 
    The watch-dog will restart a program in case it crashes between WDStart call
    and the WDEnd call.
    It is typically used to protect a critical code section that might lead to
    the crashing of the program.

    Note -
    SIGUSR1 & SIGUSR2 signals are reserved for the watchdog exclusively 
    and are not to be used by the calling program.
*/


/*  wd_status description -

    INIT_FAIL -        returned when a failure occured during the initialization
                       of WD.
                       Includes failure of memory allocation, system calls and 
                       standard library functions. 

    TERMINATION_FAIL - returned when a failure occured at program termination.
*/

typedef enum wd_status
{
    WD_SUCCESS = 0,
    WD_INIT_FAIL = 1,
    WD_TERMINATION_FAIL = 2
} wd_status_t;

/* -----------------------------------------------------------------------------
    @ description: activates watch-dog's protection.

    @ params:      interval_in_sec - The calling process will post a "life sign" 
                   every interval_in_sec seconds onto the watch-dog process.
                   
                   intervals_per_check - The watch-dog process will check the
                   "life sign" of the calling process every intervals_per_check
                   intervals.

                   intervals_per_check will get the value of the maximum between
                   the user's input and 2.

                   argc & argv - should be sent to WDStart unchanged as recived
                   from main.

                   It is critical that argv[0] will contain the calling 
                   process's execution command, otherwise the watch-dog won't
                   be able to restart it in case of a crash.

                   

    @ note:        The Calling process's executable name must not be "wd.out".
                      
*/
wd_status_t WDStart(time_t interval_in_sec, size_t intervals_per_check,
                    int argc, char *argv[]);


/* -----------------------------------------------------------------------------
    @ description: this function deactivates the watchdog
*/
wd_status_t WDStop(void);

/* 
    if I had more time:
            - fix bug that when parent process dies and then child process dies,
              some process is left alive, and its worker thread doesn't join.
            - make a shared mem mutex on logger
            - use other scheduler with smaller intervals
            - stop user's main thread when wd crashes
            - add another param to take from user to specify max time allowed.
                handles dead-locks and infinte loops.
*/

#endif /*__WD_H__*/  
