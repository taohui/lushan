/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */
#include "lushan.h"
#include "lutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lpack_ascii(const char *req, char **outbuf, int *osize, int *obytes, int length, const char *buf){
	int req_length = strlen(req);
	//if (*osize - *obytes < 512 + length) {
	if (*osize - *obytes < req_length + 256 + length) {
		int relloc_length = *osize * 2;
		//if (relloc_length - *obytes < 512 + length) {
		if (relloc_length - *obytes < req_length + 256 + length) {
			//relloc_length = 512 + length + *obytes;
			relloc_length = req_length + 256 + length + *obytes;
			relloc_length = (relloc_length/DATA_BUFFER_SIZE + 1) * DATA_BUFFER_SIZE;
		}
		char *newbuf = (char *)realloc(*outbuf, relloc_length);
		if (newbuf) {
			*outbuf = newbuf;
			*osize = relloc_length;
		} else {
			return 0;
		}
	}
	*obytes += sprintf(*outbuf + *obytes, "VALUE %s %u %u\r\n", req, 0, length);
	if (length != 0)
		*obytes += sprintf(*outbuf + *obytes, "%s", buf);

	*obytes += sprintf(*outbuf + *obytes, "\r\n");
	return 0;
}

int hdb_query_ascii(hdb_t *hdb, uint32_t hdid, uint64_t key, char *buf, int buf_len)
{
    int res;
    off_t off;
    uint32_t length = 0;

    hdict_t *hdict;
    hdict = hdb_ref(hdb, hdid);
    if (hdict != NULL) {
        hdict->num_qry++;
        if (hdict_seek(hdict, key, &off, &length)) {
            if (length >= buf_len) {
                hdb_deref(hdb, hdict);
                return 0;
            }
            res = hdict_read(hdict, buf, length, off);
            buf[length] = '\0';
        }
        hdb_deref(hdb, hdict);
    }
    return length;
}

int hrequest_pack(char *dest, int dest_len, const char *key, const char *value, int value_len)
{
	int ret = -1;
	int len = snprintf(dest, dest_len, "%s 0 0 %d\r\n", key, value_len);
	if (dest_len - len > value_len) {
		memcpy(dest+len, value, value_len);
		ret = value_len + len;
		dest[ret] = '\0';
		return ret;
	}
	return ret;
}