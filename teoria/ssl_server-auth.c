/*************************************************/
/*** ssl_server.c                              ***/
/*************************************************/

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
#include "openssl/ssl.h"
#include "openssl/err.h"

#define CERTFILE "certificados/s_cert.pem"
#define KEYFILE "certificados/s_key.pem"
#define CA_CERT "certificados/c-ca_cert.pem"

#define REPLY "Hello, welcome¡¡¡¡¡"

#define CHK_NULL(x) if ((x) == NULL) exit(-1)
#define CHK_ERR(err, s) if ((err) == -1) { perror(s); exit(-2); }
#define CHK_SSL(err) if ((err) == -1) { ERR_print_errors_fp(stderr); exit(-3); }

/*--- main - create SSL socket server. ---------------------*/

int main(int count, char *strings[])
{
    SSL_CTX *ctx;
    SSL *ssl;
    const SSL_METHOD *method;

    X509 *cli_cert;
    char *line;

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int err, listen_sd, portnum, bytes;
    char buf[1024];

    if (count != 2)
    {
        printf("Usage: %s <portnum>\n", strings[0]);
        exit(0);
    }
    portnum = atoi(strings[1]);

    // CAMBIO PARA BORRAR

    /*--- Initialize SSL server  and create context     ------------*/

    SSL_library_init();
    SSL_load_error_strings(); /* load all error messages */

    method = TLS_server_method(); /* Create new client-method instance */
    ctx = SSL_CTX_new(method);    /* Create new context */
    CHK_NULL(ctx);

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

    /*--- Create server socket    -------------------------*/

    listen_sd = socket(PF_INET, SOCK_STREAM, 0);
    CHK_ERR(listen_sd, "socket");

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portnum);
    addr.sin_addr.s_addr = INADDR_ANY;

    err = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
    CHK_ERR(err, "Can't bind port");

    err = listen(listen_sd, 5);
    CHK_ERR(err, "Can't configure listening port");

    while (1)
    {
        int client = accept(listen_sd, (struct sockaddr *)&addr, &len); /* accept connection as usual */
        CHK_ERR(client, "No se acepta conexión");
        printf("Connection: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        ssl = SSL_new(ctx); /* get new SSL state with context */
        CHK_NULL(ssl);

        SSL_set_fd(ssl, client); /* set connection socket to SSL state */

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

        /*--------Service connection ------------------------------------*/

        bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
        CHK_SSL(bytes);
        buf[bytes] = '\0';
        printf("Client msg: \"%s\"\n", buf);

        err = SSL_write(ssl, REPLY, strlen(REPLY)); /* send reply */
        CHK_SSL(err);

        close(client); /* close connection */

        SSL_free(ssl); /* release SSL state */
    }

    SSL_CTX_free(ctx); /* release context */
}
