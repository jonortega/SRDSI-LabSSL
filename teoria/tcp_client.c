/***********************************************/
/*** tcp_client.c                           ***/
/**********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>

#define MSG "Hello, world????"

#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(-1);  }


/*--- main ----------------*/
int main(int count, char *strings[])
{   char buf[1024];
    struct hostent *host;
    struct sockaddr_in addr;
    int bytes, err, sd;
    char *hostname, *portnum;

    if ( count != 3 )
    {
        printf("usage: %s <hostname> <portnum>\n", strings[0]);
        exit(0);
    }
    hostname = strings[1];
    portnum = strings[2];

    host = gethostbyname(hostname); 
    CHK_ERR((long)host, hostname);

    sd = socket(PF_INET, SOCK_STREAM, 0); 	/* Create socket */
    CHK_ERR(sd, "NO se ha establecido el socket");

    bzero(&addr, sizeof(addr));			    /* Set server address */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(portnum));
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    err = connect(sd, (struct sockaddr *)&addr, sizeof(addr)); 	/* Connect to server */
    CHK_ERR(err, "Conexión no aceptada");

    err = write(sd, MSG, strlen(MSG));		/* Send message */
    CHK_ERR(err, "Error de escritura");
	
    bytes = read(sd, buf, sizeof(buf));		/* Get reply */
    CHK_ERR(bytes, "Error de lectura");
    buf[bytes] = '\0';
    printf("Received: \"%s\"\n", buf);

    close(sd);						        /* Close socket */
    
    exit(0);
}


