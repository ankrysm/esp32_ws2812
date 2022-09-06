/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


extern T_EVENT *s_event_list;
extern T_EVT_OBJECT *s_object_list;

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

void init_eventlist_utils() {
	xSemaphore = xSemaphoreCreateMutex();
}

/**
 * free an event
 */
void delete_event(T_EVENT *evt) {
	if (!evt)
		return;

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
 * find an event by id
 * (without lock)
 */
T_EVENT *find_event(uint32_t id) {
	if (!s_event_list) {
		return NULL; // nothing available
	}

	for( T_EVENT *e = s_event_list; e; e=e->nxt) {
		if ( e->id == id ) {
			return e; // found!
		}
	}
	return NULL; // not found
}

T_EVT_TIME *find_timer_event4marker(T_EVT_TIME *tevt_list, char *marker) {
	if (!tevt_list || !marker || !strlen(marker)) {
		return NULL; // nothing to find
	}

	for( T_EVT_TIME *e = tevt_list; e; e=e->nxt) {
		if ( e->marker && strlen(e->marker) && !strcasecmp(e->marker, marker) ) {
			return e; // found!
		}
	}
	return NULL; // not found
}

/**
 * find a new event id larger than the max id,
 * (without lock)
 */
uint32_t get_new_event_id() {

	uint32_t max_id = 0;
	if (s_event_list) {

		for( T_EVENT *e = s_event_list; e; e=e->nxt) {
			if ( e->id > max_id ) {
				max_id = e->id;
			}
		}
	}
	return max_id + 1; // not found
}


esp_err_t delete_event_by_id(uint32_t id) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}
	bool found = false;
	if ( s_event_list)  {
		T_EVENT *prev = NULL;
		for (T_EVENT *evt=s_event_list; evt; evt=evt->nxt) {
			if ( evt->id == id ) {
				// found, delete it
				if (prev == NULL ) {
					s_event_list = evt->nxt;
				} else {
					prev->nxt = evt->nxt;
				}
				delete_event(evt);
				found = true;
				break;
			}
			prev = evt;
		}
	}

	if (found)
		ESP_LOGI(__func__,"event %d deleted", id);
	else
		ESP_LOGI(__func__,"event %d not found", id);

	esp_err_t rc = release_eventlist_lock();
	if ( rc == ESP_OK && !found )
		rc = ESP_ERR_NOT_FOUND;

	return rc;
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
			delete_event(s_event_list);
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
			//evt->id = t->id +1;
			t->nxt = evt;
		} else {
			// first entry
			//evt->id = 1;
			s_event_list = evt;
		}
		reset_event(evt);
		reset_timing_events(evt->evt_time_list);
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

// ***********************************************************************
void delete_object(T_EVT_OBJECT *obj) {
	if (!obj)
		return;

	if ( obj->data) {
		T_EVT_OBJECT_DATA *t, *d = obj->data;
		while(d) {
			t = d->nxt;
			free(d);
			d=t;
		}
	}
	free(obj);
}

esp_err_t delete_object_by_oid(char *oid) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	bool found = false;
	if ( s_object_list)  {
		T_EVT_OBJECT *prev = NULL;
		for (T_EVT_OBJECT *obj=s_object_list; obj; obj=obj->nxt) {
			if (!strcasecmp( obj->oid, oid) ) {
				// found, delete it
				if (prev == NULL ) {
					s_object_list = obj->nxt;
				} else {
					prev->nxt = obj->nxt;
				}
				delete_object(obj);
				found = true;
				break;
			}
			prev = obj;
		}
	}

	if (found)
		ESP_LOGI(__func__,"object '%s' deleted", oid);
	else
		ESP_LOGI(__func__,"object '%s' not found", oid);

	esp_err_t rc = release_eventlist_lock();
	if ( rc == ESP_OK && !found )
		rc = ESP_ERR_NOT_FOUND;

	return rc;
}


esp_err_t object_list_free() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if (s_object_list) {
		T_EVT_OBJECT *nxt;
		while(s_object_list) {
			nxt = s_object_list->nxt;
			delete_object(s_object_list);
			s_object_list = nxt;
		}
	}

	return release_eventlist_lock();
}

T_EVT_OBJECT* find_object4oid(char *oid) {
	if (!s_object_list) {
		return NULL;
	}

	T_EVT_OBJECT *t;
	for (t = s_object_list; t; t = t->nxt) {
		if (!strcasecmp(t->oid, oid)) {
			return t;
		}
	}
	return NULL;
}


// creates a new what to do and adds it to the event body
T_EVT_OBJECT *create_object(char *oid) {
	if ( oid == NULL || strlen(oid) == 0)
		return NULL;

	T_EVT_OBJECT *obj=calloc(1, sizeof(T_EVT_OBJECT));
	if ( !obj) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'object't", sizeof(T_EVT_OBJECT));
		return NULL;
	}
	// some useful values:
	strlcpy(obj->oid, oid, sizeof(obj->oid));

	return obj;
}

esp_err_t object_list_add(T_EVT_OBJECT *obj) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( obj)  {
		if ( s_object_list) {
			// add at the end of the list
			T_EVT_OBJECT *t;
			for (t=s_object_list; t->nxt; t=t->nxt){}
			t->nxt = obj;
		} else {
			// first entry
			s_object_list = obj;
		}

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to object list");
	}
	return release_eventlist_lock();
}


// creates a new what to do and adds it to the event body
T_EVT_OBJECT_DATA *create_object_data(T_EVT_OBJECT *obj, uint32_t id) {
	if ( obj == NULL )
		return NULL;

	T_EVT_OBJECT_DATA *objdata=calloc(1, sizeof(T_EVT_OBJECT_DATA));
	if ( !objdata) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'object't", sizeof(T_EVT_OBJECT_DATA));
		return NULL;
	}
	// some useful values:
	objdata->id = id;

	if ( !obj->data) {
		obj->data = objdata;
	} else {
		T_EVT_OBJECT_DATA *t;
		for(t=obj->data; t->nxt; t=t->nxt){}
		t->nxt = objdata;
	}

	return objdata;
}

// ******************************************************************************

