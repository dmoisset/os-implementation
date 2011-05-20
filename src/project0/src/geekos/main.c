/*
 * GeekOS C code entry point
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, Iulian Neamtiu <neamtiu@cs.umd.edu>
 * $Revision: 1.51 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/bootinfo.h>
#include <geekos/string.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/crc32.h>
#include <geekos/tss.h>
#include <geekos/int.h>
#include <geekos/kthread.h>
#include <geekos/trap.h>
#include <geekos/timer.h>
#include <geekos/keyboard.h>


static void Project0(ulong_t arg);


/*
 * Kernel C code entry point.
 * Initializes kernel subsystems, mounts filesystems,
 * and spawns init process.
 */
void Main(struct Boot_Info* bootInfo)
{
    Init_BSS();
    Init_Screen();
    Init_Mem(bootInfo);
    Init_CRC32();
    Init_TSS();
    Init_Interrupts();
    Init_Scheduler();
    Init_Traps();
    Init_Timer();
    Init_Keyboard();


    Set_Current_Attr(ATTRIB(BLACK, GREEN|BRIGHT));
    Print("Welcome to GeekOS!\n");
    Set_Current_Attr(ATTRIB(BLACK, GRAY));


    Start_Kernel_Thread(Project0, 0, PRIORITY_NORMAL, false);
    //TODO("Start a kernel thread to echo pressed keys and print counts");



    /* Now this thread is done. */
    Exit(0);
}


/* 
 * This is the body of the Project0 thread. Its job is to print
 * "Hello from Victor" and then use the Wait_For_Key() until (control-d)
 * character is pressed
 */
static void Project0(ulong_t arg)
{
    Keycode keycode;
    int count = 1;
    
    Print("Hello from Victor\n");

    do {
        /* Wait for key pressed */
        keycode = Wait_For_Key();
        /* Check if the key was pressed and actually keep only the 
         * release event (which is the same key)
         */
        if (keycode & KEY_RELEASE_FLAG) 
            Print("Key Pressed: %c\n Count: %d\n", keycode, count++);
        /* Stop when we recive 0x4064, which is:
         * KEY_CTRL_FLAG = 0x4000
         * d = 0x0064
         */
    } while (keycode != 0x4064); 

    Print("Echo terminated :)\n");
    /* I saw that this gives up the CPU for other threads so 
     * ... lets play nice
     */
    Yield();
}








