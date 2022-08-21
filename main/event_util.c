/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


extern T_EVENT *s_event_list;

// to lock access to event-List
static  SemaphoreHandle_t xSemaphore = NULL;

static 	TickType_t xSemDelay = 5000 / portTICK_PERIOD_MS;


esp_err_t obtain_eventlist_lock() {
	if( xSemaphoreTake( xSemaphore, xSemDelay ) == pdTRUE ) {
		return ESP_OK;
	}
	ESP_LOGE(__func__, "xSemaphoreTake failed");
	return ESP_FAIL;
}

esp_err_t release_eventlist_lock(){
	if (xSemaphoreGive( xSemaphore ) == pdTRUE) {
		return ESP_OK;
	}
	ESP_LOGE(__func__, "xSemaphoreGive failed");
	return ESP_FAIL;
}


/**
 * frees the event list
 */
esp_err_t event_list_free() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}
	if ( s_event_list)  {
		T_EVENT *nxt;
		while (s_event_list) {
			nxt = s_event_list->nxt;
			free(s_event_list);
			s_event_list = nxt;
		}
	}
	// after this s_event_list is NULL
	return release_eventlist_lock();
}

/**
 * adds an event
 */
esp_err_t event_list_add(T_EVENT *pevt) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}
	T_EVENT *evt = calloc(1,sizeof(T_EVENT));
	if ( evt)  {
		memcpy(evt,pevt,sizeof(T_EVENT));
		if ( s_event_list) {
			// add at the end of the list
			T_EVENT *t;
			for (t=s_event_list; t->nxt; t=t->nxt){}
			evt->id = t->id +1;
			t->nxt = evt;
		} else {
			// first entry
			evt->id = 1;
			s_event_list = evt;
		}
	} else {
		ESP_LOGE(__func__,"couldn't allocate memory for event");
	}
	return release_eventlist_lock();
}

void init_eventlist_utils() {
	xSemaphore = xSemaphoreCreateMutex();
}
