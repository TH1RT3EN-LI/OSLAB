#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

// define tlb_out
LEAF(tlb_out)
// Save original ENTRYHI to k1. ENTRYHI is a register in the CP0 coprocessor
// of MIPS, which handles system-level control and address translation，is a 
// baunch of registers in the MIPS.
	mfc0    k1,CP0_ENTRYHI

// Write argument a0 to ENTRYHI. a0 is the function argument, typically 
// PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id).
	mtc0    a0,CP0_ENTRYHI
	nop

// Probe the TLB for the current ENTRYHI value.
	tlbp
	nop
	nop
	nop
	nop

// Read the probe result into k0.
	mfc0    k0,CP0_INDEX

// If not found (k0 < 0), branch to NOFOUND.
	bltz    k0,NOFOUND
	nop

// Clear ENTRYHI and ENTRYLO0 to zero.
// Like ENTRYHI, ENTRYLO0 is also a register in the CP0 coprocessor
// of MIPS, which holds the physical address and other information for a TLB entry.
	mtc0    zero,CP0_ENTRYHI
	mtc0    zero,CP0_ENTRYLO0
	nop

// Overwrite the TLB entry with zeros(the content of ENTRYHI and ENTRYLO0).
	tlbwi

NOFOUND:
// Restore the original ENTRYHI value.
	mtc0    k1,CP0_ENTRYHI

// Return from function above.
	j   ra
	nop
END(tlb_out)
