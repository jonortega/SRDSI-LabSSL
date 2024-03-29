#include <string.h>   // Necesario para usar bcopy
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
#include <fcntl.h>  //  Para poder hacer open
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CA_CERT  "certificados/s-ca_cert.pem"

#define CHK_NULL(x) if ((x)==NULL) { ERR_print_errors_fp(stderr); exit(1); }
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1);  }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

#define PUERTO (IPPORT_RESERVED + 40)  // Puerto tomado al azar
#define TAMBUF 1024 			// Maxima lectura/escritura

/*--- main - create SSL context and connect  ---------------*/

int main(int count, char *strings[]){
   int sock;
   int  lon, total, err;
   long tam_fich;
   int bytes_rec, k, i;
   struct sockaddr_in serv;
   struct hostent *host;
   FILE *fd;
   char buf[TAMBUF];

   SSL_CTX *ctx;
   SSL *ssl;
   const SSL_METHOD *method;
	
   X509 *serv_cert;
   char *line;
   
   char *hostname;
   
   if ( count != 3 ){
        printf("Uso: %s <hostip> <filename>\n", strings[0]);
        exit(1);
    	}
   
   hostname = strings[1];

/*--- Initialize the SSL engine (CTX)  -----------------*/

   SSL_library_init();
   SSL_load_error_strings();

   method =  TLS_client_method();
   ctx = SSL_CTX_new(method);		// Crear un nuevo contexto
   CHK_NULL(ctx);
	
   err=SSL_CTX_set_cipher_list(ctx, "AES128-SHA256:AES256-CCM:AES256-GCM-SHA384");
   CHK_SSL(err);

   err=SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);
   CHK_SSL(err);

/*--- (END) Initialize the SSL engine (CTX)  -----------------*/

/*---- Load CA Certificate ----------*/

   // Cargar el certificado de la CA del cliente, para verificar el cert. del servidor
   err = SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL);
   CHK_SSL(err);
		
   // Flag para requerir la verificación del cert. del servidor
   SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
   SSL_CTX_set_verify_depth(ctx, 1);   

/*---- (END) Load CA Certificate ----------*/

/*--- Create socket and connect to server. --------*/

   host = gethostbyname(hostname);
   CHK_ERR((long)host, hostname);
	
   sock = socket(PF_INET, SOCK_STREAM, 0); // Crear el socket TCP
   CHK_ERR(sock, "No crea socket");

   bzero(&serv, sizeof(serv));
   serv.sin_family = AF_INET;
   serv.sin_port = htons(PUERTO); // Indicar puerto
   serv.sin_addr.s_addr = *(long *)(host->h_addr); // Indicar direccion IP

   err = connect(sock, (struct sockaddr *)&serv, sizeof serv); 
   CHK_ERR(err, "No acepta conexion");

/*--- (END) Create socket and connect to server. --------*/

/*---- Create new SSL connection ------------------------*/

   ssl = SSL_new(ctx);		// Crear nuevo ESTADO de conexion
   CHK_NULL(ssl);    

   SSL_set_fd(ssl, sock);	// Adjuntar el descriptor de socket TCP al estado
        
   err = SSL_connect(ssl) ;	// Realizar la conexion con el socket SSL/TLS
   CHK_SSL(err);
   printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

/*---- (END) Create new SSL connection ------------------------*/

/*--- Print out the certificates. -------------------*/

   if (SSL_get_verify_result(ssl) == X509_V_OK)
      printf("Server verification succeeded.\n");

   serv_cert = SSL_get_peer_certificate(ssl);	// Obtener el certificado del servidor
   if ( serv_cert == NULL )
      printf("No certificates.\n");
   else
   {
      printf("Server certificates:\n");
      line = X509_NAME_oneline(X509_get_subject_name(serv_cert), 0, 0);
      CHK_NULL(line);
      printf("Subject: %s\n", line); // Mostrar el cert. del servidor por consola
      free(line);			
      X509_free(serv_cert);
   }

/*--- (END) Print out the certificates. -------------------*/

   bzero(&buf, TAMBUF );
   bytes_rec = SSL_read(ssl, buf, sizeof(tam_fich)-2);
   CHK_ERR(bytes_rec, "Error de lectura");
   
   if ( strncmp(buf, "OK:", 3) ){
      printf("Encontrado error en comando %s!!!\n",buf);
	   close(ssl);
      close(sock);
      SSL_free(ssl);
      SSL_CTX_free(ctx);
      exit(1);
   }

   tam_fich = atol(&buf[3]); // Obtener el tamaño del fichero

   // Obtener nombre del fichero y crearlo localmente
   if ((fd = fopen(strings[2], "w")) == NULL){
		perror("Error abriendo fichero local");
		exit(1);
   }

	bytes_rec = 0;
	while(bytes_rec < tam_fich){ 
		k = SSL_read(ssl, buf, TAMBUF); // Lectura de datos desde el socket SSL/TLS

		if ((k <= 0)|| !strncmp(buf, "KO:" ,3)){
			perror("Error leyendo respuesta. Abandono");
			close(ssl);
         close(sock);
         SSL_free(ssl);
         SSL_CTX_free(ctx);
			fclose(fd);
			exit(1);
		}
		for (i=0; i<k; i++)
			if (putc(buf[i],fd)<0){ // Escritura de los datos en el fichero local
				perror("Error escribiendo en el fichero local. Abandono");
				close(ssl);
            close(sock);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
				fclose(fd);
				exit(1);
			}
		bytes_rec += k;
	}

	fclose(fd);
   
   close(ssl); // Cerrar el socket SSL/TLS
   close(sock); // Cerrar el socket TCP

   SSL_free(ssl);	 // Liberar el estado de la conexion
   SSL_CTX_free(ctx); // Liberar el contexto

   exit(0);
}

