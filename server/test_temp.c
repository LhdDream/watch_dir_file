#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    FILE *fp =  fopen("5/@homed", "w");
    printf("fffffffffffffffff\n");
    fclose(fp);
}