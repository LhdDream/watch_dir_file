
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    FILE *fp = NULL;

    int i = 20;

    if ((fp = fopen("main.hpp", "r+b")) == NULL) //打开文件

        printf("file open error!\n");

    if (flock(fp->_fileno, LOCK_EX) != 0) //给该文件加锁

        printf("file lock by others\n");

    while (1) //进入循环,加锁时间为20秒,打印倒计时

    {

        printf("%d\n", i--);

        sleep(1);

        if (i == 0)

            break;
    }

    fclose(fp); //20秒后退出,关闭文件
    sleep(30);
    flock(fp->_fileno, LOCK_UN); //文件解锁

    return 0;
}
