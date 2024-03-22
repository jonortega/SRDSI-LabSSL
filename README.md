# SRDSI - Laboratorio SSL

## tcp_server.c

- Podemos usarlo tal cual, es una "pieza del puzle":
```c
listen_sd = socket(PF_INET, SOCK_STREAM, 0);	 /* Create server socket */
CHK_ERR(listen_sd, "No se ha establecido el socket");

bzero(&addr, sizeof(addr)); 			/* Set server address */
addr.sin_family = AF_INET;
addr.sin_port = htons(portnum);
addr.sin_addr.s_addr = INADDR_ANY;
```
- `AF_INET`: Address Family InterNET
- `INADDR_ANY`: Ssume la direccion IP de la máquina en la que se está ejecutando de forma automática. Si lo ejecuto en cualquier otra máquina compatible, no tengo que cambiar nada. 

- Asignar la direccion y el puerto al socket:
```c
err = bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) ; /* Assign address to socket */
```

- Ponerte a escucha y puedes encolar 5 posiciones SECUENCIALES, no concurrentes.
```c
err = listen(listen_sd, 5) ;
```

- Hacer el handshake de TCP y estableces conexión. Socket -> client
```c
int client = accept(listen_sd, (struct sockaddr*)&addr, &len);		/* accept connection */
```

- Hacer la comunicación usando el protocolo preestablecido:
```c
bytes = read(client, buf, sizeof(buf));	/* get request */
CHK_ERR(bytes, "Error de lectura");

buf[bytes] = '\0'; // Como usamos C, hay que cambiar los bytes recibidos a un string (en python no hace falta)
printf("Client msg: \"%s\"\n", buf);

err = write(client, REPLY, strlen(REPLY));	/* send reply */
CHK_ERR(err, "Error de escritura");

close(client);					/* close connection */
```

## tcp_client.c

```c
char buf[1024];
struct hostent *host; // Guarda el nombre del host al que va a hablar
struct sockaddr_in addr;
int bytes, err, sd;
char *hostname, *portnum;
```

- Si se sabe la IP lo pone, si no, va a llamar al DNS para obtenerlo:
```c
if ( count != 3 )
    {
        printf("usage: %s <hostname> <portnum>\n", strings[0]);
        exit(0);
    }
hostname = strings[1];
portnum = strings[2];

host = gethostbyname(hostname); 
CHK_ERR((long)host, hostname);
```

- Crear el socket: socket -> sd
```c
sd = socket(PF_INET, SOCK_STREAM, 0); 	/* Create socket */
CHK_ERR(sd, "NO se ha establecido el socket");
```

```c
bzero(&addr, sizeof(addr));			    /* Set server address */
addr.sin_family = AF_INET;
addr.sin_port = htons(atoi(portnum));
addr.sin_addr.s_addr = *(long*)(host->h_addr); // Obtener el host address de la estructura host, que es una structura compleja
```
- Conectarse al socket del servidor:
```c
err = connect(sd, (struct sockaddr *)&addr, sizeof(addr)); 	/* Connect to server */
CHK_ERR(err, "Conexión no aceptada");
```

- Hacer la comunicacíon a traves del socket:
```c
err = write(sd, MSG, strlen(MSG));		/* Send message */
CHK_ERR(err, "Error de escritura");

bytes = read(sd, buf, sizeof(buf));		/* Get reply */
CHK_ERR(bytes, "Error de lectura");
buf[bytes] = '\0';
printf("Received: \"%s\"\n", buf);

close(sd);						        /* Close socket */
```

## ssl_client-auth.c

- Inicializar la librería SSL(TLS) cargando los alg cript:
```c
SSL_library_init();
SSL_load_error_strings();		/* Bring in and register error messages */
```
- El cliente propone para que servidor elija los alg cript que quiera (el primero suele ser):
```c
method =  TLS_client_method();	/* Create new client-method instance */
ctx = SSL_CTX_new(method);		/* Create new context */
CHK_NULL(ctx);

// Con los siguientes comandos asignamos las opciones posibles
err=SSL_CTX_set_cipher_list(ctx, "AES128-SHA256:AES256-CCM:AES256-GCM-SHA384");
CHK_SSL(err);

err=SSL_CTX_set_max_proto_version(ctx, TLS1_2_VERSION);
CHK_SSL(err);
```

- Configurar el contexto, indicando el fichero del **certificado**:
```c
/* set the local certificate from CertFile */
err= SSL_CTX_use_certificate_file(ctx, CERTFILE, SSL_FILETYPE_PEM) ;
CHK_SSL(err);
```

- Cargar la **clave privada** del cliente:
```c
/* set the private key from KeyFile (may be the same as CertFile) */
err= SSL_CTX_use_PrivateKey_file(ctx, KEYFILE, SSL_FILETYPE_PEM) ;
CHK_SSL(err);
```

- Para **verificar** el certificado, necesito una CA:
```c
/* Load Trusted CA certificate, to verify the server's certificate */
err = SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL);
CHK_SSL(err);
    
/*Set flag in context to require server certificate verification */
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL); // Si no puedes verificar el certificado, cierra conexion
SSL_CTX_set_verify_depth(ctx, 1); // Ese certificado tiene que estar verificado por la CA principal (depth=1)
```

- Crear socket TCP:
```c
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
```

- **Crear el socket SSL** y luego **asociarle el socket TCP**:
```c
ssl = SSL_new(ctx);		/* create new SSL connection state */
CHK_NULL(ssl);    

SSL_set_fd(ssl, sd);	/* attach the socket descriptor */ // Vincular socket SSL con el socket TCP
    
err = SSL_connect(ssl);	/* perform the connection */
CHK_SSL(err);
printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
```

- **Verificar el certificad**o ("otra pieza del puzle"):
```c
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
```

- Realizar la **comunicación**:
```c
err = SSL_write(ssl, MSG, strlen(MSG));	/* encrypt & send message */
CHK_SSL(err);

bytes = SSL_read(ssl, buf, sizeof(buf));	/* get reply & decrypt */
CHK_SSL(bytes);
buf[bytes] = '\0';
printf("Received: \"%s\"\n", buf);

close(sd);			/* close socket */

SSL_free(ssl);		/* release connection state */
SSL_CTX_free(ctx);		/* release context */
```

## ssl_server-auth.c

- Carga de certificados igual que en el cliente
- Verificar el certificado del cliente -> cargar la lista de certificados
> **Nota:** Ahora las librerías hay que cargarlas al final, no al principio. Si no da error. (No entiendo muy bien qué librerías o qué)

## ACTIVIDAD

- Hay una aplicacion que nos da implementada.
- **Tenemos que coger eso y hacer los sockets seguros (SSL) para hacerla segura.**
- El puerto el fijo, no lo tocamos.
1. El cliente se va a conectar a todos los servidores de la red.
2. Va a recopilar todos los clientes conectados a ese servidor.
3. Va a guardar esos datos en un fichero en local.

> **Nota:** El cliente no debe presentar ningún certificado. Solamente el servidor.

