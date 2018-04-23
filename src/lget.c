#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "lushan.h"

int lget(const char *host, uint16_t port, uint32_t hdid, uint64_t key, char **buf, int *buf_size)
{
	struct sockaddr_in *addr;
	int sock = -1;
	struct addrinfo hints = {0}, *res = NULL;
	int errcode;

	fd_set rfds;
	fd_set wfds;
	struct timeval tv;
	tv.tv_sec = 0; 
	tv.tv_usec = 3000;

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

	int flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
		goto error;

	if (connect(sock, res->ai_addr, res->ai_addrlen) == -1)
	{
		if (errno != EINPROGRESS)
			goto error;
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		errcode = select(sock+1, &rfds, NULL, NULL, &tv);
		if (errcode <= 0)
			goto error;
  		socklen_t size = sizeof(int);
		getsockopt(sock, SOL_SOCKET, SO_ERROR, &errcode, &size);
		if (errcode)
			goto error;
	}

	char request[512];
	char response[4096];
	int cur = 0;
	int n, need;
	int len = snprintf(request, "get %u-%llu\r\n", hdid, key);
	do {
		need = len - cur;
		n = write(sock, request + cur, need);
		if (n > 0) {
			cur += n;
			if (cur == len) break;
		} else if (n == -1 && errno != EAGAIN && errno != EINTR) {
			goto error;
		}

		FD_ZERO(&wfds);
		FD_SET(sock, &wfds);
		errcode = select(sock+1, NULL, &wfds, NULL, &tv);
		if (errcode <= 0)
			goto error;
	} while(1);

	int need = 4096;
	cur = 0;
	char *el;
	do {
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		errcode = select(sock+1, &rfds, NULL, NULL, &tv);
		if (errcode <= 0)
			goto error;

		need = 4096 - cur;
		n = read(sock, response + cur, need);
		if (n == -1)
			goto error;
		else if (n > 0) {
			cur += n;
		
            	el = (char *)memchr(response, '\n', cur);
            	if (el)
			break;
	} while(1);

	char *cont = el + 1;
	if (el - response > 1 && *(el - 1) == '\r') {
		el--;
	}
	*el = '\0'; 
	char key[256];
	uint32_t cas;
	uint32_t cont_length;
	if (strncmp(response, "VALUE", 5) == 0) {
		n = sscanf(response, "%*s %250s %u %u\n", &key, &cas, &cont_length);
		if (n != 3)
			goto error;
	} else {
		goto error;
	}

	if (cont_length > 4000) goto error;
	if (cur - (cont - response) >= cont_length + 7) {
		close(sock);
		freeaddrinfo(res);
		memcpy(*buf, cont, cont_length);
		*buf_size = cont_length;
		return 0;
	}

	do {
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		errcode = select(sock+1, &rfds, NULL, NULL, &tv);
		if (errcode <= 0)
			goto error;

		need = 4096 - cur;
		n = read(sock, response + cur, need);
		if (n == -1)
			goto error;
		else if (n > 0) {
			cur += n;

		if (cur - (cont - response) >= cont_length + 7) {
			close(sock);
			freeaddrinfo(res);
			memcpy(*buf, cont, cont_length);
			*buf_size = cont_length;
			return 0;
		}
	} while(1);


error:
	if (sock != -1)
		close(sock);
	if (res)
		freeaddrinfo(res);
	return -1;

}

