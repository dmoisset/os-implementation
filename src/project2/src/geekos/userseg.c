/*
 * Segmentation-based user mode implementation
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.23 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

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

/* TODO: Implement
*/
static struct User_Context* Create_User_Context(ulong_t size)
{   
    struct User_Context *userContext = 0;
    /* TODO ... do this?!?!?!? */

    /* Allocate space for User_Context 'user.h' */
    userContext = (struct User_Context*) Malloc(sizeof(*userContext));

    /* Allocate 'size' memory and fill size fild of userContext 
     * (should it be cleared?)
     */
    userContext->memory = Malloc(size);
    userContext->size   = size;

    /* Allocate a LDT */
    /* METODO - Handle NULL (could not allocate descriptor) */
    userContext->ldtDescriptor = Allocate_Segment_Descriptor();

    /* Init LDT */
    Init_LDT_Descriptor(userContext->ldtDescriptor, userContext->ldt,
            NUM_USER_LDT_ENTRIES);    

    /* Create a selector that contains the location of the LDT descriptor
     * whitin the GDT 
     */
    userContext->ldtSelector = Selector(USER_PRIVILEGE, false, 
            Get_Descriptor_Index(userContext->ldtDescriptor));

    /* Create descriptors for the code and the data segments of the
     * user program and add these descriptors to the LDT
     */
    Init_Code_Segment_Descriptor(&userContext->ldt[0], 
            (ulong_t) userContext->memory,
            size/PAGE_SIZE, 
            USER_PRIVILEGE);

    Init_Data_Segment_Descriptor(&userContext->ldt[1], 
            (ulong_t) userContext->memory,
            size/PAGE_SIZE, 
            USER_PRIVILEGE);

    /* Create selectors that contain the location of the two descriptors
     * within the LDT
     */
    userContext->csSelector = Selector(USER_PRIVILEGE, false, 0);
    userContext->dsSelector = Selector(USER_PRIVILEGE, false, 1);

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
    /*
     * Hints:
     * - you need to free the memory allocated for the user process
     * - don't forget to free the segment descriptor allocated
     *   for the process's LDT
     */
    TODO("Destroy a User_Context");
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
   // TODO("Load a user executable into a user memory space using segmentation");
    unsigned long virtSize;
    int i = 0;
    ulong_t maxva = 0;
    unsigned numArgs = 0;
    ulong_t argBlockSize = 0;
    ulong_t start_argBlock_mem = 0;
    
    /* Find maximum virtual address */
    for (i = 0; i < exeFormat->numSegments; ++i) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];
        ulong_t topva = segment->startAddress + segment->sizeInMemory;

        if (topva > maxva)
            maxva = topva;
    }

    /* Find Argument Block Size */
    Get_Argument_Block_Size(command, &numArgs, &argBlockSize);
    virtSize = Round_Up_To_Page(maxva) + 
        Round_Up_To_Page(DEFAULT_USER_STACK_SIZE); 
    
    start_argBlock_mem = virtSize;

    virtSize += argBlockSize;

    *pUserContext = Create_User_Context(virtSize);


    Format_Argument_Block((*pUserContext)->memory + start_argBlock_mem, 
            numArgs, 
            start_argBlock_mem,
            command);


    /* Create the user context */
    (*pUserContext)->entryAddr          = exeFormat->entryAddr;
    (*pUserContext)->argBlockAddr       = start_argBlock_mem;
    (*pUserContext)->stackPointerAddr   = virtSize;

    /* Copy segments over into process' memory space */
    for (i = 0; i < exeFormat->numSegments; i++){
        struct Exe_Segment *segment = &exeFormat->segmentList[i];

        memcpy((*pUserContext)->memory + segment->startAddress,
            exeFileData + segment->offsetInFile,
            segment->lengthInFile);
    }


    return 0;
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
    /*
     * Hints:
     * - the User_Context of the current process can be found
     *   from g_currentThread->userContext
     * - the user address is an index relative to the chunk
     *   of memory you allocated for it
     * - make sure the user buffer lies entirely in memory belonging
     *   to the process
     */
    TODO("Copy memory from user buffer to kernel buffer");
    Validate_User_Memory(NULL,0,0); /* delete this; keeps gcc happy */
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
    /*
     * Hints: same as for Copy_From_User()
     */
    TODO("Copy memory from kernel buffer to user buffer");
}

/*
 * Switch to user address space belonging to given
 * User_Context object.
 * Params:
 * userContext - the User_Context
 */
void Switch_To_Address_Space(struct User_Context *userContext)
{
    /*
     * Hint: you will need to use the lldt assembly language instruction
     * to load the process's LDT by specifying its LDT selector.
     */
    TODO("Switch to user address space using segmentation/LDT");
}

