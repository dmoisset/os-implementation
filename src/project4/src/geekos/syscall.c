/*
 * System call handlers
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003,2004 David Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.59 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/syscall.h>
#include <geekos/errno.h>
#include <geekos/kthread.h>
#include <geekos/int.h>
#include <geekos/elf.h>
#include <geekos/malloc.h>
#include <geekos/screen.h>
#include <geekos/keyboard.h>
#include <geekos/string.h>
#include <geekos/user.h>
#include <geekos/timer.h>
#include <geekos/vfs.h>

/*
 * Null system call.
 * Does nothing except immediately return control back
 * to the interrupted user program.
 * Params:
 *  state - processor registers from user mode
 *
 * Returns:
 *   always returns the value 0 (zero)
 */
static int Sys_Null(struct Interrupt_State* state)
{
    Print("Sys_Null was called.\n");
    return 0;
}

/*
 * Exit system call.
 * The interrupted user process is terminated.
 * Params:
 *   state->ebx - process exit code
 * Returns:
 *   Never returns to user mode!
 */
static int Sys_Exit(struct Interrupt_State* state)
{
    Enable_Interrupts();
    Detach_User_Context(g_currentThread);
    Disable_Interrupts();
    Exit(state->ebx);
}

/*
 * Print a string to the console.
 * Params:
 *   state->ebx - user pointer of string to be printed
 *   state->ecx - number of characters to print
 * Returns: 0 if successful, -1 if not
 */
static int Sys_PrintString(struct Interrupt_State* state)
{
    char *message = NULL;
    ulong_t len = state->ecx;

    if (len > 1024){
        return -1;
    }

    message = Malloc(sizeof(char) * len + 1);
    if (message == NULL) {
        return -1;
    }

    if (!Copy_From_User(message, state->ebx, len)) {
        Free(message);
        return -1;
    }

    Put_Buf(message, len);
    Free(message);
    return 0;
}

/*
 * Get a single key press from the console.
 * Suspends the user process until a key press is available.
 * Params:
 *   state - processor registers from user mode
 * Returns: the key code
 */
static int Sys_GetKey(struct Interrupt_State* state)
{
    int keyCode = Wait_For_Key();
    return keyCode;
}

/*
 * Set the current text attributes.
 * Params:
 *   state->ebx - character attributes to use
 * Returns: always returns 0
 */
static int Sys_SetAttr(struct Interrupt_State* state)
{
    Set_Current_Attr(state->ebx);
    return 0;
}

/*
 * Get the current cursor position.
 * Params:
 *   state->ebx - pointer to user int where row value should be stored
 *   state->ecx - pointer to user int where column value should be stored
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_GetCursor(struct Interrupt_State* state)
{
    int r=0, c=0;
    Get_Cursor(&r, &c);
    if (!Copy_To_User(state->ebx, &r, sizeof(int))) {
        return -1;
    }
    if (!Copy_To_User(state->ecx, &c, sizeof(int))) {
        return -1;
    }
    return 0;
}

/*
 * Set the current cursor position.
 * Params:
 *   state->ebx - new row value
 *   state->ecx - new column value
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_PutCursor(struct Interrupt_State* state)
{
    if (Put_Cursor(state->ebx, state->ecx))
        return 0;
    else
        return -1;
}

/*
 * Create a new user process.
 * Params:
 *   state->ebx - user address of name of executable
 *   state->ecx - length of executable name
 *   state->edx - user address of command string
 *   state->esi - length of command string
 * Returns: pid of process if successful, error code (< 0) otherwise
 */
static int Sys_Spawn(struct Interrupt_State* state)
{
    int retVal = -1;
    char *exeName = NULL;
    char *command = NULL;
    ulong_t exeNameLen = state->ecx + 1; /* +1 to add the 0 NULL later */
    ulong_t commandLen = state->esi + 1; /* +1 to add the 0 NULL later */
    struct Kernel_Thread* kthread = NULL;

    /* get some memory for the exe name and the args */
    exeName = (char*) Malloc(exeNameLen);
    if (exeName == NULL)
        goto memfail;
    command = (char*) Malloc(commandLen);
    if (command == NULL)
        goto memfail;

    memset(exeName, '\0', exeNameLen);
    if(!Copy_From_User(exeName, state->ebx, exeNameLen)){
        retVal = EUNSPECIFIED;
        goto fail;
    }
    memset(command, '\0', commandLen);
    if(!Copy_From_User(command, state->edx, commandLen)) {
        retVal = EUNSPECIFIED;
        goto fail;
    }

    Enable_Interrupts();
    if ((retVal = Spawn(exeName, command, &kthread))) {
        goto fail;
    }
    Disable_Interrupts();

    if (exeName!=NULL)
        Free(exeName);
    if (command!=NULL)
        Free(command);

    return kthread->pid;

memfail:
    retVal = ENOMEM;

fail:
    if(exeName)
        Free(exeName);
    if (command)
        Free(command);
    return retVal;
}

/*
 * Wait for a process to exit.
 * Params:
 *   state->ebx - pid of process to wait for
 * Returns: the exit code of the process,
 *   or error code (< 0) on error
 */
static int Sys_Wait(struct Interrupt_State* state)
{
    int exit_code = -1;
    struct Kernel_Thread *kthread = Lookup_Thread(state->ebx);
    if (kthread==NULL)
        return -1;

    Enable_Interrupts();
    exit_code = Join(kthread);
    Disable_Interrupts();

    return exit_code;
}

/*
 * Get pid (process id) of current thread.
 * Params:
 *   state - processor registers from user mode
 * Returns: the pid of the current thread
 */
static int Sys_GetPID(struct Interrupt_State* state)
{
    return g_currentThread->pid;
}

/*
 * Set the scheduling policy.
 * Params:
 *   state->ebx - policy,
 *   state->ecx - number of ticks in quantum
 * Returns: 0 if successful, -1 otherwise
 */
static int Sys_SetSchedulingPolicy(struct Interrupt_State* state)
{
    TODO("SetSchedulingPolicy system call");
    return -1;
}

/*
 * Get the time of day.
 * Params:
 *   state - processor registers from user mode
 *
 * Returns: value of the g_numTicks global variable
 */
static int Sys_GetTimeOfDay(struct Interrupt_State* state)
{
    TODO("GetTimeOfDay system call");
    return -1;
}

/*
 * Create a semaphore.
 * Params:
 *   state->ebx - user address of name of semaphore
 *   state->ecx - length of semaphore name
 *   state->edx - initial semaphore count
 * Returns: the global semaphore id
 */
static int Sys_CreateSemaphore(struct Interrupt_State* state)
{
    TODO("CreateSemaphore system call");
    return -1;
}

/*
 * Acquire a semaphore.
 * Assume that the process has permission to access the semaphore,
 * the call will block until the semaphore count is >= 0.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_P(struct Interrupt_State* state)
{
    TODO("P (semaphore acquire) system call");
    return -1;
}

/*
 * Release a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_V(struct Interrupt_State* state)
{
    TODO("V (semaphore release) system call");
    return -1;
}

/*
 * Destroy a semaphore.
 * Params:
 *   state->ebx - the semaphore id
 *
 * Returns: 0 if successful, error code (< 0) if unsuccessful
 */
static int Sys_DestroySemaphore(struct Interrupt_State* state)
{
    TODO("DestroySemaphore system call");
    return -1;
}


/*
 * Global table of system call handler functions.
 */
const Syscall g_syscallTable[] = {
    Sys_Null,
    Sys_Exit,
    Sys_PrintString,
    Sys_GetKey,
    Sys_SetAttr,
    Sys_GetCursor,
    Sys_PutCursor,
    Sys_Spawn,
    Sys_Wait,
    Sys_GetPID,
    /* Scheduling and semaphore system calls. */
    Sys_SetSchedulingPolicy,
    Sys_GetTimeOfDay,
    Sys_CreateSemaphore,
    Sys_P,
    Sys_V,
    Sys_DestroySemaphore,
};

/*
 * Number of system calls implemented.
 */
const int g_numSyscalls = sizeof(g_syscallTable) / sizeof(Syscall);
