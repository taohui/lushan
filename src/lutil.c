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


