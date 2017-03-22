/*
 * Useful utilities and wrapper functions
 * modifed from csapp.c used in CSAPP textbook 2nd edition
 */

#include "helper.h"

/*********************************************************
 * Error handling functions
 *********************************************************/
void unix_error(char *msg) {	/*  Unix-style error */
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(0);
}

void app_error(char *msg) { /* aPPLication error */
	fprintf(stderr, "%s\n", msg);
	exit(0);
}

void dns_error(char *msg) { /* DNS-style error */
	fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
	exit(0);
}

/**********************************************************
 * The Robust I/O package
 **********************************************************/
/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
	size_t nleft = n;
	ssize_t nread;
	char *bufp = usrbuf;

	while (nleft > 0) {
		if ((nread = read(fd, bufp, nleft)) < 0) {
			if (errno == EINTR) 	/* Interrupted by sig handler return */
				nread = 0;
			else
				return -1;
		} else if (nread == 0) {	/* EOF */
			break;
		}
		nleft -= nread;
		bufp += nread;
	}
	return (n-nleft);				/* return >= 0 */
}

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
	size_t nleft = n;
	ssize_t nwritten;
	char *bufp = usrbuf;

	while (nleft > 0) {
		if ((nwritten = write(fd, bufp, n)) < 0) {
			if (errno == EINTR)		/*  Interrupted by sig handler return */
				nwritten = 0;
			else
				return -1;
		}
		nleft -= nwritten;
		bufp += nwritten;
	}
	return n;
}

/*  
 * rio_read - This is a wrapper for the Unix read() function that
 * transfers min(n, rio_cnt) bytes from an internal buffer to a user
 * buffer, where n is the number of bytes requested by the user and
 * rio_cnt is the number of unread bytes in the internal buffer. On
 * entry, rio_read() refills the internal buffer via a call to
 * read() if the internal buffer is empty.
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
	int cnt;

	while (rp->rio_cnt <= 0) {	/* Refill if buf is empty */
		rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
		if (rp->rio_cnt < 0) {
			if (errno != EINTR) /* Interrupted by sig handler return */
				return -1;
		} else if (rp->rio_cnt == 0) /* EOF */
			return 0;
		else
			rp->rio_bufptr = rp->rio_buf;	/* Reset buffer ptr */
	}

	/* Copy min(n, rp->cnt) bytes from internal buffer to user buf */
	cnt = n;
	if (rp->rio_cnt < n)
		cnt = rp->rio_cnt;
	memcpy(usrbuf, rp->rio_bufptr, cnt);
	rp->rio_bufptr += cnt;
	rp->rio_cnt -= cnt;
	return cnt;
}

/* 
 * rio_readinitb - Associate a descriptor with a read buffer and reset 
 * buffer
 */
void rio_readinitb(rio_t *rp, int fd) {
	rp->rio_fd = fd;
	rp->rio_cnt = 0;
	rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
	size_t nleft = n;
	ssize_t nread;
	char *bufp = usrbuf;
	
	while (nleft > 0) {
		if ((nread = rio_read(rp, bufp, nleft)) < 0)
			return -1;		/* errno set by read() */
		else if (nread == 0)	/* EOF */
			break;
		nleft -= nread;
		bufp += nread;
	}
	return (n - nleft);
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
	int n, rc;
	char c, *bufp = usrbuf;
	
	for (n = 1; n < maxlen; n++) {
		if ((rc = rio_read(rp, &c, 1)) == 1) {
			*bufp++ = c;
			if (c == '\n') {
				n++;
				break;
			}
		} else if (rc == 0) {
			if (n == 1)	/* EOF, no data read */
				return 0;
			else		/* EOF, some data read */
				break;
		} else			/* Error */
			return -1;
	}
	*bufp = 0;
	return n-1;
}

/******************************************************
 * Wrappers for Robust I/O routines
 *******************************************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes) {
	ssize_t n;
	if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
		fprintf(stderr, "Rio_readn error: %s\n", strerror(errno));
		exit(0);
	}
}

void Rio_writen(int fd, void *usrbuf, size_t n) {
	if (rio_writen(fd, usrbuf, n) != n) {
		fprintf(stderr, "Rio_writen error: %s\n", strerror(errno));
		exit(0);
	}
}

void Rio_readinitb(rio_t *rp, int fd) {
	rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
	ssize_t rc;

	if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
		fprintf(stderr, "Rio_readnb error: %s\n", strerror(errno));
		exit(0);
	}
	return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
	ssize_t rc;

	if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
		fprintf(stderr, "Rio_readlineb error: %s\n", strerror(errno));
		exit(0);
	}
	return rc;
}

/*********************************************************
 * Socket interface wrappers
 *********************************************************/

int Socket(int domain, int type, int protocol) {
	int rc;

	if ((rc = socket(domain, type, protocol)) < 0)
		unix_error("Socket error");
	return rc;
}

void Setsocketopt(int s, int level, int optname, const void *optval, int optlen) {
	int rc;
	
	if ((rc = setsocketopt(s, level, optname, optval, optlen)) < 0)
		unix_error("Setsocketopt error");
}
void Bind(int sockfd, struct sockaddr *my_addr, int addrlen) {
	int rc;

	if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
		unix_error("Bind error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
	int rc;

	if ((rc = accept(s, addr, addrlen)) < 0)
		unix_error("Accept error");
	return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) {
	int rc;

	if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
		unix_error("Connect error");
}

/***************************************************************
 * client server helper routines
 ***************************************************************/
int open_listenfd(int port) {
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* Eliminates "Address already in use" error from bind */
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
		return -1;

	/* Listenfd will be an end point for all requests to port on any IP address for this host */
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
		return -1;

	/* Make it a listening socket ready to accept connection */
	if (listen(listenfd, LISTENQ) < 0)
		return -1;
	return listenfd;
}

/********************************************************************
 * Wrapper for the client/server helper routines
 ********************************************************************/
int Open_listenfd(int port) {
	int rc;

	if ((rc = open_listenfd(port)) < 0)
		unix_error("Open_listenfd error");
	return rc;
}


/**********************************************************************
 * Wrappers for Unix I/O routines
 **********************************************************************/
void Close(int fd) {
	int rc;

	if ((rc = close(fd)) < 0)
		unix_error("Close error");
}


/**********************************************************************
 * DNS interface wrappers
 **********************************************************************/
struct hostent *Gethostbyaddr(const char *addr, int len, int type) {
	struct hostent *p;

	if ((p = gethostbyaddr(addr, len, type)) == NULL)
		dns_error("Gethostbyaddr error");
	return p;
}
