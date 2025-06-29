/*
 * Copyright (C) 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
.section .text.exc_vec3
NESTED(except_vec3, 0, sp) // the entry point of the exception vector 3
		.set	noat
		.set	noreorder
		/*
		 * Register saving is delayed as long as we don't know
		 * which registers really need to be saved.
		 */
1:	//j	1b
	nop

		mfc0	k1,CP0_CAUSE
		la	k0,exception_handlers
		/*
		 * Next lines assumes that the used CPU type has max.
		 * 32 different types of exceptions. We might use this
		 * to implement software exceptions in the future.
		 */

		andi	k1,0x7c 
		addu	k0,k1
		lw	k0,(k0)
		NOP
		jr	k0
		nop
		END(except_vec3)
		.set	at


.data // get ready for data section, set 3 global variables to 0
			.globl mCONTEXT 
mCONTEXT:
			.word 0

			.globl delay
delay:
			.word 0

			.globl tlbra 
tlbra:
			.word 0

// Create a new section for stack data
			.section .data.stk // means this section is for stack Deallocate

KERNEL_STACK:           # Reserve space for the kernel stack
			.space 0x8000   # Allocate 32KB for the stack

			.text           # Switch back to the code section
LEAF(_start) // it should be the entry point of the kernel, the first function to be executed
	
	.set	mips2
	.set	reorder

	/* Disable interrupts */
	mtc0	zero, CP0_STATUS

        /* Disable watch exception. */
        mtc0    zero, CP0_WATCHLO
        mtc0    zero, CP0_WATCHHI

	/* disable kernel mode cache */
	mfc0	t0, CP0_CONFIG
	and	t0, ~0x7
	ori	t0, 0x2
	mtc0	t0, CP0_CONFIG

	/* set up stack */
	li	sp, 0x80400000 // stack started from 0x80400000

	li		t0,0x80400000 
	sw		t0,mCONTEXT // save the initial stack pointer to mCONTEXT, so the C code can use it later
	
	/* jump to main */
	jal	main
			
loop: // I think this loop is used to keep the kernel running, so that it can handle interrupts and other events
	j	loop
	nop
END(_start)