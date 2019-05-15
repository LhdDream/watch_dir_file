#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd =open("server.cpp",O_RDONLY);
    printf("kkkkkk\n");
    close(fd);
}