#ifndef LUSHAN_H
#define LUSHAN_H

#include <stdint.h>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t seq;
    uint16_t command;
    int16_t status;
    uint32_t length;
    void *pkt[];
} hpacket_t;

enum protocol {
    ascii_prot = 3, /* arbitrary value. */
    binary_prot,
};

typedef struct {
    int key_off;
    int value_off;
    int value_len;
    int hmid;
    char *key;
    char *value;
} hrequest_t;

#define HREQUEST_LENGTH_MAX 8192

#define DATA_BUFFER_SIZE 8192
#define KEY_BUFFER_SIZE 4096

#endif
