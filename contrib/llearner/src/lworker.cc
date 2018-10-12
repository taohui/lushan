#include "lworker.h"
#include "lutil.h"
#include "lushan.h"
#include "utils.h"
#include "vlong.h"
#include "ps.h"
#include "samples.h"
#include "lworker_net.h"
#include "lworker_evaluate.h"
#include "llog.h"
#include "lconf.h"
#include <libmemcached/memcached.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <assert.h>
#include <pthread.h>
#include <stdexcept>

using namespace std;

void extract_features(const SampleSplit& split, map<uint64_t, FeatureEntry>& features, const LWorkerContext *cxt)
{
    FeatureEntry entry;
    features.insert(pair<uint64_t, FeatureEntry>(0, entry));

    char *p = split.start;
    int left = split.end - split.start;
    for (int j = 0; j < split.sample_count; j++)
    {
        long len = 0;
        long label;
        read_vlong(&p, &left, &len);
        assert(len > 2);
        read_vlong(&p, &left, &label);

        long count;
        read_vlong(&p, &left, &count);

        long index;
        for (int k = 0; k < len - 2; k++)
        {
            read_vlong(&p, &left, &index);
            if (cxt->factor_num > 0) {
		for (int i=0; i < cxt->factor_num + 1; i++)
                    features.insert(pair<uint64_t, FeatureEntry>(index * LWORKER_FACTOR_NUM_MAX + i, entry));
            } else {
                features.insert(pair<uint64_t, FeatureEntry>(index, entry));
            }
        }
    }
}

void *worker_thr(void *args)
{

    LWorkerContext *cxt = (LWorkerContext *)args;

    memcached_return_t rc;
    memcached_server_st *server = memcached_servers_parse(cxt->host.c_str());
    memcached_st *memc = memcached_create(NULL);
    memcached_server_push(memc, server);
    memcached_server_free(server);
    memcached_config(memc);

    char *buf = new char[HREQUEST_LENGTH_MAX+4096];
    char *key_ptr[1];
    size_t len_ptr[1];
    int hrequest_value_length, len;

    char *ret = NULL;
    char ret_key[MEMCACHED_MAX_KEY];
    size_t key_len;
    size_t value_len;
    uint32_t flag;

    map<uint64_t, FeatureEntry> features;
    map<uint64_t, FeatureEntry>::iterator it;
    vector<SampleSplit> all_splits = cxt->samples->split();
    double fx;

    while (1) {
        uint32_t iter = 0;
        pthread_mutex_lock(&cxt->mutex);
        if (cxt->iter >= cxt->max_iterations * all_splits.size()) {
            pthread_mutex_unlock(&cxt->mutex);
            break;
        }
        SampleSplit split = all_splits[cxt->samples->cur_mini_batch()];
        cxt->samples->next_mini_batch();
        cxt->iter++;
        iter = cxt->iter;
	if (cxt->iter % all_splits.size() == 0) {
            // It's not accurate here, fortunately, it is only for display
            llog_write(NULL, LL_NOTICE, "Iter [%ld], fx = [%f], i = [%d]", iter/all_splits.size(), cxt->fx_sum/all_splits.size(), cxt->iter);
	    cxt->fx_sum = 0;
        }
        pthread_mutex_unlock(&cxt->mutex);

	features.clear();
        extract_features(split, features, cxt);

	hrequest_value_length = features.size()*sizeof(uint64_t);
	if (hrequest_value_length >= HREQUEST_LENGTH_MAX - 2) {
            llog_write(NULL, LL_FATAL, "hrequest value length [%d] is too long", hrequest_value_length);
            exit(1);
	}
        len = sprintf(buf, "m%d?op=pull&svr=0&id=%s 0 0 %d\r\n", cxt->hmid, cxt->worker_id.c_str(), hrequest_value_length);
        for (it = features.begin(); it != features.end(); ++it) {
            memcpy(buf+len, &it->first, sizeof(uint64_t));
            len += sizeof(uint64_t);
        }

        key_ptr[0] = buf;
        len_ptr[0] = len;
        rc = memcached_mget(memc, key_ptr, len_ptr, 1);

        key_len = 0;
        value_len = 0;
        if ((ret = memcached_fetch(memc, ret_key, &key_len, &value_len, &flag, &rc)) != NULL)
        {
            if (rc == MEMCACHED_SUCCESS)
            {
                int value_count = value_len/sizeof(KVPair);
                if (value_len % sizeof(KVPair) != 0 || value_count != features.size()) {
                    llog_write(NULL, LL_FATAL, "invalid pull response, value_len [%d], features_size [%d]", value_len, features.size());
                    exit(1);
                }
                for (int j = 0; j < value_count; j++) {
                    KVPair *pairs = (KVPair *)ret;
                    it = features.find(pairs[j].key);
                    if (it == features.end()) {
                        llog_write(NULL, LL_FATAL, "failed to find %lu in features", pairs[j].key);
                        exit(1);
                    } else {
                        it->second.x = pairs[j].value;
                    }
                }
                free(ret);

                fx = cxt->evaluate(split, features, cxt);

                pthread_mutex_lock(&cxt->mutex);
		cxt->fx_sum += fx;
                pthread_mutex_unlock(&cxt->mutex);

		hrequest_value_length = features.size()*sizeof(KVPair);
		if (hrequest_value_length >= HREQUEST_LENGTH_MAX - 2) {
                    llog_write(NULL, LL_FATAL, "hrequest value length [%d] is too long\n", hrequest_value_length);
		    exit(1);
		}
                len = sprintf(buf, "m%d?op=push&fx=%f&svr=0&id=%s 0 0 %d\r\n", cxt->hmid, fx, cxt->worker_id.c_str(), hrequest_value_length);
                for (it = features.begin(); it != features.end(); ++it) {
                    memcpy(buf+len, &it->first, sizeof(uint64_t));
                    memcpy(buf+len+sizeof(uint64_t), &it->second.g, sizeof(double));
                    len += sizeof(KVPair);
                }
                key_ptr[0] = buf;
                len_ptr[0] = len;
                rc = memcached_mget(memc, key_ptr, len_ptr, 1);
                if ((ret = memcached_fetch(memc, ret_key, &key_len, &value_len, &flag, &rc)) != NULL)
                {
                    if (rc == MEMCACHED_SUCCESS)
                    {
                        free(ret);
                    } else {
                        llog_write(NULL, LL_WARNING, "push, rc [%d] is not MEMCACHED_SUCCESS", rc);
			continue;
                    }
                } else {
                    llog_write(NULL, LL_WARNING, "push, failed to fetch result [%d]", rc);
		    continue;
                }
            }
            else
            {
                llog_write(NULL, LL_WARNING, "pull, rc [%d] is not MEMCACHED_SUCCESS", rc);
		continue;
            }
        }
        else
        {
            llog_write(NULL, LL_WARNING, "pull, failed to fetch result [%d]", rc);
            continue;
        }

    }
    delete[] buf;
    memcached_free(memc);

    return NULL;
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stdout, "%s <configure file>\n", argv[0]);
		exit(0);
	}

	uint32_t hmid;
	char host[LCONF_LINE_LEN_MAX];
	if (lconf_read_uint32(argv[1], "hmid", &hmid) != 1 ||
		lconf_read_string(argv[1], "host", host, LCONF_LINE_LEN_MAX) != 1) {
		fprintf(stderr, "failed to read hmid or host from %s.\n", argv[1]);
		exit(1);
	}
	uint32_t mini_batch_size = LWORKER_MINI_BATCH_SIZE_DEFAULT;
	lconf_read_uint32(argv[1], "mini_batch_size", &mini_batch_size);

	uint32_t max_iterations = LWORKER_MAX_ITERATIONS_DEFAULT;
	lconf_read_uint32(argv[1], "max_iterations", &max_iterations);

	uint32_t thr_num = LWORKER_THREAD_NUM_DEFAULT;
	lconf_read_uint32(argv[1], "thread_num", &thr_num);

	uint32_t factor_num = 0;
	if (lconf_read_uint32(argv[1], "factor_num", &factor_num) == 1) {
		fprintf(stderr, "invalid factor_num [%d]\n", factor_num);
        	exit(1);
	}

	char train_dir[LCONF_LINE_LEN_MAX];
	train_dir[0] = '\0';
	lconf_read_string(argv[1], "train_dir", train_dir, LCONF_LINE_LEN_MAX);

	llog_write(NULL, LL_NOTICE, "host [%s], hmid [%u], mini_batch_size [%u], max_iterations [%u], thread_num [%u], factor_num [%u]", 
		host, hmid, mini_batch_size, max_iterations, thr_num, factor_num);

	const char *worker_id = getenv("ML_TASK_ID");
	if(worker_id == NULL) {
		llog_write(NULL, LL_WARNING, "failed to getenv(ML_TASK_ID)");
	}

	Samples samples;
	samples.set_with_count(0);
	samples.set_mini_batch_size(mini_batch_size);
	samples.set_check_bound(false);

	if (train_dir[0] == '\0') {
		string line;
		while (getline(cin, line)) {
			if (samples.load_sample(line) == -1) {
				llog_write(NULL, LL_FATAL, "failed to load sample: %s", line.c_str());
				exit(1);
			}
		}
	} else {
		ReadFileSet fileset(train_dir, "");
		if (fileset.read_dir() == -1 || samples.load_fileset(fileset) == -1) {
			llog_write(NULL, LL_FATAL, "failed to load samples");
			exit(1);
		} 
	}

	LWorkerContext cxt;
	cxt.worker_id = worker_id;
	cxt.samples = &samples;
	cxt.host = host;
	cxt.hmid = hmid;
	cxt.max_iterations = max_iterations;
	cxt.factor_num = factor_num;
	if (cxt.factor_num > 0) {
		cxt.evaluate = fm_evaluate;
	} else {
		cxt.evaluate = lr_evaluate;
	}

	pthread_t *pid = new pthread_t[thr_num];
	for (size_t i=0; i<thr_num; i++) {
		pthread_create(pid+i, NULL, worker_thr, &cxt);
	}
	for (size_t i=0; i<thr_num; i++) {
		pthread_join(pid[i], NULL);
	}

	exit(0);
}
