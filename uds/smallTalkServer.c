#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define QLEN 10
#define SOC_FILE "/var/fpwork1/chrhong/foo.socket"

int main()
{
    int fd, size, clifd, len;
    struct sockaddr_un sa_un;
    struct stat statbuf = {};

    memset(&sa_un,0,sizeof(sa_un));

    sa_un.sun_family = AF_UNIX;
    strcpy(sa_un.sun_path, SOC_FILE);

    if ( (fd = socket(AF_UNIX, SOCK_STREAM,0)) < 0 )
    {
        perror("bind error");
        exit(1);
    }

    size = offsetof(struct sockaddr_un, sun_path) + strlen(sa_un.sun_path);

    if (bind(fd, (struct sockaddr *)&sa_un, size) < 0) 
    {
        perror("bind error");
        exit(1);
    }

    if (listen(fd, QLEN) < 0)
    {
        perror("listen error");
        return -2;
    }

    len = sizeof(sa_un);
    if ((clifd = accept(fd, (struct sockaddr *)&sa_un, &len)) < 0)
    {
        perror("accept error");
        return(-1);
    }

    len -= offsetof(struct sockaddr_un, sun_path);  /* len of pathname */
    sa_un.sun_path[len] = 0;                          /* null terminate */

    if (stat(sa_un.sun_path, &statbuf) < 0) {
        perror("stat error");
        return(-1);
    }

    if (S_ISSOCK(statbuf.st_mode) == 0) {
        perror("S_ISSOCK error");
        return(-1);
    }

    unlink(sa_un.sun_path);        /* we're done with pathname now */

    char szText[] = "Connected with a client!";
    send(clifd, szText, strlen(szText), 0);   
    printf("Server thread start!\n");
    //use clifd to communication
    char buff[256]; 

    for(;;)
    {
        int nRecv = recv(clifd, buff, 256, 0);   
        if(nRecv > 0)
        {   
            buff[nRecv] = '\0';
            printf("Server Received:\n");
            printf("    %s\n", buff);
        }
        else
        {
            printf("Client exit!\n");
            break;
        }
        
        printf("Response:\n");

        int i = 0;
        if ( (i = scanf("%[^\n]", buff)) > 0)
        {
            send(clifd, buff, sizeof(buff), 0);
        }
        else
        {
            printf("No input %d\n",i);
        }

        setbuf(stdin, NULL);
    }

    close(clifd);
    close(fd);

    unlink(SOC_FILE);

    printf("UNIX domain socket over\n");
    return 0;
}