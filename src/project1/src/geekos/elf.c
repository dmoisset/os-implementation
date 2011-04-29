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
  /* Simple sanity check */
  if (exeFileData == NULL || exeFileLength <= 0 || exeFormat == NULL)
      return -1;

  elfHeader *elf = (elfHeader *) exeFileData;

  /* Check for magic number */
  if (elf->ident[0] != 0x7f || elf->ident[1] != 'E' || 
          elf->ident[2] != 'L' || elf->ident[3] != 'F') 
      return -1;

  /* Fail if not executable */
  if (elf->type != 0x2)
      return -1;

  /* Fail if not supported version */
  if (elf->version != 0x1)
      return -1;

  /* Fail if no valid program headers */
  if (elf->phnum < 0)
      return -1;

  /* Fail if not supported architecture (Intel) */
  if (elf->machine != 0x3)
      return -1;

  /* Fail if memory address are not "inside" the ELF */
  if (elf->entry > exeFileLength || elf->phoff > exeFileLength ||
          elf->sphoff > exeFileLength)
      return -1;

  /* Fail if reported ELF header size does not match actual
   * ELF header size */
  if (elf->ehsize != sizeof(elfHeader))
      return -1;

  /* Fail if reported program header size does not match actual
   * program header size */
  if (elf->phentsize != sizeof(programHeader))
      return -1;

  exeFormat->entryAddr = elf->entry;
  exeFormat->numSegments = elf->phnum;
  programHeader *program =
      (programHeader *) (exeFileData + elf->phoff);

  int i;
  for (i = 0; i < elf->phnum; i++) {

    /* Fail if memory address are not "inside" the ELF */
    if (program->offset > exeFileLength ||
            program->fileSize > exeFileLength ||
            program->memSize > exeFileLength)
        return -1;

    exeFormat->segmentList[i].offsetInFile = program->offset;
    exeFormat->segmentList[i].lengthInFile = program->fileSize;
    exeFormat->segmentList[i].startAddress = program->vaddr;
    exeFormat->segmentList[i].sizeInMemory = program->memSize;
    exeFormat->segmentList[i].protFlags = program->flags;
    program++;

  }

  return 0;
}
