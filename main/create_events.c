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

static esp_err_t evt_get_number(cJSON *element, char *attr, int *val, char *errmsg, size_t sz_errmsg) {
	if ( !element || !attr) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element' or 'attr'");
		return ESP_FAIL;
	}
	cJSON *found = NULL;
	*val = 0;
    found = cJSON_GetObjectItemCaseSensitive(element, attr);
    if ( !found) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return ESP_FAIL;
    }

    if (!cJSON_IsNumber(found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not a number", attr);
		return ESP_FAIL;
    }

    *val = cJSON_GetNumberValue(found);
    ESP_LOGI(__func__, "got number: '%s'=%d", attr, *val);

	return ESP_OK;

}

static esp_err_t decode_json4event_element(cJSON *element, char *errmsg, size_t sz_errmsg) {
	if ( !cJSON_IsObject(element)) {
		snprintf(errmsg, sz_errmsg, "element is not an object");
		return ESP_FAIL;
	}

	char *attr;

	int id;

	attr="id";
	if (evt_get_number(element, attr, &id, errmsg, sz_errmsg) != ESP_OK) {
		return ESP_FAIL;
	}
    ESP_LOGI(__func__, "event with id %d", id);

	return ESP_OK;
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
			if (decode_json4event_element(element, l_errmsg, sizeof(l_errmsg)) != ESP_OK) {
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
