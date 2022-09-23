/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "esp32_ws2812.h"

#define MDNS_INSTANCE "esp home web server"


#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

extern T_EVT_OBJECT *s_object_list;

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

#define LOG_MEM(c) {ESP_LOGI(__func__, "MEMORY(%d): free_heap_size=%lu, minimum_free_heap_size=%lu", c, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());}



typedef enum {
	// GET
	HP_STATUS,
	HP_LIST,
	HP_CLEAR,
	HP_RUN,
	HP_STOP,
	HP_PAUSE,
	HP_BLANK,
	HP_CONFIG,
	HP_SAVE,
	HP_RESTART,
	HP_RESET,
	HP_HELP,
	// POST
	HP_LOAD,
	// End of list
	HP_END_OF_LIST
} t_http_processing;;

typedef struct {
	char *short_path;
	char  *path;
	t_http_processing todo;
	char *help;
} T_HTTP_PROCCESSING_TYPE;

static T_HTTP_PROCCESSING_TYPE http_processing[] = {
		{"","/load", HP_LOAD, "load events, replaces stored data, uses JSON-POST-data"},
		{"/", "/help", HP_HELP, "API help"},
		{"/st", "/status", HP_STATUS, "status"},
		{"/l","/list",HP_LIST, "list events"},
		{"","/clear",HP_CLEAR,"clear event list"},
		{"/r","/run",HP_RUN,"run"},
		{"/s","stop",HP_STOP,"stop"},
		{"/p","/pause",HP_PAUSE,"pause"},
		{"/b","blank", HP_BLANK, "blank strip"},
		{"/c","/config",HP_CONFIG,"shows config, set values: uses JSON-POST-data"},
		{"","/save",HP_SAVE,"save event list specified by fname=<fname> default: 'playlist' "},
		{"","/restart",HP_RESTART,"restart ESP32"},
		{"","/reset",HP_RESET,"reset ESP32 to default"},
		{"","", HP_END_OF_LIST,""}
};

static esp_err_t http_help(httpd_req_t *req) {
	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str), "short - long - description\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	for  (int i=0; http_processing[i].todo != HP_END_OF_LIST; i++) {
		if (strlen(http_processing[i].short_path)) {
			snprintf(resp_str, sizeof(resp_str),"'%s' - '%s' - %s\n",
					http_processing[i].short_path,
					http_processing[i].path,
					http_processing[i].help
			);
		} else {
			snprintf(resp_str, sizeof(resp_str),"'%s' - %s\n",
					http_processing[i].path,
					http_processing[i].help
			);
		}
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
	}
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;

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


static void get_path_from_uri(const char *uri,char *dest, size_t destsize)
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
		if ( !strcmp(http_processing[i].path, path)) {
			return &http_processing[i];
		}
		if (strlen(http_processing[i].short_path) && !strcmp(http_processing[i].short_path, path)) {
			return &http_processing[i];
		}
	}
	return NULL;
}

static void get_handler_data_list(httpd_req_t *req) {
	// list events
	char buf[255];
	const size_t sz_buf = sizeof(buf);
	const int l = HTTPD_RESP_USE_STRLEN;

	extern T_SCENE *s_scene_list;
	if (obtain_eventlist_lock() != ESP_OK) {
		snprintf(buf, sz_buf, "%s couldn't get lock on eventlist\n", __func__);
		httpd_resp_send_chunk(req, buf, l);
		return;
	}

	if ( !s_object_list) {
		snprintf(buf, sz_buf, "\nno objectst");
		httpd_resp_send_chunk(req, buf, l);
	} else {
		for (T_EVT_OBJECT *obj=s_object_list; obj; obj=obj->nxt) {
			snprintf(buf, sz_buf, "\n   object '%s'", obj->oid);
			httpd_resp_send_chunk(req, buf, l);
			if (obj->data) {
				for(T_EVT_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
					snprintf(buf, sz_buf,"\n    id=%d, type=%d/%s, pos=%d, len=%d",
							data->id, data->type, WT2TEXT(data->type), data->pos, data->len
					);
					httpd_resp_send_chunk(req, buf, l);

				}
			} else {
				snprintf(buf, sz_buf, "\n   no data list in object");
				httpd_resp_send_chunk(req, buf, l);
			}

		}
	}


	if ( !s_scene_list) {
		snprintf(buf, sz_buf, "\nno scenes");
		httpd_resp_send_chunk(req, buf, l);

	} else {
		for (T_SCENE *scene = s_scene_list; scene; scene=scene->nxt) {
			if ( !scene->events) {
				snprintf(buf, sz_buf, "\nno events");
				httpd_resp_send_chunk(req, buf, l);
			} else {
				for ( T_EVENT *evt= scene->events; evt; evt = evt->nxt) {
					snprintf(buf, sz_buf, "\nEvent id='%s', repeats=%u:", evt->id, evt->t_repeats);
					httpd_resp_send_chunk(req, buf, l);

					// INIT events
					if (evt->evt_time_init_list) {
						snprintf(buf, sz_buf,"\n  INIT events:");
						httpd_resp_send_chunk(req, buf, l);

						for (T_EVT_TIME *tevt = evt->evt_time_init_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk(req, buf, l);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no INIT events.");
						httpd_resp_send_chunk(req, buf, l);
					}

					// WORK events
					if (evt->evt_time_list) {
						snprintf(buf, sz_buf,"\n  WORK events:");
						httpd_resp_send_chunk(req, buf, l);

						for (T_EVT_TIME *tevt = evt->evt_time_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s'",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk(req, buf, l);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no WORK events.");
						httpd_resp_send_chunk(req, buf, l);
					}

					// FINAL events
					if (evt->evt_time_final_list) {
						snprintf(buf, sz_buf,"\n  FINAL events:");
						httpd_resp_send_chunk(req, buf, l);

						for (T_EVT_TIME *tevt = evt->evt_time_final_list; tevt; tevt=tevt->nxt) {
							snprintf(buf, sz_buf,"\n    id=%d, time=%llu ms, type=%d/%s, val=%.3f, sval='%s', marker='%s'",
									tevt->id, tevt->time, tevt->type, ET2TEXT(tevt->type), tevt->value, tevt->svalue, tevt->marker);
							httpd_resp_send_chunk(req, buf, l);

						}
					} else {
						snprintf(buf, sz_buf,"\n  no FINAL events.");
						httpd_resp_send_chunk(req, buf, l);
					}
				}
			}
		}
	}
	httpd_resp_send_chunk(req, "\n", l);

	release_eventlist_lock();
}

/**
 * delivers status informations in JSON
 */
static void get_handler_data_status(httpd_req_t *req, char *msg, run_status_type status) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    if (msg && strlen(msg))
    	cJSON_AddStringToObject(root, "msg", msg);
    cJSON_AddStringToObject(root, "status", RUN_STATUS_TYPE2TEXT(status));
    cJSON_AddNumberToObject(root, "scene_time", get_scene_time());
    cJSON_AddNumberToObject(root, "timer_period", get_event_timer_period());

    add_system_informations(root);

    const char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"RESP=%s", resp?resp:"nix");

    httpd_resp_send_chunk(req, "STATUS=", HTTPD_RESP_USE_STRLEN);
	httpd_resp_send_chunk(req, resp, strlen(resp));
	httpd_resp_send_chunk(req, "\n", 1); // response more readable

    free((void *)resp);
    cJSON_Delete(root);
}

static void get_handler_data_status_current(httpd_req_t *req) {
	get_handler_data_status(req, "", get_scene_status());
}

static void get_handler_data_scene_status(httpd_req_t *req, run_status_type new_status) {
	run_status_type old_status = get_scene_status();
	if ( old_status != new_status) {
		old_status = set_scene_status(new_status);
	}
	get_handler_data_status(req, old_status != new_status ? "new status set" : "", new_status);
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
		snprintf(&(msg[strlen(msg)]),sz_msg - strlen(msg),"event list cleared, ");
	} else {
		hasError=true;
		snprintf(&(msg[strlen(msg)]),sz_msg - strlen(msg),"clear event list failed, ");
	}

	if ( object_list_free() == ESP_OK) {
		snprintf(&(msg[strlen(msg)]),sz_msg - strlen(msg),"object list cleared");
	} else {
		hasError = true;
		snprintf(&(msg[strlen(msg)]),sz_msg - strlen(msg),"clear object list failed");
	}
	return hasError ? ESP_FAIL : ESP_OK;

}


static esp_err_t get_handler_data_clear(httpd_req_t *req) {
	char msg[255];

    LOG_MEM(1);
	// stop display program
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg), new_status);

	get_handler_data_status(req, msg, new_status);
    LOG_MEM(2);

	return res;
}

static void get_handler_data_restart(httpd_req_t *req) {
	char resp_str[64];

	snprintf(resp_str, sizeof(resp_str),"Restart initiated\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	httpd_resp_send_chunk(req, NULL, 0);

	ESP_LOGI(__func__,"Restarting in a second...\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(__func__, "Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

static void get_handler_data_reset(httpd_req_t *req) {

	// clear nvs
	nvs_flash_erase();
	scenes_stop();

	char resp_str[64];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	get_handler_data_restart(req);
}

static void get_handler_data_blank(httpd_req_t *req) {

	scenes_blank();

	get_handler_data_status(req, "BLANK done", get_scene_status());
}

static void get_handler_data_config(httpd_req_t *req) {
	/*
	bool restart_needed = false;
	bool store_config_needed = false;

	char *buf;
	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char val[256];

			if (httpd_query_key_value(buf, "numleds", val, sizeof(val)) == ESP_OK) {
				int numleds = atoi(val);
				gConfig.numleds = numleds;
				ESP_LOGI(__func__, "numleds=%d", gConfig.numleds);

				store_config_needed = true;
				restart_needed  = true;

			} else if (httpd_query_key_value(buf, "cycle", val, sizeof(val)) == ESP_OK) {
				gConfig.cycle = atoi(val);
				set_event_timer_period(gConfig.cycle);
				ESP_LOGI(__func__, "cycle=%d ms", gConfig.numleds);
				store_config_needed = true;

			} else if (httpd_query_key_value(buf, "showstatus", val, sizeof(val)) == ESP_OK) {
				gConfig.flags &= !CFG_SHOW_STATUS;
				if ( trufal(val)) {
					gConfig.flags |= CFG_SHOW_STATUS;
				}
				ESP_LOGI(__func__, "showstatus=%s", gConfig.flags & CFG_SHOW_STATUS ? "true" : "false");
				store_config_needed = true;

			} else if (httpd_query_key_value(buf, "autostart", val, sizeof(val)) == ESP_OK) {
				snprintf(gConfig.autoplayfile, sizeof(gConfig.autoplayfile), "%s", val);
				store_config_needed = 1;
				ESP_LOGI(__func__, "autostart=%s", gConfig.autoplayfile);

			}
		}
		free(buf);
		if (store_config_needed) {
			ESP_LOGI(__func__, "store config");
			store_config();
		}
	} else {
		ESP_LOGI(__func__,"buf_len == 0");
	}
	*/

	extern uint32_t cfg_flags;
	extern uint32_t cfg_trans_flags;
	extern uint32_t cfg_numleds;
	extern uint32_t cfg_cycle;
	extern char *cfg_autoplayfile;

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    // configuration
    add_config_informations(root);

    // some system informations
    add_system_informations(root);

    const char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_send_chunk(req, resp, strlen(resp));
	httpd_resp_send_chunk(req, "\n", 1);

    free((void *)resp);
    cJSON_Delete(root);

/*
	if ( restart_needed ) {
		// End response
		httpd_resp_send_chunk(req, NULL, 0);

		ESP_LOGI(__func__,"Restarting in a second...\n");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	    ESP_LOGI(__func__, "Restarting now.\n");
	    fflush(stdout);
	    esp_restart();
	}
	*/
}

static void get_handler_data_save(httpd_req_t *req) {
	char resp_str[255];
	char *buf;
	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char fname[256];
			char *paramname="fname";
			if (httpd_query_key_value(buf, paramname, fname, sizeof(fname)) == ESP_OK) {
				snprintf(resp_str, sizeof(resp_str),"NYI save to '%s'\n", fname);

			} else {
				snprintf(resp_str, sizeof(resp_str),"ERROR: missing query parameter 'fname=<fname>'\n");
			}
		}
		free(buf);
	} else {
		ESP_LOGI(__func__,"buf_len == 0");
	}

	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
}

static esp_err_t get_handler_data(httpd_req_t *req)
{
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	char path[256];
	get_path_from_uri(req->uri, path, sizeof(path));
	ESP_LOGI(__func__,"uri='%s', contenlen=%d, path='%s'", req->uri, req->content_len, path);

	T_HTTP_PROCCESSING_TYPE *pt = get_http_processing(path);
	if (!pt ) {
		return http_help(req);
	}

	char resp_str[255];

	switch(pt->todo) {
	case HP_STATUS:
		get_handler_data_status_current(req);
		break;
	case HP_HELP:
		http_help(req);
		break;
	case HP_LIST:
		get_handler_data_list(req);
		break;
	case HP_CLEAR:
		get_handler_data_clear(req);
		break;
	case HP_RUN:
		get_handler_data_scene_status(req, RUN_STATUS_RUNNING);
		break;
	case HP_STOP:
		get_handler_data_scene_status(req, RUN_STATUS_STOPPED);
		break;
	case HP_PAUSE:
		get_handler_data_scene_status(req, RUN_STATUS_PAUSED);
		break;
	case HP_BLANK:
		get_handler_data_blank(req);
		break;
	case HP_CONFIG:
		get_handler_data_config(req);
		break;
	case HP_SAVE:
		get_handler_data_save(req);
		break;
	case HP_RESTART:
		get_handler_data_restart(req);
		break;
	case HP_RESET:
		get_handler_data_reset(req);
		break;
	case HP_LOAD:
		snprintf(resp_str, sizeof(resp_str),"path='%s' POST only\n", path);
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
		return http_help(req);
		break;
	default:
		snprintf(resp_str, sizeof(resp_str),"path='%s' todo %d NYI\n", path, pt->todo);
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
		return http_help(req);
	}

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;
}


static esp_err_t post_handler_data_load(httpd_req_t *req, char *buf) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

    LOG_MEM(1);

	// stop display, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	esp_err_t res = clear_data(msg, sizeof(msg),new_status);

    LOG_MEM(2);

	char errmsg[64];
	res = decode_json4event_root(buf, errmsg, sizeof(errmsg), true);

	if (res == ESP_OK) {
		snprintf(&msg[strlen(msg)],sizeof(msg) - strlen(msg), ", decoding data done: %s",errmsg);
	} else {
		snprintf(&msg[strlen(msg)],sizeof(msg) - strlen(msg), ", decoding data failed: %s",errmsg);
	}
    LOG_MEM(3);

	get_handler_data_status(req, msg, new_status);

	return res;

}

static esp_err_t post_handler_config_load(httpd_req_t *req, char *buf) {
	char msg[255];
	memset(msg, 0, sizeof(msg));

	// stop display, clear data
	run_status_type new_status = RUN_STATUS_STOPPED;
	set_scene_status(new_status);

	char errmsg[64];
	esp_err_t res = decode_json4config_root(buf, errmsg, sizeof(errmsg));

	if (res == ESP_OK) {
		snprintf(&msg[strlen(msg)],sizeof(msg) - strlen(msg), ", decoding data done: %s",errmsg);
	} else {
		snprintf(&msg[strlen(msg)],sizeof(msg) - strlen(msg), ", decoding data failed: %s",errmsg);
	}

	get_handler_data_status(req, msg, new_status);

	return res;

}

/**
 * uri should be data/add or data/set with POST-data
 * /set?id=<id> - replaces event with this id
 * /add - adds event
 */
static esp_err_t post_handler_data(httpd_req_t *req)
{
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	char path[256];
	get_path_from_uri(req->uri, path, sizeof(path));
	ESP_LOGI(__func__,"uri='%s', contenlen=%d, path='%s'", req->uri, req->content_len, path);

	char resp_str[255];

	T_HTTP_PROCCESSING_TYPE *pt = get_http_processing(path);

	if (!pt ) {
		return http_help(req);
	}

	if ( req->content_len > SCRATCH_BUFSIZE) {
        ESP_LOGE(__func__, "Content too large : %d bytes\n", req->content_len);
        // Respond with 400 Bad Request
        snprintf(resp_str,sizeof(resp_str),"Data size must be less then %d bytes!\n",SCRATCH_BUFSIZE);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp_str);
        /// Return failure to close underlying connection else the
        // incoming file content will keep the socket busy
        return ESP_FAIL;
	}

    int remaining = req->content_len;
    // Retrieve the pointer to scratch buffer for temporary storage
    char *buf = ((rest_server_context_t *)req->user_ctx)->scratch;
    memset(buf, 0, SCRATCH_BUFSIZE);
    int received;

    while (remaining > 0) {

        ESP_LOGI(__func__, "Remaining size : %d", remaining);
        // Receive the file part by part into a buffer
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
        	// recv failed
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry if timeout occurred
                continue;
            }

            ESP_LOGE(__func__, "Data reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data\n");
            return ESP_FAIL;
        }

        // Keep track of remaining size of
        // the file left to be uploaded
        remaining -= received;
    }

    esp_err_t res;
    switch(pt->todo) {
    case HP_LOAD:
    	res = post_handler_data_load(req, buf);
    	break;
    case HP_CONFIG:
    	res=post_handler_config_load(req, buf);
    	break;
    default:
		snprintf(resp_str, sizeof(resp_str),"path='%s' GET only\n", path);
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
		return http_help(req);
    }

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	return res;
}


static httpd_handle_t server = NULL;

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
        .handler   = post_handler_data,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &data_post_uri);

    // GET
    httpd_uri_t data_get_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = get_handler_data,
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
