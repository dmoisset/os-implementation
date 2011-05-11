/*
 * Segmentation-based user mode implementation
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.23 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/ktypes.h>
#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/mem.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/int.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/tss.h>
#include <geekos/kthread.h>
#include <geekos/argblock.h>
#include <geekos/user.h>

/* ----------------------------------------------------------------------
 * Variables
 * ---------------------------------------------------------------------- */

#define DEFAULT_USER_STACK_SIZE 8192


/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */


/*
 * Create a new user context of given size
 */

static struct User_Context* Create_User_Context(ulong_t size)
{
    /* Tiene que ser múltiplo de PAGE_SIZE */
    KASSERT(size%PAGE_SIZE == 0);

    /* Pido memoria para el proceso */
    char *mem = (char *) Malloc(size);
    if (mem == NULL)
        goto error;

    /* Reset memory with zeros */
    memset(mem, '\0', size);

    /* Pido memoria para el User_Context */
    struct User_Context *userContext = Malloc(sizeof(struct User_Context));
    if (userContext ==  NULL)
        goto error;

    /* Guardo el segment descriptor de la ldt en la gdt */
    struct Segment_Descriptor *ldt_desc = Allocate_Segment_Descriptor(); //LDT-Descriptor for the process
    if (ldt_desc == NULL)
        goto error;

    Init_LDT_Descriptor(ldt_desc, userContext->ldt, NUM_USER_LDT_ENTRIES);
    /* Creo un selector para el descriptor de ldt */
    ushort_t ldt_selector = Selector(KERNEL_PRIVILEGE, true, Get_Descriptor_Index(ldt_desc)); 

    /* Inicializo los segmentos */
    Init_Code_Segment_Descriptor(&(userContext->ldt[0]), (ulong_t)mem, size/PAGE_SIZE, USER_PRIVILEGE);
    Init_Data_Segment_Descriptor(&(userContext->ldt[1]), (ulong_t)mem, size/PAGE_SIZE, USER_PRIVILEGE);

    /* Creo los selectores */
    ushort_t cs_selector = Selector(USER_PRIVILEGE, false, 0);
    ushort_t ds_selector = Selector(USER_PRIVILEGE, false, 1);

    /* Asigno todo al userContext */
    userContext->ldtDescriptor = ldt_desc;
    userContext->ldtSelector = ldt_selector;
    userContext->csSelector = cs_selector;
    userContext->dsSelector = ds_selector;
    userContext->size = size;
    userContext->memory = mem;
    userContext->refCount = 0;

    goto success;

error:
    if (mem != NULL)
        Free(mem);
    if (userContext != NULL)
        Free(userContext);
    return NULL;

success:
    return userContext;
}

static bool Validate_User_Memory(struct User_Context* userContext,
    ulong_t userAddr, ulong_t bufSize)
{
    ulong_t avail;

    if (userAddr >= userContext->size)
        return false;

    avail = userContext->size - userAddr;
    if (bufSize > avail)
        return false;

    return true;
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context* userContext)
{
    KASSERT(userContext != NULL);
    KASSERT(userContext->memory != NULL);
    KASSERT(userContext->ldtDescriptor != NULL);
    /*
     * Hints:
     * - you need to free the memory allocated for the user process
     * - don't forget to free the segment descriptor allocated
     *   for the process's LDT
     */
    Free_Segment_Descriptor(userContext->ldtDescriptor);
    Free(userContext->memory);
    Free(userContext);
}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat, const char *command,
    struct User_Context **pUserContext)
{
    KASSERT(exeFileData != NULL);
    KASSERT(command != NULL);
    KASSERT(exeFormat != NULL);
    
    /*
     * Hints:
     * - Determine where in memory each executable segment will be placed
     * - Determine size of argument block and where it memory it will
     *   be placed
     * - Copy each executable segment into memory
     * - Format argument block in memory
     * - In the created User_Context object, set code entry point
     *   address, argument block address, and initial kernel stack pointer
     *   address
     */
    /* Por Victor Rosales */
    int i = 0;
    int ret = 0;
    ulong_t maxva = 0;
    ulong_t argBlockSize = 0;
    ulong_t stackAddr = 0;
    unsigned numArgs = 0;
    unsigned long virtSize;
    struct User_Context *userContext = 0;

    /* Find maximum virtual address */
    for (i = 0; i < exeFormat->numSegments; ++i) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];
        ulong_t topva = segment->startAddress + segment->sizeInMemory;

        if (topva > maxva)
            maxva = topva;
    }

    Get_Argument_Block_Size(command, &numArgs, &argBlockSize);

    virtSize = Round_Up_To_Page(maxva);
    virtSize += Round_Up_To_Page(DEFAULT_USER_STACK_SIZE);
    stackAddr = virtSize;
    virtSize += Round_Up_To_Page(argBlockSize);

    userContext = Create_User_Context(virtSize);

    /* Copy segments over into process' memory space */
    for (i = 0; i < exeFormat->numSegments; i++) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];

        memcpy(userContext->memory + segment->startAddress,
            exeFileData + segment->offsetInFile,
            segment->lengthInFile);
    }

    Format_Argument_Block(userContext->memory + stackAddr,
                            numArgs,
                            stackAddr,
                            command);

    /* Create the user context */
    userContext->entryAddr          = exeFormat->entryAddr;
    userContext->argBlockAddr       = stackAddr;
    userContext->stackPointerAddr   = stackAddr;

    if (userContext == NULL)
        ret = ENOMEM;

    *pUserContext = userContext;

    return ret;
}

/*
 * Copy data from user memory into a kernel buffer.
 * Params:
 * destInKernel - address of kernel buffer
 * srcInUser - address of user buffer
 * bufSize - number of bytes to copy
 *
 * Returns:
 *   true if successful, false if user buffer is invalid (i.e.,
 *   doesn't correspond to memory the process has a right to
 *   access)
 */
bool Copy_From_User(void* destInKernel, ulong_t srcInUser, ulong_t bufSize)
{
    KASSERT(destInKernel != NULL);
    KASSERT(srcInUser != 0);
    /*
     * Hints:
     * - the User_Context of the current process can be found
     *   from g_currentThread->userContext
     * - the user address is an index relative to the chunk
     *   of memory you allocated for it
     * - make sure the user buffer lies entirely in memory belonging
     *   to the process
     */
    bool mem_valid = false;

    if (g_currentThread->userContext != NULL) {
        mem_valid = Validate_User_Memory(g_currentThread->userContext,
                                            srcInUser, bufSize);
        if (mem_valid) {
                memcpy(destInKernel,
                        g_currentThread->userContext->memory+srcInUser,
                        bufSize);
        }
    }
    return mem_valid;
}

/*
 * Copy data from kernel memory into a user buffer.
 * Params:
 * destInUser - address of user buffer
 * srcInKernel - address of kernel buffer
 * bufSize - number of bytes to copy
 *
 * Returns:
 *   true if successful, false if user buffer is invalid (i.e.,
 *   doesn't correspond to memory the process has a right to
 *   access)
 */
bool Copy_To_User(ulong_t destInUser, void* srcInKernel, ulong_t bufSize)
{
    KASSERT(srcInKernel != NULL);
    KASSERT(destInUser != 0);
    /*
     * Hints: same as for Copy_From_User()
     */
    bool mem_valid = false;

    if (g_currentThread->userContext != NULL) {
        mem_valid = Validate_User_Memory(g_currentThread->userContext,
                                            destInUser, bufSize);
        if (mem_valid) {
            memcpy(g_currentThread->userContext->memory+destInUser, srcInKernel,
                    bufSize);
        }
    }
    return mem_valid;
}

/*
 * Switch to user address space belonging to given
 * User_Context object.
 * Params:
 * userContext - the User_Context
 */
void Switch_To_Address_Space(struct User_Context *userContext)
{
    KASSERT(userContext != NULL);
    KASSERT(userContext->ldtSelector != 0);
    /*
     * Hint: you will need to use the lldt assembly language instruction
     * to load the process's LDT by specifying its LDT selector.
     */
    /* Load the task register */
    __asm__ __volatile__ (
        "lldt %0"
        :
        : "a" (userContext->ldtSelector)
    );
}

