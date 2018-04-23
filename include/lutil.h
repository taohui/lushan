#ifndef LUTIL_H
#define LUTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MEMCACHED_MAX_KEY 251

int lpack_ascii(const char *req, char **outbuf, int *osize, int *obytes, int length, const char *buf);


#ifdef __cplusplus
}
#endif

#endif
