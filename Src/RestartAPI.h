#ifndef RESTART_API_H
#define RESTART_API_H

// Command line switch for restarted application
#ifndef RA_CMDLINE_RESTART_PROCESS
	#define RA_CMDLINE_RESTART_PROCESS	TEXT("--Restart")
#endif

// Mutex unique name
#ifndef RA_MUTEX_OTHER_RESTARTING
	#define RA_MUTEX_OTHER_RESTARTING	TEXT("YOUR_RESTART-MUTEX_NAME")
#endif

// Return TRUE if Process was restarted
BOOL RA_CheckProcessWasRestarted();

// Check process command line for restart switch
// Call this function to check that is restarted instance
BOOL RA_CheckForRestartProcessStart();

// Wait till previous instance of process finish
BOOL RA_WaitForPreviousProcessFinish();

// Call it when process finish
BOOL RA_DoRestartProcessFinish();

// Call this function when you need restart application
// After call you must close an active instance of your application
BOOL RA_ActivateRestartProcess();

#endif // RESTART_API_H
