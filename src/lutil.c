/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */
#include "lushan.h"
#include "lutil.h"
#include "lconf.h"
#include "llog.h"

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
			return -1;
		}
	}
	*obytes += sprintf(*outbuf + *obytes, "VALUE %s %u %u\r\n", req, 0, length);
	if (length != 0)
		*obytes += sprintf(*outbuf + *obytes, "%s", buf);

	*obytes += sprintf(*outbuf + *obytes, "\r\n");
	return 0;
}

int lpack_header(const char *key, int key_length, char **outbuf, int *osize, int *obytes, int length)
{
	//if (*osize - *obytes < 512 + length) {
	if (*osize - *obytes < key_length + 256 + length) {
		int relloc_length = *osize * 2;
		//if (relloc_length - *obytes < 512 + length) {
		if (relloc_length - *obytes < key_length + 256 + length) {
			//relloc_length = 512 + length + *obytes;
			relloc_length = key_length + 256 + length + *obytes;
			relloc_length = (relloc_length/DATA_BUFFER_SIZE + 1) * DATA_BUFFER_SIZE;
		}
		char *newbuf = (char *)realloc(*outbuf, relloc_length);
		if (newbuf) {
			*outbuf = newbuf;
			*osize = relloc_length;
		} else {
			return -1;
		}
	}
	*obytes += sprintf(*outbuf + *obytes, "VALUE %s %u %u\r\n", key, 0, length);
	return 0;
}

void lpack_append_body(char *outbuf, int *obytes, const char *value, int value_len)
{
	memcpy(outbuf + *obytes, value, value_len);
	*obytes += value_len;
	return;
}

void lpack_footer(char *outbuf, int *obytes)
{
	memcpy(outbuf + *obytes, "\r\n", 2);
	*obytes += 2;
	return;
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

llog_t *llog_open_with_conf(const char *dir, const char *prefix, const char *config_file)
{
    llog_level_t level = LL_WARNING;
    char level_buf[LCONF_LINE_LEN_MAX];
    if (lconf_read_string(config_file, "llog_level", level_buf, LCONF_LINE_LEN_MAX) 
            == 1) {
                if (strcasecmp(level_buf, "all") == 0) level = LL_ALL;
                else if (strcasecmp(level_buf, "debug") == 0) level = LL_DEBUG;
                else if (strcasecmp(level_buf, "trace") == 0) level = LL_TRACE;
                else if (strcasecmp(level_buf, "notice") == 0) level = LL_NOTICE;
    }

    llog_t *llog;
    if ((llog = llog_open(dir, prefix, level, LL_ROTATE_DAY)) == NULL) {
		return NULL;
	}
	
	return llog;
}