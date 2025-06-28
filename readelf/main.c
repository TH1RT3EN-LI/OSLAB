#include <stdio.h>
#include <stdlib.h>

extern int readelf(u_char* binary, int size);
/*
        overview: input a elf format file name from control line, call the readelf function
                  to parse it.
        params:
                argc: the number of parameters
                argv: array of parameters, argv[1] shuold be the file name.
.
*/
int main(int argc,char *argv[])
{
        FILE* fp;
        int fsize;
        unsigned char *p;

        // 检查参数数量是否足够
        if(argc < 2)
        {
                printf("Please input the filename.\n");
                return 0;
        }
        // 尝试以二进制方式打开文件
        if((fp = fopen(argv[1],"rb"))==NULL)
        {
                printf("File not found\n");
                return 0;
        }
        // 移动到文件末尾以获取文件大小
        fseek(fp,0L,SEEK_END);
        fsize = ftell(fp);
        // 分配内存存放文件内容
        p = (u_char *)malloc(fsize+1);
        if(p == NULL)
        {
                fclose(fp);
                return 0;
        }
        // 回到文件开头，读取文件内容到内存
        fseek(fp,0L,SEEK_SET);
        fread(p,fsize,1,fp);
        p[fsize] = 0; // 末尾添加结束符

        // 调用 readelf 函数解析 ELF 文件，所以到这一步 p 指向的是一个 ELF 格式的二进制文件内容
        readelf(p,fsize);
        return 0;
}
