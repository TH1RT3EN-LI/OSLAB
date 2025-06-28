#include <asm/regdef.h>      // 寄存器定义头文件
#include <asm/cp0regdef.h>   // CP0 寄存器定义头文件
#include <asm/asm.h>         // 汇编辅助宏定义头文件

			.section .data.stk
KERNEL_STACK:
			.space 0x8000    // 分配 32KB 的内核栈空间

			.text
LEAF(_start) /*LEAF is defined in asm.h and LEAF functions don't call other functions*/                                       
	
	.set	mips2      /*.set is used to instruct how the assembler works and control the order of instructions 这块是允许使用mips指令集 */
	.set	reorder    /* 允许指令重排序 */

	/* 关闭中断 */
	mtc0	zero, CP0_STATUS

	/* 关闭监视异常 */
	mtc0    zero, CP0_WATCHLO
	mtc0    zero, CP0_WATCHHI

	/* 关闭内核模式下的缓存 */
	mfc0	t0, CP0_CONFIG
	and	t0, ~0x7
	ori	t0, 0x2
	mtc0	t0, CP0_CONFIG
	

// My work:
li sp, 0x80400000    // 设置堆栈指针 sp
jal main             // 跳转调用 main 函数
nop                  // 延迟槽指令，让 main 函数执行完毕后继续执行下一条指令
			
loop:
	j	loop         // 死循环,可能防止程序跑飞？
	nop
END(_start)       
