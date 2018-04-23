#include "hmod.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

#define LOCK(hmods)   pthread_mutex_lock(&(hmods)->mutex)
#define UNLK(hmods)   pthread_mutex_unlock(&(hmods)->mutex)

int get_hmod_meta(const char *path, hmod_meta_t *hmod_meta)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
	return -1;
    }

    char line[LINE_SIZE];

    while (fgets(line, LINE_SIZE, fp)) {

	if (strncasecmp(line, "version", 7) == 0) {
		int i = 7;
		while((line[i] && line[i] != '\r' && line[i] != '\n') && 
			(line[i] == ' ' || line[i] == ':' || line[i] == '=')) i++;
		if (line[i] && line[i] != '\r' && line[i] != '\n')
			hmod_meta->version = atoi(line+i);
	} else if (strncasecmp(line, "label", 5) == 0) {
		int i = 5;
		while((line[i] && line[i] != '\r' && line[i] != '\n') && 
			(line[i] == ' ' || line[i] == ':' || line[i] == '=')) i++;
		int j = 0;
		while(line[i] && line[i] != '\r' && line[i] != '\n' && j < 20) {
			hmod_meta->label[j++] = line[i++];
		}
		hmod_meta->label[j] = '\0';
	}
    }

    fclose(fp);

    return 0;
}

hmod_t* hmod_open(const char *path, int *hmod_errnop)
{
	char pathname[256];

	hmod_t *hmod = (hmod_t *)calloc(1, sizeof(hmod[0]));
	hmod->path = strdup(path);

	snprintf(pathname, sizeof(pathname), "%s/hmodule.so", hmod->path);

	hmod->handle = dlopen(pathname, RTLD_NOW);
	if (!hmod->handle) {
		fprintf(stderr, "dl cant open\n");
		*hmod_errnop = EHMOD_BAD_FILE;
		goto error;
	}

	char *error;

	dlerror();
	hmod->open_fun = dlsym(hmod->handle, "hmodule_open");
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "no open\n");
		*hmod_errnop = EHMOD_BAD_FILE;
		goto error;
	}
	hmod->close_fun = dlsym(hmod->handle, "hmodule_close");
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "no close\n");
		*hmod_errnop = EHMOD_BAD_FILE;
		goto error;
	}
	hmod->handle_fun = dlsym(hmod->handle, "hmodule_handle");
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "no handle\n");
		*hmod_errnop = EHMOD_BAD_FILE;
		goto error;
	}

	if (hmod->open_fun != NULL) {
		int ret = (*hmod->open_fun)(hmod->path, &hmod->xdata);
		if (ret) {
			fprintf(stderr, "open func fail\n");
			*hmod_errnop = EHMOD_BAD_FILE;
			goto error;
		}
	}
	hmod->open_time = time(NULL);

	// meta
	snprintf(pathname, sizeof(pathname), "%s/meta", hmod->path);

	hmod_meta_t *hmod_meta = (hmod_meta_t*)calloc(1, sizeof(hmod_meta_t));
	if (hmod_meta == NULL) {
	    *hmod_errnop = EHMOD_OUT_OF_MEMERY;
	    goto error;
	}

	get_hmod_meta(pathname, hmod_meta);
	hmod->hmod_meta = hmod_meta;

	return hmod;

error:
	if (hmod) hmod_close(hmod);
	return NULL;
}

void hmod_close(hmod_t *hmod)
{
	if (hmod->close_fun) (*hmod->close_fun)(hmod->xdata);
	if (hmod->handle) dlclose(hmod->handle);
	free(hmod->path);
	free(hmod);
}

void *hmods_mgr(void *arg)
{
	hmods_t *hmods = (hmods_t *)arg;

	for (;;) {
		if (TAILQ_FIRST(&hmods->close_list)) {
			hmod_t *hmod, *next;
			hmod_t *hmod_list[100];
			int i, k = 0;

			LOCK(hmods);
			for (hmod = TAILQ_FIRST(&hmods->close_list); hmod; hmod = next) {
				next = TAILQ_NEXT(hmod, link);
				if (hmod->ref == 0) {
					hmod_list[k++] = hmod;
					TAILQ_REMOVE(&hmods->close_list, hmod, link);
					hmods->num_close--;
					if (k == 100)
						break;
				}
			}
			UNLK(hmods);

			for (i = 0; i < k; ++i) {
				hmod = hmod_list[i];
				hmod_close(hmod);
			}
		}
		sleep(1);
	}
	return NULL;
}

int hmods_init(hmods_t *hmods)
{
	if (pthread_mutex_init(&hmods->mutex, NULL)) return -1;
	TAILQ_INIT(&hmods->open_list);
	TAILQ_INIT(&hmods->close_list);
	hmods->num_open = 0;
	hmods->num_close = 0;

	int i;
	LIST_HEAD(, hmod_t) hmod_tab[HMOD_TAB_SIZE];
	for (i = 0; i < HMOD_TAB_SIZE; i++) {
		LIST_INIT(hmod_tab + i);
	}
	return 0;
}

int hmods_reopen(hmods_t *hmods, const char *hmod_path, uint32_t hmid)
{
	hmod_t *hd, *next;
	uint32_t hash;
	char rpath[1024];
	realpath(hmod_path, rpath);

	int hmod_errno = 0;
	hmod_t *hmod = hmod_open(rpath, &hmod_errno);
	if (hmod == NULL) return hmod_errno;

	LOCK(hmods);
	for (hd = TAILQ_FIRST(&hmods->open_list); hd; hd = next) {
		next = TAILQ_NEXT(hd, link);
		if (hd->hmid == hmid) {
			LIST_REMOVE(hd, h_link);
			TAILQ_REMOVE(&hmods->open_list, hd, link);
			hmods->num_open--;
			TAILQ_INSERT_TAIL(&hmods->close_list, hd, link);
			hmods->num_close++;
			break;
		}
	}
	hmod->hmid = hmid;
	TAILQ_INSERT_TAIL(&hmods->open_list, hmod, link);
	hash = HMOD_HASH(hmod->hmid);
	LIST_INSERT_HEAD(&hmods->hmod_tab[hash], hmod, h_link);
	hmods->num_open++;
	UNLK(hmods);

	return 0;
}

int hmods_close(hmods_t *hmods, uint32_t hmid)
{
	int found = 0;
	hmod_t *hd, *next;
	LOCK(hmods);
	for (hd = TAILQ_FIRST(&hmods->open_list); hd; hd = next) {
		next = TAILQ_NEXT(hd, link);
		if (hd->hmid == hmid) {
			LIST_REMOVE(hd, h_link);
			TAILQ_REMOVE(&hmods->open_list, hd, link);
			hmods->num_open--;
			TAILQ_INSERT_TAIL(&hmods->close_list, hd, link);
			hmods->num_close++;
			found = 1;
			break;
		}
	}
	UNLK(hmods);

	return found;
}

int hmods_info(hmods_t *hmods, char *buf, int size)
{
	int len = 0;
	len += snprintf(buf+len, size-len, "%2s %20s %5s %3s %9s %13s %s\n", 
			"id", "label", "state", "ref", "num_qry", "open_time", "path");
	if (len < size) 
		len += snprintf(buf+len, size-len, "----------------------------------------------------------------\n");
	int pass, k;
	hmod_t *hmod;
	LOCK(hmods);
	for (pass = 0; pass < 2; ++pass) {
		const char *state;
		struct hmod_list_t *hlist;
		switch (pass) {
		case 0:
			state = "OPEN";
			hlist = &hmods->open_list;
			break;
		case 1:
			state = "CLOSE";
			hlist = &hmods->close_list;
			break;
		default:
			state = NULL;
			hlist = NULL;
		}

		k = 0;
		for (hmod = TAILQ_FIRST(hlist); hmod; hmod = TAILQ_NEXT(hmod, link)) {
			++k;
			if (len < size) {
				struct tm tm;
				localtime_r(&hmod->open_time, &tm);
				len += snprintf(buf+len, size-len, "%2d %20s %5s %2d %10d %02d%02d%02d-%02d%02d%02d %s\n",
					hmod->hmid,
					hmod->hmod_meta->label,
					state,
					hmod->ref,
					hmod->num_qry,
					tm.tm_year - 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
					hmod->path);
			}
		}
	}
	UNLK(hmods);
	return len;
}

hmod_t *hmods_ref(hmods_t *hmods, uint32_t hmid)
{
	hmod_t *hmod = NULL;
	hmod_t *hd;
	LOCK(hmods);
	uint32_t hash = HMOD_HASH(hmid);
	for (hd = LIST_FIRST(&hmods->hmod_tab[hash]); hd; hd = LIST_NEXT(hd, h_link)) {
		if (hd->hmid == hmid) {
			hd->ref++;
			hmod = hd;
			break;
                }
	}
	UNLK(hmods);
	return hmod;
}

int hmods_deref(hmods_t *hmods, hmod_t *hmod)
{
	LOCK(hmods);
	hmod->ref--;
	UNLK(hmods);
	return 0;
}
