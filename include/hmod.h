#ifndef HMOD_H
#define HMOD_H

#include "hdict.h"
#include <sys/queue.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define LINE_SIZE 1024
#define BUF_SIZE 255
#define LABEL_SIZE 13

typedef int open_functor_t(const char *path, void **xdata);
typedef void close_functor_t(void *xdata);
typedef int handle_functor_t(const char *req, char **outbuf, int *osize, int *obytes, void **xdata, hdb_t *hdb);

typedef struct {
        uint32_t version;
        char label[21];
} hmod_meta_t;

typedef struct hmod_t hmod_t;
struct hmod_t {
	TAILQ_ENTRY(hmod_t) link;
	LIST_ENTRY(hmod_t) h_link;
	char *path;
	void *handle;
	open_functor_t *open_fun;
	close_functor_t *close_fun;
	handle_functor_t *handle_fun;
	void *xdata;
	time_t open_time;
	uint32_t num_qry;
	uint32_t ref;
	uint32_t hmid;
    hmod_meta_t *hmod_meta;
};

TAILQ_HEAD(hmod_list_t, hmod_t);

#define HMOD_TAB_SIZE   1024
#define HMOD_HASH(mod_id)  ((mod_id) % HMOD_TAB_SIZE)

typedef struct hmods_t hmods_t;
struct hmods_t {
	pthread_mutex_t mutex;
	struct hmod_list_t open_list;
	struct hmod_list_t close_list;
	int num_open;
	int num_close;
	LIST_HEAD(, hmod_t) hmod_tab[HMOD_TAB_SIZE];
};

#define EHMOD_OUT_OF_MEMERY	-1
#define EHMOD_BAD_FILE		-2

#ifdef __cplusplus
extern "C" {
#endif

hmod_t* hmod_open(const char *path, int *hmod_errnop);

void hmod_close(hmod_t *hmod);

void *hmods_mgr(void *arg);

int hmods_init(hmods_t *hmods);

int hmods_reopen(hmods_t *hmods, const char *hmod_path, uint32_t hmid);

int hmods_close(hmods_t *hmods, uint32_t hmid);

int hmods_info(hmods_t *hmods, char *buf, int size);

hmod_t *hmods_ref(hmods_t *hmods, uint32_t hdid);

int hmods_deref(hmods_t *hmods, hmod_t *hmod);

#ifdef __cplusplus
}
#endif

#endif
