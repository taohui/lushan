#ifndef URL_H
#define URL_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define URL_PARSER_URL_LENGTH_MAX 1024
#define URL_PARSER_PARAMETER_COUNT_MAX 128

    typedef struct _url_parser_t {
        char url[URL_PARSER_URL_LENGTH_MAX];
        char *parameter_name[URL_PARSER_PARAMETER_COUNT_MAX];
        char *parameter_value[URL_PARSER_PARAMETER_COUNT_MAX];
        int parameter_count;
    } url_parser_t;

    int url_parser_parse(url_parser_t *parser, const char *url);

    int url_parser_get(url_parser_t *parser, const char *parameter_name, char *parameter_value);
    int url_parser_get_int64(url_parser_t *parser, const char *parameter_name, int64_t *parameter_value);
    int url_parser_get_char(url_parser_t *parser, const char *parameter_name, char *parameter_value);

/* d buffer size should no less than "len * 3 + 1" */
    int url_encode(const char *s, int len, char *d);

/* str is also output */
    int url_decode(char *str, int len);

#ifdef __cplusplus
}
#endif

#endif
