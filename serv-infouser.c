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
#include "openssl/ssl.h"
#include "openssl/err.h"

#define CERTFILE "certificados/s_cert.pem"
#define KEYFILE "certificados/s_key.pem"
#define CA_CERT "certificados/c-ca_cert.pem"

#define CHK_NULL(x) if ((x) == NULL) exit(-1)
#define CHK_ERR(err, s) if ((err) == -1) { perror(s); exit(-2); }
#define CHK_SSL(err) if ((err) == -1) { ERR_print_errors_fp(stderr); exit(-3); }

#define PUERTO (IPPORT_RESERVED + 40)  // Puerto tomado al azar
#define TAMBUF 1460 			// Maxima lectura/escritura

void enviar_usuarios(SSL *s){
	int fp;  	// Descriptor del fichero a enviar
	struct stat f;	// Estructura de donde sacaremos el tama�o del fichero
	int i,k, total;
	char c, buf[TAMBUF];
	long int tam;	// tama�o del fichero, a leer de la estructura stat
	
	system("finger > /tmp/file_finger");
	
	if ((fp = open("/tmp/file_finger", O_RDONLY)) < 0 ) {
		sprintf(buf, "KO: Error al abrir el fichero");
		SSL_write(s, buf, strlen(buf));
		return;
	}
	
	i = stat("/tmp/file_finger", &f);
	if (i != 0) {
		sprintf(buf, "KO: Error stat");
		SSL_write(s, buf, strlen(buf));
		close(fp);
		return;
	}
	
	tam = f.st_size;
	sprintf(buf, "OK:%ld", tam);
	SSL_write(s, buf, strlen(buf));

	total = 0;
	while (total < tam){
		if ((i = read(fp, buf, TAMBUF))<0) {
			sprintf(buf, "KO: Error read file");
			SSL_write(s, buf, strlen(buf));
			perror("read socket");
			close(fp);
			return;
		}			
		if ((SSL_write(s,buf,i))<i) {
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
	socklen_t len = sizeof(dir);
	char buf[TAMBUF];	// buffer para lectura/excritura en el socket. 

	SSL_CTX *ctx;
    SSL *ssl;
    const SSL_METHOD *method;

    X509 *cli_cert;
    char *line;

/*--- Initialize SSL server  and create context     ------------*/

    SSL_library_init();
    SSL_load_error_strings(); /* load all error messages */

    method = TLS_server_method(); /* Create new client-method instance */
    ctx = SSL_CTX_new(method);    /* Create new context */
    CHK_NULL(ctx);

/*--- (END) Initialize SSL server  and create context     ------------*/

/*--- Load certificates from files. ------------------------*/

    /* set the local certificate from CertFile */
    err = SSL_CTX_use_certificate_file(ctx, CERTFILE, SSL_FILETYPE_PEM);
    CHK_SSL(err);

    /* set the private key from KeyFile (may be the same as CertFile) */
    err = SSL_CTX_use_PrivateKey_file(ctx, KEYFILE, SSL_FILETYPE_PEM);
    CHK_SSL(err);

    err = SSL_CTX_check_private_key(ctx); /* verify private key */
    CHK_ERR(err, "Private key does not match the public certificate\n");

    /* Load trusted CA's certificate (or pathfile), to verify  client's certificates */
    err = SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL);
    CHK_SSL(err);

    /*Set flag in context to require client certificate verification */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);

/*--- (END) Load certificates from files. ------------------------*/

/*--- Create server socket    -------------------------*/
	
	s = socket(PF_INET, SOCK_STREAM, 0);
	CHK_ERR(s, "Apertura de socket");

	bzero(&dir, sizeof(dir));
	dir.sin_family = AF_INET;
	dir.sin_port = htons(PUERTO);
	dir.sin_addr.s_addr = htonl(INADDR_ANY); 
	
	err = bind (s, (struct sockaddr *)&dir, sizeof(dir));
	CHK_ERR(err, "Can't bind port");
	
	err = listen (s, 5);
	CHK_ERR(err, "Can't configure listenig port");

	while (1) {
		c = accept(s, (struct sockaddr *)&dir, &len); /* accept connection as usual */
        CHK_ERR(c, "No se acepta conexión");
        printf("Connection: %d:%d\n", inet_ntoa(dir.sin_addr), ntohs(dir.sin_port));

		ssl = SSL_new(ctx); /* get new SSL state with context */
		CHK_NULL(ssl);

		SSL_set_fd(ssl, c); /* set connection socket to SSL state */

		err = SSL_accept(ssl);
		CHK_SSL(err);

		/*--- Print out certificates.    ------------------------*/
		if (SSL_get_verify_result(ssl) == X509_V_OK)
			printf("Client verification succeeded.\n");

		cli_cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */

		if (cli_cert != NULL)
		{
			printf("Client certificates:\n");
			line = X509_NAME_oneline(X509_get_subject_name(cli_cert), 0, 0);
			CHK_NULL(line);
			printf("Subject: %s\n", line);
			free(line);

			line = X509_NAME_oneline(X509_get_issuer_name(cli_cert), 0, 0);
			CHK_NULL(line);
			printf("Issuer: %s\n", line);
			free(line);

			X509_free(cli_cert);
		}
		else
			printf("No certificates.\n");

		printf("Server: SSL connection using %s\n", SSL_get_cipher(ssl));
		/*--- (END) Print out certificates.    ------------------------*/
		
		enviar_usuarios(ssl);

		close(c);
		SSL_free(ssl); /* release SSL state */
		
	}
		
	close(s);
	SSL_CTX_free(ctx); /* release context */
	exit(0);
}

