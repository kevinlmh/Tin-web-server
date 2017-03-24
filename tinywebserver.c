/*
 * tinywebserver.c - A simple HTTP/1.0 web server
 * 
 * modified from CSAPPP 2nd edition book example tiny.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "helper.h"

int main(int argc, char **argv) {
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		// handle connection	
		Close(connfd);
	}

	return 0;
}


