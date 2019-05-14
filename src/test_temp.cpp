#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <limits.h>
#include <iostream>
std::string get_file_name(const int fd)
{
    if (0 >= fd)
    {
        return std::string();
    }

    char buf[1024] = {'\0'};
    char file_path[PATH_MAX] = {'0'}; // PATH_MAX in limits.h
    snprintf(buf, sizeof(buf), "/proc/self/fd/%d", fd);
    if (readlink(buf, file_path, sizeof(file_path) - 1) != -1)
    {
        return std::string(file_path);
    }

    return std::string();
}

int main()
{
    int fd =open("server.cpp",O_RDONLY);
    printf("kkkkkk\n");
    std::string c = get_file_name(fd);
    std::cout << c << std::endl;
    close(fd);
}