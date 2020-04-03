#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#define CLI_PATH    "/var/fpwork1/chrhong/"      /* +5 for pid = 14 chars */

int main()
{
    const char *name = "/var/fpwork1/chrhong/foo.socket";
    int fd, len, err, rval;
    struct sockaddr_un un;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(-1);

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    sprintf(un.sun_path, "%s%05d", CLI_PATH, getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

    unlink(un.sun_path);
    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
        return -2;
    }

    if (chmod(un.sun_path, S_IRWXU)<0)
    {
        return -3;
    }

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

    if (connect(fd, (struct sockaddr *)&un, len) < 0) {
        printf("connect error");
        return -1;
    }

    char buff[256];
    for(;;)
    {
        int nRecv = recv(fd, buff, 256, 0);
        if(nRecv > 0)   
        {   
            buff[nRecv] = '\0';
            printf("Client Received:\n");
            printf("    %s\n", buff);
        }else
        {
            printf("Server exit!\n");
            break;
        }

        printf("Response:\n");

        int i = 0;
        if ( (i = scanf("%[^\n]", buff)) > 0)
        {
            send(fd, buff, sizeof(buff), 0);
        }
        else
        {
            printf("No input %d\n",i);
        }
        //fflush(stdin) is not standard c API, it is just an extension API which is not valid under Linux
        setbuf(stdin, NULL);
    }

    close(fd);
    unlink(name);

    return 0;
}