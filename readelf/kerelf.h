/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995, 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ian Lance Taylor <ian@cygnus.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _KER_ELF_H
#define _KER_ELF_H

#include "types.h"

// 基本类型定义
typedef u_int64_t uint64_t;
typedef u_int32_t uint32_t;
typedef u_int16_t uint16_t;

/* 16位数量类型 */
typedef uint16_t Elf32_Half;

/* 有符号和无符号32位数量类型 */
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;

/* 有符号和无符号64位数量类型 */
typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;

/* 地址类型 */
typedef uint32_t Elf32_Addr;

/* 文件偏移类型 */
typedef uint32_t Elf32_Off;

/* 段索引类型，16位 */
typedef uint16_t Elf32_Section;

/* 符号索引类型 */
typedef uint32_t Elf32_Symndx;

/* ELF文件头。每个ELF文件开头都出现此结构体 */

#define EI_NIDENT (16) // ELF头部标识字节数

typedef struct
{
        unsigned char e_ident[EI_NIDENT]; /* 魔数和其他信息 */
        Elf32_Half e_type;                /* 文件类型 */
        Elf32_Half e_machine;             /* 架构类型 */
        Elf32_Word e_version;             /* 版本号 */
        Elf32_Addr e_entry;               /* 程序入口虚拟地址 */
        Elf32_Off e_phoff;                /* 程序头表文件偏移 */
        Elf32_Off e_shoff;                /* 节区头表文件偏移 */
        Elf32_Word e_flags;               /* 处理器相关标志 */
        Elf32_Half e_ehsize;              /* ELF头部大小（字节） */
        Elf32_Half e_phentsize;           /* 程序头表项大小 */
        Elf32_Half e_phnum;               /* 程序头表项数量 */
        Elf32_Half e_shentsize;           /* 节区头表项大小 */
        Elf32_Half e_shnum;               /* 节区头表项数量 */
        Elf32_Half e_shstrndx;            /* 节区头字符串表索引 */
} Elf32_Ehdr;

/* e_ident数组中的字段。EI_*宏是数组的索引。
        每个EI_*下的宏是该字节可能的值。 */

#define EI_MAG0 0    /* 文件标识字节0索引 */
#define ELFMAG0 0x7f /* 魔数字节0 */

#define EI_MAG1 1   /* 文件标识字节1索引 */
#define ELFMAG1 'E' /* 魔数字节1 */

#define EI_MAG2 2   /* 文件标识字节2索引 */
#define ELFMAG2 'L' /* 魔数字节2 */

#define EI_MAG3 3   /* 文件标识字节3索引 */
#define ELFMAG3 'F' /* 魔数字节3 */

/* 节区头结构体 */
typedef struct
{
        Elf32_Word sh_name;      /* 节区名称 */
        Elf32_Word sh_type;      /* 节区类型 */
        Elf32_Word sh_flags;     /* 节区标志 */
        Elf32_Addr sh_addr;      /* 节区地址 */
        Elf32_Off sh_offset;     /* 节区偏移 */
        Elf32_Word sh_size;      /* 节区大小 */
        Elf32_Word sh_link;      /* 节区链接信息 */
        Elf32_Word sh_info;      /* 节区附加信息 */
        Elf32_Word sh_addralign; /* 节区对齐 */
        Elf32_Word sh_entsize;   /* 节区项大小 */
} Elf32_Shdr;

/* 程序段头结构体 */

typedef struct
{
        Elf32_Word p_type;   /* 段类型 */
        Elf32_Off p_offset;  /* 段在文件中的偏移 */
        Elf32_Addr p_vaddr;  /* 段虚拟地址 */
        Elf32_Addr p_paddr;  /* 段物理地址 */
        Elf32_Word p_filesz; /* 段在文件中的大小 */
        Elf32_Word p_memsz;  /* 段在内存中的大小 */
        Elf32_Word p_flags;  /* 段标志 */
        Elf32_Word p_align;  /* 段对齐 */
} Elf32_Phdr;

/* p_type（段类型）的合法值 */

#define PT_NULL 0            /* 未使用的程序头表项 */
#define PT_LOAD 1            /* 可加载程序段 */
#define PT_DYNAMIC 2         /* 动态链接信息 */
#define PT_INTERP 3          /* 程序解释器 */
#define PT_NOTE 4            /* 辅助信息 */
#define PT_SHLIB 5           /* 保留 */
#define PT_PHDR 6            /* 程序头表自身的入口 */
#define PT_NUM 7             /* 已定义类型数量 */
#define PT_LOOS 0x60000000   /* OS专用范围起始 */
#define PT_HIOS 0x6fffffff   /* OS专用范围结束 */
#define PT_LOPROC 0x70000000 /* 处理器专用范围起始 */
#define PT_HIPROC 0x7fffffff /* 处理器专用范围结束 */

/* p_flags（段标志）的合法值 */

#define PF_X (1 << 0)          /* 可执行段 */
#define PF_W (1 << 1)          /* 可写段 */
#define PF_R (1 << 2)          /* 可读段 */
#define PF_MASKPROC 0xf0000000 /* 处理器专用 */

int readelf(u_char *binary, int size);

#endif /* kerelf.h */
