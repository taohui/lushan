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

enum request_method {
    GET = 1,
    POST,
};

typedef struct {
    request_method;
    int value_offset;
    int value_length;
    void *pkt[];
} request_t;

#define REQ_key(req) ((char*)&((req)->pkt[0]))
#define REQ_value(req) ((char*) &((req)->pkt[0]) + (req)->value_offset)

#define DATA_BUFFER_SIZE 8192
#define KEY_BUFFER_SIZE 4096
#define REQUEST_LENGTH_MAX 4096

#endif
