#ifndef LNET_H
#define LNET_H
/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int net_tcp_connect(const char *host, unsigned short port);
int net_tcp_connect_nonblock(const char *host, unsigned short port);

int net_read_n(int fd, void *buf, int len);
int net_write_n(int fd, const void *buf, int len);

int net_read_nonblock(int fd, void *buf, int len);
int net_write_nonblock(int fd, const void *buf, int len);

int net_peek_nonblock(int fd, void *buf, int len);

int net_get_so_error(int fd);

int net_set_nonblock(int fd);
int net_set_block(int fd);

int net_set_close_on_exec(int fd);
int net_clear_close_on_exec(int fd);

uint16_t net_get_sock_ip_port(int sock, char ip[16]);
uint16_t net_get_peer_ip_port(int sock, char ip[16]);

void net_swap2(void *p);
void net_swap4(void *p);
void net_swap8(void *p);

void net_swap_int16(int16_t *p);
void net_swap_uint16(uint16_t *p);

void net_swap_int32(int32_t *p);
void net_swap_uint32(uint32_t *p);

void net_swap_int64(int64_t *p);
void net_swap_uint64(uint64_t *p);

void net_swap_float(float *p);
void net_swap_double(double *p);


#ifdef __cplusplus
}
#endif

#endif

