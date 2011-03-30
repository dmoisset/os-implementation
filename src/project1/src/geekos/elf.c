/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/elf.h>


/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat)
{
    /* Start sanity tests before anything */

    /* The standard states that the first 4 bytes of an ELF file must
     * be: 0x7F, 'E', 'L', 'F'
     */
    if (exeFileData[0] != 0x007f || exeFileData[1] != 'E' || 
            exeFileData[2] != 'L' || exeFileData[3] != 'F'){
        return -1;
    }

    elfHeader *elfh = (elfHeader *) exeFileData;

    /* Check that exeFileLength is big enough at least to contain ELF header
     * and program headers */
    if (exeFileLength <= ((ulong_t) elfh->ehsize + 
                ((ulong_t) elfh->phnum * (ulong_t) elfh->phentsize)))
        return -2;

    /* Check the ELF has less than EXE_MAX_SEGMENTS */
    if (elfh->phnum > EXE_MAX_SEGMENTS)
       return -3; 

    programHeader *ph = (programHeader *) (exeFileData + elfh->phoff);
    int i = 0;
 
    exeFormat->numSegments = (int) elfh->phnum;
    exeFormat->entryAddr = (ulong_t) elfh->entry;

    for (i = 0; i < exeFormat->numSegments; i++) {
        exeFormat->segmentList[i].offsetInFile = (ulong_t) ph->offset;
        exeFormat->segmentList[i].lengthInFile = (ulong_t) ph->fileSize;
        exeFormat->segmentList[i].startAddress = (ulong_t) ph->paddr;
        exeFormat->segmentList[i].sizeInMemory = (ulong_t) ph->memSize;
        exeFormat->segmentList[i].protFlags    = (int) ph->flags;

        ph++;
    } 

    return 0;
}

