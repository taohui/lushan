#include <libmemcached/memcached.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stdout, "%s <host> <port>\n", argv[0]);
        exit(0);
    }

    memcached_return_t rc;
    memcached_st *memc = memcached_create(NULL);
    memcached_server_st *server = memcached_server_list_append(NULL, argv[1], atoi(argv[2]), &rc);
    memcached_server_push(memc, server);
    memcached_server_free(server);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 120);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 120);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 120);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 120);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_VERIFY_KEY, 0);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);


    char key[] = "m14?k=168";
    char value[] = "m14?k=168 0 0 14\r\nthis is a test";
    char *key_ptr[] = { value };
    size_t len_ptr[] = { strlen(value) };
    rc = memcached_mget(memc, key_ptr, len_ptr, 1);

    char *ret = NULL;
    char ret_key[MEMCACHED_MAX_KEY];
    size_t key_len = 0;
    size_t value_len = 0;
    uint32_t flag;
    if ((ret = memcached_fetch(memc, ret_key, &key_len, &value_len, &flag, &rc)) != NULL) {
        if (rc == MEMCACHED_SUCCESS) {
            ret_key[key_len] = '\0';
            ret[value_len] = '\0';
            fprintf(stdout, "key [%s], key_len [%d], value [%s] value_len [%d]\n", 
                ret_key, key_len, ret, value_len);
        } else {
		    fprintf(stderr, "return [%d]\n" ,rc);
	    }
    } else {
		fprintf(stderr, "return [%d]\n" ,rc);
	}
    memcached_free(memc);
    exit(0);
}
