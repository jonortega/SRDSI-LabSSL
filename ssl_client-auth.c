/***********************************************/
/*** ssl_client.c                            ***/
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
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MSG "Hello, world????"

#define CERTFILE "certificados/c_cert.pem"
#define KEYFILE  "certificados/c_key.pem"
#define CA_CERT  "certificados/s-ca_cert.pem"

#define CHK_NULL(x) if ((x)==NULL) { ERR_print_errors_fp(stderr); exit(1); }
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1);  }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }


/*--- main - create SSL context and connect  ---------------*/

int main(int count, char *strings[])
{	
    SSL_CTX *ctx;
    SSL *ssl;
    const SSL_METHOD *method;

    char buf[1024];
    struct hostent *host;
    struct sockaddr_in addr;
	
    X509 *serv_cert;
    char *line;

    int bytes, sd, err;
    char *hostname, *portnum;

    if ( count != 3 )
    {
        printf("usage: %s <hostname> <portnum>\n", strings[0]);
	exit(0);
    }
    hostname=strings[1];
    portnum=strings[2];


/*--- Initialize the SSL engine (CTX)  -----------------*/

    SSL_library_init();
    SSL_load_error_strings();		/* Bring in and register error messages */

    method =  TLS_client_method();	/* Create new client-method instance */
    ctx = SSL_CTX_new(method);		/* Create new context */
    CHK_NULL(ctx);
	
    err=SSL_CTX_set_cipher_list(ctx, "AES128-SHA256:AES256-CCM:AES256-GCM-SHA384");
    CHK_SSL(err);

    err=SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);
    CHK_SSL(err);
	
/*---- Load Client Certificate ----------*/

    /* set the local certificate from CertFile */
    err= SSL_CTX_use_certificate_file(ctx, CERTFILE, SSL_FILETYPE_PEM) ;
    CHK_SSL(err);
	
    /* set the private key from KeyFile (may be the same as CertFile) */
    err= SSL_CTX_use_PrivateKey_file(ctx, KEYFILE, SSL_FILETYPE_PEM) ;
    CHK_SSL(err);
	
    /* verify private key */
    err=SSL_CTX_check_private_key(ctx);
    CHK_ERR(err,"Private key does not match the public certificate\n");
	
    /* Load Trusted CA certificate, to verify the server's certificate */
    err = SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL);
    CHK_SSL(err);
		
    /*Set flag in context to require server certificate verification */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);

/*--- Create socket and connect to server. --------*/

    host = gethostbyname(hostname);
    CHK_ERR((long)host, hostname);
	
    sd = socket(PF_INET, SOCK_STREAM, 0);
    CHK_ERR(sd, "socket");

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(portnum));
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    err = connect(sd, (struct sockaddr*)&addr, sizeof(addr));
    CHK_ERR(err, "No accept connection");

/*---- Create new SSL connection ------------------------*/

    ssl = SSL_new(ctx);		/* create new SSL connection state */
    CHK_NULL(ssl);    

    SSL_set_fd(ssl, sd);	/* attach the socket descriptor */
        
    err = SSL_connect(ssl) ;	/* perform the connection */
    CHK_SSL(err);
    printf("Connected with %s encryption\n", SSL_get_cipher(ssl));

/*--- Print out the certificates. -------------------*/

    if (SSL_get_verify_result(ssl) == X509_V_OK)
        printf("Server verification succeeded.\n");

    serv_cert = SSL_get_peer_certificate(ssl);	/* get the server's certificate */
    if ( serv_cert == NULL )
        printf("No certificates.\n");
    else
    {
        printf("Server certificates:\n");
	line = X509_NAME_oneline(X509_get_subject_name(serv_cert), 0, 0);
	CHK_NULL(line);
	printf("Subject: %s\n", line);
	free(line);				/* free the malloc'ed string */
		
	line = X509_NAME_oneline(X509_get_issuer_name(serv_cert), 0, 0);
	CHK_NULL(line);
	printf("Issuer: %s\n", line);
	free(line);				
		
	X509_free(serv_cert);			/* free the malloc'ed certificate copy */	
    }

    err = SSL_write(ssl, MSG, strlen(MSG));	/* encrypt & send message */
    CHK_SSL(err);
	
    bytes = SSL_read(ssl, buf, sizeof(buf));	/* get reply & decrypt */
    CHK_SSL(bytes);
    buf[bytes] = '\0';
    printf("Received: \"%s\"\n", buf);

    close(sd);			/* close socket */
	
    SSL_free(ssl);		/* release connection state */
    SSL_CTX_free(ctx);		/* release context */
    
    exit(0);
}


