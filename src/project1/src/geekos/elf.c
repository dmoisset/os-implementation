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
int Parse_ELF_Executable(char *exeFileData,
			 ulong_t exeFileLength,
			 struct Exe_Format *exeFormat)
{
  KASSERT(exeFileData!=NULL);
  KASSERT(exeFileLength>=0);
  KASSERT(exeFormat!=NULL);

  elfHeader *elf = (elfHeader *) exeFileData;

  KASSERT(elf->ident[0] == 0x7f);
  KASSERT(elf->ident[1] == 'E');
  KASSERT(elf->ident[2] == 'L');
  KASSERT(elf->ident[3] == 'F');
  KASSERT(elf->type = 0x2);
  KASSERT(elf->version == 0x1);
  KASSERT(elf->phnum > 0);
  KASSERT(elf->machine = 0x3);
  KASSERT(elf->entry < exeFileLength);
  KASSERT(elf->phoff < exeFileLength);
  KASSERT(elf->sphoff < exeFileLength);
  KASSERT(elf->ehsize == sizeof(elfHeader));
  KASSERT(elf->phentsize == sizeof(programHeader));

  exeFormat->entryAddr = elf->entry;
  exeFormat->numSegments = elf->phnum;

  int i;
  for (i = 0; i < elf->phnum; i++) {

    programHeader *program =
      (programHeader *) (exeFileData + elf->phoff + elf->phentsize * i);
    
    KASSERT(program->offset < exeFileLength);
    KASSERT(program->fileSize < exeFileLength);
    KASSERT(program->memSize < exeFileLength);

    exeFormat->segmentList[i].offsetInFile = program->offset;
    exeFormat->segmentList[i].lengthInFile = program->fileSize;
    exeFormat->segmentList[i].startAddress = program->vaddr;
    exeFormat->segmentList[i].sizeInMemory = program->memSize;
    exeFormat->segmentList[i].protFlags = program->flags;
  }

  return 0;
}
