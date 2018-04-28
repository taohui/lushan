#include "lushan.h"
#include "lconf.h"
#include "llog.h"
#include "lutil.h"
#include "url.h"
#include "hmod.h"
#include "hdict.h"
#include "hiredis/hiredis.h"
#include <pthread.h>
#include <string>
#include <queue>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

#define TIMEOUT_DEFAULT 3
#define BUF_LEN 8192

using namespace std;

class LProxyContext
{
public:
    LProxyContext(const char *host, int port, llog_t *llog, int timeout) : host_(host),
        port_(port), llog_(llog) {
        tv_.tv_sec = 0;
        tv_.tv_usec = 1000 * timeout;
        pthread_mutex_init(&mutex_, NULL);
    }

    ~LProxyContext() {
        pthread_mutex_lock(&mutex_);
        while (!redisContext_.empty()) {
            redisContext *redis = redisContext_.front();
            redisContext_.pop();
            redisFree(redis);
        }
        llog_close(llog_);
        pthread_mutex_unlock(&mutex_);
    }

    llog_t *get_llog() { return llog_; }

    redisContext *get_redis() {
        redisContext *redis = NULL;
        pthread_mutex_lock(&mutex_);
        if (!redisContext_.empty()) {
            redis = redisContext_.front();
            redisContext_.pop();
        } else {
            redis = redisConnectWithTimeout(host_.c_str(), port_, tv_);
        }
        pthread_mutex_unlock(&mutex_);
        return redis;
    }

    void put_redis(redisContext *redis) {
        pthread_mutex_lock(&mutex_);
        redisContext_.push(redis);
        pthread_mutex_unlock(&mutex_);
    }
private:
    string host_;
    uint32_t port_;
    llog_t *llog_;
    struct timeval tv_;
    queue<redisContext * > redisContext_;
    pthread_mutex_t mutex_;
};
   

extern "C" int hmodule_open(const char *path, void **xdata)
{
    char config[BUF_LEN];
    snprintf(config, BUF_LEN, "%s/hmodule.conf", path);

    int ret;
    char host[LCONF_LINE_LEN_MAX];
    ret = lconf_read_string(config, "host", host, LCONF_LINE_LEN_MAX);
    if (ret != 1) {
        fprintf(stderr, "failed to read host from %s", config);
        return -1;
    }

    uint32_t port;
    ret = lconf_read_uint32(config, "port", &port);
    if (ret != 1) {
        fprintf(stderr, "failed to read port from %s", config);
        return -1;
    }

    uint32_t timeout = TIMEOUT_DEFAULT;
    /* if can't find "timeout", the variable timeout will keep what it is */
    lconf_read_uint32(config, "timeout", &timeout);

    llog_level_t level = LL_WARNING;
    char level_buf[LCONF_LINE_LEN_MAX];
    if (lconf_read_string(config, "llog_level", level_buf, LCONF_LINE_LEN_MAX) 
            == 1) {
                if (strcasecmp(level_buf, "all") == 0) level = LL_ALL;
                else if (strcasecmp(level_buf, "debug") == 0) level = LL_DEBUG;
                else if (strcasecmp(level_buf, "trace") == 0) level = LL_TRACE;
                else if (strcasecmp(level_buf, "notice") == 0) level = LL_NOTICE;
    }

    llog_t *llog;
    if ((llog = llog_open(path, "lproxy", level, LL_ROTATE_DAY)) == NULL) {
        fprintf(stderr, "failed to open log\n");
        return -1;
    }
    LProxyContext *xd = new LProxyContext(host, port, llog, timeout);

    *xdata = xd;
    return 0;
}

static int hdb_query(hdb_t *hdb, uint32_t hdid, uint64_t key, char *buf)
{
    int res;
    off_t off;
    uint32_t length = 0;

    hdict_t *hdict;
    hdict = hdb_ref(hdb, hdid);
    if (hdict != NULL) {
        hdict->num_qry++;
        if (hdict_seek(hdict, key, &off, &length)) {
            if (length >= BUF_LEN) {
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

extern "C" void hmodule_close(void *xdata)
{
    if (xdata) {
        LProxyContext *xd = (LProxyContext *)xdata;
        delete xd;
    }
    return;
}

extern "C" int hmodule_handle(const char *req, char **outbuf, int *osize, int *obytes, void **xdata, hdb_t *hdb)
{
    int ret;
    LProxyContext *cxt = (LProxyContext *)(*xdata);
    llog_t *llog = cxt->get_llog();

    url_parser_t url_parser;

    if (url_parser_parse(&url_parser, req) == -1) {
        llog_write(llog, LL_WARNING, "failed to parse [%s]", req);
        return 0;
    }

    char key_parameter[URL_PARSER_URL_LENGTH_MAX];
    key_parameter[0] = '\0';
    url_parser_get(&url_parser, "k", key_parameter);
    if (key_parameter[0] == '\0') {
        llog_write(llog, LL_WARNING, "invalid parameter [%s]", req);
        return 0;
    }
    
    int hdid = 0;
    uint64_t key;
    char *p = strchr(key_parameter, '-');
    if (!p) {
        key = strtoull(key_parameter, NULL, 10);
    } else {
        *p = '\0';
        hdid = atoi(key_parameter);
        key = strtoull(p+1, NULL, 10);
    }

    redisContext *redis = cxt->get_redis();
    if (!redis) {
        return 0;
    }

    redisReply *reply;
    reply = (redisReply *)redisCommand(redis, "SELECT %d", hdid);
    bool should_return = false;
    if (reply) {
        freeReplyObject(reply);
        reply = (redisReply *)redisCommand(redis, "GET %llu", key);
        if (reply) {
		if (reply->type == REDIS_REPLY_STRING) {
			lpack_ascii(req, outbuf, osize, obytes, reply->len, reply->str);
			should_return = true;
		}
		freeReplyObject(reply);
        }
    }
    cxt->put_redis(redis);
    if (should_return) return 0;

    char buf[BUF_LEN];
    ret = hdb_query(hdb, hdid, key, buf);
    if (ret > 0) {
        lpack_ascii(req, outbuf, osize, obytes, ret, buf);
    }
    return 0;
}
