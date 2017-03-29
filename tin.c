/*
 * tin.c - A simple HTTP/1.0 web server
 * 
 * modified from CSAPPP 2nd edition book example
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "helper.h"
#include "threadpool.h"

#define MAXBUF 8192			/* Max I/O buffer size */
#define NUM_THREADS 512		/* Number of threads in thread pool */
#define JOBQUEUE_SIZE 1024	/* Size of job queue */

/* External variables */
extern char **environ;		/* Defined by libc */

struct pkg {
	int fd;
	int filesize;
	char *filename;
};

void handle_request(threadpool_t *thpool, int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void *serve_static(void *pkg);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsm);
void sigint_handler(int signum);

/* global data */
threadpool_t *thpool;			/* threadpool */
int shutdown_server = 0;		/* shutdown server */

int main(int argc, char **argv) {
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	// register signal handler
	Signal(SIGINT, sigint_handler);

	/* create a thread pool */
	thpool = threadpool_create(NUM_THREADS, JOBQUEUE_SIZE);


	listenfd = Open_listenfd(port);
	while (!shutdown_server) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		handle_request(thpool, connfd);
	}

	return 0;
}

/*
 * handle_request - handle one HTTP request/response transaction
 */
void handle_request(threadpool_t *thpool, int fd) {
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
		return;
	}
	read_requesthdrs(&rio);

	/* Parse URI from GET request */
	is_static = parse_uri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) {
		clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
		return;
	}

	if (is_static) { /* Server static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", "Tiny couln't read the file");
			return;
		}
	
		/* add job to thread pool */
		struct pkg p = { fd, sbuf.st_size, filename };
		threadpool_add_work(thpool, serve_static, &p);
	} else { /* Server dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
		Close(fd);
	}
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><head><title>Tiny Error</title></head> \
<body bgcolor=""ffffff"">\r\n \
%s: %s\r\n \
<p>%s: %s\r\n \
<hr><em>The Tiny Web server</em>\r\n \
</body></html>", errnum, shortmsg, longmsg, cause);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
	char *ptr;

	if (!strstr(uri, "cgi-bin")) { /* Static content */
		strcpy(cgiargs, "");
		strcpy(filename, "./site/");
		strcat(filename, uri);
		if (uri[strlen(uri)-1] == '/')
			strcat(filename, "index.html");
		return 1;
	} else { /* Dynamic content */
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		} else {
			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}

/*
 * serve_static - copy a file back to the client
 */
void *serve_static(void *pkg) {
	struct pkg *args = (struct pkg *)pkg;
	int fd = args->fd;
	char *filename = args->filename;
	int filesize = args->filesize;
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));

	/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	Close(srcfd);
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) {
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}

/* 
 * server_dynamic - run a cgi binary program and redirect output to client 
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0) { /* Child */
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);
		Dup2(fd, STDOUT_FILENO);	/* Redirect stdout to client */
		Execve(filename, emptylist, environ);	/* Run CGI program */
	}
	Wait(NULL);		/* Parent waits for and reaps child */
}

/*
 * Handle SIGINT when user press CTRL-C and terminate gracefully
 */
void sigint_handler(int signum) {
	// wait for all threads to finish current work and destroy thread pool 
	threadpool_destroy(thpool);
	// shutdown_server
	shutdown_server = 1;
	exit(0);
}
