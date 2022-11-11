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


#endif /* MAIN_LOGGER_H_ */
