/*
 * json_util.c
 *
 *  Created on: 20.09.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

// ************ common functions ****************************
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
t_result evt_get_number(cJSON *element, char *attr, double *val, char *errmsg, size_t sz_errmsg) {

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
t_result evt_get_string(cJSON *element, char *attr, char *sval, size_t sz_sval, char *errmsg, size_t sz_errmsg) {

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
 * reads a list from a JSON node
 */
t_result evt_get_list(cJSON *element, char *attr, cJSON **found, int *array_size, char *errmsg, size_t sz_errmsg) {

	memset(errmsg, 0, sz_errmsg);

	*array_size = 0;
    *found = cJSON_GetObjectItemCaseSensitive(element, attr);

	if ( !element) {
		snprintf(errmsg, sz_errmsg, "missing parameter 'element'");
		return RES_FAILED;
	}

    if ( !(*found)) {
		snprintf(errmsg, sz_errmsg, "missing attribute '%s'", attr);
		return RES_NOT_FOUND;
    }

    if (!cJSON_IsArray(*found)) {
		snprintf(errmsg, sz_errmsg, "attribute '%s' is not an array", attr);
		return RES_INVALID_DATA_TYPE;
    }

	*array_size = cJSON_GetArraySize(*found);
	ESP_LOGI(__func__, "size of '%s'=%d", attr, *array_size);
	if (*array_size == 0) {
		snprintf(errmsg, sz_errmsg, "array '%s' has no content", attr);
		return RES_NOT_FOUND;
	}
	return RES_OK;
}

// *************************** color get functions *********************************************************
/**
 * reads a color by name from the attribute and converts it to HSV
 */
t_result decode_json_getcolor_by_name(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	memset(errmsg, 0, sz_errmsg);

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
t_result decode_json_getcolor_as_hsv(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	memset(errmsg, 0, sz_errmsg);

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
t_result decode_json_getcolor_as_rgb(cJSON *element, char *attr, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	char sval[32];
	char l_errmsg[64];

	memset(errmsg, 0, sz_errmsg);

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
t_result decode_json_getcolor(cJSON *element, char *attr4colorname, char *attr4hsv, char *attr4rgb, T_COLOR_HSV *hsv, char *errmsg, size_t sz_errmsg) {
	t_result rc;
	memset(errmsg, 0, sz_errmsg);

	rc = decode_json_getcolor_by_name(element, attr4colorname, hsv, errmsg, sz_errmsg);
	if (rc != ESP_ERR_NOT_FOUND)
		return rc;

	rc = decode_json_getcolor_as_hsv(element, attr4hsv, hsv, errmsg, sz_errmsg);
	if (rc != ESP_ERR_NOT_FOUND)
		return rc;

	rc = decode_json_getcolor_as_rgb(element, attr4rgb, hsv, errmsg, sz_errmsg);

	return rc;
}

void cJSON_addBoolean(cJSON *element, char *attribute_name, bool flag) {
	if ( flag ) {
		cJSON_AddTrueToObject(element, attribute_name);
	} else {
		cJSON_AddFalseToObject(element, attribute_name);
	}
}
