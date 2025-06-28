/*
 *  $Id: hello.c,v 1.3 2006/05/22 04:53:52 debug Exp $
 *
 *  GXemul demo:  Hello World
 *
 *  This file is in the Public Domain.
 */

#include "dev_cons.h"

/*  Note: The ugly cast to a signed int (32-bit) causes the address to be
	sign-extended correctly on MIPS when compiled in 64-bit mode  */
#define PHYSADDR_OFFSET     ((signed int)0x80000000)

/* 定义输出字符的物理地址 */
#define PUTCHAR_ADDRESS     (PHYSADDR_OFFSET +       \
				DEV_CONS_ADDRESS + DEV_CONS_PUTGETCHAR)
/* 定义停止（关机）操作的物理地址 */
#define HALT_ADDRESS       (PHYSADDR_OFFSET +        \
				DEV_CONS_ADDRESS + DEV_CONS_HALT)

/* 向控制台输出一个字符 */
void printcharc(char ch)
{
	*((volatile unsigned char *) PUTCHAR_ADDRESS) = ch;
}

/* 停止（关机）系统 */
void halt(void)
{
	*((volatile unsigned char *) HALT_ADDRESS) = 0;
}

/* 输出一个字符串到控制台 */
void printstr(char *s)
{
	while (*s)
		printcharc(*s++);
}
