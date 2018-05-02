#ifndef LUTIL_H
#define LUTIL_H

#include "hdict.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MEMCACHED_MAX_KEY 251

int lpack_ascii(const char *req, char **outbuf, int *osize, int *obytes, int length, const char *buf);

int hdb_query_ascii(hdb_t *hdb, uint32_t hdid, uint64_t key, char *buf, int buf_len);

#ifdef __cplusplus
}
#endif

#endif
