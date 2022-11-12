/*
 * logger.h
 *
 *  Created on: Nov 11, 2022
 *      Author: andreas
 */

#ifndef MAIN_LOGGER_H_
#define MAIN_LOGGER_H_

#define N_LOG_ENTRIES 50
#define LEN_LOG 256

typedef struct LOG_ENTRY {
	esp_log_level_t level;
	struct timeval tv;
	char msg[LEN_LOG];
} T_LOG_ENTRY;

void log_err(const char *func, const char *fmt, ...);
void log_warn(const char *func, const char *fmt, ...);
void log_info(const char *func, const char *fmt, ...);
void log_deb(const char *func, const char *fmt, ...);
void init_logging(esp_log_level_t initial_log_level);
esp_err_t obtain_logsem_lock();
esp_err_t release_logsem_lock();
esp_err_t log_entry2text(int idx, char *text, size_t sz_text);
void log_entry_basics4buffer(T_LOG_ENTRY *buf, char *text, size_t sz_text);
void log_entry2text4buffer(T_LOG_ENTRY *buf, char *text, size_t sz_text);

#endif /* MAIN_LOGGER_H_ */
