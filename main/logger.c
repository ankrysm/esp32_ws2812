/*
 * logger.c
 *
 *  Created on: 11.11.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

extern T_LOG_ENTRY logtable[];
extern int log_write_idx;

static esp_log_level_t log_level = ESP_LOG_VERBOSE;

static  SemaphoreHandle_t xLogSemaphore = NULL;
static 	TickType_t xLogSemDelay = 5000 / portTICK_PERIOD_MS;


esp_err_t obtain_logsem_lock() {
	if( xSemaphoreTake( xLogSemaphore, xLogSemDelay ) == pdTRUE ) {
		return ESP_OK;
	}
	ESP_LOGE(__func__, "xSemaphoreTake failed");
	return ESP_FAIL;
}

esp_err_t release_logsem_lock(){
	if (xSemaphoreGive( xLogSemaphore ) == pdTRUE) {
		return ESP_OK;
	}
	ESP_LOGE(__func__, "xSemaphoreGive failed");
	return ESP_FAIL;
}


static void log_doit(esp_log_level_t level, const char *func, const char *fmt, va_list ap) {

	if ( level > log_level )
		return; // nothing todo

	if( obtain_logsem_lock() != ESP_OK ) {
		ESP_LOGE(__func__, "xSemaphoreTake failed");
		return;
	}

	T_LOG_ENTRY *buf = &(logtable[log_write_idx]);
	buf->level = level;
	gettimeofday(&(buf->tv), NULL);
	vsnprintf(buf->msg, LEN_LOG, fmt, ap);

	char txt[64];
	log_entry_basics4buffer(buf, txt, sizeof(txt));
	ESP_LOG_LEVEL(level, func, "%s - %s", txt, buf->msg);

	log_write_idx++;
	if (log_write_idx >= N_LOG_ENTRIES)
		log_write_idx = 0;

	release_logsem_lock();
}

void log_err(const char *func, const char *fmt, ...) {
	va_list		ap;
	va_start(ap, fmt);
	log_doit(ESP_LOG_ERROR, func, fmt, ap);
	va_end(ap);
}

void log_warn(const char *func, const char *fmt, ...) {
	va_list		ap;
	va_start(ap, fmt);
	log_doit(ESP_LOG_WARN, func, fmt, ap);
	va_end(ap);
}

void log_info(const char *func, const char *fmt, ...) {
	va_list		ap;
	va_start(ap, fmt);
	log_doit(ESP_LOG_INFO, func, fmt, ap);
	va_end(ap);
}

void log_deb(const char *func, const char *fmt, ...) {
	va_list		ap;
	va_start(ap, fmt);
	log_doit(ESP_LOG_DEBUG, func, fmt, ap);
	va_end(ap);
}

void init_logging(esp_log_level_t initial_log_level) {
	ESP_LOGI(__func__, "start");

	log_level=initial_log_level;
	xLogSemaphore = xSemaphoreCreateMutex();
}

/**
 * log entry to buffer without thr text
 */
void log_entry_basics4buffer(T_LOG_ENTRY *buf, char *text, size_t sz_text) {
	memset(text, 0, sz_text);

	if ( buf->level == ESP_LOG_NONE)
		return;

    struct tm timeinfo = { 0 };
	char tformat[32];
	char tbuf[32];

    localtime_r(&(buf->tv.tv_sec), &timeinfo);
	snprintf(tformat, sizeof(tformat), "%%Y-%%m-%%d %%H:%%M:%%S.%06ld", buf->tv.tv_sec);
	strftime(tbuf, sizeof(tbuf), tformat, &timeinfo);

	snprintf(text, sz_text, "%s: %s:", tbuf, (
			buf->level == ESP_LOG_ERROR ? "ERROR" :
			buf->level == ESP_LOG_WARN  ? "WARN" :
			buf->level == ESP_LOG_INFO  ? "INFO" :
			buf->level == ESP_LOG_DEBUG ? "DEBUG" :
			buf->level == ESP_LOG_VERBOSE ? "VERBOSE" : "UNKNOWN" ));

}


void log_entry2text4buffer(T_LOG_ENTRY *buf, char *text, size_t sz_text) {
	memset(text, 0, sz_text);

	if ( buf->level == ESP_LOG_NONE)
		return;

    struct tm timeinfo = { 0 };
	char tformat[32];
	char tbuf[32];

    localtime_r(&(buf->tv.tv_sec), &timeinfo);
	snprintf(tformat, sizeof(tformat), "%%Y-%%m-%%d %%H:%%M:%%S.%06ld", buf->tv.tv_sec);
	strftime(tbuf, sizeof(tbuf), tformat, &timeinfo);

	snprintf(text, sz_text, "%s: %s: %s", tbuf, (
			buf->level == ESP_LOG_ERROR ? "ERROR" :
			buf->level == ESP_LOG_WARN  ? "WARN" :
			buf->level == ESP_LOG_INFO  ? "INFO" :
			buf->level == ESP_LOG_DEBUG ? "DEBUG" :
			buf->level == ESP_LOG_VERBOSE ? "VERBOSE" : "UNKNOWN" ),
			buf->msg);

}

esp_err_t log_entry2text(int idx, char *text, size_t sz_text) {
	memset(text, 0, sz_text);
	if ( idx < 0 || idx >= N_LOG_ENTRIES)
		return ESP_ERR_NOT_FOUND;

	T_LOG_ENTRY *buf = &(logtable[idx]);

	if ( buf->level == ESP_LOG_NONE)
		return ESP_ERR_NOT_FOUND;

	log_entry2text4buffer(buf, text, sz_text);

	return ESP_OK;
}

void log_current_time() {
	time_t now;
	struct tm timeinfo;
	char strftime_buf[64];

	char *tz = getenv("TZ");
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	log_info(__func__, "The current date/time for TZ '%s' is: %s", tz?tz:"not set", strftime_buf);
}
