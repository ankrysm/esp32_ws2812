/*
 * https_get.h
 *
 *  Created on: Oct 4, 2022
 *      Author: andreas
 */

#ifndef COMPONENTS_HTTPS_GET_HTTPS_GET_H_
#define COMPONENTS_HTTPS_GET_HTTPS_GET_H_


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sdkconfig.h"

#include "esp_http_client.h"

#define N_HTTPS_CLIENTS 10
#define LEN_HTTPS_CLIENT_SLOT_NAME 16
#define LEN_HTTPS_CLIENT_ERRMSG 64

typedef enum {
	HCT_INIT,
	HCT_READING,
	HCT_FINISH,
	HCT_FAILED
} t_https_callback_todo;

typedef enum {
	HSS_EMPTY,
	HSS_ACTIVE
} t_https_slot_status;

// forward
struct HTTPS_CLIENT_SLOT;
typedef struct HTTPS_CLIENT_SLOT  T_HTTPS_CLIENT_SLOT;

typedef int (*https_get_callback)(T_HTTPS_CLIENT_SLOT *slot, uint8_t **buf, uint32_t *buf_len);

struct HTTPS_CLIENT_SLOT {
	char name[LEN_HTTPS_CLIENT_SLOT_NAME];
	t_https_slot_status status;
	t_https_callback_todo todo;
	TaskHandle_t xHandle;
	esp_http_client_handle_t client;
	esp_http_client_config_t request_config;
	https_get_callback callback;
	char errmsg[LEN_HTTPS_CLIENT_ERRMSG];

	void *user_args;

	bool logflag;
	int64_t t_task_start;
	int64_t t_task_end;
};

T_HTTPS_CLIENT_SLOT *https_get(char *url, https_get_callback callback, void *user_args);
//bool is_http_client_task_active();

#endif /* COMPONENTS_HTTPS_GET_HTTPS_GET_H_ */
