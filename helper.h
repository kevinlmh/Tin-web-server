#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>


/* Misc constants */
#define MAXLINE 8192	/* Max text line length */
#define LISTENQ 1024	/* Second argument to listen() */

/* handler type for signal routines */
typedef void handler_t(int);

/* Persistent state for the robust I/O (Rio) package */
#define RIO_BUFSIZE 8192
typedef struct {
	int rio_fd;					/* Descriptor for this internal buf */
	int rio_cnt;				/* Unread bytes in internal buf */
	char *rio_bufptr;			/* Next unread byte in internal buf */
	char rio_buf[RIO_BUFSIZE]; /* Internal buffer */
} rio_t;

/* External variables */
extern int h_errno;	 			/* Defined by BIND for DNS errors */

/* Error handling functions */
void unix_error(char *msg);
void app_error(char *msg);
void dns_error(char *msg);

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Wrappers for Rio package */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, void *usrbuf, size_t n);
void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Client/server helper funcitons */
int open_clientfd(char *hostname, int portno);
int open_listenfd(int portno);

/* Wrappers for client/server helper functions */
int Open_clientfd(char *hostname, int port);
int Open_listenfd(int port);

/* Wrappers for Unix I/O routines */
int Open(const char *pathname, int flags, mode_t mode);
void Close(int fd);
int Dup2(int fd1, int fd2);

/* DNS interface wrappers */
struct hostent *Gethostbyaddr(const char *addr, int len, int type);

/* Wrappers for Standard I/O functions */
char* Fgets(char *ptr, int n, FILE *stream);
void Fputs(const char *ptr, FILE *stream);
#endif 

/* Wrappers for memory mapping functions */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

/* Wrappers for Unix process control functions */
pid_t Fork(void);
pid_t Wait(int *status);
void Execve(const char *filename, char *const argv[], char *const envp[]);

/* Signal wrappers */
handler_t *Signal(int signum, handler_t *handler);
