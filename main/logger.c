/*
 * logger.c
 *
 *  Created on: 11.11.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

#define N_LOG_ENTRIES 50
#define LEN_LOG 256

typedef enum {
	LL_ERROR,
	LL_WARNING,
	LL_INFO,
	LL_DEBUG
} t_loglevel;


static char logtable[N_LOG_ENTRIES][LEN_LOG];
static int log_level = 99;
static pthread_mutex_t loggingMutex;
static int write_idx=0;

void get_current_timestamp(char tbuf, size_t sz_tbuf) {
	struct timeval tv;
	struct tm *zeit;
	char tformat[32];

	gettimeofday(&tv, NULL);
	zeit = localtime(&tv.tv_sec);
	snprintf(tformat, sizeof(tformat), "%%Y-%%m-%%d %%H:%%M:%%S.%06ld", tv.tv_usec);
	strftime(tbuf, sz_tbuf, tformat, zeit);

}


static void log_doit(int level, const char *fmt, va_list ap)
{
	// soll überhaupt geloggt werden
	if ( level > log_level )
		return; // nothing todo

	pthread_mutex_lock(&loggingMutex);

	char *buf = &(logtable[write_idx]);



	char header[100];
	char tbuf[80];

	vsnprintf(buf, sizeof(buf), fmt, ap);

		fill_tbuf( tbuf,sizeof(tbuf),SVloggingLogfile, logfilename, sizeof(logfilename));
		snprintf(header,sizeof(header),"%s%-6s(%d/%lu): ", tbuf, LOGLEVEL2TEXT(level), getpid(), getThreadId() );

		if ( (f = strlen(logfilename) ? fopen( logfilename, "a") : stderr)) {
			fprintf(f, "%s %s\n", header, buf);
			// nicht den stderr zu machen
			if ( strlen(logfilename) )
				fclose(f);
		} else {
			// konnte logfile nicht öffnen, probiere einen Ersatz
			char ftxt[PATH_MAX+256];
			snprintf(ftxt,sizeof(ftxt),"open %s failed, errno %d(%s)", logfilename, errno, strerror(errno));
			notfallLog("[%s] %s %s\n",ftxt,header,buf);
		}

	pthread_mutex_unlock(&loggingMutex);
}

void init_logging(int initial_log_level) {
	ESP_LOGI(__func__, "start");

	memset(logtable, 0, sizeof(logtable));
	log_level=initial_log_level;
	write_idx = 0;
	pthread_mutex_init( &loggingMutex, NULL);
}
