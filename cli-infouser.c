#include <string.h>   // NEcesario para usar bcopy
// Ficheros necesarios para el manejo de sockets, direcciones, etc.
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>  // Para atol
#include <stdio.h>  //  Manejo de ficheros
#include <fcntl.h>  //  PAra poder hacer open
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1);  }

#define PUERTO (IPPORT_RESERVED + 40)  // Puerto tomado al azar
#define TAMBUF 1024 			// Maxima lectura/escritura

int main(int count, char *strings[]){
   int sock;
   int  lon, total, err;
   long tam_fich;
   int bytes_rec, k, i;
   struct sockaddr_in serv;
   struct hostent *host;
   FILE *fd;
   char buf[TAMBUF];
   
   if ( count != 3 ){
        printf("Uso: %s <hostip> <filename>\n", strings[0]);
        exit(1);
    	}
   
   host = gethostbyname(strings[1]);

   sock = socket(PF_INET, SOCK_STREAM, 0);
   CHK_ERR(sock, "No crea socket");
   
   serv.sin_family = AF_INET;
   serv.sin_port = htons(PUERTO);
   serv.sin_addr.s_addr = *(long *)(host->h_addr);
   
   err = connect(sock, (struct sockaddr *)&serv, sizeof serv);
   CHK_ERR(err, "No acepta conexion");
   
   bzero(&buf, TAMBUF );
   bytes_rec = read(sock, buf, sizeof(tam_fich)-2);  //bytes_rec = read(sock, buf, 3+sizeof(tam_fich)-1);
   CHK_ERR(bytes_rec, "Error de lectura");
   
   if ( strncmp(buf, "OK:", 3) ){
      printf("Encontrado error en comando %s!!!\n",buf);
	  close(sock);
      exit(1);
   }
   tam_fich = atol(&buf[3]);

   // Recibir fichero del servidor
   if ((fd = fopen(strings[2], "w")) == NULL){
		perror("Abriendo fichero local");
		exit(1);
   }

	bytes_rec = 0;
	while(bytes_rec < tam_fich){ 
		k = read (sock, buf, TAMBUF);

		if ((k <= 0)|| !strncmp(buf, "KO:" ,3)){
			perror("Leyendo respuesta. Abandono");
			close(sock);
			fclose(fd);
			exit(1);
		}
		for (i=0; i<k; i++)
			if (putc(buf[i],fd)<0){
				perror("Escribiendo en el fichero local. Abandono");
				close(sock);
				fclose(fd);
				exit(1);
			}
		bytes_rec += k;
	}

	fclose(fd);
   
   close(sock);
   exit(0);
}

