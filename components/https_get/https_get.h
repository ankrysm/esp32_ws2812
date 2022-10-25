/*
 * https_get.h
 *
 *  Created on: Oct 4, 2022
 *      Author: andreas
 */

#ifndef COMPONENTS_HTTPS_GET_HTTPS_GET_H_
#define COMPONENTS_HTTPS_GET_HTTPS_GET_H_


typedef enum {
	HCT_INIT,
	HCT_READING,
	HCT_FINISH,
	HCT_FAILED
} t_https_callback_todo;

typedef int (*https_get_callback)(t_https_callback_todo todo, uint8_t **buf, uint32_t *buf_len);

esp_err_t https_get(char *url, https_get_callback callback);
bool is_http_client_task_active();

#endif /* COMPONENTS_HTTPS_GET_HTTPS_GET_H_ */
