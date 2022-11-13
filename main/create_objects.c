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

static esp_err_t decode_json4event_object_data(cJSON *element, T_DISPLAY_OBJECT *obj, int id, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_DISPLAY_OBJECT_DATA *object_data;

	//char *attr;
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

		if ( (lrc = decode_object_attr_string(element, OBJATTR_TYPE, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg))) != RES_OK) {
			snprintf(errmsg, sz_errmsg,"oid=%s, id=%d: error decoding attribute: %s", obj->oid, id, lerrmsg);
			break;
		}
		T_OBJECT_CONFIG *obj_cfg = object4type_name(sval);
		if ( !obj_cfg) {
			snprintf(errmsg, sz_errmsg,"object type '%s' unknown", sval);
			break;
		}

		object_data->type = obj_cfg->type;

		ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d(%s)", obj->oid, id, object_attrtype2text(OBJATTR_TYPE), object_data->type, object_type2text(object_data->type));


		lrc = decode_object_attr_numeric(element, OBJATTR_LEN, &val, lerrmsg, sizeof(lerrmsg));
		if (lrc== RES_OK) {
			object_data->len = val;
			ESP_LOGI(__func__, "oid=%s, id=%d: %s=%d", obj->oid, id, object_attrtype2text(OBJATTR_LEN), object_data->len);
		} else if ( lrc != RES_NOT_FOUND) {
			snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode data '%s': %s", obj->oid, id, object_attrtype2text(OBJATTR_LEN), lerrmsg);
			break;
		}

		// *** additional attributes depends on object type
		if (object_data->type == OBJT_RAINBOW) {
			// no attributes needed

		} else if (object_data->type == OBJT_BMP) {
			//url needed
			if ( (lrc = decode_object_attr_string(element, OBJATTR_URL, sval, sizeof(sval), lerrmsg, sizeof(lerrmsg))) != RES_OK) {
				snprintf(errmsg, sz_errmsg,"oid=%s, id=%d: error decoding attribute: %s", obj->oid, id, lerrmsg);
				break;
			}
			object_data->para.url = strdup(sval);
			ESP_LOGI(__func__,"oid=%s, id=%d: url='%s'", obj->oid, id, object_data->para.url);

		} else if ( object_data->type == OBJT_COLOR_TRANSITION) {
			// a "color from" needed
			lrc = decode_json_getcolor(element, OBJATTR_COLOR_FROM, OBJATTR_HSV_FROM, OBJATTR_RGB_FROM, &hsv, lerrmsg, sizeof(lerrmsg));
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
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data : %s", obj->oid, id, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s",obj->oid, id, errmsg);
				break; // failed
			}

			// a "color to" needed
			lrc = decode_json_getcolor(element, OBJATTR_COLOR_TO, OBJATTR_HSV_TO, OBJATTR_RGB_TO, &hsv, lerrmsg, sizeof(lerrmsg));
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
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data: %s", obj->oid, id, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s", obj->oid, id, lerrmsg);
				break; // failed
			}

		} else {
			// default: a color needed
			lrc = decode_json_getcolor(element,OBJATTR_COLOR, OBJATTR_HSV, OBJATTR_RGB, &hsv, lerrmsg, sizeof(lerrmsg));
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
				snprintf(errmsg, sz_errmsg,"id=%s, id=%d: could not decode object_data: %s", obj->oid, id, lerrmsg);
				ESP_LOGE(__func__, "oid=%s, id=%d: Error: %s", obj->oid, id, errmsg);
				break; // failed
			}
		}

		/// end of additional  parameter

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
 * - id
 * - list of display objects
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
 * decode an "object" list
 */
esp_err_t decode_json4event_object_list(cJSON *element, char *errmsg, size_t sz_errmsg) {
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


