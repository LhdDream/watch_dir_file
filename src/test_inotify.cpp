#include <stdio.h>
#include "main.hpp"
#include "recv_img.hpp"
#include <thread>
#include <string>
#include <limits.h>
Inotify main_important;
// struct filename_fd_desc FileArray[main_important.array_length];
// struct epoll_event Epollarray[main_important.epoll_number] ;
int array_index = 0;
const char *base_dir;
int temp_flag = 0;
void send_img(int sockfd)
{
    char buffer[20];
    char num[4];
    strcpy(buffer, "begin");
    sprintf(num, "%d", strlen(buffer));
    int k = send(sockfd, num, strlen(num), 0);
    k = send(sockfd, buffer, strlen(buffer), 0);
    int ret = -1;
    int msg_flags, msg_id;
    key_t key;
    struct msgmbuf msg_mbuf;
    int flag = 0;
    key = 1024;
    msg_flags = IPC_CREAT;
    while (1)
    {
        msg_id = msgget(key, msg_flags | 0666);
        if (-1 == msg_id)
        {
            printf("create message failed!\n");
          
        }

        ret = msgrcv(msg_id, &msg_mbuf, sizeof(struct msgmbuf), 0, 0);
        if (-1 == ret)
        {
            printf("receive message failed!\n");
        }
        else
        {
            printf("receive message successful!message[%s]   \n", msg_mbuf.mtext);
            if(strcmp(msg_mbuf.mtext,"@@@@") == 0)
            {
                flag++;
                
            }
            if (flag == 1)
            {
                msg_mbuf.mtype = 12;
            }
            else if (flag == 2)
            {
                msg_mbuf.mtype = 13;
            }
            k = send(sockfd, &msg_mbuf, sizeof(struct msgmbuf), 0);
            if( k < 0 )
            {
                return ;
            }
        }
        if(strcmp(msg_mbuf.mtext, "@@@@") == 0 && flag == 2)
        {
               break;
        }
    }
}

static int handle_events(int epollfd, int fd, int argc, char *argv[], struct filename_fd_desc *FileArray,int sockfd)
{
    char buffer[1024];
    char num[4];
    
    char buf[2048];
    int i,k;
    ssize_t len;
    char * ptr;
    const struct inotify_event *event;
    memset(buf,0,sizeof(buf));

    len = read(fd,buf,sizeof(buf));

    for(ptr = buf ; ptr < buf + len;ptr += sizeof(struct inotify_event) + event->len)
    {
        event = (const struct inotify_event *) ptr;
        memset(buffer, 0, 1024);
        memset(num,0,4);
        if(event->len)
        {
            if(event->mask & IN_OPEN)
            {
                cout << "OPEN file    " << event->name << endl;
                strcpy(buffer,"open file");
                //strcat(buffer,event->name);
            }
            if(event->mask & IN_CLOSE_NOWRITE)
            {
                cout << "CLOSE_NOWRITE   " << event->name << endl;
                // strcpy(buffer, "close_NOWRITE file");
                // strcat(buffer, event->name);
            }
            if(event->mask & IN_CLOSE_WRITE)
            {
                cout << "IN_CLOSE_WRITE   " <<event->name <<endl;
                //strcpy(buffer, "INCLOSE_NOWRITE file");
                // strcat(buffer, event->name);
            }
            if (event->mask & IN_CREATE)
            { /* 如果是创建文件则打印文件名 */
                cout<< "ggg " << endl;
                sprintf(FileArray[array_index].name,"%s",event->name);
                sprintf(FileArray[array_index].base_name,"%s%s",base_dir,event->name);
                cout << "ppppp" << endl;
                int temp_fd = open(FileArray[array_index].base_name,O_RDWR);
                if(temp_fd == -1)
                {
                    return -1;
                }
               // cout << "create file" << endl;
                FileArray[array_index].fd =temp_fd;
                main_important.addfd(epollfd,temp_fd,false);
                array_index++;
                cout << "add file   " << event->name<< endl;
            }
            if (event->mask & IN_DELETE)
            { /* 如果是删除文件也是打印文件名 */
                for (i = 0; i < main_important.array_length; i++)
                {
                    if (!strcmp(FileArray[i].name, event->name))
                    {
                        main_important.rm_fd(epollfd, FileArray[i].fd);
                        FileArray[i].fd = 0;
                        memset(FileArray[i].name, 0, sizeof(FileArray[i].name));
                        memset(FileArray[i].base_name, 0, sizeof(FileArray[i].base_name));
                        printf("delete file to epoll %s\n", event->name);
                        break;
                    }
                }
                    cout << "delete file   " << event->name << endl;
            }
        }
        if(strcmp(buffer,"open file") == 0)
        {
          //strcat(buffer,event->name);
          sprintf(num, "%d", strlen(buffer));
          k = send(sockfd, num, strlen(num), 0);
          if (k < 0) {
            cout << "error 1" << endl;
            return 1;
            }
            k = send(sockfd, buffer, strlen(buffer), 0);
            if (k < 0)
            {
                cout << "error 2" << endl;
                return 1;
            }
           if(temp_flag == 0)
           {
               send_img(sockfd);
               temp_flag++;
           }
        }
    }
    
}

int main(int argc , char** argv)
{

    struct filename_fd_desc FileArray[main_important.array_length];
    struct epoll_event Epollarray[main_important.epoll_number];
    const char * ip = "127.0.0.1";
    int port = 8080;

    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    
    int fd,i,epollfd,wd;
    char readbuf[1024];
    epollfd =epoll_create(8);
    
    fd = inotify_init();
    base_dir = argv[1];
    if(argc < 2)
    {
        cout << "error argc too simple" << endl;
        return 1;
    }

    for(i = 1 ; i < argc ;i++)
    {
        wd = inotify_add_watch(fd,argv[i],IN_OPEN |IN_CLOSE |IN_CREATE | IN_DELETE);
    }
    main_important.addfd(epollfd,fd,false);

    if(connect(sockfd,(struct sockaddr *)&server_address,sizeof(server_address)) < 0)
    {
        close(sockfd);
        return 1;
    }

    

    while(1)
    {
        int ret = epoll_wait(epollfd,Epollarray,32,-1);
        // cout << ret << endl;
        for( i = 0; i<ret ;i++)
        {
            if(Epollarray[i].data.fd == fd)
            {
                if (-1 == (handle_events(epollfd,fd,argc,argv,FileArray,sockfd)))
                {
                    return -1;
                }
                
            }
            else
            {
               int  readlen = read(Epollarray[i].data.fd,readbuf,1024);
               readbuf[readlen] = '\0';
               //cout << readbuf[readlen] << endl;
            }
        }
    }
    close(fd);
    return 0;
}