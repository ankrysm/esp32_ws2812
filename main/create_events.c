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
		bool optional_para = evtcfg->evt_para_type & EVT_PARA_OPTIONAL;
		int evt_para_type = evtcfg->evt_para_type & 0x0F;

		if (evt_para_type == EVT_PARA_STRING) {
			attr="value";
			lrc = evt_get_string(element, attr, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg));
			if (lrc == RES_OK) {
				strlcpy(evt->para.svalue, sval,sizeof(evt->para.svalue));
			} else if (lrc == RES_NOT_FOUND) {
				if ( !optional_para) {
					snprintf(errmsg, sz_errmsg,"%s/%s/%d: missing string parameter '%s'", hint, evtgrp->id, id, attr);
					break;
				}
			}  else {
				snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not decode '%s': '%s'", hint, evtgrp->id, id, attr, lerrmsg);
				break;
			}
			ESP_LOGI(__func__, "%s/%s/%d: %s='%s'", hint, evtgrp->id, id, attr, evt->para.svalue);
		} else if (evt_para_type == EVT_PARA_NUMERIC) {
			attr="value";
			lrc = evt_get_number(element, attr, &val, lerrmsg, sizeof(lerrmsg));
			if (lrc == RES_OK) {
				evt->para.value = val;
			} else if (lrc == RES_NOT_FOUND) {
				if ( !optional_para) {
					snprintf(errmsg, sz_errmsg,"%s/%s/%d: missing numerical parameter '%s'", hint, evtgrp->id, id, attr);
					break;
				} else {
					evt->para.value = -1.0;
				}
			} else {
				snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not decode '%s': '%s'", hint, evtgrp->id, id, attr, lerrmsg);
				break;
			}
			ESP_LOGI(__func__, "%s/%s/%d: %s=%.3f",hint, evtgrp->id,  id, attr, evt->para.value);
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

static esp_err_t decode_json4event_group_event_list(cJSON *element, T_EVENT_GROUP *evt, int *id, t_processing_type processing_type, char *errmsg, size_t sz_errmsg) {

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



/**
 * decode element of events list,
 * expected data:
 *  - an id
 *  - list of init events (optional),
 *  - list of working events
 *  - listof finishing events (optional)
 */
static esp_err_t decode_json4event_group(cJSON *element, /*T_SCENE *scene,*/ char *errmsg, size_t sz_errmsg) {
	if ( !cJSON_IsObject(element)) {
		snprintf(errmsg, sz_errmsg, "element is not an object");
		return ESP_FAIL;
	}

	char *attr;
	char sval[64];
	esp_err_t lrc, rc = ESP_FAIL;
	T_EVENT_GROUP *evtgrp = NULL;

	memset(sval, 0, sizeof(sval));
	do {
		attr="id";
		if ((lrc=evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)) {
			break;
		}
		if ( !(evtgrp = create_event_group(sval)))
			break;
		ESP_LOGI(__func__, "id='%s': event created", evtgrp->id);

		int id=0;
		// to do for init (before run)
		if ( decode_json4event_group_event_list(element, evtgrp, &id, PT_INIT, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list INIT created (%s)", evtgrp->id, errmsg);

		// when to do
		if ( decode_json4event_group_event_list(element, evtgrp, &id, PT_WORK, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evtgrp->id, errmsg);

		if ( decode_json4event_group_event_list(element, evtgrp, &id, PT_FINAL, errmsg, sz_errmsg) != ESP_OK )
			break; // decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evtgrp->id, errmsg);

		rc = ESP_OK;
	} while (0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg, "event created");
		ESP_LOGI(__func__,"id='%s': %s", evtgrp->id, errmsg);
		event_list_add(evtgrp);

	} else {
		ESP_LOGE(__func__, "id='%s': error: %s", sval, errmsg);
		// delete created data
		delete_event_group(evtgrp);
	}

	return rc;
}


/**
 * eventgroups list containes objects with
 *  - an id
 *  - a list of events
 *
 */
esp_err_t decode_json_list_of_events(cJSON *element, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	cJSON *found = NULL;
	int array_size;

	char *attr = "events";
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
		if (decode_json4event_group(element, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
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

