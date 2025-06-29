#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <kclock.h>



.macro	setup_c0_status set clr
	.set	push
	mfc0	t0, CP0_STATUS
	or	t0, \set|\clr
	xor	t0, \clr
	mtc0	t0, CP0_STATUS			
	.set	pop
.endm

	.text
LEAF(set_timer)

	li t0, 0x01 // set the timer interrupt enable bit
	sb t0, 0xb5000100 // write to the timer control register
	sw	sp, KERNEL_SP // save the current stack pointer to KERNEL_SP
setup_c0_status STATUS_CU0|0x1001 0 // set the CP0 status register to enable user mode and interrupts, like before
	jr ra

	nop
END(set_timer)
