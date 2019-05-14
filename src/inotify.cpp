#include "ini.h"
#include "ip.hpp"
#include "main.hpp"
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/types.h>
#define SHM_NAME "/memmap"
#define SHM_NAME_SEM "/memmap_sem"

int array_index = 0;

const char *base_dir;
int temp_flag = 0;

struct msgmbuf {
  int mtype;
  char mtext[1024];
};

Inotify main_important;

void get_shm(int sockfd)
{
    int fd;
    sem_t *sem;
    fd = shm_open(SHM_NAME, O_RDWR, 0);
    sem = sem_open(SHM_NAME_SEM, 0);

    if (fd < 0 || sem == SEM_FAILED)
    {
        cout << "shm_open or sem_open failed...";
        cout << strerror(errno) << endl;
        return ;
    }

    struct stat fileStat;
    fstat(fd, &fileStat);
    char *memPtr;
    char buffer[fileStat.st_size];
    memPtr = (char *)mmap(NULL, fileStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < fileStat.st_size; i++)
    {
        buffer[i] =  memPtr[i];
    }
    cout << buffer << endl;
    sem_wait(sem);
    sem_close(sem);
    shm_unlink(SHM_NAME);
    shm_unlink(SHM_NAME_SEM);
}

void send_img(int sockfd)
{
    int ret = -1;
    int msg_flags, msg_id;
    key_t key;
    struct msgmbuf msg_mbuf;
    int flag = 0;
    key = 1024;
    msg_flags = IPC_CREAT;
    msg_id = msgget(key, msg_flags | 0666);
    if (-1 == msg_id)
    {
        cout << "message failed !" << endl;
    }

    ret = msgrcv(msg_id, &msg_mbuf, sizeof(struct msgmbuf), 0, 0);
    if (-1 == ret)
    {
        cout << "receive message failed!" << endl;
    }
    else
    {
        cout << "receive message  " << msg_mbuf.mtext << endl;
    }
    ret = msgctl(key,IPC_RMID,NULL);
}
void printdir(char *dir, int depth,int fd)
{
    //打开目录指针
    DIR *Dp;
    //文件目录结构体
    struct dirent *enty;
    //详细文件信息结构体
    struct stat statbuf;
    //打开指定的目录，获得目录指针
    if (NULL == (Dp = opendir(dir)))
    {
        fprintf(stderr, "can not open dir:%s\n", dir);
        return;
    }
    //切换到这个目录
    chdir(dir);
    //遍历这个目录下的所有文件
    while (NULL != (enty = readdir(Dp)))
    {
        //通过文件名，得到详细文件信息
        lstat(enty->d_name, &statbuf);
        //判断是不是目录
        if (S_ISDIR(statbuf.st_mode))
        {
            //当前目录和上一目录过滤掉
            if (0 == strcmp(".", enty->d_name) ||
                0 == strcmp("..", enty->d_name))
            {
                continue;
            }
            //输出当前目录名
            int wd = inotify_add_watch(fd, enty->d_name, IN_OPEN | IN_CLOSE | IN_CREATE | IN_DELETE);
            //继续递归调用        printdir(enty->d_name,depth+4);
        }
    }
    //切换到上一及目录
    chdir("..");
    //关闭文件指针
    closedir(Dp);
}

static int handle_events(int epollfd, int fd, int argc, char *argv[], struct filename_fd_desc *FileArray,int sockfd)
{
    int i,k;
    ssize_t len;
    char * ptr;
    char buffer[1024];
    char num[4];
    char buf[2048];
    struct inotify_event *events;
    memset(buf,0,sizeof(buf));

    len = read(fd,buf,sizeof(buf));

    for(ptr = buf ; ptr < buf + len;ptr += sizeof(struct inotify_event) + events->len)
    {
        events = (struct inotify_event *) ptr;
        memset(buffer, '\0', 1024);
        memset(num,'\0',4);
        if(events->len)
        {
            if(events->mask & IN_OPEN)
            {
                cout << "OPEN file    " << events->name << endl;
                strcpy(buffer,"open file");
                //strcat(buffer,event->name);
            }
            if(events->mask & IN_CLOSE_NOWRITE)
            {
                cout << "CLOSE_NOWRITE   " << events->name << endl;
                // strcpy(buffer, "close_NOWRITE file");
                // strcat(buffer, event->name);
            }
            if(events->mask & IN_CLOSE_WRITE)
            {
                cout << "IN_CLOSE_WRITE   " <<events->name <<endl;
                //strcpy(buffer, "INCLOSE_NOWRITE file");
                // strcat(buffer, event->name);
            }
            if (events->mask & IN_CREATE)
            { /* 如果是创建文件则打印文件名 */
                sprintf(FileArray[array_index].name,"%s",events->name);
                sprintf(FileArray[array_index].base_name,"%s%s",base_dir,events->name);
                int temp_fd = open(FileArray[array_index].base_name,O_RDWR);
                if(temp_fd == -1)
                {
                    return -1;
                }
               // cout << "create file" << endl;
                FileArray[array_index].fd =temp_fd;
                main_important.addfd(epollfd,temp_fd,false);
                array_index++;
                cout << "add file   " << events->name<< endl;
            }
            if (events->mask & IN_DELETE)
            { /* 如果是删除文件也是打印文件名 */
                for (i = 0; i < main_important.array_length; i++)
                {
                    if (!strcmp(FileArray[i].name, events->name))
                    {
                        main_important.rm_fd(epollfd, FileArray[i].fd);
                        FileArray[i].fd = 0;
                        memset(FileArray[i].name, 0, sizeof(FileArray[i].name));
                        memset(FileArray[i].base_name, 0, sizeof(FileArray[i].base_name));
                        printf("delete file to epoll %s\n", events->name);
                        break;
                    }
                }
                    cout << "delete file   " << events->name << endl;
            }
        }
        if(strcmp(buffer,"open file") == 0)
        {
            cout << "this is \n";
          //strcat(buffer,event->name);
        //   sprintf(num, "%d", strlen(buffer));
        //   k = send(sockfd, num, strlen(num), 0);
        //   if (k < 0) {
        //     cout << "error 1" << endl;
        //     return 1;
        //     }
        //     k = send(sockfd, buffer, strlen(buffer), 0);
        //     if (k < 0)
        //     {
        //         cout << "error 2" << endl;
        //         return 1;
        //     }
          IP real;
          real.getLocalInfo();
        //   get_shm(sockfd);
          //cout << "open file sockfd" << endl;
          send_img(sockfd);
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
        printdir(argv[1],0,fd);
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
        for( i = 0; i < ret ;i++)
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
            }
        }
    }
    close(fd);
    return 0;
}