/**
 * HTTP Restful API Server - GET services
 * based on esp-idf examples
 */

#include "esp32_ws2812.h"


extern esp_vfs_spiffs_conf_t fs_conf;
extern T_LOG_ENTRY logtable[];
extern size_t sz_logtable;
extern int log_write_idx;

// from config.c
extern char sha256_hash_boot_partition[];
extern char sha256_hash_run_partition[];

// from global_data.c
extern T_TRACK tracks[];
extern T_DISPLAY_OBJECT *s_object_list;
extern T_EVENT_GROUP *s_event_group_list;
extern char last_loaded_file[];
extern size_t sz_last_loaded_file;


static void add_system_informations(cJSON *root) {
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);

	const esp_app_desc_t *app_desc = esp_app_get_description();

	size_t total,used;
	storage_info(&total,&used);

//	cJSON *sysinfo = cJSON_AddObjectToObject(root,"system");
//	cJSON *fs_size = cJSON_AddObjectToObject(sysinfo,"filesystem");

	cJSON_AddStringToObject(root, "version", IDF_VER);
	cJSON_AddNumberToObject(root, "cores", chip_info.cores);
	cJSON_AddNumberToObject(root, "free_heap_size",esp_get_free_heap_size());
	cJSON_AddNumberToObject(root, "minimum_free_heap_size",esp_get_minimum_free_heap_size());
	cJSON_AddNumberToObject(root, "filesystem_total", total);
	cJSON_AddNumberToObject(root, "filesystem_used", used);
	cJSON_AddStringToObject(root, "compile_date", app_desc->date);
	cJSON_AddStringToObject(root, "compile_time", app_desc->time);
	cJSON_AddStringToObject(root, "app_version", app_desc->version);
	cJSON_AddStringToObject(root, "app_name", app_desc->project_name);
	cJSON_AddStringToObject(root, "sha256boot", sha256_hash_boot_partition);
	cJSON_AddStringToObject(root, "sha256run", sha256_hash_run_partition);

}

void get_handler_list_err(httpd_req_t *req) {
	char buf[512];

	if( obtain_logsem_lock() != ESP_OK ) {
		ESP_LOGE(__func__, "xSemaphoreTake failed");
		return;
	}

	int pos=log_write_idx;
	for ( int i=0; i < N_LOG_ENTRIES; i++) {
		esp_err_t res = log_entry2text(pos, buf, sizeof(buf));
		pos++;
		if ( pos >= N_LOG_ENTRIES) {
			pos=0;
		}
		if ( res == ESP_ERR_NOT_FOUND )
			continue;

		strlcat(buf, "\n", sizeof(buf));
		httpd_resp_sendstr_chunk(req, buf);
	}
	release_logsem_lock();
}


void get_handler_clear_err(httpd_req_t *req) {

	if( obtain_logsem_lock() != ESP_OK ) {
		ESP_LOGE(__func__, "xSemaphoreTake failed");
		return;
	}

	log_write_idx = 0;
	memset(logtable, 0, sz_logtable);

	httpd_resp_sendstr_chunk(req, "done.\n");
	release_logsem_lock();
}


void get_handler_list(httpd_req_t *req) {
	// list events
	char buf[255];
	const size_t sz_buf = sizeof(buf);

	//extern T_SCENE *s_scene_list;
	if (obtain_eventlist_lock() != ESP_OK) {
		snprintf(buf, sz_buf, "%s couldn't get lock on eventlist\n", __func__);
		httpd_resp_sendstr_chunk(req, buf);
		return;
	}

	snprintf(buf, sz_buf, "\n*** objects ***\n");
	httpd_resp_sendstr_chunk(req, buf);

	if ( !s_object_list) {
		snprintf(buf, sz_buf, "   no objects\n");
		httpd_resp_sendstr_chunk(req, buf);

	} else {
		for (T_DISPLAY_OBJECT *obj=s_object_list; obj; obj=obj->nxt) {
			snprintf(buf, sz_buf, "   object '%s'\n", obj->oid);
			httpd_resp_sendstr_chunk(req, buf);
			if (obj->data) {
				for(T_DISPLAY_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
					if ( data->type == OBJT_BMP) {
						snprintf(buf, sz_buf,"    id=%d, type=%d/%s, len=%d, url=%s\n",
								data->id, data->type, object_type2text(data->type), data->len,
								data->para.bmp.url
						);
					} else if (data->type == OBJT_COLOR_TRANSITION) {
						snprintf(buf, sz_buf,"    id=%d, type=%d/%s, len=%d hsv from=%d/%d/%d hsv to=%d/%d/%d\n",
								data->id, data->type, object_type2text(data->type), data->len,
								data->para.tr.hsv_from.h, data->para.tr.hsv_from.s, data->para.tr.hsv_from.v,
								data->para.tr.hsv_to.h, data->para.tr.hsv_to.s, data->para.tr.hsv_to.v
						);
					} else if (data->type == OBJT_RAINBOW) {
						snprintf(buf, sz_buf,"    id=%d, type=%d/%s, len=%d\n",
								data->id, data->type, object_type2text(data->type), data->len
						);
					} else {
						snprintf(buf, sz_buf,"    id=%d, type=%d/%s, len=%d hsv=%d/%d/%d\n",
								data->id, data->type, object_type2text(data->type), data->len,
								data->para.hsv.h, data->para.hsv.s, data->para.hsv.v
						);
					}
					httpd_resp_sendstr_chunk(req, buf);

				}
			} else {
				snprintf(buf, sz_buf, "   no data list in object\n");
				httpd_resp_sendstr_chunk(req, buf);
			}

		}
	}

	snprintf(buf, sz_buf, "\n*** events ***\n");
	httpd_resp_sendstr_chunk(req, buf);

	if ( !s_event_group_list) {
		snprintf(buf, sz_buf, "   no events\n");
		httpd_resp_sendstr_chunk(req, buf);

	} else {
		for ( T_EVENT_GROUP *evtgrp= s_event_group_list; evtgrp; evtgrp = evtgrp->nxt) {
			snprintf(buf, sz_buf, "   Event id='%s':\n", evtgrp->id);
			httpd_resp_sendstr_chunk(req, buf);

			// INIT events
			if (evtgrp->evt_init_list) {
				snprintf(buf, sz_buf,"    INIT events:\n");
				httpd_resp_sendstr_chunk(req, buf);

				for (T_EVENT *evt = evtgrp->evt_init_list; evt; evt=evt->nxt) {
					httpd_resp_sendstr_chunk(req,"     ");
					event2text(evt, buf, sizeof(buf));
					httpd_resp_sendstr_chunk(req, buf);
					httpd_resp_sendstr_chunk(req,"\n");
				}
			} else {
				snprintf(buf, sz_buf,"    INIT events: none\n");
				httpd_resp_sendstr_chunk(req, buf);
			}

			// WORK events
			if (evtgrp->evt_work_list) {
				snprintf(buf, sz_buf,"    WORK events:\n");
				httpd_resp_sendstr_chunk(req, buf);

				for (T_EVENT *evt = evtgrp->evt_work_list; evt; evt=evt->nxt) {
					httpd_resp_sendstr_chunk(req,"     ");
					event2text(evt, buf, sizeof(buf));
					httpd_resp_sendstr_chunk(req, buf);
					httpd_resp_sendstr_chunk(req,"\n");

				}
			} else {
				snprintf(buf, sz_buf,"    WORK events: none\n.");
				httpd_resp_sendstr_chunk(req, buf);
			}

			// FINAL events
			if (evtgrp->evt_final_list) {
				snprintf(buf, sz_buf,"    FINAL events:\n");
				httpd_resp_sendstr_chunk(req, buf);

				for (T_EVENT *evt = evtgrp->evt_final_list; evt; evt=evt->nxt) {
					httpd_resp_sendstr_chunk(req,"     ");
					event2text(evt, buf, sizeof(buf));
					httpd_resp_sendstr_chunk(req, buf);
					httpd_resp_sendstr_chunk(req,"\n");

				}
			} else {
				snprintf(buf, sz_buf,"    FINAL events: none.\n");
				httpd_resp_sendstr_chunk(req, buf);
			}
		}
	}


	snprintf(buf, sz_buf, "\n*** tracks: ***\n");
	httpd_resp_sendstr_chunk(req, buf);

	for (int i = 0; i < N_TRACKS; i++) {
		T_TRACK *track = &(tracks[i]);
		if ( ! track->element_list) {
			snprintf(buf, sz_buf, "* track %2d is empty\n", i);
			httpd_resp_sendstr_chunk(req, buf);
			continue; // nothing to process
		}
		snprintf(buf, sz_buf, "* track %2d:\n", i);
		httpd_resp_sendstr_chunk(req, buf);
		for( T_TRACK_ELEMENT *ele = track->element_list; ele; ele = ele->nxt) {
			snprintf(buf,sz_buf, "    id=%d, event='%s', repeat=%d\n", ele->id, ele->evtgrp ? ele->evtgrp->id : "", ele->repeats);
			httpd_resp_sendstr_chunk(req, buf);
		}
	}

	httpd_resp_sendstr_chunk(req, "\n");

	release_eventlist_lock();
}

esp_err_t get_handler_file_list(httpd_req_t *req) {
    char entrypath[LEN_PATH_MAX];
    char msg[64];
    const char *entrytype;

    struct dirent *entry;
    struct stat entry_stat;

    DIR *dir = opendir(fs_conf.base_path);

    if (!dir) {
    	snprintf(msg, sizeof(msg),"Failed to stat dir : '%s'", fs_conf.base_path);
        ESP_LOGE(__func__, "%s", msg);
        // Respond with 404 Not Found
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON *files = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "files", files);

    // Iterate over all files / folders and fetch their names and sizes
    while ((entry = readdir(dir)) != NULL) {
        entrytype = (entry->d_type == DT_DIR ? "dir" : "file");

        snprintf(entrypath, sizeof(entrypath), "%s/%s",fs_conf.base_path, entry->d_name);
        if (stat(entrypath, &entry_stat) == -1) {
        	snprintf(msg, sizeof(msg),"Failed to stat '%s' : '%s'", entrytype, entry->d_name);
            ESP_LOGE(__func__, "%s", msg);
            closedir(dir);
    		snprintfapp(msg, sizeof(msg), "\n");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
            return ESP_FAIL;
        }
        cJSON *fentry = cJSON_CreateObject();
        cJSON_AddStringToObject(fentry,"type",  entrytype);
        cJSON_AddStringToObject(fentry,"name",  entry->d_name);
        cJSON_AddNumberToObject(fentry,"sz",entry_stat.st_size);
        char tbuf[64];
        get_shorttime4(entry_stat.st_mtime, tbuf, sizeof(tbuf));
        cJSON_AddStringToObject(fentry,"mtime",  tbuf);

        cJSON_AddItemToArray(files, fentry);

        snprintf(msg, sizeof(msg), "%s '%s' (%ld bytes, mtime %s)", entrytype, entry->d_name, entry_stat.st_size, tbuf);
        //httpd_resp_sendstr_chunk(req, msg);
        ESP_LOGI(__func__, "%s", msg);
    }
    closedir(dir);

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

    //httpd_resp_sendstr_chunk(req, "STATUS=");
	httpd_resp_sendstr_chunk(req, resp);
	httpd_resp_sendstr_chunk(req, "\n"); // response more readable

    free((void *)resp);
    cJSON_Delete(root);

    ESP_LOGI(__func__,"ended.");
    return ESP_OK;
}

/**
 * delivers status informations in JSON
 */
static void response_with_status(httpd_req_t *req, char *msg, run_status_type status) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    if (msg && strlen(msg))
    	cJSON_AddStringToObject(root, "msg", msg);
    cJSON_AddStringToObject(root, "status", RUN_STATUS_TYPE2TEXT(status));
    cJSON_AddNumberToObject(root, "scene_time", get_scene_time());
    cJSON_AddNumberToObject(root, "timer_period", get_event_timer_period());
    if (strlen(last_loaded_file)) {
    	cJSON_AddStringToObject(root, "last_loaded_file", last_loaded_file);
    }
    add_system_informations(root);

    char txt[64];
    get_current_timestamp(txt, sizeof(txt));
	cJSON_AddStringToObject(root, "current_time_stamp", txt);

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

    //httpd_resp_sendstr_chunk(req, "STATUS=");
	httpd_resp_sendstr_chunk(req, resp);
	httpd_resp_sendstr_chunk(req, "\n"); // response more readable

    free((void *)resp);
    cJSON_Delete(root);
}

void get_handler_status_current(httpd_req_t *req) {
	response_with_status(req, "", get_scene_status());
}

// run/pause/stop/blank
void get_handler_scene_new_status(httpd_req_t *req, run_status_type new_status) {

	bool changed = false;
	run_status_type old_status = get_scene_status();

	if ( new_status == RUN_STATUS_ASK) {
		new_status = old_status;
	} else {
		// not ask, set it
		if ( old_status != new_status) {
			changed = true;
			old_status = set_scene_status(new_status);
		}
	}
	httpd_resp_set_type(req, "application/json");

	cJSON *root = cJSON_CreateObject();

	char txt[32];

	switch (new_status) {
	case RUN_STATUS_STOPPED:
	case RUN_STATUS_STOP_AND_BLANK:
		strlcpy(txt,"STOPPED", sizeof(txt));
		break;
	case RUN_STATUS_RUNNING:
		if ( changed) {
			if ( old_status == RUN_STATUS_PAUSED) {
				strlcpy(txt, "CONTINUE", sizeof(txt));
			} else {
				strlcpy(txt, "STARTING", sizeof(txt));
			}
		} else {
			strlcpy(txt, "RUNNING", sizeof(txt));
		}
		break;
	case RUN_STATUS_PAUSED:
		strlcpy(txt,"PAUSE", sizeof(txt));
		break;
	default:
		strlcpy(txt,"UNKNOWN", sizeof(txt));
	}
	cJSON_AddStringToObject(root, "text", txt);
	cJSON_AddNumberToObject(root, "scene_time", get_scene_time());
	cJSON_AddStringToObject(root, "last_loaded_file", strlen(last_loaded_file) ? last_loaded_file :"");

	char *resp = cJSON_PrintUnformatted(root);
	ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

	httpd_resp_sendstr_chunk(req, resp);
	httpd_resp_sendstr_chunk(req, "\n"); // response more readable

	free((void *)resp);
	cJSON_Delete(root);

}



esp_err_t get_handler_clear(httpd_req_t *req) {
	char msg[255];

    LOG_MEM(__func__,1);
	// stop display program, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg), new_status);

    LOG_MEM(__func__,2);

    httpd_resp_set_type(req, "plain/text");
 	snprintf(msg,sizeof(msg), "clear data done");
 	httpd_resp_sendstr_chunk(req, msg);

	return res;
}

void get_handler_restart(httpd_req_t *req) {
	char resp_str[64];

	snprintf(resp_str, sizeof(resp_str),"Restart initiated\n");
	httpd_resp_sendstr_chunk(req, resp_str);

	httpd_resp_send_chunk(req, NULL, 0);

	ESP_LOGI(__func__,"Restarting in a second...\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(__func__, "Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void get_handler_reset(httpd_req_t *req) {

	// clear nvs
	nvs_flash_erase();
	scenes_stop(true);

	char resp_str[64];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");
	httpd_resp_sendstr_chunk(req, resp_str);

	get_handler_restart(req);
}

void get_handler_config(httpd_req_t *req, char *msg) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    if (msg && strlen(msg))
    	cJSON_AddStringToObject(root, "msg", msg);

    // configuration
    add_config_informations(root);

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_sendstr_chunk(req, resp);
	httpd_resp_sendstr_chunk(req, "\n");

    free((void *)resp);
    cJSON_Delete(root);

}

/**
 * load file from flash into memory
 */
esp_err_t get_handler_file_load(httpd_req_t *req, char *fname, size_t sz_fname) {
	char errmsg[128];
	char msg[256];

	memset(msg,0,sizeof(msg));
	memset(errmsg,0,sizeof(errmsg));

	ESP_LOGI(__func__, "fname '%s'", fname);

	// stop display program and clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	if ( clear_data(msg, sizeof(msg), new_status) != ESP_OK) {
		log_warn(__func__, "clear_data failed, %s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	// load new data
	if ( load_events_from_file(fname, errmsg, sizeof(errmsg)) != ESP_OK) {
		snprintfapp(msg,sizeof(msg), ", load content from %s failed: %s",fname, errmsg);
		log_warn(__func__, "load_events_from_file failed, %s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	// successful
	log_info(__func__,"content loaded from %s",fname);
	snprintf(last_loaded_file, sz_last_loaded_file,"%s", fname);

	snprintfapp(msg,sizeof(msg), ", content loaded from %s",fname);
	httpd_resp_sendstr_chunk(req, msg);
	return ESP_OK;
}

/**
 * delete flash file
 */
esp_err_t get_handler_file_delete(httpd_req_t *req, char *fname, size_t sz_fname) {
	char msg[256];
	memset(msg,0,sizeof(msg));

	int lrc;

	add_base_path(fname, sz_fname);
	if ((lrc=unlink(fname))) {
		snprintfapp(msg, sizeof(msg),", deletion '%s' failed, lrc=%d(%s)", fname, lrc, esp_err_to_name(lrc));
		ESP_LOGW(__func__, "error %s", msg);
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}
	snprintfapp(msg, sizeof(msg),", '%s' deleted", fname);
	return ESP_OK;
}

/**
 * get file content
 */
esp_err_t get_handler_file_get(httpd_req_t *req, char *fname, size_t sz_fname) {
	esp_err_t res = ESP_FAIL;
	char msg[255];

	add_base_path(fname, sz_fname);
	FILE *f = fopen(fname, "r");
	if (f == NULL) {
		snprintf(msg, sizeof(msg), "failed to open '%s' for reading", fname);
		ESP_LOGW(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, msg);
        return ESP_FAIL;

	}

    httpd_resp_set_type(req, "plain/text");

	size_t n;
	while ( (n=fread(msg, sizeof(char),sizeof(msg), f)) > 0) {
		httpd_resp_send_chunk(req, msg, n);
	}
	fclose(f);
	res = ESP_OK;


	return res;
}


/**
 * isn't a file name
 * is <r>/<g>/<b>/[len]/[start]
 * len default = 10
 * start default=0
 */
esp_err_t get_handler_test(httpd_req_t *req, char *fname, size_t sz_fname) {
	char msg[256];
	memset(msg,0,sizeof(msg));

	char *sR, *sG, *sB, *sLen, *sStart, *l, *s;
	unsigned long lR, lG, lB, lLen,lStart;
	lR=lG=lB=0;
	lLen=10;
	lStart=0;
	sR=sG=sB=sLen=sStart=NULL;

	T_COLOR_RGB rgb= {.r=0, .g=0, .b=0};
	if ( strlen(fname)) {
		s = strdup(fname);
		do {
			sR = strtok_r(s, "/", &l);
			lR = strtoul(sR, NULL, 0);

			if ( !(sG = strtok_r(NULL, "/", &l))) break;
			lG = strtoul(sG, NULL, 0);

			if ( !(sB = strtok_r(NULL, "/", &l))) break;
			lB = strtoul(sB, NULL, 0);

			if ( !(sLen = strtok_r(NULL, "/", &l))) break;
			lLen = strtoul(sLen, NULL, 0);
			if ( lLen < 1) lLen=1;
			if ( lLen > 1000) lLen = 1000;

			if ( !(sStart = strtok_r(NULL, "/", &l))) break;
			lStart = strtoul(sStart, NULL, 0);
			if ( lStart > 1000 ) lStart = 1000;

		} while(0);
		free(s);
	}
	rgb.r=lR & 0xFF;
	rgb.g=lG & 0xFF;
	rgb.b=lB & 0xFF;

	strip_set_range(lStart, lStart+lLen-1, &rgb);
	strip_show(true);

	snprintf(msg, sizeof(msg),", '%s' processed, set pixel pos=%d, len=%d, RGB=[%d,%d,%d]=[%x,%x,%x]",
			fname, lStart, lLen,
			rgb.r, rgb.g, rgb.b,
			rgb.r, rgb.g, rgb.b);
	ESP_LOGI(__func__, "%s", msg);

	httpd_resp_sendstr_chunk(req, msg);
	return ESP_OK;
}

