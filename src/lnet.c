/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */
#include "lnet.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <arpa/inet.h>

int _tcp_connect(const char *host, unsigned short port, int nonblock)
{
	struct sockaddr_in *addr;
	int sock = -1;
	struct addrinfo hints = {0}, *res = NULL;
	int errcode;

	/* Don't use gethostbyname(), it's not thread-safe. 
	   Use getaddrinfo() instead.
	 */
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0)
	{
		fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(errcode));
		goto error;
	}
	assert(res->ai_family == AF_INET && res->ai_socktype == SOCK_STREAM);
	assert(res->ai_addrlen == sizeof(*addr));

	addr = (struct sockaddr_in *)res->ai_addr;
	addr->sin_port = htons(port);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		goto error;

	if (nonblock)
	{
		int flags = fcntl(sock, F_GETFL, 0);
		if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
			goto error;
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) == -1)
	{
		if (!nonblock || errno != EINPROGRESS)
			goto error;
	}

	freeaddrinfo(res);
	return sock;
error:
	if (sock != -1)
		close(sock);
	if (res)
		freeaddrinfo(res);
	return -1;
}

int net_tcp_connect(const char *host, unsigned short port)
{
	return _tcp_connect(host, port, 0);
}

int net_tcp_connect_nonblock(const char *host, unsigned short port)
{
	return _tcp_connect(host, port, 1);
}

int net_read_n(int fd, void *buf, int len)
{
	int cur = 0;

	if (len <= 0)
		return -1;

	do {
		int need = len - cur;
		int n = read(fd, buf + cur, need);
		if (n == 0)
			break;
		if (n == -1)
		{
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
		cur += n;
	} while (cur < len);
	return cur;
}

int net_write_n(int fd, const void *buf, int len)
{
	int cur = 0;

	if (len <= 0)
		return -1;

	do {
		int need = len - cur;
		int n = write(fd, buf + cur, need);
		if (n == -1)
		{
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
		cur += n;
	} while (cur < len);
	return cur;
}


int net_read_nonblock(int fd, void *buf, int len)
{
	int n;
again:
	n = read(fd, buf, len);
	if (n == -1)
	{
		switch (errno)
		{
		case EINTR: goto again; break;
		case EAGAIN: n = 0; break;
		}
	}
	else if (n == 0)
		return -2;
	return n;
}

int net_peek_nonblock(int fd, void *buf, int len)
{
	int n;
again:
	n = recv(fd, buf, len, MSG_PEEK | MSG_DONTWAIT); 
	if (n == -1)
	{
		switch (errno)
		{
		case EINTR: goto again; break;
		case EAGAIN: n = 0; break;
		}
	}
	else if (n == 0)
		return -2;
	return n;
}


int net_write_nonblock(int fd, const void *buf, int len)
{
	int n;
again:
	n = write(fd, buf, len);
	if (n == -1)
	{
		switch (errno)
		{
		case EINTR: goto again; break;
		case EAGAIN: n = 0; break;
		case EPIPE: return -2; break;
		}
	}
	return n;
}

int net_get_so_error(int fd)
{
	int so_error = 0;
	socklen_t len = sizeof(so_error);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) == -1)
		return -1;
	return so_error;
}

int net_set_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int net_set_block(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int net_set_close_on_exec(int fd)
{
	int flags = fcntl(fd, F_GETFD, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

int net_clear_close_on_exec(int fd)
{
	int flags = fcntl(fd, F_GETFD, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC);
}


uint16_t net_get_sock_ip_port(int sock, char ip[16])
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if (getsockname(sock, (struct sockaddr *)&addr, &addr_len) == 0)
	{
		inet_ntop(AF_INET, &addr.sin_addr, ip, 16);
		return ntohs(addr.sin_port);
	}
	ip[0] = 0;
	return 0;
}

uint16_t net_get_peer_ip_port(int sock, char ip[16])
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	if (getpeername(sock, (struct sockaddr *)&addr, &addr_len) == 0)
	{
		inet_ntop(AF_INET, &addr.sin_addr, ip, 16);
		return ntohs(addr.sin_port);
	}
	ip[0] = 0;
	return 0;
}


#define SWAP2(p)	do {					\
	unsigned int _c___;					\
	char *_s___ = (char *)(p), *_d___ = (char*)(p) + 2;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
} while (0)

#define SWAP4(p)	do {					\
	unsigned int _c___;					\
	char *_s___ = (char *)(p), *_d___ = (char*)(p) + 4;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
} while (0)

#define SWAP8(p)	do {					\
	unsigned int _c___;					\
	char *_s___ = (char *)(p), *_d___ = (char*)(p) + 8;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
	_c___ = *--_d___; *_d___ = *_s___; *_s___++ = _c___;	\
} while (0)

void net_swap2(void *p)
{
	SWAP2(p);
}

void net_swap4(void *p)
{
	SWAP4(p);
}

void net_swap8(void *p)
{
	SWAP8(p);
}

void net_swap_int16(int16_t *p)
{
	SWAP2(p);
}

void net_swap_uint16(uint16_t *p)
{
	SWAP2(p);
}

void net_swap_int32(int32_t *p)
{
	SWAP4(p);
}

void net_swap_uint32(uint32_t *p)
{
	SWAP4(p);
}

void net_swap_int64(int64_t *p)
{
	SWAP8(p);
}

void net_swap_uint64(uint64_t *p)
{
	SWAP8(p);
}

void net_swap_float(float *p)
{
	SWAP4(p);
}

void net_swap_double(double *p)
{
	SWAP8(p);
}

