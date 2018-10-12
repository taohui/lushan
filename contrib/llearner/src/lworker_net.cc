#include "lworker_net.h"
#include <libmemcached/memcached.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

using namespace std;

static uint32_t my_hashkit_hash_fn(const char *key, size_t key_length, void *context)
{
    uint64_t svr_count = (uint64_t)context;
    const char *ptr = strstr(key, "svr=");
    if (ptr) {
        ptr += 4;
    } else {
        throw logic_error("invalid key, can't find svr parameter.");
    }
    int k = atoi(ptr);

    return k % svr_count;
}

void memcached_config(memcached_st *memc)
{
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 6000); // in milliseconds
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 6000); // in milliseconds
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 6000000); // in usec
    // retry after MEMCACHED_BEHAVIOR_RETRY_TIMEOUT seconds
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 0);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 6000000); // in usec

    int svr_count = memcached_server_count(memc);
    hashkit_set_custom_function(&memc->hashkit, my_hashkit_hash_fn, (void *)svr_count);

    memcached_behavior_set_distribution(memc, MEMCACHED_DISTRIBUTION_MODULA);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_VERIFY_KEY, 0);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);
}

