#include <asm/asm.h>
#include <pmap.h>
#include <env.h>
#include <printf.h>
#include <kclock.h>
#include <trap.h>

// MIPS 初始化函数
void mips_init()
{
	printf("init.c:\tmips_init() is called\n"); 

	// 为了你的学位，不要删除这些。
	//------------|
	#ifdef FTEST
	FTEST(); // 如果定义了FTEST，调用FTEST函数
	#endif

	#ifdef PTEST
	ENV_CREATE(PTEST); // 如果定义了PTEST，创建PTEST环境
	#endif
	//-----------|
	panic("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"); // 抛出异常，终止程序
}
