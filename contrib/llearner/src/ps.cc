#include "ps.h"
#include "ps_ftrl.h"
#include "ps_adam.h"
#include "lushan.h"
#include "lconf.h"
#include "llog.h"
#include "lutil.h"
#include "url.h"
#include "hmod.h"
#include "hdict.h"
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <tr1/unordered_map>
#include <fstream>
#include <sstream>
#include <map>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

#define BUF_LEN 8192

using namespace std;
using namespace std::tr1;

extern "C" int hmodule_open(const char *path, void **xdata)
{
    char config[BUF_LEN];
    snprintf(config, BUF_LEN, "%s/hmodule.conf", path);

    int ret;
    char optimizer[LCONF_LINE_LEN_MAX];    
    ret = lconf_read_string(config, "optimizer", optimizer, LCONF_LINE_LEN_MAX);
    if (ret != 1) {
        fprintf(stderr, "failed to read optimizer from %s", config);
        return -1;
    }

    ModelType model_type = LR;
    char model[LCONF_LINE_LEN_MAX];    
    lconf_read_string(config, "model", model, LCONF_LINE_LEN_MAX);
    if (strcmp(model, "fm") == 0) model_type = FM;

    llog_t *llog;
    if ((llog = llog_open_with_conf(path, "ps", config)) == NULL) {
        fprintf(stderr, "failed to open log\n");
        return -1;
    }

    bool have_state = false;
    char filepath[BUF_LEN];
    snprintf(filepath, BUF_LEN, "%s/%s", path, STATE_FILE_NAME);
    if (access(filepath, F_OK) == 0) have_state = true;

    bool use_tbb = false;
#ifdef USE_TBB
    use_tbb = true;
#endif

    ParameterServer *xd;
    if (strcmp(optimizer, "ftrl") == 0) {
        FTRLParam param;
        lconf_read_double(config, "l1", &param.l1);
        lconf_read_double(config, "l2", &param.l2);
        lconf_read_double(config, "alpha", &param.alpha);
        lconf_read_double(config, "beta", &param.beta);

        llog_write(llog, LL_DEBUG, "l1 [%f], l2 [%f], alpha [%f], beta [%f], use_tbb [%d]", param.l1, param.l2, param.alpha, param.beta, use_tbb);
        xd = new FTRLParameterServer(param, llog, path);
    } else if (strcmp(optimizer, "adam") == 0) {
        ADAMParam param;
        lconf_read_double(config, "l1", &param.l1);
        lconf_read_double(config, "l2", &param.l2);
        lconf_read_double(config, "alpha", &param.alpha);
        lconf_read_double(config, "beta1", &param.beta1);
        lconf_read_double(config, "beta2", &param.beta2);
        lconf_read_double(config, "epsilon", &param.epsilon);

        llog_write(llog, LL_DEBUG, "l1 [%f], l2 [%f], alpha [%f], beta1 [%f], beta2 [%f], epsilon [%f], use_tbb [%d]", param.l1, param.l2, param.alpha, param.beta1, param.beta2, param.epsilon, use_tbb);
        xd = new ADAMParameterServer(param, llog, path);
    } else {
        llog_write(llog, LL_FATAL, "invalid optimizer, only support ftrl or adam");
        llog_close(llog);
        return -1;
    }

    if (have_state && xd->read_state(STATE_FILE_NAME) != 0) {
        llog_write(llog, LL_FATAL, "failed to read %s", STATE_FILE_NAME);
        delete xd;
        return -1;
    }

    xd->set_model_type(model_type);

    *xdata = xd;
    return 0;
}

extern "C" void hmodule_close(void *xdata)
{
    if (xdata) {
        ParameterServer *xd = (ParameterServer *)xdata;
        delete xd;
    }
    return;
}

extern "C" int hmodule_handle(const char *req, char **outbuf, int *osize, int *obytes, void **xdata, hdb_t *hdb)
{

    ParameterServer *ps = (ParameterServer *)(*xdata);
    llog_t *llog = ps->get_llog();
    const hrequest_t *r = (hrequest_t *)req;

    url_parser_t url_parser;
    
    if (url_parser_parse(&url_parser, r->key) == -1) {
        llog_write(llog, LL_WARNING, "failed to parse [%s]", r->key);
        return 0;
    }

    int key_len = strlen(r->key);

    char op[URL_PARSER_URL_LENGTH_MAX];
    char worker_id[URL_PARSER_URL_LENGTH_MAX];    
    op[0] = '\0';
    worker_id[0] = '\0';
    double fx = 0.0;
    url_parser_get(&url_parser, "op", op);
    url_parser_get(&url_parser, "id", worker_id);
    url_parser_get_double(&url_parser, "fx", &fx);
    time_t expire_time = 0;
    url_parser_get_int64(&url_parser, "exp", &expire_time);

    if (strcmp(op, "pull") == 0) {
        if (r->value_len % sizeof(uint64_t) != 0) {
	    llog_write(llog, LL_DEBUG, "invalid pull request length, len is %d, str is %s", r->value_len, r->key);
            return 0;
        }

        lpack_header(r->key, key_len, outbuf, osize, obytes, r->value_len*2); 
        int len = r->value_len / sizeof(uint64_t);
        uint64_t *key = (uint64_t *)r->value;
#ifndef USE_TBB
        ps->lock();
#endif
        for (int i=0; i<len; i++) {
            double x = ps->get(key[i]);
            lpack_append_body(*outbuf, obytes, (char *)(key+i), sizeof(uint64_t));
            lpack_append_body(*outbuf, obytes, (char *)&x, sizeof(double));
        }
#ifndef USE_TBB
        ps->unlock();
#endif
        lpack_footer(*outbuf, obytes);
        return 0;
    } else if (strcmp(op, "push") == 0) {
        if (r->value_len % sizeof(KVPair) != 0) {
            return 0;
        }
        int len = r->value_len / sizeof(KVPair);
        KVPair *pair = (KVPair *)r->value;

#ifndef USE_TBB
        ps->lock();
#endif
        time_t timestamp = time(NULL);
        for (int i=0; i<len; i++) {
            ps->update(pair[i].key, pair[i].value, timestamp);
        }
#ifndef USE_TBB
        ps->unlock();
#endif

        lpack_ascii(r->key, outbuf, osize, obytes, 2, "OK");
        return 0;
    } else if (strcmp(op, "status") == 0) {
        char status[1024];
        int len = snprintf(status, 1024, "xnorm = %f, gnorm = %f, x[0] = %f", ps->xnorm(), ps->gnorm(), ps->get(0));
        lpack_ascii(r->key, outbuf, osize, obytes, len, status);
        return 0;
    } else if (strcmp(op, "write_weights") == 0) {

        ps->write_weights("weights.txt");
        
    } else if (strcmp(op, "write_state") == 0) {
        
        ps->write_state(STATE_FILE_NAME);

    } else if (strcmp(op, "reset") == 0) {

#ifndef USE_TBB
        ps->lock();
#endif
        ps->reset();
        ps->remove_file(STATE_FILE_NAME);
#ifndef USE_TBB
        ps->unlock();
#endif

    } else if (strcmp(op, "remove_expired") == 0) {
        
        if (expire_time == 0) lpack_ascii(r->key, outbuf, osize, obytes, 5, "ERROR");
        else {
#ifndef USE_TBB
            ps->lock();
#endif
            ps->remove_expired(time(NULL) - expire_time);
#ifndef USE_TBB
            ps->unlock();
#endif
        }
    }

    return 0;
}
