/*
 * create_events.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 *
 *  get functions from JSON nodes
 *
 */

#include "esp32_ws2812.h"

extern char *cfg_autoplayfile;

/**
 * reads a single T_EVT_TIME element
 */
static esp_err_t decode_json4event_scene_event_events_data(cJSON *element, uint32_t id, T_EVENT_GROUP *evtgrp, t_processing_type processing_type, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_EVENT *evt = NULL;

	char *attr;
	double val;
	char sval[64];
	char hint[32]; // for logging
	char lerrmsg[64];
	t_result lrc;

	do {
		switch ( processing_type) {
		case PT_INIT:
			snprintf(hint, sizeof(hint),"%s", "INIT");
			evt = create_event_init(evtgrp,id);
			break;
		case PT_WORK:
			snprintf(hint, sizeof(hint),"%s", "WORK");
			evt = create_event_work(evtgrp, id);
			break;
		case PT_FINAL:
			snprintf(hint, sizeof(hint),"%s", "FINAL");
			evt = create_event_final(evtgrp, id);
			break;
		}
		if ( !evt ) {
			snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not create event data", hint, evtgrp->id, id);
			break;

		}

		attr="type";
		lrc = evt_get_string(element, attr, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg));
		if (lrc != RES_OK) {
			snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not decode '%s': '%s'", hint, evtgrp->id, id, attr, lerrmsg);
			break;
		}

		T_EVENT_CONFIG *evtcfg = find_event_config(sval);
		if ( !evtcfg ) {
			snprintf(errmsg, sz_errmsg,"%s/%s/%d: attr='%s' '%s' unknown", hint, evtgrp->id, id, attr, sval);
			break;
		}

		evt->type = evtcfg->evt_type;
		ESP_LOGI(__func__, "%s/%s/%d: event '%s' created", hint, evtgrp->id, id, eventype2text(evt->type));

		// parameter
		if (evtcfg->evt_para_type == EVT_PARA_STRING) {
			attr="value";
			lrc = evt_get_string(element, attr, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg));
			if (lrc == RES_OK) {
				strlcpy(evt->para.svalue, sval,sizeof(evt->para.svalue));
				ESP_LOGI(__func__, "%s/%s/%d: %s='%s'", hint, evtgrp->id, id, attr, evt->para.svalue);
			}  else {
				snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not decode '%s': '%s'", hint, evtgrp->id, id, attr, lerrmsg);
				break;
			}
		} else if (evtcfg->evt_para_type == EVT_PARA_NUMERIC) {
			attr="value";
			lrc = evt_get_number(element, attr, &val, lerrmsg, sizeof(lerrmsg));
			if (lrc == RES_OK) {
				evt->para.value = val;
				ESP_LOGI(__func__, "%s/%s/%d: %s=%.3f",hint, evtgrp->id,  id, attr, evt->para.value);
			} else {
				snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not decode '%s': '%s'", hint, evtgrp->id, id, attr, lerrmsg);
				break;
			}
		} else {
			// no parameter expected
		}

		rc = ESP_OK;
	} while(0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"wid=%d: 'time event' created", id);
		ESP_LOGI(__func__,"%s", errmsg);
	} else {
		ESP_LOGE(__func__, "error: %s", errmsg);
	}

	return rc;
}

static esp_err_t decode_json4event_scene_events_events(cJSON *element, T_EVENT_GROUP *evt, int *id, t_processing_type processing_type, char *errmsg, size_t sz_errmsg) {

	char lerrmsg[64];
	t_result lrc;

	if ( !element) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	char *attr = NULL;
	switch(processing_type){
		case PT_INIT:
			attr="init";
			break;
		case PT_WORK:
			attr="work";
			break;
		case PT_FINAL:
			attr="final";
			break;
		default:
			break;
	}
	int array_size = 0;
	lrc = evt_get_list(element, attr, &found, &array_size, lerrmsg, sizeof(lerrmsg));
	if ( lrc == RES_NOT_FOUND) {
		return ESP_OK;

	} if ( lrc != ESP_OK) {
		snprintf(errmsg, sz_errmsg,"get list failed: '%s'", lerrmsg);
		return ESP_FAIL;
	}

	ESP_LOGI(__func__, "size of '%s'=%d", attr, array_size);

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		(*id)++;
		if (decode_json4event_scene_event_events_data(element, *id, evt, processing_type, lerrmsg, sizeof(lerrmsg)) != ESP_OK) {
			snprintf(errmsg, sz_errmsg,"[%s]", lerrmsg);
			ESP_LOGE(__func__, "%s", errmsg);
			rc = ESP_FAIL;
			break;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg, "id='%s': ok", evt->id);
	}

	return rc;

}

static esp_err_t decode_json4event_object_data(cJSON *element, T_DISPLAY_OBJECT *obj, int id, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_DISPLAY_OBJECT_DATA *object_data;

	char *attr;
	char sval[256];
	double val;
	t_result lrc;
	T_COLOR_HSV hsv = {.h=0, .s=0, .v=0};
	char lerrmsg[64];
	do {
		if ( !(object_data = create_object_data(obj, id))) {
			snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not create data object", obj->oid, id);
			break;
		}

		attr="type";
		if (evt_get_string(element, attr, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg)) != RES_OK) {
			snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decocde data: '%s'", obj->oid, id, lerrmsg);
			break;
		}
		object_data->type = TEXT2OBJT(sval);
		if ( object_data->type == OBJT_UNKNOWN) {
			snprintf(errmsg, sz_errmsg,"oid=%s, id=%d: object type '%s' unknown", obj->oid, id, sval);
			break;
		}
		ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d(%s)", obj->oid, id, attr, object_data->type, OBJT2TEXT(object_data->type));

		attr="len";
		lrc = evt_get_number(element, attr, &val, lerrmsg, sizeof(lerrmsg));
		if (lrc== RES_OK) {
			object_data->len = val;
			ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d", obj->oid, id, attr, object_data->len);
		} else if ( lrc != RES_NOT_FOUND) {
			snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode data '%s': %s", obj->oid, id, attr, lerrmsg);
			break;
		}

		// *** need colors ?
		if (object_data->type == OBJT_RAINBOW) {
			// no color needed

		} else if (object_data->type == OBJT_BMP) {
			// no color needed, but url
			attr="url";
			if (evt_get_string(element, attr, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg)) != RES_OK) {
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data: '%s'", obj->oid, id, lerrmsg);
				break;
			}
			object_data->para.url = strdup(sval);
			ESP_LOGI(__func__,"oid=%s, id=%d: url='%s'", obj->oid, id, object_data->para.url);

		} else if ( object_data->type == OBJT_COLOR_TRANSITION) {
			// a "color from" needed
			lrc = decode_json_getcolor(element, "color_from", "hsv_from", "rgb_from", &hsv, lerrmsg, sizeof(lerrmsg));
			if ( lrc == RES_OK ) {
				object_data->para.tr.hsv_from.h = hsv.h;
				object_data->para.tr.hsv_from.s = hsv.s;
				object_data->para.tr.hsv_from.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d: %s", obj->oid, id, lerrmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no 'color from' specified", obj->oid);
				ESP_LOGI(__func__, "%s", errmsg);
				break;

			} else {
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data '%s': %s", obj->oid, id, attr, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s",obj->oid, id, errmsg);
				break; // failed
			}

			// a "color to" needed
			lrc = decode_json_getcolor(element, "color_to", "hsv_to", "rgb_to", &hsv, lerrmsg, sizeof(lerrmsg));
			if ( lrc == RES_OK ) {
				object_data->para.tr.hsv_to.h = hsv.h;
				object_data->para.tr.hsv_to.s = hsv.s;
				object_data->para.tr.hsv_to.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d: %s", obj->oid, id, lerrmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no 'color to' from specified", obj->oid, id);
				ESP_LOGI(__func__, "%s",errmsg);
				break;
			} else {
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data '%s': %s", obj->oid, id, attr, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s", obj->oid, id, lerrmsg);
				break; // failed
			}

		} else {
			// default: a color needed
			lrc = decode_json_getcolor(element, "color", "hsv", "rgb", &hsv, lerrmsg, sizeof(lerrmsg));
			if ( lrc == RES_OK ) {
				object_data->para.hsv.h = hsv.h;
				object_data->para.hsv.s = hsv.s;
				object_data->para.hsv.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d, %s", obj->oid, id, errmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no color specified", obj->oid, id);
				ESP_LOGI(__func__, "%s",errmsg);
				break;

			} else {
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data '%s': %s", obj->oid, id, attr, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s", obj->oid, id, errmsg);
				break; // failed
			}
		}

		/// end of color parameter

		rc = ESP_OK;
	} while(0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"oid=%s: 'object' created", obj->oid);
		ESP_LOGI(__func__,"%s", errmsg);
	} else {
		ESP_LOGE(__func__, "error: %s", errmsg);
	}

	return rc;
}



/**
 * reads a single T_EVT_OBJECT element
 * expected:
 * - oid
 * - data list
 */
static esp_err_t decode_json4event_object(cJSON *element,  char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_DISPLAY_OBJECT *obj = NULL;

	char *attr;
	char sval[64];

	do {
		attr="id";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;
		obj = create_object(sval);
		ESP_LOGI(__func__, "object '%s' created", obj->oid);

		// check for data list
		cJSON *found = NULL;
		attr = "list";
	    found = cJSON_GetObjectItemCaseSensitive(element, attr);
	    if ( !found) {
			snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
			return ESP_ERR_NOT_FOUND;
	    }

	    if (!cJSON_IsArray(found)) {
			snprintf(errmsg, sz_errmsg, "attribute '%s' is not an array", attr);
			return ESP_FAIL;
	    }

		int array_size = cJSON_GetArraySize(found);
		ESP_LOGI(__func__, "size of '%s'=%d", attr, array_size);
		if (array_size == 0) {
			snprintf(errmsg, sz_errmsg, "array '%s' has no content", attr);
			return ESP_ERR_NOT_FOUND;
		}

		rc = ESP_OK;
		for (int i=0; i < array_size; i++) {
			cJSON *list_element = cJSON_GetArrayItem(found, i);
			//JSON_Print(list_element);
			char l_errmsg[64];
			if (decode_json4event_object_data(list_element, obj, i+1, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
				snprintf(errmsg, sz_errmsg,"[%s]", l_errmsg);
				ESP_LOGE(__func__, "%s", errmsg);
				rc = ESP_FAIL;
				break;
			}
		}

	} while(false);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
		object_list_add(obj);
		ESP_LOGI(__func__, "object '%s' added to list", obj->oid);
	} else {
		if ( obj) {
			ESP_LOGI(__func__, "object '%s' not valid, deleted", obj->oid);
			delete_object(obj);
		}
	}

	return rc;
}



/**
 * decode data for an event,
 * expected data:
 *  - an id
 *  - number of repeates
 *  - list of init events,
 *  - list of working events
 *  - listof finishing events
 */
static esp_err_t decode_json4event_scene_events(cJSON *element, T_SCENE *scene, char *errmsg, size_t sz_errmsg) {
	if ( !cJSON_IsObject(element)) {
		snprintf(errmsg, sz_errmsg, "element is not an object");
		return ESP_FAIL;
	}

	char *attr;
	double val;
	char sval[64];
	//bool bval;
	//int id = -1;
	esp_err_t lrc, rc = ESP_FAIL;
	T_EVENT_GROUP *evt = NULL;

	memset(sval, 0, sizeof(sval));
	do {
		attr="id";
		if ((lrc=evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)) {
			break;
		}
		if ( !(evt = create_event(sval)))
			break;
		ESP_LOGI(__func__, "id='%s': event created", evt->id);

		attr="repeats";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == RES_OK) {
			evt->t_repeats = val;
			ESP_LOGI(__func__, "id=%s, %s=%d", evt->id, attr, evt->t_repeats);
		}

		int id=0;
		// to do for init (before run)
		if ( decode_json4event_scene_events_events(element, evt, &id, PT_INIT, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list INIT created (%s)", evt->id, errmsg);

		// when to do
		if ( decode_json4event_scene_events_events(element, evt, &id, PT_WORK, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evt->id, errmsg);

		if ( decode_json4event_scene_events_events(element, evt, &id, PT_FINAL, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evt->id, errmsg);

		rc = ESP_OK;
	} while (0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg, "event created");
		ESP_LOGI(__func__,"id='%s': %s", evt->id, errmsg);

		reset_event_group(evt);
		reset_event_repeats(evt);

		event_list_add(scene, evt);
	} else {
		ESP_LOGE(__func__, "id='%s': error: %s", sval, errmsg);
		// delete created data
		delete_event(evt);
	}

	return rc;
}

/**
 * a scene object has
 * - an id,
 * - a list of events
 */
static esp_err_t decode_json4event_scene(cJSON *element, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_SCENE *obj = NULL;

	char *attr;
	char sval[64];

	do {
		attr="id";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;
		ESP_LOGI(__func__, "%s='%s'", attr, sval);

		obj = create_scene(sval);
		ESP_LOGI(__func__, "object '%s' created", obj->id);

		// check for data list
		cJSON *found = NULL;
		int array_size =0;
		attr = "events";

		t_result lrc = evt_get_list(element, attr, &found, &array_size, errmsg, sz_errmsg);
		if ( lrc == RES_NOT_FOUND) {
			return ESP_OK;
		} if ( lrc != ESP_OK) {
			return ESP_FAIL;
		}

		ESP_LOGI(__func__, "size of '%s'=%d", attr, array_size);


		rc = ESP_OK;

		for (int i=0; i < array_size; i++) {
			cJSON *list_element = cJSON_GetArrayItem(found, i);
			char l_errmsg[64];

			if (decode_json4event_scene_events(list_element, obj, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
				snprintf(errmsg, sz_errmsg,"[%s]", l_errmsg);
				ESP_LOGE(__func__, "%s", errmsg);
				rc = ESP_FAIL;
				break;
			}
		}
	} while(false);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
		scene_list_add(obj);
		ESP_LOGI(__func__, "object '%s' added to list", obj->id);
	} else {
		if ( obj) {
			ESP_LOGI(__func__, "object '%s' not valid, deleted", obj->id);
			delete_scene(obj);
		}
	}

	return rc;
}



/**
 * "object" list contains objects with
 *   - an id and
 *   - a data list
 */
static esp_err_t decode_json4event_object_list(cJSON *element, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	cJSON *found = NULL;
	int array_size =0;

	char *attr = "objects";
	t_result lrc = evt_get_list(element, attr, &found, &array_size, errmsg, sz_errmsg);
	if ( lrc == RES_NOT_FOUND) {
		return ESP_OK;
	} if ( lrc != ESP_OK) {
		return ESP_FAIL;
	}

	ESP_LOGI(__func__, "size of '%s'=%d", attr, array_size);

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		char l_errmsg[64];
		if (decode_json4event_object(element, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(errmsg, sz_errmsg,"[%s]", l_errmsg);
			ESP_LOGE(__func__, "%s", errmsg);
			rc = ESP_FAIL;
			break;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
	}

	return rc;

}

/**
 * scenes list containes objects with
 *  - an id
 *  - a list of events
 *
 */
static esp_err_t decode_json4event_scenes_list(cJSON *element, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	cJSON *found = NULL;
	int array_size;

	char *attr = "scenes";
	t_result lrc = evt_get_list(element, attr, &found, &array_size, errmsg, sz_errmsg);
	if ( lrc == RES_NOT_FOUND) {
		return ESP_OK;
	} if ( lrc != ESP_OK) {
		return ESP_FAIL;
	}

	ESP_LOGI(__func__, "size of '%s'=%d", attr, array_size);

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		char l_errmsg[64];
		if (decode_json4event_scene(element, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(errmsg, sz_errmsg,"[%s]", l_errmsg);
			ESP_LOGE(__func__, "%s", errmsg);
			rc = ESP_FAIL;
			break;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
	}

	return rc;

}


/**
 * json data should contain 2 lists:
 * "objects" - what do paint
 * "scenes" - time dependend scenes
 */
esp_err_t decode_json4event_root(char *content, char *errmsg, size_t sz_errmsg) {
	cJSON *tree = NULL;
	esp_err_t rc = ESP_FAIL;

	ESP_LOGI(__func__, "content='%s'", content);
	memset(errmsg, 0, sz_errmsg);

	char filename[64];
	memset(filename, 0, sizeof(filename));

	do {
		tree = cJSON_Parse(content);
		if ( !tree) {
			snprintf(errmsg, sz_errmsg, "could not decode JSON-Data");
			break;
		}

		if ( decode_json4event_object_list(tree, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"object list created");

		if ( decode_json4event_scenes_list(tree, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"event list created");



		rc = ESP_OK;

	} while(false);

	if ( tree)
		cJSON_Delete(tree);

	ESP_LOGI(__func__, "done: %s", errmsg);
	return rc;
}

esp_err_t store_events_to_file(char *filename, char *content, char *errmsg, size_t sz_errmsg) {

	ESP_LOGI(__func__,"save data as '%s'", filename);
	char fname[LEN_PATH_MAX];
	snprintf(fname,sizeof(fname),"%s", filename);
	add_base_path(fname, sizeof(fname));

    FILE* f = fopen(fname, "w");
    if (f == NULL) {
    	snprintf(errmsg, sz_errmsg, "Failed to open file '%s' for writing", fname);
    	ESP_LOGE(__func__, "%s", errmsg);
        return ESP_FAIL;
    }

    fprintf(f, content);

    fclose(f);

    ESP_LOGI(__func__, "%d bytes saved in '%s'",strlen(content),fname);

    return ESP_OK;
}

esp_err_t load_events_from_file(char *filename, char *errmsg, size_t sz_errmsg) {

	char fname[LEN_PATH_MAX];
	snprintf(fname,sizeof(fname),"%s", filename);
	add_base_path(fname, sizeof(fname));

	extern uint32_t cfg_trans_flags;
	extern char last_loaded_file[];

	memset(errmsg, 0, sz_errmsg);
	char *content = NULL;
	esp_err_t rc = ESP_FAIL;

	do {
		struct stat st;
		if (stat(fname, &st) != 0) {
			ESP_LOGI(__func__,"could not load '%s'", filename);
			rc = ESP_ERR_NOT_FOUND;
			break;
		}

		content = calloc(st.st_size+1, sizeof(char));

		FILE *f = fopen(fname, "r");
		if (f == NULL) {
			ESP_LOGE(__func__, "Failed to open file for reading");
			rc = ESP_FAIL;
			break;
		}

		fread(content, sizeof(char),st.st_size, f);
		fclose(f);

		rc = decode_json4event_root(content, errmsg, sz_errmsg);

	} while(0);

	if ( rc == ESP_OK ) {
		ESP_LOGI(__func__, "decode '%s' successfull", filename);
		cfg_trans_flags |=CFG_AUTOPLAY_LOADED;

		strlcpy(last_loaded_file, filename, LEN_PATH_MAX);

	} else {
		ESP_LOGE(__func__, "could not decode '%s': %s", filename, errmsg);
	}

	if ( content)
		free(content);

    return rc;
}


esp_err_t load_autostart_file() {
	if ( !cfg_autoplayfile || !strlen(cfg_autoplayfile)) {
		ESP_LOGI(__func__,"no autostart file specified");
		return ESP_OK;
	}
	char errmsg[128];

	return load_events_from_file(cfg_autoplayfile, errmsg, sizeof(errmsg));

}

