/*
 * decode_json.c
 *
 *  Created on: 03.11.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

extern char *cfg_autoplayfile;


/**
 * json data should contain 3 lists:
 * "objects" - what do paint
 * "events" - list of events
 * "tracks" - list of max. 16 tracks
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

		if ( decode_json_list_of_events(tree, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"event list created");

		if ( decode_json_list_of_tracks(tree, errmsg, sz_errmsg) != ESP_OK )
			break; // no list or decode error
		ESP_LOGI(__func__,"track list created");


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
			log_err(__func__,"could not load '%s'", filename);
			rc = ESP_ERR_NOT_FOUND;
			break;
		}

		content = calloc(st.st_size+1, sizeof(char));

		FILE *f = fopen(fname, "r");
		if (f == NULL) {
			log_err(__func__, "Failed to open file for reading");
			rc = ESP_FAIL;
			break;
		}

		fread(content, sizeof(char),st.st_size, f);
		fclose(f);

		rc = decode_json4event_root(content, errmsg, sz_errmsg);

	} while(0);

	if ( rc == ESP_OK ) {
		log_info(__func__, "decode '%s' successfull", filename);
		cfg_trans_flags |=CFG_AUTOPLAY_LOADED;

		strlcpy(last_loaded_file, filename, LEN_PATH_MAX);

	} else {
		log_err(__func__, "could not decode '%s': %s", filename, errmsg);
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

	log_info(__func__, "load file %s", cfg_autoplayfile);
	esp_err_t res = load_events_from_file(cfg_autoplayfile, errmsg, sizeof(errmsg));
	if (res != ESP_OK) {
		log_err(__func__, "load autostart file %s failed:%s", cfg_autoplayfile, errmsg);
	}
	return res;
}

