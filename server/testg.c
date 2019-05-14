#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
struct msgmbuf
{
    int mtype;
    char mtext[1024];
};

int main()
{
    int ret = -1;
    int msg_flags, msg_id;
    key_t key;
    struct msgmbuf msg_mbuf;
    key = 1024;
    msg_flags = IPC_CREAT;
    while (1)
    {
        msg_id = msgget(key, msg_flags | 0666);
        if (-1 == msg_id)
        {
            printf("create message failed!\n");
            return 0;
        }

        ret = msgrcv(msg_id, &msg_mbuf, sizeof(struct msgmbuf), 0, 0);
        if (-1 == ret)
        {
            printf("receive message failed!\n");
        }
        else
        {
            printf("receive message successful!message[%s]\n", msg_mbuf.mtext);
        }
    }
    return 0;
}