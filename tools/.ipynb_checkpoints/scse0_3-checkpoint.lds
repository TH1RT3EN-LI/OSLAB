OUTPUT_ARCH(mips)
/*
Set the architecture to mips.
*/
ENTRY(_start)
/*
Set the ENTRY point of the program to _start.
*/
SECTIONS
{
    . = 0x80001000;
    .text : { *(.text) }
    . = 0x80002000;        /* 举例把 .data 放到另一个固定地址 */
    .data : { *(.data) }
    . = 0x80003000;        /* 再把 .bss 放到后面 */
    .bss : { *(.bss) }
}