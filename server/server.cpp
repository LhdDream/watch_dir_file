
#include <thread>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "main.hpp"
//将文件描述符的fd的epoll注册到内核事件表中，看是不是采用ET模式
using namespace std;

void work(int epollfd,int sock)
{
    char buf[1024];
    char num[4];
    char temp_buf[1024];
    char dir_num[512];
    string c;
    while(1)
    {
        memset(buf,0,strlen(buf));
        int ret = recv(sock,num,1,0);
        int number = atoi(num);
       // cout << "number    " << number << endl;
        ret = recv(sock,buf,number,0);
        if(ret == 0)
        {
            close(sock);
            break;
        }
        else if(ret < 0)
        {
            if(errno == EAGAIN)
            {
                break;
            }
        }
         else
        {
            cout << buf << endl;
        }
        if(strcmp(buf,"begin") == 0)
        {
            while(1)
            {
                struct msgmbuf temp;
                ret = recv(sock, &temp, sizeof(struct msgmbuf), 0);
                if (ret > 0)
                {
                    if(strcmp(temp.mtext,"@@@@")!= 0 )
                    { 
                      if(temp.mtype == 12)
                      {
                         strcpy(temp_buf,temp.mtext);
                      }
                      if(temp.mtype == 10)
                      {
                          c+= temp.mtext;
                      }
                    }
                    if(strcmp(temp.mtext,"@@@@") == 0 && (temp.mtype == 13))
                    {
                        break;
                    }
                }
            }
            ///根据套接字来进行还原
           sprintf(dir_num,"%d",sock);
           mkdir(dir_num,0755);
           for(int j = 0 ; j< strlen(temp_buf);j++)
           {
               if(temp_buf[j] == '/')
               {
                   temp_buf[j] = '@';
               }
           }
           strcat(dir_num,temp_buf);
           ofstream out;
           out.open(dir_num,ios::app);
           out << c;
           out.close();
        }
        
    }
}
int main(int argc , char * argv[])
{
    const char * ip = "127.0.0.1";
    int port  = 8080 ;

    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET,SOCK_STREAM,0);
    int reuse = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    int ret = bind(sock,(struct sockaddr *)&address,sizeof(address));
    
    ret = listen(sock,5);

    epoll_event event[200];
    int epollfd = epoll_create(5);
    addfd(epollfd,sock,false);
    while(1)
    {
        ret = epoll_wait(epollfd,event,200,-1);
        for(int i = 0 ;i < ret ; i++)
        {
            int sockfd = event[i].data.fd;
            if((event[i].events & EPOLLERR) ||(event[i].events & EPOLLHUP))
            {
                close(event[i].data.fd);
                continue;
            }
            else if( sockfd == sock)
            {
               struct sockaddr_in client_address;
               socklen_t client_length = sizeof(client_address);
               int connfd = accept(sock,(struct sockaddr *)&client_address,&client_length);
               addfd(epollfd,connfd,false);
            }
            else if(event[i].events & EPOLLIN)
            {
                thread t1(work,epollfd,sockfd);
                t1.join();
            }
            else  if(event[i].events & EPOLLOUT)
            {

            }
        }
    }
    return 0;
}