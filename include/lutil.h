#ifndef LUTIL_H
#define LUTIL_H

#include "hdict.h"
#include "llog.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MEMCACHED_MAX_KEY 251

int lpack_ascii(const char *req, char **outbuf, int *osize, int *obytes, int length, const char *buf);

int hdb_query_ascii(hdb_t *hdb, uint32_t hdid, uint64_t key, char *buf, int buf_len);

int hrequest_pack(char *dest, int dest_len, const char *key, const char *value, int value_len);

int lpack_header(const char *key, int key_length, char **outbuf, int *osize, int *obytes, int length);
void lpack_append_body(char *outbuf, int *obytes, const char *value, int value_len);
void lpack_footer(char *outbuf, int *obytes);

llog_t *llog_open_with_conf(const char *dir, const char *prefix, const char *config_file);

#ifdef __cplusplus
}
#endif

#endif
