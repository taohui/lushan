#include "llog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>

#define MAX_LOG_MSG_LEN 4096

llog_t* llog_open(const char *dir, const char *prefix, llog_level_t min_level, llog_rotate_time_t rotate_time)
{
	llog_t *llog = (llog_t *)calloc(1, sizeof(llog[0]));
	if (!llog) {
		return NULL;
	}
	pthread_mutex_init(&llog->llog_mutex, NULL);

	snprintf(llog->dir, MAX_PATH_LEN, "%s", dir);
	snprintf(llog->prefix, MAX_PATH_LEN, "%s", prefix);
	llog->min_level = min_level;
	llog->rotate_time = rotate_time;
	
	time_t now = time(NULL);
	llog->open_hour = now - now % (int)llog->rotate_time;

	char path[MAX_PATH_LEN];
	struct tm tm;
	localtime_r(&llog->open_hour, &tm);
	if (llog->rotate_time == LL_ROTATE_HOUR) {
		snprintf(path, MAX_PATH_LEN, "%s/%s-%d%02d%02d%02d.log", llog->dir, llog->prefix, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
	} else {
		snprintf(path, MAX_PATH_LEN, "%s/%s-%d%02d%02d.log", llog->dir, llog->prefix, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	}
	
	llog->fp = fopen(path, "a");

	if (!llog->fp) {
		free(llog);
		return NULL;
	}

	return llog;
}

int llog_write(llog_t *llog, llog_level_t llog_level, char *format, ... )
{
	if (llog != NULL && llog_level < llog->min_level)
		return 0;

	time_t now;
	va_list args;
	char msg[MAX_LOG_MSG_LEN];
	char *p = msg;
	struct tm tm;
	int len;

	switch (llog_level)
	{
	case LL_DEBUG:
		strcpy(p, "DEBUG ");
		p += 6;
		break;
	case LL_TRACE:
		strcpy(p, "TRACE ");
		p += 6;
		break;
	case LL_NOTICE:
		strcpy(p, "NOTICE ");
		p += 7;
		break;
	case LL_WARNING:
		strcpy(p, "WARNING ");
		p += 8;
		break;
	case LL_FATAL:
		strcpy(p, "FATAL ");
		p += 6;
		break;
	default:
		fprintf(stderr, "Invalid log level '%d'.\n", llog_level);
		return -1;
	}

	now = time(NULL);
	localtime_r(&now, &tm);
	p += sprintf(p, "%d-%02d-%02d %02d:%02d:%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	va_start(args, format);
	len = vsnprintf(p, MAX_LOG_MSG_LEN-(p-msg)-1, format, args);
	if (len >= MAX_LOG_MSG_LEN - (p - msg) - 1)
		len = MAX_LOG_MSG_LEN - (p - msg) - 2;
	p += len;

	if (*(p - 1) != '\n')
		*p++ = '\n';
	*p = '\0';

	if (llog) {
		pthread_mutex_lock(&llog->llog_mutex);

		if (now - llog->open_hour >= (int)llog->rotate_time) {
			fclose(llog->fp);
			char path[MAX_PATH_LEN];
			if (llog->rotate_time == LL_ROTATE_HOUR) {
				snprintf(path, MAX_PATH_LEN, "%s/%s-%d%02d%02d%02d.log", llog->dir, llog->prefix, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
			} else {
				snprintf(path, MAX_PATH_LEN, "%s/%s-%d%02d%02d.log", llog->dir, llog->prefix, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
			}
			llog->fp = fopen(path, "a");

			if (!llog->fp) {
				pthread_mutex_unlock(&llog->llog_mutex);
				return -1;
			}
			llog->open_hour = now - now % (int)llog->rotate_time;
		}

		fprintf(llog->fp, "%s", msg);
		fflush(llog->fp);
		pthread_mutex_unlock(&llog->llog_mutex);
	} else {
		fprintf(stderr, "%s", msg);
	}
	return 0;
}
		
int llog_close(llog_t *llog)
{
	if (llog) {
		pthread_mutex_lock(&llog->llog_mutex);
		fclose(llog->fp);
		pthread_mutex_unlock(&llog->llog_mutex);
		free(llog);
	}
	return 0;
}

