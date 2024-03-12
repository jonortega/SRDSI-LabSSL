#include <string.h>   // NEcesario para usar bcopy
// Ficheros necesarios para el manejo de sockets, direcciones, etc.
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>  // Para fstat
#include <stdlib.h>  // Para atol
#include <stdio.h>  //  Manejo de ficheros
#include <fcntl.h>  //  PAra poder hacer open

#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1);  }

#define PUERTO (IPPORT_RESERVED + 40)  // Puerto tomado al azar
#define TAMBUF 1460 			// Maxima lectura/escritura

void enviar_usuarios(int s){
	int fp;  	// Descriptor del fichero a enviar
	struct stat f;	// Estructura de donde sacaremos el tamaño del fichero
	int i,k, total;
	char c, buf[TAMBUF];
	long int tam;	// tamaño del fichero, a leer de la estructura stat
	
	system("finger > /tmp/file_finger");
	
	if ((fp = open("/tmp/file_finger", O_RDONLY)) < 0 ) {
		sprintf(buf, "KO: Error al abrir el fichero");
		write(s, buf, strlen(buf));
		return;
	}
	
	i = stat("/tmp/file_finger", &f);
	if (i != 0) {
		sprintf(buf, "KO: Error stat");
		write(s, buf, strlen(buf));
		close(fp);
		return;
	}
	
	tam = f.st_size;
	sprintf(buf, "OK:%ld", tam);
	write(s, buf, strlen(buf));

	total = 0;
	while (total < tam){
		if ((i = read(fp, buf, TAMBUF))<0) {
			sprintf(buf, "KO: Error read file");
			write(s, buf, strlen(buf));
			perror("read socket");
			close(fp);
			return;
		}			
		if ((write(s,buf,i))<i) {
			perror("write socket");
			close(fp);
			return;
		}
		total += i;
	}
	
	close(fp);
	return;
}

int main(void){
	int s, c;	// sockets de conexion y dialogo
	int err, n;		// variable auxiliar
	struct sockaddr_in dir;
	char buf[TAMBUF];	// buffer para lectura/excritura en el socket. 
	
	s = socket(PF_INET, SOCK_STREAM, 0);
	CHK_ERR(s, "Apertura de socket");

	dir.sin_family = AF_INET;
	dir.sin_port = htons(PUERTO);
	dir.sin_addr.s_addr = htonl(INADDR_ANY); 
	
	err = bind (s, (struct sockaddr *)&dir, sizeof(dir));
	CHK_ERR(err, "Bind");
	
	err = listen (s, 5);
	CHK_ERR(err, "Listen");

	do {
			c = accept(s, NULL, 0);
			CHK_ERR(c, "Accept");

			enviar_usuarios(c); 
			close(c);
		
		} while (1);
		
	close(s);
	exit(0);
}

