/*
 * create_tracks.c
 *
 *  Created on: 03.11.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"




static esp_err_t decode_json4track_element(cJSON *element, int tidx, int *id, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_TRACK_ELEMENT *ele = NULL;

	char *attr;
	char sval[64];
	double val;
	t_result lrc;
	do {
		(*id)++;
		ESP_LOGI(__func__, "use track %d, id=%d", tidx, *id);
		ele = create_track_element(tidx, *id);
		if ( !ele) {
			snprintf(errmsg, sz_errmsg,"couldn't create track element");
			break;

		}
		attr = "repeat";
		lrc = evt_get_number(element, attr, &val, errmsg, sz_errmsg);
		if (lrc == RES_OK) {
			ele->repeats = val;
			if ( ele->repeats < 0 ) {
				ESP_LOGW(__func__, "number of repeats out of range, use 1");
				ele->repeats = 1;
			}
		}

		// get the T_EVENT_GROUP
		attr="event";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;

		T_EVENT_GROUP *evtgrp = find_event_group(sval);
		if ( !evtgrp) {
			snprintf(errmsg, sz_errmsg,"unknown event '%s'", sval);
			break;
		}
		ele->evtgrp = evtgrp;
		rc = ESP_OK;

	} while(false);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
		ESP_LOGI(__func__, "new element for track %d: repeat=%d,events='%s'", tidx, ele->repeats, ele->evtgrp->id);
	} else {
		ESP_LOGW(__func__, "couldn't create element for track %d: %s", tidx, errmsg);
	}

	return rc;
}


/**
 * tracks with id and list of elements
 */
static esp_err_t decode_json4tracks(cJSON *element, int *id, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	char *attr;
	char sval[64];
	double val;
	t_result lrc;
	int tidx=0;

	do {
		attr="id"; // track number
		if ( (lrc = evt_get_number(element, attr, &val, errmsg, sz_errmsg)) != RES_OK)
			break;
		tidx = val;
		ESP_LOGI(__func__, "%s='%d'", attr, tidx);
		if (tidx < 0 || tidx >= N_TRACKS ) {
			snprintf(errmsg, sz_errmsg, "track number out of range 0 .. %d", N_TRACKS -1 );
			break;
		}
		//track = &tracks[tidx];
		ESP_LOGI(__func__, "use track %d", tidx);

		// check for data list
		cJSON *found = NULL;
		int array_size =0;
		attr = "elements";

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

			if (decode_json4track_element(list_element, tidx, id, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
				snprintf(errmsg, sz_errmsg,"[%s]", l_errmsg);
				ESP_LOGE(__func__, "%s", errmsg);
				rc = ESP_FAIL;
				break;
			}
		}
	} while(false);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
		ESP_LOGI(__func__, "elements added to track %d", tidx);
	} else {
		ESP_LOGW(__func__, "could not add elements to track %d: %s", tidx, errmsg);
	}

	return rc;
}


/**
 * tracks list containes track elements
 *  - an id
 *  - a list of events
 *
 */
esp_err_t decode_json_list_of_tracks(cJSON *element, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	cJSON *found = NULL;
	int array_size;
	int id =0;

	char *attr = "tracks";
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
		if (decode_json4tracks(element, &id, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
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

