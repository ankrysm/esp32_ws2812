/*
 * event_util.c
 *
 *  Created on: 05.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


extern T_SCENE *s_scene_list;
extern T_DISPLAY_OBJECT *s_object_list;

// to lock access to event-List
static  SemaphoreHandle_t xSemaphore = NULL;

static 	TickType_t xSemDelay = 5000 / portTICK_PERIOD_MS;

T_EVENT_CONFIG event_config_tab[] = {
		{ET_WAIT, EVT_PARA_NUMERIC, "wait", "wait for n ms"},
		{ET_WAIT_FIRST, EVT_PARA_NUMERIC, "wait_first", "wait for n ms at first init"},
		{ET_PAINT, EVT_PARA_NUMERIC, "paint", "paint leds with the given parameter"},
		{ET_DISTANCE, EVT_PARA_NUMERIC, "distance", "paint until object has moved n leds"},
		{ET_SPEED, EVT_PARA_NUMERIC, "speed", "speed in leds per second"},
		{ET_SPEEDUP, EVT_PARA_NUMERIC, "speedup", "speedup delta speed per display cycle"},
		{ET_BOUNCE, EVT_PARA_NONE, "bounce", "reverse speed"},
		{ET_REVERSE, EVT_PARA_NONE, "reverse", "reverse paint direction"},
		{ET_GOTO_POS, EVT_PARA_NUMERIC, "goto", "go to led position"},
		{ET_MARKER, EVT_PARA_STRING, "marker", "set marker"},
		{ET_JUMP_MARKER, EVT_PARA_STRING, "jump_marker", "jump to marker"},
		{ET_CLEAR,EVT_PARA_NONE, "clear", "blank the strip"},
		{ET_SET_BRIGHTNESS, EVT_PARA_NUMERIC,"brightness", "set brightness"},
		{ET_SET_BRIGHTNESS_DELTA, EVT_PARA_NUMERIC,"brightness_delta", "set brightness delta per display cycle"},
		{ET_SET_OBJECT, EVT_PARA_STRING, "object","set objectid from object table"},
		{ET_BMP_OPEN, EVT_PARA_NONE, "bmp_open", "open BMP stream, defined by 'bmp' object"},
		{ET_BMP_READ, EVT_PARA_NUMERIC, "bmp_read","read BMP data line by line and display it, max n lines, -1 all lines"},
		{ET_BMP_CLOSE, EVT_PARA_NONE, "bmp_close", "close BMP stream"},
		{ET_NONE, EVT_PARA_NONE, "", ""} // end of table
};

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

/**
 * print event config on position 'pos'
 * returns true, when more data is available by a new call
 */
bool print_event_config_r(int *pos, char *buf, size_t sz_buf) {
	if (event_config_tab[*pos].evt_type == ET_NONE) {
		*pos = 0;
		return false; // end of table
	}
	switch (event_config_tab[*pos].evt_para_type) {
	case EVT_PARA_NONE:
		snprintf(buf, sz_buf, "\"type\":\"%s\" - %s", event_config_tab[*pos].name, event_config_tab[*pos].help);
		break;
	case EVT_PARA_NUMERIC:
		snprintf(buf, sz_buf, "\"type\":\"%s\", \"value\":<numeric value>  - %s",
				event_config_tab[*pos].name, event_config_tab[*pos].help
		);
		break;
	case EVT_PARA_STRING:
		snprintf(buf, sz_buf, "\"type\":\"%s\", \"value\":\"<string value>\"  - %s",
				event_config_tab[*pos].name, event_config_tab[*pos].help
		);
		break;
	}
	(*pos)++;
	return true;
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
		snprintf(buf, sz_buf,"id=%d, type=%d/%s, val=%.3f",
			evt->id, evt->type, eventype2text(evt->type), evt->para.value);
		break;

		// with string parameter
	case ET_MARKER:
	case ET_JUMP_MARKER:
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
	case ET_BMP_READ:
	case ET_BMP_CLOSE:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s",
			evt->id, evt->type, eventype2text(evt->type));
		break;

	default:
		snprintf(buf, sz_buf,"id=%d, type=%d/%s, unknown values",
			evt->id, evt->type, eventype2text(evt->type));
	}

}

T_EVENT *find_event4marker(T_EVENT *evt_list, char *marker) {
	if (!evt_list || !marker || !strlen(marker)) {
		return NULL; // nothing to find
	}

	for( T_EVENT *e = evt_list; e; e=e->nxt) {
		if ( e->type == ET_MARKER && e->para.svalue && strlen(e->para.svalue) && !strcasecmp(e->para.svalue, marker) ) {
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

/**
 * adds an event to the list
 */
esp_err_t event_list_add(T_SCENE *scene, T_EVENT_GROUP *evt) {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock");
		return ESP_FAIL;
	}

	if ( evt)  {
		if ( scene->event_groups) {
			// add at the end of the list
			T_EVENT_GROUP *t;
			for (t=scene->event_groups; t->nxt; t=t->nxt){}
			t->nxt = evt;
		} else {
			// first entry
			scene->event_groups = evt;
		}

	} else {
		ESP_LOGE(__func__,"couldn't add  NULL to event list");
	}
	return release_eventlist_lock();
}

// creates a new event body
T_EVENT_GROUP *create_event(char *id) {
	T_EVENT_GROUP *evt=calloc(1, sizeof(T_EVENT_GROUP));
	if ( !evt) {
		ESP_LOGE(__func__,"couldn't allocate %d bytes for new event", sizeof(T_EVENT_GROUP));
		return NULL;
	}
	// some useful values:
	strlcpy(evt->id, id, sizeof(evt->id));
	evt->t_repeats = 1;
	return evt;
}

// creates a new timing event and adds it to the event body
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


void delete_object(T_DISPLAY_OBJECT *obj) {
	if (!obj)
		return;

	if ( obj->data) {
		T_DISPLAY_OBJECT_DATA *t, *obj_data = obj->data;
		while(obj_data) {
			t = obj_data->nxt;
			if ( obj_data->type == OBJT_BMP ) {
				if (obj_data->para.url)
					free(obj_data->para.url);
			}

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
		T_DISPLAY_OBJECT *nxt;
		while(s_object_list) {
			nxt = s_object_list->nxt;
			delete_object(s_object_list);
			s_object_list = nxt;
		}
	}

	return release_eventlist_lock();
}


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
void delete_event(T_EVENT_GROUP *evt) {
	if (!evt)
		return;

	delete_event_list(evt->evt_init_list);
	delete_event_list(evt->evt_work_list);
	delete_event_list(evt->evt_final_list);

	free(evt);
}


// delete a scene, caller must free obj itselves
void delete_scene(T_SCENE *scene) {
	if (!scene)
		return;

	if ( scene->event_groups) {
		T_EVENT_GROUP *t, *d = scene->event_groups;
		while(d) {
			t = d->nxt;
			delete_event(d);
			d=t;
		}
	}
	free(scene);
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


