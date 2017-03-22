/**********************************************************
 * The Robust I/O package
 * 
 * Modified from RIO code example in CSAPP 2nd edition
 * 
 **********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "rio.h"

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


































