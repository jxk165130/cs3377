#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<time.h>

typedef unsigned char byte;

#define BUFMAX          1024
int port = 1234;
int opt_svr;
int opt_debug;

#define FLAG            0x10
#define ESC             0x11
#define ESC_FLAG        0x01
#define ESC_ESC         0x02

#define dbgprt(_fmt...) \
    do { \
        if (opt_debug) \
            printf(_fmt); \
    } while (0)


int main(int argc , char *argv[])
{
   int sock;
    struct sockaddr_in serv = { 0 };
    char *cp;
    char buf[BUFMAX + 1];
    int nread;
    int flag;
    int nread2;
    int exitflg;
    time_t curtime;
    struct tm *loc_time;
    FILE *fp;
    fp = fopen("a3p2Client2.log","a");


    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *) &serv, sizeof(serv));


    while (1) {

        cp = fgets(buf,BUFMAX,stdin);
        if (cp == NULL)
            break;

        exitflg = (strcmp(buf,"exit\n") == 0);

        // send the command
        nread = strlen(buf);

        write(sock, buf, nread);

        if (exitflg)
        {
            fwrite(buf,1,nread2,fp);
            fclose(fp);

            break;
        }



        curtime = time (NULL);
        loc_time = localtime (&curtime);
        nread2=strftime(buf,140,"Time is %I:%M %p.\n",loc_time);
        fwrite(buf,1,nread2,fp);

        fputs (buf, stdout);
        printf("------------------------------------------\n");
        printf("program will sleep for 3second after command execute \n");
        while (1) {

            dbgprt("client: PREREAD\n");
            nread = read(sock, buf, 1024);
            dbgprt("client: POSTREAD nread=%d\n",nread);
            if (nread <= 0)
                break;

            cp = memchr(buf,FLAG,nread);
            flag = (cp != NULL);
            if (flag)
                nread = cp - buf;
            printf("------------------------------------------\n");

            write(1,buf,nread);
            fwrite(buf,1,nread,fp);

            sleep(2);

          if (flag)

            break;

        }
    }


    close(sock);

    return 0;
}

