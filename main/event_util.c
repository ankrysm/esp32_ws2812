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
 * free an event
 */
void delete_event(T_EVENT *evt) {
	if (!evt)
		return;

	if (evt->what_list) {
		T_EVT_WHAT *t, *w = evt->what_list;
		while(w) {
			t = w->nxt;
			free(w);
			w=t;
		}
	}

	if (evt->evt_time_list) {
		T_EVT_TIME *t, *w = evt->evt_time_list;
		while(w) {
			t = w->nxt;
			free(w);
			w=t;
		}
	}

	if (evt->evt_where_list) {
		T_EVT_WHERE *t, *w = evt->evt_where_list;
		while(w) {
			t = w->nxt;
			free(w);
			w=t;
		}
	}

	free(evt);

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
 * adds an event to the list
 */
esp_err_t event_list_add(T_EVENT *evt) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( evt)  {
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
		reset_event(evt);
		reset_event_repeats(evt);

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to event list");
	}
	return release_eventlist_lock();
}

// creates a new event body
T_EVENT *create_event(uint32_t id) {
	T_EVENT *evt=calloc(1, sizeof(T_EVENT));
	if ( !evt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new event", sizeof(T_EVENT));
		return NULL;
	}
	// some useful values:
	evt->id=id;
	evt->pos = 0;
	evt->delta_pos = 1;
	evt->brightness = 1.0;
	evt->len_factor = 1.0;
	return evt;
}

// creates a new timing event and adds it to the event body
T_EVT_TIME *create_timing_event(T_EVENT *evt, uint32_t id) {
	T_EVT_TIME *tevt=calloc(1,sizeof(T_EVT_TIME));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new timing event", sizeof(T_EVT_TIME));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_time_list) {
		evt->evt_time_list = tevt; // first in list
	} else {
		T_EVT_TIME *t;
		for (t = evt->evt_time_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}

// creates a new what to do and adds it to the event body
T_EVT_WHAT *create_what(T_EVENT *evt, uint32_t id) {
	T_EVT_WHAT *w=calloc(1, sizeof(T_EVENT));
	if ( !w) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'what't", sizeof(T_EVT_WHAT));
		return NULL;
	}
	// some useful values:
	w->id=id;

	if ( !evt->what_list) {
		evt->what_list = w;
	} else {
		T_EVT_WHAT *t;
		for(t=evt->what_list; t->nxt; t=t->nxt){}
		t->nxt = w;
	}

	return w;
}

void init_eventlist_utils() {
	xSemaphore = xSemaphoreCreateMutex();
}
