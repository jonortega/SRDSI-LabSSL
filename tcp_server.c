/************************************************/
/*** tcp_server.c                             ***/
/************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(-1);}
#define REPLY "Hello, welcome¡¡¡¡¡"

int main(int count, char *strings[])
{       struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int err, bytes, listen_sd, portnum;
		char buf[1024];

		if ( count != 2 )
		{
			printf("Usage: %s <portnum>\n", strings[0]);
			exit(0);
		}
		portnum = atoi(strings[1]);
		
		listen_sd = socket(PF_INET, SOCK_STREAM, 0);	 /* Create server socket */
		CHK_ERR(listen_sd, "No se ha establecido el socket");
		
		bzero(&addr, sizeof(addr)); 			/* Set server address */
		addr.sin_family = AF_INET;
		addr.sin_port = htons(portnum);
		addr.sin_addr.s_addr = INADDR_ANY;
		
		err = bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) ; /* Assign address to socket */
		CHK_ERR(err, "No se puede asignar dirección");
		
		err = listen(listen_sd, 5) ;				/* passive open of listening socket */
		CHK_ERR(err, "No se puede configurar el puerto de escucha");
		
		while (1)
		{
				int client = accept(listen_sd, (struct sockaddr*)&addr, &len);		/* accept connection */
				printf("Connection: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
				
				bytes = read(client, buf, sizeof(buf));	/* get request */
				CHK_ERR(bytes, "Error de lectura");
				
				buf[bytes] = '\0';
				printf("Client msg: \"%s\"\n", buf);
				
				err = write(client, REPLY, strlen(REPLY));	/* send reply */
				CHK_ERR(err, "Error de escritura");
				
				close(client);					/* close connection */
		}
}




