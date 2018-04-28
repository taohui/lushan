/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */
#include "url.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* rfc1738:

   ...The characters ";",
   "/", "?", ":", "@", "=" and "&" are the characters which may be
   reserved for special meaning within a scheme...

   ...Thus, only alphanumerics, the special characters "$-_.+!*'(),", and
   reserved characters used for their reserved purposes may be used
   unencoded within a URL...

   For added safety, we only leave -_. unencoded.
 */

static unsigned char hexchars[] = "0123456789ABCDEF";

/* d buffer size should no less than "len * 3 + 1" */
int url_encode(const char *s, int len, char *d)
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;

    from = s;
    end = s + len;
    start = to = (unsigned char *) d;

    while (from < end) {
        c = *from++;

        if (c == ' ') {
            *to++ = '+';
        }
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') || (c > 'z')) {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
        else {
            *to++ = c;
        }
    }
    *to = 0;
    return to - start;
}

static int htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *) s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *) s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

/* str is also output */
int url_decode(char *str, int len)
{
    char *dest = str;
    char *data = str;

    while (len--) {
        if (*data == '+') {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) *(data + 1))
                 && isxdigit((int) *(data + 2))) {
            *dest = (char) htoi(data + 1);
            data += 2;
            len -= 2;
        }
        else {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return dest - str;
}

url_parser_t *url_parser_create()
{
    url_parser_t *parser = (url_parser_t *)malloc(sizeof(parser[0]));
    if (!parser) return NULL;
    parser->url[0] = '\0';
    parser->parameter_count = 0;
    return parser;
}

void url_parser_destroy(url_parser_t *parser)
{
    if (parser) free(parser);
}

int url_parser_parse(url_parser_t *parser, const char *url)
{
    parser->parameter_count = 0;
    if (strlen(url) >= URL_PARSER_URL_LENGTH_MAX) {
        return -1;
    }
    strcpy(parser->url, url);

    char *ptr = strchr(parser->url, '?');
    if (ptr == NULL) return -1;
    ptr++;
    int count = 0;
    while (1) {
        char *name_start = ptr;
        while (*ptr != '\0' && *ptr != '=') ptr++;
        if (*ptr == '\0') break;

        *ptr = '\0';
        parser->parameter_name[count++] = name_start;

        ptr++;
        char *value_start = ptr;

        while (*ptr != '\0' && *ptr != '&') ptr++;
        if (*ptr == '\0') {
                parser->parameter_value[count-1] = value_start;
                break;
        } else {
                *ptr = '\0';
                ptr++;
                parser->parameter_value[count-1] = value_start;
        }
    }
    parser->parameter_count = count;
    return parser->parameter_count;
}

int url_parser_get(url_parser_t *parser, const char *parameter_name, char *parameter_value)
{
    int i = 0;
    while (i < parser->parameter_count) {
        if (strcmp(parameter_name, parser->parameter_name[i]) == 0) {
            strcpy(parameter_value, parser->parameter_value[i]);
            return 1;
        }
        i++;
    }

    return 0;
}
 
int url_parser_get_int64(url_parser_t *parser, const char *parameter_name, int64_t *parameter_value)
{
    int i = 0;
    while (i < parser->parameter_count) {
        if (strcmp(parameter_name, parser->parameter_name[i]) == 0) {
            *parameter_value = strtoll(parser->parameter_value[i], NULL, 10);
            return 1;
        }
	i++;
    }

    return 0;
}

int url_parser_get_char(url_parser_t *parser, const char *parameter_name, char *parameter_value)
{
    int i = 0;
    while (i < parser->parameter_count) {
        if (strcmp(parameter_name, parser->parameter_name[i]) == 0) {
            if (parser->parameter_value[i][0] != 0 && parser->parameter_value[i][1] == '\0');
            *parameter_value = parser->parameter_value[i][0];
            return 1;
        }
	i++;
    }

    return 0;
}
