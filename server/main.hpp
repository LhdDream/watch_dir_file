#ifndef _MAIN_HPP
#define _MAIN_HPP
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

struct msgmbuf
{
    int mtype;
    char mtext[1024];
};
int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
//把文件描述副的fd注册到epoll指示的内核注册表中
void addfd(int epollfd,int fd, bool enable)
{
    epoll_event event;
    event.data.fd =fd;
    event.events = EPOLLIN;
    if(enable)
    {
        event.events |= EPOLLIN;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}
//文件描述符设置为非阻塞的
/* code */
#endif //_MAIN_HPP
