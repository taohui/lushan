#ifndef LLOG_H
#define LLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define MAX_PATH_LEN 4096

typedef enum {
	LL_ALL,
	LL_DEBUG,
	LL_TRACE,
	LL_NOTICE,
	LL_WARNING,
	LL_FATAL,
	LL_NONE
} llog_level_t;

typedef enum {
	LL_ROTATE_HOUR = 3600,
	LL_ROTATE_DAY = 86400
} llog_rotate_time_t;

typedef struct {
	time_t open_hour;
	llog_rotate_time_t rotate_time;
	char dir[MAX_PATH_LEN];
	char prefix[MAX_PATH_LEN];
	llog_level_t min_level;
	pthread_mutex_t llog_mutex;
	FILE *fp;
} llog_t;

llog_t* llog_open(const char *dir, const char *prefix, llog_level_t min_level, llog_rotate_time_t rotate_time);

int llog_write(llog_t *llog, llog_level_t llog_level, char *format, ... );

int llog_close(llog_t *llog);

#ifdef __cplusplus
}
#endif

#endif
