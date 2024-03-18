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