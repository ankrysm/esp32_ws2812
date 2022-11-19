/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

// from global_data.c
extern T_EVENT_CONFIG event_config_tab[];

extern T_TRACK tracks[];
extern size_t sz_tracks;
extern T_EVENT_GROUP *s_event_group_list;
extern T_DISPLAY_OBJECT *s_object_list;
extern T_OBJECT_ATTR_CONFIG object_attr_config_tab[];
extern T_OBJECT_CONFIG object_config_tab[];

// to lock access to track-/event-/objects-list
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


T_EVENT_CONFIG *find_event_config(char *name) {
	for (int i=0; event_config_tab[i].evt_type != ET_NONE; i++ ) {
		if ( !strcasecmp(name, event_config_tab[i].name)) {
			// matched
			return &event_config_tab[i];
		}
	}
	return NULL;
}


char *eventype2text(event_type type) {
	for (int i=0; event_config_tab[i].evt_type != ET_NONE; i++ ) {
		if ( event_config_tab[i].evt_type == type) {
			// matched
			return event_config_tab[i].name;
		}
	}
	return "????";
}

void event2text(T_EVENT *evt, char *buf, size_t sz_buf) {
	switch(evt->type) {

		// with numeric parameter
	case ET_WAIT:
	case ET_WAIT_FIRST:
	case ET_PAINT:
	case ET_SPEED:
	case ET_SPEEDUP:
	case ET_SET_BRIGHTNESS:
	case ET_SET_BRIGHTNESS_DELTA:
	case ET_GOTO_POS:
	case ET_DISTANCE:
	case ET_BMP_READ:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s, val=%.3f",
			evt->id, evt->type, eventype2text(evt->type), evt->para.value);
		break;

		// with string parameter
	case ET_SET_OBJECT:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s, val='%s'",
			evt->id, evt->type, eventype2text(evt->type), evt->para.svalue);
		break;

		// type only, without parameter
	case ET_NONE:
	case ET_BOUNCE:
	case ET_REVERSE:
	case ET_CLEAR:
	case ET_BMP_OPEN:
	case ET_BMP_CLOSE:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s",
			evt->id, evt->type, eventype2text(evt->type));
		break;

	default:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s, unknown values",
			evt->id, evt->type, eventype2text(evt->type));
	}

}


/**
 * adds an event to the event group list
 */
esp_err_t event_list_add(T_EVENT_GROUP *evt) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( evt)  {
		if ( s_event_group_list) {
			// add at the end of the list
			T_EVENT_GROUP *t;
			for (t=s_event_group_list; t->nxt; t=t->nxt){}
			t->nxt = evt;
		} else {
			// first entry
			s_event_group_list = evt;
		}

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to event list");
	}
	return release_eventlist_lock();
}

// creates a new event body
T_EVENT_GROUP *create_event_group(char *id) {
	T_EVENT_GROUP *evt=calloc(1, sizeof(T_EVENT_GROUP));
	if ( !evt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new event", sizeof(T_EVENT_GROUP));
		return NULL;
	}
	// some useful values:
	strlcpy(evt->id, id, sizeof(evt->id));
	//evt->t_repeats = 1;
	return evt;
}

// creates a new init event and adds it to the event body
T_EVENT *create_event_init(T_EVENT_GROUP *evt, uint32_t id) {
	T_EVENT *tevt=calloc(1,sizeof(T_EVENT));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new init timing event", sizeof(T_EVENT));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_init_list) {
		evt->evt_init_list = tevt; // first in list
	} else {
		T_EVENT *t;
		for (t = evt->evt_init_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}

// creates a new  event and adds it to the event body
T_EVENT *create_event_work(T_EVENT_GROUP *evt, uint32_t id) {
	T_EVENT *tevt=calloc(1,sizeof(T_EVENT));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new timing event", sizeof(T_EVENT));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_work_list) {
		evt->evt_work_list = tevt; // first in list
	} else {
		T_EVENT *t;
		for (t = evt->evt_work_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}


// creates a new init timing event and adds it to the event body
T_EVENT *create_event_final(T_EVENT_GROUP *evt, uint32_t id) {
	T_EVENT *tevt=calloc(1,sizeof(T_EVENT));
	if ( !tevt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new init timing event", sizeof(T_EVENT));
		return NULL;
	}
	// some useful values:
	tevt->id=id;

	if ( !evt->evt_final_list) {
		evt->evt_final_list = tevt; // first in list
	} else {
		T_EVENT *t;
		for (t = evt->evt_final_list; t->nxt; t=t->nxt ) {}
		t->nxt = tevt; // added at the end of the list
	}

	return tevt;
}
// ***********************************************************************

T_EVENT_GROUP *find_event_group(char *id) {
	if ( !id || !strlen(id) || !s_event_group_list )
		return NULL;

	for( T_EVENT_GROUP *evtgrp = s_event_group_list; evtgrp; evtgrp = evtgrp->nxt) {
		if (!strcmp(evtgrp->id, id)) {
			// got it
			return evtgrp;
		}
	}
	return NULL;
}

// ***********************************************************************

T_DISPLAY_OBJECT* find_object4oid(char *oid) {
	if (!s_object_list) {
		return NULL;
	}

	T_DISPLAY_OBJECT *t;
	for (t = s_object_list; t; t = t->nxt) {
		if (!strcasecmp(t->oid, oid)) {
			return t;
		}
	}
	return NULL;
}


// creates a new what to do and adds it to the event body
T_DISPLAY_OBJECT *create_object(char *oid) {
	if ( oid == NULL || strlen(oid) == 0)
		return NULL;

	T_DISPLAY_OBJECT *obj=calloc(1, sizeof(T_DISPLAY_OBJECT));
	if ( !obj) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'object't", sizeof(T_DISPLAY_OBJECT));
		return NULL;
	}
	// some useful values:
	strlcpy(obj->oid, oid, sizeof(obj->oid));

	return obj;
}

esp_err_t object_list_add(T_DISPLAY_OBJECT *obj) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( obj)  {
		if ( s_object_list) {
			// add at the end of the list
			T_DISPLAY_OBJECT *t;
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
T_DISPLAY_OBJECT_DATA *create_object_data(T_DISPLAY_OBJECT *obj, uint32_t id) {
	if ( obj == NULL )
		return NULL;

	T_DISPLAY_OBJECT_DATA *objdata=calloc(1, sizeof(T_DISPLAY_OBJECT_DATA));
	if ( !objdata) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new 'object'", sizeof(T_DISPLAY_OBJECT_DATA));
		return NULL;
	}
	// some useful values:
	objdata->id = id;

	if ( !obj->data) {
		obj->data = objdata;
	} else {
		T_DISPLAY_OBJECT_DATA *t;
		for(t=obj->data; t->nxt; t=t->nxt){}
		t->nxt = objdata;
	}

	return objdata;
}

// create new track element
T_TRACK_ELEMENT *create_track_element(int tidx, int id) {
	if ( tidx < 0 || tidx > N_TRACKS)
		return NULL;

	T_TRACK_ELEMENT *ele = calloc(1,sizeof(T_TRACK_ELEMENT));

	T_TRACK *track= &(tracks[tidx]);
	if ( track->element_list) {
		T_TRACK_ELEMENT *te;
		for ( te= track->element_list; te->nxt; te = te->nxt){}
		te->nxt = ele;
	} else {
		track->element_list = ele;
	}
	ele->repeats = 1;
	ele->id = id;
	ele->status = EVT_STS_READY;
	return ele;
}

// #################### Free data structures ########################################


esp_err_t clear_tracks() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}


	for ( int i=0; i < N_TRACKS; i++) {
		T_TRACK *track = &(tracks[i]);
		if ( ! track->element_list)
			continue; // nothing to free

		T_TRACK_ELEMENT *t, *d = track->element_list;
		while(d) {
			t = d->nxt;
			if ( d->w_bmp) {
				free(d->w_bmp);
			}
			free(d);
			d=t;
		}
	}
	memset(tracks, 0, sz_tracks);

	return release_eventlist_lock();
}



void delete_object(T_DISPLAY_OBJECT *obj) {
	if (!obj)
		return;

	if ( obj->data) {
		T_DISPLAY_OBJECT_DATA *t, *obj_data = obj->data;
		while(obj_data) {
			t = obj_data->nxt;
			if ( obj_data->type == OBJT_BMP ) {
				if (obj_data->para.bmp.url)
					free(obj_data->para.bmp.url);
			}

			free(obj_data);
			obj_data=t;
		}
	}
	free(obj);
}

esp_err_t clear_object_list() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if (s_object_list) {
		T_DISPLAY_OBJECT *nxt;
		while(s_object_list) {
			nxt = s_object_list->nxt;
			delete_object(s_object_list);
			s_object_list = nxt;
		}
	}

	return release_eventlist_lock();
}

// **************************************************************
void delete_event_list(T_EVENT *list) {
	if (list) {
		T_EVENT *t, *w = list;
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
void delete_event_group(T_EVENT_GROUP *evtgrp) {
	if (!evtgrp)
		return;

	delete_event_list(evtgrp->evt_init_list);
	delete_event_list(evtgrp->evt_work_list);
	delete_event_list(evtgrp->evt_final_list);

	free(evtgrp);
}


esp_err_t clear_event_group_list() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if (s_event_group_list) {
		T_EVENT_GROUP *t, *w = s_event_group_list;
		while(w) {
			t = w->nxt;
			delete_event_group(w);
			w=t;
		}
	}
	s_event_group_list = NULL;

	return release_eventlist_lock();
}

// ******************************************************************************

T_OBJECT_ATTR_CONFIG *object_attr4type_id(int id) {

	for  (int i=0; object_attr_config_tab[i].type != OBJATTR_EOT; i++) {
		if ( object_attr_config_tab[i].type == id)
			return &(object_attr_config_tab[i]);
	}

	return NULL;
}


char *object_attrtype2text(int id) {

	for  (int i=0; object_attr_config_tab[i].type != OBJATTR_EOT; i++) {
		if ( object_attr_config_tab[i].type == id)
			return object_attr_config_tab[i].name;
	}

	return "???";
}

T_OBJECT_ATTR_CONFIG *object_attr4type_name(char *name) {

	for  (int i=0; object_attr_config_tab[i].type != OBJATTR_EOT; i++) {
		if ( !strcmp(object_attr_config_tab[i].name, name))
			return &(object_attr_config_tab[i]);
	}

	return NULL;
}

void object_attr_group2text(object_attr_type attr_group, char *text, size_t sz_text) {
	if (attr_group == 0) {
		strlcpy(text ,"--", sz_text);
		return;
	}
	memset(text, 0, sz_text);
	for (int i=1; i< OBJATTR_EOT; i=i<<1) {
		if ( attr_group & i) {
			T_OBJECT_ATTR_CONFIG *attr_config = object_attr4type_id(i);
			if ( !attr_config ) {
				ESP_LOGE(__func__, "group attr 0x%08x unknown", i);
			} else {
				if ( strlen(text))
					strlcat(text,",", sz_text);
				strlcat(text, attr_config->name, sz_text);
			}
		}
	}
}

// ******************************************************************************
T_OBJECT_CONFIG *object4type_name(char *name) {

	for  (int i=0; object_config_tab[i].type != OBJT_EOT; i++) {
		if ( !strcmp(object_config_tab[i].name, name))
			return &(object_config_tab[i]);
	}

	return NULL;
}

char *object_type2text(object_type type) {
	for  (int i=0; object_config_tab[i].type != OBJT_EOT; i++) {
		if ( object_config_tab[i].type == type)
			return object_config_tab[i].name;
	}

	return "???";

}
