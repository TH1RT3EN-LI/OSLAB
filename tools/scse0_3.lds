OUTPUT_ARCH(mips)
/*
Set the architecture to mips.
*/
ENTRY(_start)
/*
Set the ENTRY point of the program to _start.
*/


/* - My work: 
 * - Sets the program start address to 0x80001000.
 * - Places all code sections (.text) at the start.
 * - Follows with initialized data (.data) and uninitialized data (.bss).
 */
SECTIONS
{
    . = 0x80001000;
    .text : { *(.text) } 
    .data : { *(.data) }
    .bss : { *(.bss) }
    
    
    end = . ;
}