/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "esp32_ws2812.h"

//#define MDNS_INSTANCE "esp home web server"

#define MAX_CONTENTLEN 10240

extern T_EVT_OBJECT *s_object_list;
extern esp_vfs_spiffs_conf_t fs_conf;

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

#define LOG_MEM(c) {ESP_LOGI(__func__, "MEMORY(%d): free_heap_size=%lu, minimum_free_heap_size=%lu", c, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());}

static httpd_handle_t server = NULL;


typedef enum {
	HP_STATUS,
	HP_LIST,
	HP_FILE_LIST,
	HP_FILE_STORE,
	HP_FILE_GET,
	HP_FILE_LOAD,
	HP_FILE_DELETE,
	HP_CLEAR,
	HP_RUN,
	HP_STOP,
	HP_PAUSE,
	HP_BLANK,
	HP_CONFIG_GET,
	HP_CONFIG_SET,
	HP_RESET,
	HP_CFG_RESET,
	HP_HELP,
	HP_LOAD,
	// End of list
	HP_END_OF_LIST
} t_http_processing;

typedef enum {
	HPF_PATH_FROM_URL = 0x0001,
	HPF_POST          = 0x0002
} t_http_processing_flag;

typedef struct {
	char  *path;
	int  flags;
	t_http_processing todo;
	char *help;
} T_HTTP_PROCCESSING_TYPE;

static T_HTTP_PROCCESSING_TYPE http_processing[] = {
		{"/r",         0,                          HP_RUN,         "run"},
		{"/s",         0,                          HP_STOP,        "stop"},
		{"/p",         0,                          HP_PAUSE,       "pause"},
		{"/b",         0,                          HP_BLANK,       "blank strip"},
		{"/i",         0,                          HP_STATUS,      "info"},
		{"/cl",        0,                          HP_CLEAR,       "clear event list"},
		{"/li",        0,                          HP_LIST,        "list events"},
		{"/lo",        HPF_POST,                   HP_LOAD,        "load events, replaces data in memory"},
		{"/f/list",    0,                          HP_FILE_LIST,   "list stored files"},
		{"/f/store/",  HPF_PATH_FROM_URL|HPF_POST, HP_FILE_STORE,  "store JSON event lists into flash memory as <fname>"},
		{"/f/get/",    HPF_PATH_FROM_URL,          HP_FILE_GET,    "get content of stored file <fname>"},
		{"/f/load/",   HPF_PATH_FROM_URL,          HP_FILE_LOAD,   "load JSON event list stored in <fname> into memory"},
		{"/f/delete/", HPF_PATH_FROM_URL,          HP_FILE_DELETE, "delete file <fname>"},
		{"/cfg/get",   0,                          HP_CONFIG_GET,  "show config"},
		{"/cfg/set",   HPF_POST,                   HP_CONFIG_SET,  "set config"},
		{"/cfg/restart", 0,                        HP_RESET,       "restart the controller"},
		{"/cfg/tabula_rasa", 0,                    HP_CFG_RESET,   "reset all data to default"},
		{"/",          0,                          HP_HELP,        "API help"},
		{"",           0,                          HP_END_OF_LIST, ""}
};

static void httpd_resp_send_chunk_l(httpd_req_t *req, char *resp_str) {
	httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);
}

static void http_help(httpd_req_t *req) {
	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str), "path - description\n");
	httpd_resp_send_chunk_l(req, resp_str);

	for  (int i=0; http_processing[i].todo != HP_END_OF_LIST; i++) {
		snprintf(resp_str, sizeof(resp_str),"'%s%s' - %s%s\n",
				http_processing[i].path,
				(http_processing[i].flags & HPF_PATH_FROM_URL ? "<fname>" : ""),
				http_processing[i].help,
				(http_processing[i].flags & HPF_POST ? ", requires POST data" : "")
		);
		httpd_resp_send_chunk_l(req, resp_str);
	}
}

static void add_system_informations(cJSON *root) {
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);

	size_t total,used;
	storage_info(&total,&used);

	cJSON *sysinfo = cJSON_AddObjectToObject(root,"system");
	cJSON *fs_size = cJSON_AddObjectToObject(sysinfo,"filesystem");

	cJSON_AddStringToObject(sysinfo, "version", IDF_VER);
	cJSON_AddNumberToObject(sysinfo, "cores", chip_info.cores);
	cJSON_AddNumberToObject(sysinfo, "free_heap_size",esp_get_free_heap_size());
	cJSON_AddNumberToObject(sysinfo, "minimum_free_heap_size",esp_get_minimum_free_heap_size());
	cJSON_AddNumberToObject(fs_size, "total", total);
	cJSON_AddNumberToObject(fs_size, "used", used);

}


static void get_path_from_uri(const char *uri, char *dest, size_t destsize)
{
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }
    strlcpy(dest, uri, MIN(destsize, pathlen + 1));
}

static T_HTTP_PROCCESSING_TYPE *get_http_processing(char *path) {

	for  (int i=0; http_processing[i].todo != HP_END_OF_LIST; i++) {
		if ( http_processing[i].flags & HPF_PATH_FROM_URL) {
			// if a filename is expected compare with strstr
			if ( strstr(path, http_processing[i].path) == path) {
				return &http_processing[i];
			}
		} else {
			// if no filename expected compare with strcmp
			if ( !strcmp(path, http_processing[i].path)) {
				return &http_processing[i];
			}
		}
	}
	return NULL;
}

static void get_handler_list(httpd_req_t *req) {
	// list events
	char buf[255];
	const size_t sz_buf = sizeof(buf);

	extern T_SCENE *s_scene_list;
	if (obtain_eventlist_lock() != ESP_OK) {
		snprintf(buf, sz_buf, "%s couldn't get lock on eventlist\n", __func__);
		httpd_resp_send_chunk_l(req, buf);
		return;
	}

	if ( !s_object_list) {
		snprintf(buf, sz_buf, "\nno objectst");
		httpd_resp_send_chunk_l(req, buf);
	} else {
		for (T_EVT_OBJECT *obj=s_object_list; obj; obj=obj->nxt) {
			snprintf(buf, sz_buf, "\n   object '%s'", obj->oid);
			httpd_resp_send_chunk_l(req, buf);
			if (obj->data) {
				for(T_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
					snprintf(buf, sz_buf,"\n    id=%d, type=%d/%s, pos=%d, len=%d",
							data->id, data->type, OBJT2TEXT(data->type), data->pos, data->len
					);
					httpd_resp_send_chunk_l(req, buf);

				}
			} else {
				snprintf(buf, sz_buf, "\n   no data list in object");
				httpd_resp_send_chunk_l(req, buf);
			}

		}
	}


	if ( !s_scene_list) {
		snprintf(buf, sz_buf, "\nno scenes");
		httpd_resp_send_chunk_l(req, buf);

	} else {
		for (T_SCENE *scene = s_scene_list; scene; scene=scene->nxt) {
			if ( !scene->events) {
				snprintf(buf, sz_buf, "\nno events");
				httpd_resp_send_chunk_l(req, buf);
			} else {
				for ( T_EVENT_GROUP *evt= scene->events; evt; evt = evt->nxt) {
					snprintf(buf, sz_buf, "\nEvent id='%s', repeats=%u:", evt->id, evt->t_repeats);
					httpd_resp_send_chunk_l(req, buf);

					// INIT events
					if (evt->evt_time_init_list) {
						snprintf(buf, sz_buf,"\n  INIT events:");
						httpd_resp_send_chunk_l(req, buf);

						for (T_EVENT *tevt = evt->evt_time_init_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk_l(req, buf);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no INIT events.");
						httpd_resp_send_chunk_l(req, buf);
					}

					// WORK events
					if (evt->evt_time_list) {
						snprintf(buf, sz_buf,"\n  WORK events:");
						httpd_resp_send_chunk_l(req, buf);

						for (T_EVENT *tevt = evt->evt_time_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s'",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk_l(req, buf);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no WORK events.");
						httpd_resp_send_chunk_l(req, buf);
					}

					// FINAL events
					if (evt->evt_time_final_list) {
						snprintf(buf, sz_buf,"\n  FINAL events:");
						httpd_resp_send_chunk_l(req, buf);

						for (T_EVENT *tevt = evt->evt_time_final_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s'",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk_l(req, buf);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no FINAL events.");
						httpd_resp_send_chunk_l(req, buf);
					}
				}
			}
		}
	}
	httpd_resp_send_chunk_l(req, "\n");

	release_eventlist_lock();
}

static esp_err_t get_handler_file_list(httpd_req_t *req) {
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

    httpd_resp_set_type(req, "plain/text");

    // Iterate over all files / folders and fetch their names and sizes
    while ((entry = readdir(dir)) != NULL) {
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        snprintf(entrypath, sizeof(entrypath), "%s/%s",fs_conf.base_path, entry->d_name);
        if (stat(entrypath, &entry_stat) == -1) {
        	snprintf(msg, sizeof(msg),"Failed to stat '%s' : '%s'", entrytype, entry->d_name);
            ESP_LOGE(__func__, "%s", msg);
            closedir(dir);
    		snprintfapp(msg, sizeof(msg), "\n");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
            return ESP_FAIL;
        }
        snprintf(msg, sizeof(msg), "%s '%s' (%ld bytes)\n", entrytype, entry->d_name, entry_stat.st_size);
        httpd_resp_send_chunk_l(req, msg);
        ESP_LOGI(__func__, "%s", msg);
    }
    closedir(dir);
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

    add_system_informations(root);

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

    httpd_resp_send_chunk_l(req, "STATUS=");
	httpd_resp_send_chunk_l(req, resp);
	httpd_resp_send_chunk_l(req, "\n"); // response more readable

    free((void *)resp);
    cJSON_Delete(root);
}

static void get_handler_status_current(httpd_req_t *req) {
	response_with_status(req, "", get_scene_status());
}

static void get_handler_scene_new_status(httpd_req_t *req, run_status_type new_status) {
	run_status_type old_status = get_scene_status();
	if ( old_status != new_status) {
		old_status = set_scene_status(new_status);
	}
	response_with_status(req, old_status != new_status ? "new status set" : "", new_status);
}

static esp_err_t clear_data(char *msg, size_t sz_msg, run_status_type new_status) {
	memset(msg, 0, sz_msg);

	// stop display program
	run_status_type old_status = get_scene_status();
	if ( old_status != new_status) {
		old_status = set_scene_status(new_status);
		snprintf(msg, sz_msg,"new status set, ");
	}

	bool hasError = false;
	if (scene_list_free() == ESP_OK) {
		snprintfapp(msg,sz_msg,"event list cleared");
	} else {
		hasError=true;
		snprintfapp(msg,sz_msg,"clear event list failed");
	}

	if ( object_list_free() == ESP_OK) {
		snprintfapp(msg, sz_msg,", object list cleared");
	} else {
		hasError = true;
		snprintfapp(msg,sz_msg - strlen(msg),", clear object list failed");
	}
	return hasError ? ESP_FAIL : ESP_OK;

}


static esp_err_t get_handler_clear(httpd_req_t *req) {
	char msg[255];

    LOG_MEM(1);
	// stop display program, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg), new_status);

	response_with_status(req, msg, new_status);
    LOG_MEM(2);

	return res;
}

static void get_handler_restart(httpd_req_t *req) {
	char resp_str[64];

	snprintf(resp_str, sizeof(resp_str),"Restart initiated\n");
	httpd_resp_send_chunk_l(req, resp_str);

	httpd_resp_send_chunk(req, NULL, 0);

	ESP_LOGI(__func__,"Restarting in a second...\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(__func__, "Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

static void get_handler_reset(httpd_req_t *req) {

	// clear nvs
	nvs_flash_erase();
	scenes_stop();

	char resp_str[64];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");
	httpd_resp_send_chunk_l(req, resp_str);

	get_handler_restart(req);
}

static void get_handler_blank(httpd_req_t *req) {

	scenes_blank();

	response_with_status(req, "BLANK done", get_scene_status());
}

static void get_handler_config(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    // configuration
    add_config_informations(root);

    // some system informations
    add_system_informations(root);

    char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_send_chunk_l(req, resp);
	httpd_resp_send_chunk_l(req, "\n");

    free((void *)resp);
    cJSON_Delete(root);

}


/**
 * decodes JSON content and stores data in memory.
 * scenes will be stopped and data will be overwritten
 */
static esp_err_t post_handler_load(httpd_req_t *req, char *content) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

	// stop display, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg), new_status);
	if ( res != ESP_OK ) {
		ESP_LOGW(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	if ( decode_json4event_root(content, msg, sizeof(msg)) != ESP_OK) {
		ESP_LOGW(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	snprintf(msg,sizeof(msg), "data load done");
	ESP_LOGI(__func__, "ended with '%s'", msg);
	response_with_status(req, msg, new_status);

	return res;

}

/**
 * stores content in flash file
 */
static esp_err_t post_handler_file_store(httpd_req_t *req, char *content, char *fname, size_t sz_fname) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

    httpd_resp_set_type(req, "plain/text");

	if ( store_events_to_file(fname, content, msg, sizeof(msg))!= ESP_OK) {
		ESP_LOGW(__func__, "%s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
        return ESP_FAIL;
	}

	snprintf(msg,sizeof(msg), "content saved to %s",fname);
	ESP_LOGI(__func__, "success: %s", msg);

	response_with_status(req, msg, get_scene_status());
	return ESP_OK;
}

/**
 * load file from flash into memory
 */
static esp_err_t get_handler_file_load(httpd_req_t *req, char *fname, size_t sz_fname) {
	char errmsg[128];
	char msg[256];

	memset(msg,0,sizeof(msg));
	memset(errmsg,0,sizeof(errmsg));

	ESP_LOGI(__func__, "fname '%s'", fname);

	// stop display program and clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	if ( clear_data(msg, sizeof(msg), new_status) != ESP_OK) {
		ESP_LOGW(__func__, "clear_data failed, %s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	// load new data
	if ( load_events_from_file(fname, errmsg, sizeof(errmsg)) != ESP_OK) {
		snprintfapp(msg,sizeof(msg), ", load content from %s failed: %s",fname, errmsg);
		ESP_LOGW(__func__, "load_events_from_file failed, %s", msg);
		snprintfapp(msg, sizeof(msg), "\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	// successful
	snprintfapp(msg,sizeof(msg), ", content loaded from %s",fname);
	httpd_resp_send_chunk_l(req, msg);
	return ESP_OK;
}

/**
 * delete flash file
 */
static esp_err_t get_handler_file_delete(httpd_req_t *req, char *fname, size_t sz_fname) {
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
static esp_err_t get_handler_file_get(httpd_req_t *req, char *fname, size_t sz_fname) {
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
 * load data into memory
 * first stop scene
 */
static esp_err_t post_handler_config_load(httpd_req_t *req, char *buf) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

	// stop display, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	set_scene_status(new_status);

	char errmsg[64];
	esp_err_t res = decode_json4config_root(buf, errmsg, sizeof(errmsg));

	if (res == ESP_OK) {
		snprintfapp(msg, sizeof(msg), ", decoding data done: %s",errmsg);
	} else {
		snprintfapp(msg, sizeof(msg), ", decoding data failed: %s",errmsg);
		ESP_LOGW(__func__, "error %s", msg);
		snprintfapp(msg, sizeof(msg),"\n");
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, msg);
		return ESP_FAIL;
	}

	response_with_status(req, msg, new_status);

	return ESP_OK;

}

/**
 * uri should be data/add or data/set with POST-data
 * /set?id=<id> - replaces event with this id
 * /add - adds event
 */
static esp_err_t post_handler_main(httpd_req_t *req)
{
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	char path[256];
	get_path_from_uri(req->uri, path, sizeof(path));
	ESP_LOGI(__func__,"uri='%s', contenlen=%d, path='%s'", req->uri, req->content_len, path);

	char resp_str[255];

	T_HTTP_PROCCESSING_TYPE *pt = get_http_processing(path);

	if (!pt ) {
        snprintf(resp_str,sizeof(resp_str),"nothing found");
        ESP_LOGE(__func__, "%s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, resp_str);
        return ESP_FAIL;
	}

	if ( req->content_len > MAX_CONTENTLEN) {
        ESP_LOGE(__func__, "Content too large : %d bytes", req->content_len);
        snprintf(resp_str,sizeof(resp_str),"Data size must be less then %d bytes!",MAX_CONTENTLEN);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
        return ESP_FAIL;
	}

	char fname[LEN_PATH_MAX];
	memset(fname, 0, sizeof(fname));
	strlcpy(fname, &path[strlen(pt->path)], sizeof(fname));
	ESP_LOGI(__func__, "fname='%s'", fname);

	if ((pt->flags & HPF_PATH_FROM_URL) && !strlen(fname) ) {
		// file name needed
        snprintf(resp_str,sizeof(resp_str),"filename in URL required");
        ESP_LOGE(__func__, "%s", resp_str);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
        return ESP_FAIL;
	}

	// content length always greater 0, otherwise it is a GET request
    int remaining = req->content_len;
    char *buf = calloc(req->content_len+1, sizeof(char));
    int received;

    LOG_MEM(1);
    while (remaining > 0) {

        ESP_LOGI(__func__, "Remaining size : %d", remaining);
        // Receive the file part by part into a buffer
        if ((received = httpd_req_recv(req, buf, MIN(remaining, req->content_len))) <= 0) {
        	// recv failed
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry if timeout occurred
                continue;
            }

            ESP_LOGE(__func__, "Data reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data\n");
            free(buf);
            return ESP_FAIL;
        }

        // Keep track of remaining size of
        // the file left to be uploaded
        remaining -= received;
    }

    LOG_MEM(2);

    esp_err_t res = ESP_FAIL;

    switch(pt->todo) {
    case HP_LOAD:
    	res = post_handler_load(req, buf);
    	break;

    case HP_FILE_STORE:
    	res = post_handler_file_store(req, buf, fname, sizeof(fname));
    	break;

    case HP_CONFIG_SET:
    	res = post_handler_config_load(req, buf);
    	break;

    default:
		snprintf(resp_str, sizeof(resp_str),"path='%s' GET only", path);
		res = ESP_FAIL;
    }

    free(buf);

	// End response
    if ( res == ESP_OK ) {
    	ESP_LOGI(__func__, "success: %s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
    	httpd_resp_send_chunk(req, NULL, 0);
    } else {
    	ESP_LOGE(__func__, "error: %s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
    	httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
    }

	LOG_MEM(3);
	return res;
}

static esp_err_t get_handler_main(httpd_req_t *req)
{
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	char path[256];
	get_path_from_uri(req->uri, path, sizeof(path));
	ESP_LOGI(__func__,"uri='%s', path='%s'", req->uri, path);

	char resp_str[64];

	T_HTTP_PROCCESSING_TYPE *pt = get_http_processing(path);
	if (!pt ) {
        snprintf(resp_str,sizeof(resp_str),"nothing found");
        ESP_LOGE(__func__, "%s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, resp_str);
        return ESP_FAIL;
	}

	if (pt->flags & HPF_POST) {
        snprintf(resp_str,sizeof(resp_str),"POST data required");
        ESP_LOGE(__func__, "%s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
        return ESP_FAIL;
	}

	char fname[LEN_PATH_MAX];
	memset(fname, 0, sizeof(fname));
	strlcpy(fname, &path[strlen(pt->path)], sizeof(fname));
	ESP_LOGI(__func__, "fname='%s'", fname);

	if ((pt->flags & HPF_PATH_FROM_URL) && !strlen(fname) ) {
		// file name needed
        snprintf(resp_str,sizeof(resp_str),"filename in URL required");
        ESP_LOGE(__func__, "%s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
        return ESP_FAIL;
	}

	esp_err_t res = ESP_OK;

	switch(pt->todo) {

	case HP_STATUS:
		get_handler_status_current(req);
		break;

	case HP_HELP:
		http_help(req);
		break;

	case HP_LIST:
		get_handler_list(req);
		break;

	case HP_FILE_LIST:
		get_handler_file_list(req);
		break;

	case HP_FILE_GET:
		res = get_handler_file_get(req, fname, sizeof(fname));
		break;

	case HP_FILE_LOAD:
		res = get_handler_file_load(req, fname, sizeof(fname));
		break;

	case HP_FILE_DELETE:
		res = get_handler_file_delete(req, fname, sizeof(fname));
		break;

	case HP_CLEAR:
		get_handler_clear(req);
		break;

	case HP_RUN:
		get_handler_scene_new_status(req, RUN_STATUS_RUNNING);
		break;

	case HP_STOP:
		get_handler_scene_new_status(req, RUN_STATUS_STOPPED);
		break;

	case HP_PAUSE:
		get_handler_scene_new_status(req, RUN_STATUS_PAUSED);
		break;

	case HP_BLANK:
		get_handler_blank(req);
		break;

	case HP_CONFIG_GET:
		get_handler_config(req);
		break;

	case HP_RESET:
		get_handler_restart(req);
		break;

	case HP_CFG_RESET:
		get_handler_reset(req);
		break;

	default:
		snprintf(resp_str, sizeof(resp_str),"path='%s' todo %d NYI", path, pt->todo);
		ESP_LOGE(__func__, "%s", resp_str);
		snprintfapp(resp_str, sizeof(resp_str), "\n");
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, resp_str);
		return ESP_FAIL;
	}

	if ( res == ESP_OK) {
		// End response
		httpd_resp_send_chunk(req, NULL, 0);
	}

	return res;
}



esp_err_t start_rest_server(const char *base_path)
{
	if ( !base_path ) {
		ESP_LOGE(__func__, "base_path missing");
		return ESP_FAIL;
	}

	rest_server_context_t *rest_context = (rest_server_context_t*) calloc(1, sizeof(rest_server_context_t));
	if ( !rest_context) {
		ESP_LOGE(__func__, "No memory for rest context" );
		return ESP_FAIL;
	}
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size=20480;
    int p = config.task_priority;
    config.task_priority =0;
    ESP_LOGI(__func__, "config HTTP Server prio %d -> %d", p, config.task_priority);

    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(__func__, "Starting HTTP Server");
    if ( httpd_start(&server, &config) != ESP_OK ) {
    	ESP_LOGE(__func__, "Start server failed");
        free(rest_context);
        return ESP_FAIL;
    }

    // Install URI Handler
    // POST
    httpd_uri_t data_post_uri = {
        .uri       = "/*",
        .method    = HTTP_POST,
        .handler   = post_handler_main,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &data_post_uri);

    // GET
    httpd_uri_t data_get_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = get_handler_main,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &data_get_uri);

    return ESP_OK;
}

void server_stop() {
	   if (server) {
	        /* Stop the httpd server */
	        httpd_stop(server);
	        ESP_LOGI(__func__, "HTTP Server STOPP");
	        server=NULL;
	    }
}

void init_restservice() {

	ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));

}

void initialise_mdns(void)
{
 /*   mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
 */
}

void initialise_netbios() {
	/*
	netbiosns_init();
	netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);
	*/
}
