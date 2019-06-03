#include"get_message.h"
#include <errno.h>
#include <stdio.h>
int main()
{
    char ip[32];
    
    bzero(ip,sizeof(ip));
    int port = 0;
    get_ip_addr(ip,&port);
    for(int i = 0 ; i<= strlen(ip) + 5;i++)
    {
        if(ip[i] == '\r')
            printf("%d\n",i);
    }
    return 0;
}