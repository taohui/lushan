#ifndef LCONF_H
#define LCONF_H
/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LCONF_LINE_LEN_MAX 1024

/**
 * Not found return 0, error return -1, otherwise return 1.
 */
int lconf_read_string(const char *filepath, const char *name, char *valuebuf,
                     int value_buf_size);

int lconf_read_int32(const char *filepath, const char *name,
                    int32_t * valuebuf);

int lconf_read_uint32(const char *filepath, const char *name,
                     uint32_t * valuebuf);

int lconf_read_int64(const char *filepath, const char *name,
                    int64_t * valuebuf);

int lconf_read_double(const char *filepath, const char *name,
                    double * valuebuf);
#ifdef __cplusplus
}
#endif

#endif
