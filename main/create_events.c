/*
 * create_events.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

/*

    data example:


// decode_json4event: 2 lists: objects, events
		{
// decode_json4event_object_list: objects
			 "objects" : [
//decode_json4event_object
				 {  "id":"o1",
					"list" :[
// decode_json4event_object_data:
						{"type":"color_transition", "color_from":"blue", "color_to":"red", "pos":0, "len":7},
						{"type":"color", "color":"yellow", "pos":7, "len":7},
						{"type":"color_transition", "color_from":"red", "color_to":"blue", "pos":14, "len":7}
					 ]
				  }
			  ],
// decode_json4event_event_list: events
			 "events" : [
// decode_json4event_event:
				{
				 "id":"10",
// decode_json4event_evt_time_init_list:
				 "init": [
// decode_json4event_evt_time  (for_init=true) :
					{"type":"repeat_count", "value":5},
					{"type":"jump", "value":10},
					{"type":"object","value":"o1"}
				 ],
// decode_json4event_evt_time_list:
				 "work": [
// decode_json4event_evt_time (for_init=false):
					{"type":"speed", "value":2.5},
					{"type":"bounce", "time": 1000},
					{"type":"continue", "time":1000}
				 ]
				}
			 ]
		}

*/

/**
 * print JSON-Element-Type (for testing)
 * /
static void JSON_Print(cJSON *element) {
	if (!element) {
		ESP_LOGI(__func__, "data missing");
		return;
	}
	if (element->type == cJSON_Invalid) ESP_LOGI(__func__, "cJSON_Invalid");
	if (element->type == cJSON_False) ESP_LOGI(__func__, "cJSON_False");
	if (element->type == cJSON_True) ESP_LOGI(__func__, "cJSON_True");
	if (element->type == cJSON_NULL) ESP_LOGI(__func__, "cJSON_NULL");
	if (element->type == cJSON_Number) ESP_LOGI(__func__, "cJSON_Number int=%d double=%f", element->valueint, element->valuedouble);
	if (element->type == cJSON_String) ESP_LOGI(__func__, "cJSON_String string=%s", element->valuestring);
	if (element->type == cJSON_Array) ESP_LOGI(__func__, "cJSON_Array");
	if (element->type == cJSON_Object) ESP_LOGI(__func__, "cJSON_Object");
	if (element->type == cJSON_Raw) ESP_LOGI(__func__, "cJSON_Raw");
}
//*/

/**
 * reads a boolean value for an attribute in the current JSON node
 */
t_result evt_get_bool(cJSON *element, char *attr, bool *val, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return RES_FAILED;
	}

	cJSON *found = NULL;
	*val = 0;
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return RES_NOT_FOUND;
    }

    if (!cJSON_IsBool(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a boolean", attr);
		return RES_INVALID_DATA_TYPE;
    }

    *val = cJSON_IsTrue(found);
    snprintf(errmsg, sz_errmsg, "got '%s'=%s", attr, (*val? "true" : "false"));

	return RES_OK;
}


/**
 * reads a numeric value for an attribute in the current JSON node
 */
static t_result evt_get_number(cJSON *element, char *attr, double *val, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return RES_FAILED;
	}

	cJSON *found = NULL;
	*val = 0;
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return RES_NOT_FOUND;
    }

    if (!cJSON_IsNumber(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a number", attr);
		return RES_INVALID_DATA_TYPE;
    }

    *val = cJSON_GetNumberValue(found);
    snprintf(errmsg, sz_errmsg, "got '%s'=%f", attr, *val);

	return RES_OK;
}

/**
 * reads a string for an attribut in the current JSON node
 */
static t_result evt_get_string(cJSON *element, char *attr, char *sval, size_t sz_sval, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	memset(sval, 0,sz_sval);

	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return RES_FAILED;
	}
	cJSON *found = NULL;

	found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return RES_NOT_FOUND;
    }

    if (!cJSON_IsString(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a string", attr);
		return RES_INVALID_DATA_TYPE;
    }

    strlcpy(sval, cJSON_GetStringValue(found),sz_sval);

    if ( !strlen(sval)) {
        snprintf(errmsg, sz_errmsg, "got '%s' has no value", attr, sval);
    	return RES_NO_VALUE;
    }

    snprintf(errmsg, sz_errmsg, "got '%s'='%s'", attr, sval);
	return RES_OK;

}

/**
 * reads a color by name from the attribute and converts it to HSV
 */
static esp_err_t decode_json_getcolor_by_name(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	if ( !attr || !strlen(attr))
		return ESP_ERR_NOT_FOUND;

	t_result rc = evt_get_string(element, attr, sval, sizeof(sval), l_errmsg, sizeof(l_errmsg));

	if ( rc == RES_OK) {
		T_NAMED_RGB_COLOR *nc= color4name(sval);
		if (nc == NULL) {
			snprintf(errmsg, sz_errmsg, "attr '%s': unknown color '%s'", attr, sval);
		} else {
			hsv->h = nc->hsv.h;
			hsv->s = nc->hsv.s;
			hsv->v = nc->hsv.v;
			snprintf(errmsg, sz_errmsg, "'color': attr '%s': hsv=%d,%d,%d", attr, hsv->h, hsv->s, hsv->v );
		}

	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}

/**
 * reads a HSV-color from attribute
 */
static t_result decode_json_getcolor_as_hsv(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	if ( !attr || !strlen(attr))
		return ESP_ERR_NOT_FOUND;

	esp_err_t rc = evt_get_string(element, attr, sval, sizeof(sval), l_errmsg, sizeof(l_errmsg));

	if ( rc == ESP_OK) {
		// expected "h,s,v"
		char *s=strdup(sval), *sH=NULL, *sS=NULL, *sV=NULL, *l;
		int32_t iH=-1, iS=-1, iV=-1;
		sH=strtok_r(s, ",", &l);
		if (sH) {
			iH=atoi(sH);
			sS=strtok_r(NULL, ",", &l);
		}
		if (sS) {
			iS = atoi(sS);
			sV=strtok_r(NULL, ",", &l);
		}
		if (sV) {
			iV=atoi(sV);
		}
		free(s);

		if ( iH<0 || iH >360 || iS<0 || iS >100|| iV<0 || iV>100) {
			snprintf(errmsg, sz_errmsg, "attr '%s': '%s' is not a valid HSV", attr, sval);
			rc = ESP_FAIL;
		} else {
			hsv->h = iH;
			hsv->s = iS;
			hsv->v = iV;
			snprintf(errmsg, sz_errmsg, "'hsv': attr '%s': hsv=%d,%d,%d", attr, hsv->h, hsv->s, hsv->v );
		}

	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}

/**
 * gets an RGB color definition as HSV
 */
static t_result decode_json_getcolor_as_rgb(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	if ( !attr || !strlen(attr))
		return ESP_ERR_NOT_FOUND;

	t_result rc = evt_get_string(element, attr, sval, sizeof(sval), l_errmsg, sizeof(l_errmsg));

	if ( rc == RES_OK) {
		char *s=strdup(sval), *sR=NULL, *sG=NULL, *sB=NULL, *l;
		int32_t iR=-1, iG=-1, iB=-1;
		sR=strtok_r(s, ",", &l);
		if (sR) {
			iR=atoi(sR);
			sG=strtok_r(NULL, ",", &l);
		}
		if (sG) {
			iG = atoi(sG);
			sB=strtok_r(NULL, ",", &l);
		}
		if (sB) {
			iB=atoi(sB);
		}
		free(s);

		if ( iR<0 || iR > 255 || iG<0 || iG >255|| iB<0 || iB>255) {
			snprintf(errmsg, sz_errmsg, "attr '%s': '%s' is not a valid RGB", attr, sval);
			rc = ESP_FAIL;
		} else {
			T_COLOR_RGB rgb={.r=iR, .g=iG, .b=iB};
			c_rgb2hsv(&rgb,hsv);
			snprintf(errmsg, sz_errmsg, "'rgb': attr '%s': rgb=%s hsv=%d,%d,%d", attr, sval, hsv->h, hsv->s, hsv->v );
		}
	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}

/**
 * tries different methods to get a color
 */
static esp_err_t decode_json_getcolor(cJSON *element, char *attr4colorname, char *attr4hsv, char *attr4rgb, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc;
	rc = decode_json_getcolor_by_name(element, attr4colorname, hsv, errmsg, sz_errmsg);
	if (rc != ESP_ERR_NOT_FOUND)
		return rc;

	rc = decode_json_getcolor_as_hsv(element, attr4hsv, hsv, errmsg, sz_errmsg);
	if (rc != ESP_ERR_NOT_FOUND)
		return rc;

	rc = decode_json_getcolor_as_rgb(element, attr4rgb, hsv, errmsg, sz_errmsg);

	return rc;
}

/**
 * reads a single T_EVT_TIME element
 */
static esp_err_t decode_json4event_evt_time(cJSON *element, uint32_t id, T_EVENT *evt, t_processing_type processing_type, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_EVT_TIME *t = NULL;

	char *attr;
	double val;
//	bool bval;
	char sval[64];
	char hint[32]; // for logging

	do {
		switch ( processing_type) {
		case PT_INIT:
			snprintf(hint, sizeof(hint),"%s", "INIT");
			t = create_timing_event_init(evt,id);
			break;
		case PT_WORK:
			snprintf(hint, sizeof(hint),"%s", "WORK");
			t = create_timing_event(evt, id);
			break;
		case PT_FINAL:
			snprintf(hint, sizeof(hint),"%s", "FINAL");
			t = create_timing_event_final(evt, id);
			break;
		}
		if ( !t ) {
			snprintf(errmsg, sz_errmsg,"%s/%s/%d: could not create event data", hint, evt->oid, id);
			break;

		}
		ESP_LOGI(__func__, "%s/%s/%d: 'time event' created", hint, evt->oid, id);

		attr="type";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;
		t->type = TEXT2ET(sval);
		if ( t->type == ET_UNKNOWN) {
			snprintf(errmsg, sz_errmsg,"%s/%s/%d: attr='%s' '%s' unknown", hint, evt->oid, id, attr, sval);
			break;
		}

		attr="time"; // optional default 0
		t->time = 0;
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			t->time = val;
			ESP_LOGI(__func__, "tid=%d: %s=%llu", id, attr, t->time);
		}

		attr="marker"; // optional, for jump to
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == RES_OK) {
			strlcpy(t->marker, sval,LEN_EVT_MARKER );
			ESP_LOGI(__func__, "%s/%s/%d: %s='%s'", hint, evt->oid, id, attr, t->marker);
		}

//		attr="init";
//		if (evt_get_bool(element, attr, &bval, errmsg, sz_errmsg) == ESP_OK) {
//			if ( bval) {
//				t->status |= TE_FOR_INIT; // start with wait
//			}
//			ESP_LOGI(__func__, "id=%d: %s=%s", id, attr, (bval?"TRUE":"FALSE"));
//		}
//		ESP_LOGI(__func__, "id=%d: status=0x%04x", id, t->status);


		//		attr="oid"; // optional
//		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == RES_OK) {
//			strlcpy(t->oid, sval,LEN_EVT_MARKER );
//			ESP_LOGI(__func__, "tid=%d: %s='%s'", id, attr, t->oid);
//		}
		/*
		attr="set_flags";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == ESP_OK) {
			t->set_flags = TEXT2EVFL(sval);
			if ( t->set_flags == EVFL_UNKNOWN) {
				snprintf(errmsg, sz_errmsg,"tid=%d: attr='%s' '%s' unknown", id, attr, sval);
				break;
			}
		}

		attr="clear_flags";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == ESP_OK) {
			t->clear_flags = TEXT2EVFL(sval);
			if ( t->clear_flags == EVFL_UNKNOWN) {
				snprintf(errmsg, sz_errmsg,"tid=%d: attr='%s' '%s' unknown", id, attr, sval);
				break;
			}
		}
	    */
		attr="value"; // may be number or string
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == RES_OK) {
			t->value = val;
			ESP_LOGI(__func__, "%s/%s/%d: %s=%.3f",hint, evt->oid,  id, attr, t->value);
			switch(t->type) {
			case ET_SET_REPEAT_COUNT:
				switch ( processing_type ) {
				case PT_INIT:
					evt->evt_time_list_repeats = t->value;
					ESP_LOGI(__func__, "%s/%s/%d: repeats -> %d", hint, evt->oid, id, attr, evt->evt_time_list_repeats);
					break;
				default:
					snprintf(errmsg, sz_errmsg,"%s only in 'init' section", ET2TEXT(t->type));
				}
				break;

			default:
				break;
			}
		} else if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == RES_OK) {
			strlcpy(t->svalue, sval,sizeof(t->svalue));
			ESP_LOGI(__func__, "%s/%s/%d: %s='%s'", hint, evt->oid, id, attr, t->svalue);
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

static esp_err_t decode_json4event_evt_time_list(cJSON *element, T_EVENT *evt, int *id, t_processing_type processing_type, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
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
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
    	// its ok if there are no items
		snprintf(errmsg, sz_errmsg, "id=%s:  missing '%s', no timing events", evt->oid, attr);
		return ESP_OK;
    }

    if (!cJSON_IsArray(found)) {
		snprintf(errmsg, sz_errmsg, "id=%s: attribute '%s' is not an array", evt->oid, attr);
		return ESP_FAIL;
    }

	int array_size = cJSON_GetArraySize(found);
	ESP_LOGI(__func__, "id='%s': size of '%s'=%d", evt->oid, attr, array_size);
	if (array_size == 0) {
		snprintf(errmsg, sz_errmsg, "id='%s': array '%s' has no content", evt->oid, attr);
		return ESP_ERR_NOT_FOUND;
	}

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		//JSON_Print(element);
		char l_errmsg[64];
		(*id)++;
		if (decode_json4event_evt_time(element, *id, evt, processing_type, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
			rc = ESP_FAIL;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg, "id='%s': ok", evt->oid);
	}

	return rc;

}

static esp_err_t decode_json4event_object_data(cJSON *element, T_EVT_OBJECT *obj, int id, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_EVT_OBJECT_DATA *data;

	char *attr;
	char sval[64];
	double val;
	t_result lrc;
	T_COLOR_HSV hsv = {.h=0, .s=0, .v=0};

	do {
		if ( !(data = create_object_data(obj, id))) {
			snprintf(errmsg, sz_errmsg,"oid=%s, id=%d: could not create data object", obj->oid, id);
			break;
		}

		attr="type";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;
		data->type = TEXT2WT(sval);
		if ( data->type == WT_UNKNOWN) {
			snprintf(errmsg, sz_errmsg,"oid=%s, id=%d: object type '%s' unknown", obj->oid, id, sval);
			break;
		}

		attr="pos";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == RES_OK) {
			data->pos = val;
			ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d", obj->oid, id, attr, data->pos);
		}

		attr="len";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == RES_OK) {
			data->len = val;
			ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d", obj->oid, id, attr, data->len);
		}

		if ( data->type == WT_COLOR_TRANSITION) {
			// a color from needed
			lrc = decode_json_getcolor(element, "color_from", "hsv_from", "rgb_from", &hsv, errmsg, sz_errmsg);
			if ( lrc == RES_OK ) {
				data->para.tr.hsv_from.h = hsv.h;
				data->para.tr.hsv_from.s = hsv.s;
				data->para.tr.hsv_from.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d: %s", obj->oid, id, errmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no 'color from' specified", obj->oid);
				ESP_LOGI(__func__, "%s",errmsg);
				break;

			} else {
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s",obj->oid, id, errmsg);
				break; // failed
			}

			// a color to needed
			lrc = decode_json_getcolor(element, "color_to", "hsv_to", "rgb_to", &hsv, errmsg, sz_errmsg);
			if ( lrc == RES_OK ) {
				data->para.tr.hsv_to.h = hsv.h;
				data->para.tr.hsv_to.s = hsv.s;
				data->para.tr.hsv_to.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d: %s", obj->oid, id, errmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no 'color to' from specified", obj->oid, id);
				ESP_LOGI(__func__, "%s",errmsg);
				break;
			} else {
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s", obj->oid, id, errmsg);
				break; // failed
			}

		} else {
			// a color needed
			lrc = decode_json_getcolor(element, "color", "hsv", "rgb", &hsv, errmsg, sz_errmsg);
			if ( lrc == RES_OK ) {
				data->para.hsv.h = hsv.h;
				data->para.hsv.s = hsv.s;
				data->para.hsv.v = hsv.v;
				ESP_LOGI(__func__,"oid=%s, id=%d, %s", obj->oid, id, errmsg);

			} else if ( lrc == RES_NOT_FOUND ) {
				snprintf(errmsg, sz_errmsg, "oid=%s, id=%d: no color specified", obj->oid, id);
				ESP_LOGI(__func__, "%s",errmsg);
				break;

			} else {
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
static esp_err_t decode_json4event_object(cJSON *element, bool overwrite, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_EVT_OBJECT *obj = NULL;

	char *attr;
	char sval[64];

	do {
		attr="id";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != RES_OK)
			break;
		ESP_LOGI(__func__, "%s='%s'", attr, sval);

		obj = find_object4oid(sval);
		if ( obj ) {
			ESP_LOGI(__func__, "object '%s' found, todo: %s", obj->oid, (overwrite?"overwrite":"add"));
			if(overwrite)  {
				if ( delete_object_by_oid(sval) != ESP_OK) {
					snprintf(errmsg, sz_errmsg,"duplicate object with oid '%s', failed to delete", sval);
					break;
				}
			} else {
				snprintf(errmsg, sz_errmsg,"duplicate object with oid '%s'", sval);
				break;
			}
		} else {
			ESP_LOGI(__func__, "object '%s' is new", sval);
		}
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
				snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
				rc = ESP_FAIL;
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
 * (1)
 * "object" structure contains
 * - a list of
 *   - an oid and
 *   - a data list
 */
static esp_err_t decode_json4event_object_list(cJSON *element, bool overwrite, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	char *attr = "objects";
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

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		//JSON_Print(element);
		char l_errmsg[64];
		if (decode_json4event_object(element, overwrite, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
			rc = ESP_FAIL;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
	}

	return rc;

}


static esp_err_t decode_json4event_event(cJSON *element, bool overwrite, char *errmsg, size_t sz_errmsg) {
	if ( !cJSON_IsObject(element)) {
		snprintf(errmsg, sz_errmsg, "element is not an object");
		return ESP_FAIL;
	}

	char *attr;
	//double val;
	char sval[64];
	//bool bval;
	//int id = -1;
	esp_err_t lrc, rc = ESP_FAIL;
	T_EVENT *evt = NULL;

	memset(sval, 0, sizeof(sval));
	do {
		attr="id";
		if ((lrc=evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == RES_OK)) {
			//id = val;
			ESP_LOGI(__func__, "id=%s: by json", sval);
			T_EVENT *t = find_event(sval);
			if ( t ) {
				if ( overwrite ) {
					ESP_LOGI(__func__, "id='%s': event already exists, will be deleted", sval);
					lrc = delete_event_by_id(sval);
					if ( lrc != ESP_OK) {
						snprintf(errmsg, sz_errmsg, "event with id '%s' already exists, delete failed rc=%d", sval, lrc);
						break;
					}

				} else {
					snprintf(errmsg, sz_errmsg, "event with id '%s' already exists", sval);
					break;
				}
			} else {
				ESP_LOGI(__func__, "id='%s': doesn't exist", sval);
			}
		} else if (lrc==ESP_ERR_NOT_FOUND) {
			// calculate a new one
			get_new_event_id(sval, sizeof(sval));
			ESP_LOGI(__func__, "id='%s': auton´matically created", sval);
		} else {
			break; // error
		}
		if ( !(evt = create_event(sval)))
			break;
		ESP_LOGI(__func__, "id='%s': event created", evt->oid);

//		attr="pos";
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->pos = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->pos);
//		}

//		attr="len_factor"; // optional, default 1.0
//		evt->len_factor = 1.0;
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->len_factor = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->len_factor);
//		}

//		attr="len_factor_delta"; // optional default 0.0
//		evt->len_factor_delta = 0.0;
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->len_factor_delta = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->len_factor_delta);
//		}

//		attr="speed"; // optional default 0.0
//		evt->speed = 0.0;
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->speed = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->speed);
//		}

//		attr="acceleration"; // optional, default 0.0.
//		evt->acceleration = 0.0;
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->acceleration = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->acceleration);
//		}

//		attr="brightness";
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->brightness = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->brightness);
//		}

//		attr="brightness_delta";
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->brightness_delta = val;
//			ESP_LOGI(__func__, "id=%d: %s=%.2f", id, attr, evt->brightness_delta);
//		}

//		attr="repeats";
//		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
//			evt->evt_time_list_repeats = val;
//			ESP_LOGI(__func__, "id=%d: %s=%d", id, attr, evt->evt_time_list_repeats);
//		}

//		attr="oid";
//		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) == RES_OK) {
//			strlcpy(evt->object_oid, sval, LEN_EVT_MARKER);
//			ESP_LOGI(__func__, "id=%d: %s=%s", id, attr, evt->object_oid);
//		}

//		attr="start_with_wait";
//		if (evt_get_bool(element, attr, &bval, errmsg, sz_errmsg) == ESP_OK) {
//			if ( bval) {
//				evt->flags = EVFL_WAIT; // start with wait
//			}
//			ESP_LOGI(__func__, "id=%d: %s=0x%04x", id, attr, evt->flags);
//		}
		// what to do
//		if ( decode_json4event_what_list(element, evt, errmsg, sz_errmsg) != ESP_OK )
//			break; // no list or decode error
//		ESP_LOGI(__func__,"id=%d: what list created", evt->id);

		int id=0;
		// to do for init (before run)
		if ( decode_json4event_evt_time_list(element, evt, &id, PT_INIT, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"id='%s': evt_time list INIT created (%s)", evt->oid, errmsg);

		// when to do
		if ( decode_json4event_evt_time_list(element, evt, &id, PT_WORK, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evt->oid, errmsg);

		if ( decode_json4event_evt_time_list(element, evt, &id, PT_FINAL, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"id='%s': evt_time list WORK created (%s)", evt->oid, errmsg);

		rc = ESP_OK;
	} while (0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"event %d created");
		ESP_LOGI(__func__,"id='%s': %s", evt->oid, errmsg);
		event_list_add(evt);
	} else {
		ESP_LOGE(__func__, "id='%s': error: %s", sval, errmsg);
		// delete created data
		delete_event(evt);
	}

	return rc;
}

static esp_err_t decode_json4event_event_list(cJSON *element, bool overwrite, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	char *attr = "events";
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

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		//JSON_Print(element);
		char l_errmsg[64];
		if (decode_json4event_event(element, overwrite, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
			rc = ESP_FAIL;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"ok");
	}

	return rc;

}


/**
 * (0)
 */
esp_err_t decode_json4event(char *content, bool overwrite, char *errmsg, size_t sz_errmsg) {
	cJSON *tree = NULL;
	esp_err_t rc = ESP_FAIL;

	ESP_LOGI(__func__, "content='%s'", content);
	memset(errmsg, 0, sz_errmsg);

	do {
		tree = cJSON_Parse(content);
		if ( !tree) {
			snprintf(errmsg, sz_errmsg, "could not decode JSON-Data");
			break;
		}

		//JSON_Print(tree);

		if ( decode_json4event_object_list(tree, overwrite, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"object list created");


		if ( decode_json4event_event_list(tree, overwrite, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"event list created");
		rc = ESP_OK;

	} while(false);

	if ( tree)
		cJSON_Delete(tree);

	ESP_LOGI(__func__, "done: %s", errmsg);
	return rc;
}
