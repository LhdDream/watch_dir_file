#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SHM_NAME "/memmap"
#define SHM_NAME_SEM "/memmap_sem"

char inotify_name[1024];
int per_flag = 0;
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

typedef int (*orig_open_f_type)(const char *pathname, int flags,...);
typedef int (*orig_close_f_type)(int fd);
typedef int (*orig_read_f_type)(int fildes, void *buf, size_t nbyte);
typedef int (*orig_readlink)(const char *restrict path, char *restrict buf,size_t bufsize);
typedef int (*orig_f_ftruncate)(int fd, off_t length);

int writen(int fd, const void *vptr, int n);
int readn(int fd, void *vptr, int n);
void set_map(const char * pathname)
{
    int fd;
    sem_t *sem;

    mode_t old_umask = umask(0);
    fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    sem = sem_open(SHM_NAME_SEM, O_RDWR | O_CREAT, 0666, 0);

    umask(old_umask);
    if (fd < 0 || sem == SEM_FAILED)
    {
        printf("shm_open or sem_open failed...\n");
        printf("%s",strerror(errno));
        return ;
    }
    orig_f_ftruncate orig_ftruncate;
    orig_ftruncate = (orig_f_ftruncate)dlsym(RTLD_NEXT, "ftruncate");
    int ftr_fd = orig_ftruncate(fd , 1024 * 16);

    if(ftr_fd == -1)
    {
        printf("%s\n",strerror(errno));
    }
    char *memPtr;
    memPtr = (char *)mmap(NULL,1024*16, PROT_READ | PROT_WRITE, MAP_SHARED , fd, 0);
    if(memPtr == (char *)-1)
    {
        printf("%s\n",strerror(errno));
    }
    printf("5\n");
    printf("6\n");
    orig_open_f_type orig_open;
    orig_open = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
    
    char buf[1024];
    int test_fd = orig_open(pathname, O_RDONLY);
    printf("6\n");
    unsigned long number = get_file_size(pathname);
    unsigned long numbern = 0;
    printf("7\n");
    while (numbern != number)
    {
        memset(buf, '\0', sizeof(buf));
        numbern += readn(test_fd, buf, sizeof(buf));
        printf("%s\n",buf);
        if(memmove(memPtr, buf, strlen(buf)) == (void *)-1)
        {
            printf("%s\n",strerror(errno));
        }
        memPtr += numbern;
    }
    printf("8\n");
    sem_post(sem);
    sem_close(sem);
}
void send_link(const char * pathname)
{
    int msg_id, msg_flags;
    struct msqid_ds msg_info;
    struct msgmbuf msg_mbuf;
    key_t key = 1025;
    msg_flags = IPC_CREAT;
    strcpy(msg_mbuf.mtext,pathname);
    msg_id = msgget(key, msg_flags | 0666);
    if (-1 == msg_id)
    {
        return ;
    }
    msg_mbuf.mtype = get_file_size(pathname);
    // 
    msgsnd(msg_id, &msg_mbuf, sizeof(struct msgmbuf), IPC_NOWAIT);

    memset(msg_mbuf.mtext, 0, strlen(msg_mbuf.mtext));
}
// int open(const char *pathname, int flags, ...)
// {
//     /* Some evil injected code goes here. */
//     int res = 0;
//     char resolved_path[100];
//     va_list ap; //可变参数列表
//     va_start(ap, flags);
//     mode_t third_agrs = va_arg(ap, mode_t);
//     if (third_agrs >= 0 && third_agrs <= 0777)
//     {
//        per_flag = 1;
//     }
//     va_end(ap);
//     realpath(pathname,resolved_path);
//     orig_open_f_type orig_open;
//     orig_open = (orig_open_f_type)dlsym(RTLD_NEXT, "open");
//     // printf("resolved_path %s\n",resolved_path);
//     if (strncmp("/home/kiosk/TCP_test/example/inotify/testd" ,resolved_path,42) == 0)
//     {
//       if (per_flag == 0)
//         res = orig_open(resolved_path, flags);
//       else
//         res = orig_open(resolved_path, flags, third_agrs);
//       if (res > 0) 
//       {
//         char temp_buf[1024] = {'\0'};
//         char file_path[1024] = {'0'}; // PATH_MAX in limits.h
//         snprintf(temp_buf, sizeof(temp_buf), "/proc/self/fd/%d", res);
//         orig_readlink orig_read_link;
//         orig_read_link = (orig_readlink)dlsym(RTLD_NEXT, "readlink");
//         orig_read_link(temp_buf, file_path, sizeof(file_path) - 1);
//         send_link(file_path);
//         set_map(resolved_path);
//         // orig_close_f_type orig_close;
//         // orig_close = (orig_close_f_type)dlsym(RTLD_NEXT, "close");
//         // orig_close(res);
//         // FILE *fp = fopen(pathname, "w");
//         // char buf[20];
//         // strcpy(buf, "It is a secret");
//         // strcat(buf, "\0");
//         // fwrite(buf, strlen(buf), 1, fp);
//         // fclose(fp);
//       }
//     }
//     else
//     {
//         if (per_flag == 0)
//             res = orig_open(resolved_path, flags);
//         else
//             res = orig_open(resolved_path, flags, third_agrs);
//     }
// }

int close(int fd)
{
    char temp_buf[1024] = {'\0'};
    char file_path[1024] = {'0'}; // PATH_MAX in limits.h
    snprintf(temp_buf, sizeof(temp_buf), "/proc/self/fd/%d", fd);
    orig_readlink orig_read_link;
    orig_read_link = (orig_readlink)dlsym(RTLD_NEXT, "readlink");
    orig_read_link(temp_buf, file_path, sizeof(file_path) - 1);
    send_link(file_path);
   // printf("file_path     %s\n",file_path);
    orig_close_f_type orig_close;
    orig_close = (orig_close_f_type)dlsym(RTLD_NEXT, "close");
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

    while (nleft > 0)
    {
        nread = orig_read(fd, ptr, nleft);
        if (nread < 0)
        {
            if (errno == EINTR)
                nread = 0; 
            else
                return -1;
        }
        else if (nread == 0)
        {
            break; 
        }

        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

