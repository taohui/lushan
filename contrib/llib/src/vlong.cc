/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#include "vlong.h"
#include <stdint.h>
#include <stdlib.h>

int write_vlong(char **dest, int *left, long i)
{
    char *p = *dest;
    int l = *left;

	if (i >= -112 && i <= 127) {
		if (l >= 1) {
			*p = (char)i;
			p++;
            l -= 1;
            *dest = p;
            *left = l;
			return 0;
		} else {
			return -1;
		}
	}

	int len = -112;
	if (i < 0) {
		i ^= -1L;
		len = -120;
	}

    long tmp = i;
    while (tmp != 0) {
      tmp = tmp >> 8;
      len--;
    }

	if (l >= 1) {
		*p = (char)len;
		p++;
		l -= 1;
	} else {
		return -1;
	}

    len = (len < -120) ? -(len + 120) : -(len + 112);
	if (l < len) return -1;

    for (int idx = len; idx != 0; idx--) {
      int shiftbits = (idx - 1) * 8;
      long mask = 0xFFL << shiftbits;
	  *p = (char)((i & mask) >> shiftbits);
	  p++;
    }
    l -= len;
    
    *dest = p;
    *left = l;
	return 0;
}

int read_vlong(char **src, int *left, long *i)
{
	char *p = *src;
	int l = *left;

	int vlong_size;
	int8_t first_byte = *p;
	if (first_byte >= -112) {
		vlong_size = 1;
	} else if (first_byte < -120) {
		vlong_size = -119 - first_byte;
	} else {
		vlong_size = -111 - first_byte;
	}

	p++;
	l--;

	if (vlong_size == 1) {
		*i = first_byte;
		*src = p;
		*left = l;
		return 0;
	} else if (vlong_size-1 > l) {
		return -1;
	}

	long v = 0;
	for (int k=0; k<vlong_size-1; k++) {
			v = v << 8;
			v = v | (0xFF & *p);
			p++;
			l--;
	}

	if (first_byte < -120 || (first_byte >= -112 && first_byte < 0)) {
		v = v ^ (-1L);
	}

	*i = v;
	*src = p;
	*left = l;
	return 0;
}

