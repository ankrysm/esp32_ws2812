/*
 * create_events.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

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

static esp_err_t evt_get_number(cJSON *element, char *attr, double *val, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	*val = 0;
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return ESP_ERR_NOT_FOUND;
    }

    if (!cJSON_IsNumber(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a number", attr);
		return ESP_FAIL;
    }

    *val = cJSON_GetNumberValue(found);
    snprintf(errmsg, sz_errmsg, "got '%s'=%.2f", attr, *val);

	return ESP_OK;
}

static esp_err_t evt_get_string(cJSON *element, char *attr, char *sval, size_t, sz_sval,char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	memset(sval, 0,sz_sval);
	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;

	found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return ESP_ERR_NOT_FOUND;
    }

    if (!cJSON_IsString(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a string", attr);
		return ESP_FAIL;
    }

    strlcpy(sval, cJSON_GetStringValue(found),sz_sval);
    snprintf(errmsg, sz_errmsg, "got '%s'='%s'", attr, *sval);

	return ESP_OK;

}

static esp_err_t decode_json_getcolor_by_name(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	esp_err_t rc = evt_get_string(element, attr, sval, sizeof(sval), l_errmsg, sizeof(l_errmsg));

	if ( rc == ESP_OK) {
		T_NAMED_RGB_COLOR *nc= color4name(sval);
		if (nc == NULL) {
			snprintf(errmsg, sz_errmsg, "attr '%s': unknown color '%s'", attr, sval);
		} else {
			hsv->h = nc->hsv.h;
			hsv->s = nc->hsv.s;
			hsv->v = nc->hsv.v;
		}

	} else if (rc == ESP_ERR_NOT_FOUND) {
		snprintf(errmsg, sz_errmsg, "attr '%s': not found", attr);

	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}

static esp_err_t decode_json_getcolor_as_hsv(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

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
		}
	} else if (rc == ESP_ERR_NOT_FOUND) {
		snprintf(errmsg, sz_errmsg, "attr '%s': not found", attr);

	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}

static esp_err_t decode_json_getcolor_as_rgb(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	esp_err_t rc = evt_get_string(element, attr, sval, sizeof(sval), l_errmsg, sizeof(l_errmsg));

	if ( rc == ESP_OK) {
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
			c_hsv2rgb(hsv, &rgb);
		}
	} else if (rc == ESP_ERR_NOT_FOUND) {
		snprintf(errmsg, sz_errmsg, "attr '%s': not found", attr);

	} else {
		snprintf(errmsg, sz_errmsg, "attr '%s': access failed: %s", attr, l_errmsg);
	}
	return rc;
}


static esp_err_t decode_json4event_what(cJSON *element, T_EVENT *evt, char *errmsg, size_t sz_errmsg) {
	esp_err_t rc = ESP_FAIL;

	T_EVT_WHAT *w = NULL;
	T_COLOR_HSV hsv = {.h=0, .s=0, .v=0};

	char *attr;
	double val;
	char sval[64];
	int id = -1;
	esp_err_t lrc;

	do {
		// *** id *** a must have
		attr="id";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) != ESP_OK)
			break;
		id = val;
		if ( !(w = create_what(evt, id)))
			break;
		ESP_LOGI(__func__, "wid=%d: 'what' created", id);

		attr="type";
		if (evt_get_string(element, attr, sval, sizeof(sval), errmsg, sz_errmsg) != ESP_OK)
			break;
		w->type = TEXT2WT(sval);
		if ( w->type == WT_UNKNOWN) {
			snprintf(errmsg, sz_errmsg,"wid=%d: what type '%s' unknown", id, sval);
			break;
		}

		attr="pos";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			w->pos = val;
			ESP_LOGI(__func__, "wid=%d: %s=%d", attr, id, w->pos);
		}

		attr="len";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			w->len = val;
			ESP_LOGI(__func__, "wid=%d: %s=%d", attr, id, w->len);
		}

		if ( w->type == WT_COLOR_TRANSITION) {

		} else {
			// a color needed
			// by name?
			do {
				attr="color";
				lrc = decode_json_getcolor_by_name(element, attr, &hsv, errmsg, sz_errmsg);
				if ( lrc == ESP_OK ) {
					w->para.hsv.h = hsv.h;
					w->para.hsv.s = hsv.s;
					w->para.hsv.v = hsv.v;
					ESP_LOGI(__func__,"wid=%d, attr='%s', hsv=%d,%d,%d", w->id, attr, w->para.hsv.h, w->para.hsv.s, w->para.hsv.v);
					break;
				} else if (lrc != ESP_ERR_NOT_FOUND) {
					break;
				}

				attr="hsv";
				lrc = decode_json_getcolor_as_hsv(element, attr, &hsv, errmsg, sz_errmsg);
				if ( lrc == ESP_OK ) {
					w->para.hsv.h = hsv.h;
					w->para.hsv.s = hsv.s;
					w->para.hsv.v = hsv.v;
					ESP_LOGI(__func__,"wid=%d, attr='%s', hsv=%d,%d,%d", w->id, attr, w->para.hsv.h, w->para.hsv.s, w->para.hsv.v);
				} else if ( lrc != ESP_ERR_NOT_FOUND) {
					break;
				}
				attr="rgb";
				lrc = decode_json_getcolor_as_rgb(element, attr, &hsv, errmsg, sz_errmsg);
				if ( lrc == ESP_OK ) {
					w->para.hsv.h = hsv.h;
					w->para.hsv.s = hsv.s;
					w->para.hsv.v = hsv.v;
					ESP_LOGI(__func__,"wid=%d, attr='%s', hsv=%d,%d,%d", w->id, attr, w->para.hsv.h, w->para.hsv.s, w->para.hsv.v);
				}
			} while(0);
			if ( lrc != ESP_OK) {
				ESP_LOGI(__func__, "wid=%d: no color specified", w->id);
				break;
			}
		}

		/// end of color parameter

		rc = ESP_OK;
	} while(0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"wid=%d: 'what' created", id);
		ESP_LOGI(__func__,"%s", errmsg);
	} else {
		ESP_LOGE(__func__, "error: %s", errmsg);
	}

	return rc;
}
static esp_err_t decode_json4event_what_list(cJSON *element, T_EVENT *evt, char *errmsg, size_t sz_errmsg) {
	memset(errmsg, 0, sz_errmsg);
	if ( !element) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	char *attr = "what";
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "id=%d: missing attribute '%s'", evt->id, attr);
		return ESP_ERR_NOT_FOUND;
    }

    if (!cJSON_IsArray(found)) {
		snprintf(errmsg, sz_errmsg, "id=%d: attribute '%s' is not an array", evt->id, attr);
		return ESP_FAIL;
    }

	int array_size = cJSON_GetArraySize(found);
	ESP_LOGI(__func__, "id=%d: size of '%s'=%d", evt->id, attr, array_size);
	if (array_size == 0) {
		snprintf(errmsg, sz_errmsg, "id=%d: array '%s' has no content", evt->id, attr);
		break;
	}

	esp_err_t rc = ESP_OK;
	for (int i=0; i < array_size; i++) {
		cJSON *element = cJSON_GetArrayItem(found, i);
		JSON_Print(element);
		char l_errmsg[64];
		if (decode_json4event_what(element, evt, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
			snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
			rc = ESP_FAIL;
		}
	}

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg, "ok", evt->id);
	}

	return rc;

}


static esp_err_t decode_json4event_start(cJSON *element, char *errmsg, size_t sz_errmsg) {
	if ( !cJSON_IsObject(element)) {
		snprintf(errmsg, sz_errmsg, "element is not an object");
		return ESP_FAIL;
	}

	char *attr;
	double val;
	int id = -1;
	esp_err_t rc = ESP_FAIL;
	T_EVENT *evt = NULL;
	do {
		// *** id *** a must have
		attr="id";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) != ESP_OK)
			break;
		id = val;
		if ( !(evt = create_event(id)))
			break;
		ESP_LOGI(__func__, "id=%d: event created", id);

		attr="pos";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->pos = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->pos);
		}

		attr="len_factor";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->len_factor = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->len_factor);
		}

		attr="len_factor_delta";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->len_factor_delta = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->len_factor_delta);
		}

		attr="speed";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->speed = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->speed);
		}
		attr="acceleration";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->acceleration = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->acceleration);
		}

		attr="brightness";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->brightness = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->brightness);
		}

		attr="brightness_delta";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->brightness_delta = val;
			ESP_LOGI(__func__, "id=%d: %s=%.2f", attr, id, evt->brightness_delta);
		}

		attr="repeats";
		if (evt_get_number(element, attr, &val, errmsg, sz_errmsg) == ESP_OK) {
			evt->evt_time_list_repeats = val;
			ESP_LOGI(__func__, "id=%d: %s=%d", attr, id, evt->evt_time_list_repeats);
		}

		// what to do
		if ( decode_json4event_what_list(element, evt, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"id=%d: what list created", evt->id);

		rc = ESP_OK;
	} while (0);

	if ( rc == ESP_OK) {
		snprintf(errmsg, sz_errmsg,"event %d created");
		ESP_LOGI(__func__,"id=%d: %s", evt->id, errmsg);
	} else {
		ESP_LOGE(__func__, "id=%d: error: %s", evt->id, errmsg);
		// delete created data
		delete_event(evt);
	}

	return rc;
}

esp_err_t decode_json4event(char *content, char *errmsg, size_t sz_errmsg) {
	cJSON *tree = NULL;
	cJSON *jEvtList = NULL;
	esp_err_t rc = ESP_FAIL;

	ESP_LOGI(__func__, "content='%s'", content);
	memset(errmsg, 0, sz_errmsg);

	do {
		tree = cJSON_Parse(content);
		if ( !tree) {
			snprintf(errmsg, sz_errmsg, "could not decode content or could not allocate memory for JSON-Data");
			break;
		}

		JSON_Print(tree);

		if ( !cJSON_IsArray(tree)){
			snprintf(errmsg, sz_errmsg, "content is not an array");
			break;
		}

		int evtlist_array_size = cJSON_GetArraySize(tree);
		ESP_LOGI(__func__, "evtlist_array_size=%d", evtlist_array_size);
		if (evtlist_array_size == 0) {
			snprintf(errmsg, sz_errmsg, "array has no content");
			break;
		}

		rc = ESP_OK;
		for (int i=0;i<evtlist_array_size;i++) {
			cJSON *element = cJSON_GetArrayItem(tree, i);
			JSON_Print(element);
			char l_errmsg[64];
			if (decode_json4event_start(element, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
				snprintf(&(errmsg[strlen(errmsg)]), sz_errmsg - strlen(errmsg),"[%s]", l_errmsg);
				rc = ESP_FAIL;
			}
		}

	} while(false);

	if ( tree)
		cJSON_Delete(tree);

	ESP_LOGI(__func__, "done: %s", errmsg);
	return rc;
}
