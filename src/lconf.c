#include "lconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

int lconf_read_string(const char *filepath, const char *name, char *valuebuf,
                     int value_buf_size)
{
    FILE *fp = NULL;
    char line[LCONF_LINE_LEN_MAX];
    char namebuf[LCONF_LINE_LEN_MAX];
    char _valuebuf[LCONF_LINE_LEN_MAX];

    if ((fp = fopen(filepath, "r")) == NULL)
        return -1;

    int i;
    for (;;) {
        if (!fgets(line, LCONF_LINE_LEN_MAX, fp)) {
            if (feof(fp))
                break;
            else
                return -1;
        }
        char *ptr = line;
        while (isspace(*ptr))
            ptr++;

        if (*ptr != '#') {
            i = 0;
            while (*ptr != '\0' && !isspace(*ptr) 
                && *ptr != '=' && *ptr != ':') {
                    namebuf[i++] = *ptr;
                    ptr++;
            }
            namebuf[i] = '\0';
		//printf("%s\n", namebuf);
            if (strcmp(namebuf, name)) continue;

            while (isspace(*ptr))
                ptr++;

            if (*ptr != '=' && *ptr != ':') continue;
            else ptr++;

            while (isspace(*ptr))
                ptr++;

            i = 0;
            while (*ptr != '\0' && !isspace(*ptr)) {
                    _valuebuf[i++] = *ptr;
                    ptr++;
            }
            _valuebuf[i] = '\0';
            strncpy(valuebuf, _valuebuf, value_buf_size);

            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int lconf_read_int32(const char *filepath, const char *name,
                    int32_t * valuebuf)
{
    char _valuebuf[LCONF_LINE_LEN_MAX];
    int rtn = lconf_read_string(filepath, name, _valuebuf, LCONF_LINE_LEN_MAX);
    if (rtn <= 0)
        return rtn;

    return sscanf(_valuebuf, "%d", valuebuf);
}

int lconf_read_uint32(const char *filepath, const char *name,
                     uint32_t * valuebuf)
{
    char _valuebuf[LCONF_LINE_LEN_MAX];
    int rtn = lconf_read_string(filepath, name, _valuebuf, LCONF_LINE_LEN_MAX);
    if (rtn <= 0)
        return rtn;

    return sscanf(_valuebuf, "%u", valuebuf);
}

int lconf_read_int64(const char *filepath, const char *name,
                    int64_t * valuebuf)
{
    char _valuebuf[LCONF_LINE_LEN_MAX];
    int rtn = lconf_read_string(filepath, name, _valuebuf, LCONF_LINE_LEN_MAX);
    if (rtn <= 0)
        return rtn;

    return sscanf(_valuebuf, "%lld", valuebuf);
}

#ifdef TESTCONF
int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s <filepath>\n", argv[0]);
		exit(1);
	}
	int i = 0;
	int rtn = lconf_read_int32(argv[1], "curr2", &i);
	fprintf(stdout, "rtn [%d], i = %d\n", rtn, i);
	exit(0);
}
#endif
