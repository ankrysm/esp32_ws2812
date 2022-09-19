/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


extern T_SCENE *s_scene_list;
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
 * find an event by id
 * (without lock)
 */
/*
T_EVENT *find_event(char *id) {
	if (!s_event_list) {
		return NULL; // nothing available
	}

	for( T_EVENT *e = s_event_list; e; e=e->nxt) {
		if ( !strcasecmp(e->id, id)) {
			return e; // found!
		}
	}
	return NULL; // not found
}
*/

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
void get_new_event_id(char *id, size_t sz_id) {

	uint32_t n = get_random(0, UINT32_MAX);
	snprintf(id,sz_id,"%u", n);

}

/*
esp_err_t delete_event_by_id(char *id) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}
	bool found = false;
	if ( s_event_list)  {
		T_EVENT *prev = NULL;
		for (T_EVENT *evt=s_event_list; evt; evt=evt->nxt) {
			if ( !strcasecmp(evt->id, id) ) {
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
		ESP_LOGI(__func__,"event %s deleted", id);
	else
		ESP_LOGI(__func__,"event %s not found", id);

	esp_err_t rc = release_eventlist_lock();
	if ( rc == ESP_OK && !found )
		rc = ESP_ERR_NOT_FOUND;

	return rc;
}
*/

/**
 * adds an event to the list
 */
esp_err_t event_list_add(T_SCENE *scene, T_EVENT *evt) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( evt)  {
		if ( scene->events) {
			// add at the end of the list
			T_EVENT *t;
			for (t=scene->events; t->nxt; t=t->nxt){}
			t->nxt = evt;
		} else {
			// first entry
			scene->events = evt;
		}

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to event list");
	}
	return release_eventlist_lock();
}

// creates a new event body
T_EVENT *create_event(char *id) {
	T_EVENT *evt=calloc(1, sizeof(T_EVENT));
	if ( !evt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new event", sizeof(T_EVENT));
		return NULL;
	}
	// some useful values:
	strlcpy(evt->id, id, sizeof(evt->id));
	evt->t_repeats = 1;
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

// creates a new init timing event and adds it to the event body
T_EVT_TIME *create_timing_event_init(T_EVENT *evt, uint32_t id) {
	T_EVT_TIME *tevt=calloc(1,sizeof(T_EVT_TIME));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new init timing event", sizeof(T_EVT_TIME));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_time_init_list) {
		evt->evt_time_init_list = tevt; // first in list
	} else {
		T_EVT_TIME *t;
		for (t = evt->evt_time_init_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}

// creates a new init timing event and adds it to the event body
T_EVT_TIME *create_timing_event_final(T_EVENT *evt, uint32_t id) {
	T_EVT_TIME *tevt=calloc(1,sizeof(T_EVT_TIME));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new init timing event", sizeof(T_EVT_TIME));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_time_final_list) {
		evt->evt_time_final_list = tevt; // first in list
	} else {
		T_EVT_TIME *t;
		for (t = evt->evt_time_final_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}

// ***********************************************************************

/*
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
*/

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
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'object'", sizeof(T_EVT_OBJECT_DATA));
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

// ############### SCENES ##############################################

// creates a new scene
T_SCENE *create_scene(char *id) {
	if ( id == NULL || strlen(id) == 0)
		return NULL;

	T_SCENE *obj=calloc(1, sizeof(T_SCENE));
	if ( !obj) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'scene'", sizeof(T_SCENE));
		return NULL;
	}
	// some useful values:
	strlcpy(obj->id, id, sizeof(obj->id));

	return obj;
}

// add a scene to the tab
esp_err_t scene_list_add(T_SCENE *obj) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( obj)  {
		if ( s_scene_list) {
			// add at the end of the list
			T_SCENE *t;
			for (t=s_scene_list; t->nxt; t=t->nxt){}
			t->nxt = obj;
		} else {
			// first entry
			s_scene_list = obj;
		}

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to scene list");
	}

	return release_eventlist_lock();
}

// #################### Free data structures ########################################


void delete_object(T_EVT_OBJECT *obj) {
	if (!obj)
		return;

	if ( obj->data) {
		T_EVT_OBJECT_DATA *t, *obj_data = obj->data;
		while(obj_data) {
			t = obj_data->nxt;
			free(obj_data);
			obj_data=t;
		}
	}
	free(obj);
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


void delete_event_list(T_EVT_TIME *list) {
	if (list) {
		T_EVT_TIME *t, *w = list;
		while(w) {
			t = w->nxt;
			free(w);
			w=t;
		}
	}
}


/**
 * free an event
 */
void delete_event(T_EVENT *evt) {
	if (!evt)
		return;

	delete_event_list(evt->evt_time_init_list);
	delete_event_list(evt->evt_time_list);
	delete_event_list(evt->evt_time_final_list);

	/*
	if (evt->evt_where_list) {
		T_EVT_WHERE *t, *w = evt->evt_where_list;
		while(w) {
			t = w->nxt;
			free(w);
			w=t;
		}
	}
	*/
	free(evt);

}


// delete a scene, caller must free obj itselves
void delete_scene(T_SCENE *obj) {
	if (!obj)
		return;

	if ( obj->events) {
		T_EVENT *t, *d = obj->events;
		while(d) {
			t = d->nxt;
			delete_event(d);
			d=t;
		}
	}
	free(obj);
}

esp_err_t scene_list_free() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	// s_scene_list will be NULL after all frees
	if ( s_scene_list)  {
		T_SCENE *nxt;
		while (s_scene_list) {
			nxt = s_scene_list->nxt;
			delete_scene(s_scene_list);
			s_scene_list = nxt;
		}
	}

	// after this s_event_list is NULL
	return release_eventlist_lock();

}


// ******************************************************************************

