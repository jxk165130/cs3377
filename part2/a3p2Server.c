#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc , char *argv[])
{
   struct sockaddr_in serv_addr = { 0 };
    int sock;
    int acc_sock;
    char buf[BUFMAX + 1];
    char command[BUFMAX + 1];
    ssize_t nread;
    int nread2;
    FILE *pin;
    FILE *xfin;
    char *cp;
    struct sockaddr_in cli_addr = { 0 };
    FILE *fp;
    int exitflg;
    fp = fopen("a3p2Server.log","a");

    opt_debug = ! opt_debug;

    dbgprt("[+] Starting\n");
    fprintf(fp,"[+] Starting\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    listen(sock, 128);

    int pid;
    while (1) {
        socklen_t cli_addrlen = sizeof(cli_addr);

        dbgprt("[+] Waiting for connection\n");


        acc_sock = accept(sock,(struct sockaddr *)&cli_addr,&cli_addrlen);

// handle multiple client using fork
        pid = fork();

        if (pid < 0){
            error("Error in new process creation");
        }

        if (pid == 0)
        {
        close(sock);

        dbgprt("[+] Connected\n");
        fprintf(fp,"[+] Connected\n");

        xfin = fdopen(acc_sock,"r");

        while (1) {
            dbgprt("[+] Waiting for command\n");
            fprintf(fp,"[+] Waiting for command\n\n");

            cp = fgets(buf,BUFMAX,xfin);
            if (cp == NULL)
                break;

            cp = strchr(buf,'\n');
            if (cp != NULL)
                *cp = 0;

            printf("My process ID : %d\n", getpid());
            dbgprt("[+] Command '%s'\n",buf);


            fprintf(fp,"My process ID : %d\n", getpid());
            fprintf(fp,"[+] Command '%s'\n\n",buf);

            exitflg = (strcmp(buf,"exit\n") == 0);
            if (exitflg)
            {
                fclose(fp);
                break;

                }
            pin = popen(buf, "r");
            while (1) {
                cp = fgets(command, BUFMAX, pin);
                if (cp == NULL)
                    break;
                nread = strlen(command);
                write(acc_sock, command, nread);
                fwrite(command,acc_sock,nread,fp);

            }
            pclose(pin);

            command[0] = FLAG;
            write(acc_sock,command,1);

        }

        dbgprt("[+] Disconnect\n");
        fclose(xfin);
        close(acc_sock);


        }
        else {
            close(acc_sock);
        }
    }
}

// packet_encode -- encode packet
// RETURNS: (outlen << 1)
int
packet_encode(void *dst,const void *src,int srclen)
{
    const byte *sp = src;
    byte *dp = dst;
    const byte *ep;
    byte chr;
    int dstlen;

    // encode packet in manner similar to PPP (point-to-point) protocol does
    // over RS-232 line

    ep = sp + srclen;
    for (;  sp < ep;  ++sp) {
        chr = *sp;

        switch (chr) {
        case FLAG:
            *dp++ = ESC;
            *dp++ = ESC_FLAG;
            break;

        case ESC:
            *dp++ = ESC;
            *dp++ = ESC_ESC;
            break;

        default:
            *dp++ = chr;
            break;
        }
    }

    dstlen = dp - (byte *) dst;
    dstlen <<= 1;

    return dstlen;
}

// packet_decode -- decode packet
// RETURNS: (outlen << 1) | flag
int
packet_decode(void *dst,const void *src,int srclen)
{
    const byte *sp = src;
    byte *dp = dst;
    const byte *ep;
    byte chr;
    int flag;
    int dstlen;

    // decode packet in manner similar to PPP (point-to-point) protocol does
    // over RS-232 line

    ep = sp + srclen;
    flag = 0;
    while (sp < ep) {
        chr = *sp++;

        flag = (chr == FLAG);
        if (flag)
            break;

        switch (chr) {
        case ESC:
            chr = *sp++;

            switch (chr) {
            case ESC_FLAG:
                *dp++ = FLAG;
                break;
            case ESC_ESC:
                *dp++ = ESC;
                break;
            }
            break;

        default:
            *dp++ = chr;
            break;
        }
    }

    dstlen = dp - (byte *) dst;
    dstlen <<= 1;

    if (flag)
        dstlen |= 0x01;

    return dstlen;
}

