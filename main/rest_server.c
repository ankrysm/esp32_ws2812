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

extern T_CONFIG gConfig;
extern T_EVENT *s_event_list;


typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension * /
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}
// */


/**
 * check text, 'true' '1' fort true, others for false
 */
static uint32_t trufal(char *txt) {
	if ( !strcmp(txt,"1") || !strcasecmp(txt,"true") || !strcasecmp(txt,"t")) {
		return 1;
	} else {
		return 0;
	}
}

/*
static esp_err_t get_handler_strip_setcolor(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char resp_str[160];

    // Read URL query string length and allocate memory for length + 1,
    // extra byte for null termination
    uint32_t start_idx=1;
    uint32_t end_idx=2; //strip_numleds()-1;

    T_COLOR_HSV hsv = {.h=0, .s=0, .v=0};
    T_COLOR_RGB rgb = {.r=0, .g=0, .b=0};

    if ( !strip_initialized()) {
         snprintf(resp_str, sizeof(resp_str),"NOT INITIALIZED\n");
         ESP_LOGI(__func__,"response=%s", resp_str);
         httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
         return ESP_OK;
    }

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
    	buf = malloc(buf_len);
    	if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    		ESP_LOGI(__func__, "Found URL query => %s", buf);
    		char param[256];
    		char *paramname;
    		char *tok1, *tok2, *tok3, *s, *l;
    		//                    0       1     2     3
    		char *paramnames[] = {"range","rgb","hsv","effect",""};
    		int i;
    		for(i=0; strlen(paramnames[i]); i++) {
    			paramname = paramnames[i];
    			if (!httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
    				continue;
    			}
    			ESP_LOGI(__func__, "Found URL query parameter => %s=%s", paramname, param);
    			switch(i) {
    			case 0: // range
    			{
    				// expected: start,end
    				s=strdup(param);
    				tok1=strtok_r(s,   ",", &l);
    				tok2=strtok_r(NULL,",", &l);
    				start_idx = atoi(tok1);
    				if (tok2) {
    					end_idx = atoi(tok2);
    				} else {
    					end_idx = start_idx +1;
    				}
    				free(s);
    			}
    			break;

    			case 1: // rgb
    			{
    				// expected: r,g,b
    				s=strdup(param);
    				tok1=strtok_r(s,   ",", &l);
    				tok2=strtok_r(NULL,",", &l);
    				tok3=tok2 ? strtok_r(NULL,",", &l) : NULL;
    				rgb.r = atoi(tok1);
    				if (tok2) {
    					rgb.g = atoi(tok2);
    				}
    				if (tok3) {
    					rgb.b = atoi(tok3);
    				}
    				free(s);
    				strip_set_color_rgb(start_idx, end_idx, &rgb);
    				strip_show();
    				ESP_LOGI(__func__,"done: %d-%d rgb=%d/%d/%d\n",
    						start_idx, end_idx, rgb.r, rgb.g, rgb.b);
    			}
    			break;

    			case 2: // hsv
    			{
    				// expected: h,s,v
    				s=strdup(param);
    				tok1=strtok_r(s,   ",", &l);
    				tok2=strtok_r(NULL,",", &l);
    				tok3=tok2 ? strtok_r(NULL,",", &l) : NULL;
    				hsv.h = atoi(tok1);
    				if (tok2) {
    					hsv.s = atoi(tok2);
    				}
    				if (tok3) {
    					hsv.v = atoi(tok3);
    				}
    				free(s);
    				c_hsv2rgb(&hsv, &rgb);
    				strip_set_color_rgb(start_idx, end_idx, &rgb);
    				strip_show();
    				ESP_LOGI(__func__,"done: %d-%d hsv=%d/%d/%d rgb=%d/%d/%d\n",
    						start_idx, end_idx, hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);

    			}
    			break;

    			case 3: // effect
    			{
    				// expected typ,parameter,parameter ...
    				// type 'solid' parameter: startpixel, #pixel, h,s,v
    				// type 'smooth' parameter: startpixel, #pixel, fade in pixel, fade out pixel, start-h,s,v, middle-h,s,v, end-h,s,v
    				// effects with different types (moving, location based etc.)
    				// can be in a list separated bei ';'
    				T_EVENT evt;
    				if (decode_effect_list(param, &evt) == ESP_OK) {
    					ESP_LOGI(__func__,"done: %s\n", param);
    					evt.isdirty=1;
    					process_loc_event(&evt);
    					strip_show();
    				} else {
    					ESP_LOGI(__func__,"FAILED: %s\n", param);
    				}

    			}
    			break;
    			default:
    				ESP_LOGW(__func__,"NYI: %s\n", param);
    			}
    		} // for
    	}
    	free(buf);
    }

	snprintf(resp_str, sizeof(resp_str),"done\n");
    ESP_LOGI(__func__,"response=%s", resp_str);

    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
// */

/// ##########################################################################
/* / new handler
static esp_err_t get_handler_strip_config(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;
	int restart_needed = 0;
	int store_config_needed = 0;

	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char *paramname;
			char param[32];
			// Get value of expected key from query string

			paramname = "autoplay";
			if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: %s=%s", paramname, param);
				gConfig.flags &= !CFG_AUTOPLAY;
				if ( trufal(param)) {
					gConfig.flags |= CFG_AUTOPLAY;
				}
				store_config_needed = 1;
			}

			paramname = "autoplayfile";
			if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: %s=%s", paramname, param);
				snprintf(gConfig.autoplayfile, sizeof(gConfig.autoplayfile), "%s", param);
				store_config_needed = 1;
			}

			paramname = "numleds";
			if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: %s=%s", paramname, param);
				int numleds = atoi(param);
				gConfig.numleds = numleds;

				store_config_needed = 1;
				restart_needed  = 1;
			}
			paramname = "cycle";
			if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: %s=%s", paramname, param);
				gConfig.cycle = atoi(param);
				// stop playing when cycle changed
				//scenes_stop();
				set_event_timer_period(gConfig.cycle);
				store_config_needed = 1;
			}
			paramname = "showstatus";
			if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: %s=%s", paramname, param);
				gConfig.flags &= !CFG_SHOW_STATUS;
				if ( trufal(param)) {
					gConfig.flags |= CFG_SHOW_STATUS;
				}
				store_config_needed = 1;
			}
		}
		free(buf);
		if (store_config_needed) {
			ESP_LOGI(__func__, "store config");
			store_config();
		}
	}


    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    // system informations
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);

    cJSON_AddNumberToObject(root, "numleds", gConfig.numleds);
    cJSON_AddStringToObject(root, "autoplayfile", gConfig.autoplayfile);
    if ( gConfig.flags & CFG_AUTOPLAY ) {
        cJSON_AddTrueToObject(root, "autoplay");
    } else {
        cJSON_AddFalseToObject(root, "autoplay");
    }
    if ( gConfig.flags & CFG_SHOW_STATUS ) {
        cJSON_AddTrueToObject(root, "showstatus");
    } else {
        cJSON_AddFalseToObject(root, "showstatus");
    }
    if ( gConfig.flags & CFG_WITH_WIFI ) {
        cJSON_AddTrueToObject(root, "with_wifi");
    } else {
        cJSON_AddFalseToObject(root, "with_wifi");
    }
    cJSON_AddNumberToObject(root, "cycle", gConfig.cycle);

    cJSON *fs_size = cJSON_AddObjectToObject(root,"filesystem");


    size_t total,used;
    storage_info(&total,&used);
    cJSON_AddNumberToObject(fs_size, "total", total);
    cJSON_AddNumberToObject(fs_size, "used", used);


    // led strip configuration
    const char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_send_chunk(req, resp, strlen(resp));
	httpd_resp_send_chunk(req, "\n", 1);

    free((void *)resp);
    cJSON_Delete(root);

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	if ( restart_needed ) {
		ESP_LOGI(__func__,"Restarting in a second...\n");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	    ESP_LOGI(__func__, "Restarting now.\n");
	    fflush(stdout);
	    esp_restart();
	}
	return ESP_OK;
}
// */

/**
 * play control: run stop pause add del ...
 * /
static esp_err_t get_handler_ctrl(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;

	extern T_CONFIG gConfig;
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

	run_status_type old_status = RUN_STATUS_STOPPED;
	run_status_type new_status = RUN_STATUS_NOT_SET;

	char resp_str[255];

	buf_len = httpd_req_get_url_query_len(req) + 1;
	int cnt=0;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);

			//                   0       1
			char *paramnames[]={"cycle", "cmd", ""};
			for (int i=0; strlen(paramnames[i]); i++) {
				char param[256];
				if (httpd_query_key_value(buf, paramnames[i], param, sizeof(param)) != ESP_OK) {
					continue;
				}
				cnt++;
				switch(i) {
				case 0: // cycle
				{
					int new_val = atoi(param);
					if ( new_val < 10 ) {
						snprintf(resp_str,sizeof(resp_str),"new cycle time %d invalid, minimum value: 10 \n", new_val);
					} else {
						int old_val = set_event_timer_period(new_val);
						snprintf(resp_str,sizeof(resp_str),"cycle time %d -> %d\n", old_val, new_val);
					}
					httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
				}
					break;

				case 1: // cmd
				{
					if ( param[0] == 'r' ) {
						new_status = RUN_STATUS_RUNNING;

					} else if (param[0] == 's' ) {
						new_status = RUN_STATUS_STOPPED;

					} else if (param[0] == 'p' ) {
						new_status = RUN_STATUS_PAUSED;

					} else if (param[0] == 'l' ) {
						// list events
						extern T_EVENT *s_event_list;
						if (obtain_eventlist_lock() != ESP_OK) {
							ESP_LOGE(__func__, "couldn't get lock on eventlist");
							break;
						}
						char buf[100];
						if ( !s_event_list) {
							snprintf(buf,sizeof(buf),"no events in list\n");
							httpd_resp_send_chunk(req, buf, strlen(buf));
						} else {
							for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
								event2text(evt,resp_str,sizeof(resp_str));
								httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
							}
						}
						release_eventlist_lock();

					} else if (param[0] == 'c' ) {
						// clear - sets status stop
						new_status = RUN_STATUS_STOPPED;
						if (event_list_free() == ESP_OK) {
							snprintf(resp_str,sizeof(resp_str),"event list cleared");
						} else {
							snprintf(resp_str,sizeof(resp_str),"clear event list failed");
						}
						httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
					}

					if (new_status != RUN_STATUS_NOT_SET) {
						old_status = set_scene_status(new_status);
						snprintf(resp_str,sizeof(resp_str),"New status %s -> %s\nTimer cycle=%lld ms\nScene time=%lld\n",
								RUN_STATUS_TYPE2TEXT(old_status), RUN_STATUS_TYPE2TEXT(new_status),
								get_event_timer_period(), get_scene_time());
					} else {
						old_status = get_scene_status();
						snprintf(resp_str,sizeof(resp_str),"Status %s\nTimer cycle=%lld ms\nScene time=%lld\n",
								RUN_STATUS_TYPE2TEXT(old_status),
								get_event_timer_period(), get_scene_time());
					}
					httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
				}
					break;

				default:
					snprintf(resp_str,sizeof(resp_str),"%d NYI",i);
					httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
					cnt--;
				}
			}
			free(buf);
		}
	} else {
		old_status = get_scene_status();
		snprintf(resp_str,sizeof(resp_str),"Status %s\nTimer cycle=%lld ms\nScene time=%lld\n",
				RUN_STATUS_TYPE2TEXT(old_status),
				get_event_timer_period(), get_scene_time());
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
	}

	snprintf(resp_str,sizeof(resp_str),"DONE(cnt=%d)\n",cnt);
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;
}
// */

/*
static esp_err_t get_handler_reset(httpd_req_t *req)
{
	//size_t buf_len;

	// clear nvs
	nvs_flash_erase();
	scenes_stop();

	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");

	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

	ESP_LOGI(__func__,"Restarting in a second...\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(__func__, "Restarting now.\n");
    fflush(stdout);
    esp_restart();

	return ESP_OK;
}
*/

/**
 * expected uri's
 * /l /list - list events
 *    /delete?id=<id> - delete event with id
 *    /clear - clears event list
 * /r /run - start playing
 * /s /stop - stop playing
 * /p /pause - pause playing
 *    /config?numleds=<nn>&cycle=<nn>&autostart=<fname>
 *    /save?fn=<fname> - save playlist to MVS
 *    /reset - reset stored values to default
 *    /restart - restart the ESP32
 * default: help
 */

typedef enum {
	// GET
	HP_STATUS,
	HP_LIST,
	HP_DELETE,
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
	HP_ADD,
	HP_SET,
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
		{"/a","/add", HP_ADD, "add event, uses POST-data"},
		{"","/set", HP_SET, "set event, specified by query parameter 'id=<nn>', uses POST-data"},
		{"/", "/help", HP_HELP, "API help"},
		{"/st", "/status", HP_STATUS, "status"},
		{"/l","/list",HP_LIST, "list events"},
		{"","/delete",HP_DELETE,"delete an event specified by query parameter 'id=<nn>'"},
		{"","/clear",HP_CLEAR,"clear event list"},
		{"/r","/run",HP_RUN,"run"},
		{"/s","stop",HP_STOP,"stop"},
		{"/p","/pause",HP_PAUSE,"pause"},
		{"/b","blank", HP_BLANK, "blank strip"},
		{"/c","/config",HP_CONFIG,"config, set values: query parameter numleds=<nn>, cycle=<nn> (in ms), showstatus=[0|1], autostart=<fname>"},
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
		snprintf(resp_str, sizeof(resp_str),"'%s' - '%s' - %s\n",
				http_processing[i].short_path,
				http_processing[i].path,
				http_processing[i].help
		);
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
	}
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;

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

	extern T_EVENT *s_event_list;
	if (obtain_eventlist_lock() != ESP_OK) {
		snprintf(buf, sz_buf, "%s couldn't get lock on eventlist\n", __func__);
		httpd_resp_send_chunk(req, buf, l);
		return;
	}

	if ( !s_event_list) {
		snprintf(buf, sz_buf, "no events in list");
		httpd_resp_send_chunk(req, buf, l);

	} else {
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			snprintf(buf, sz_buf, "\nEvent id=%d, startpos=%.2f", evt->id, evt->pos);
			httpd_resp_send_chunk(req, buf, l);

			snprintf(buf, sz_buf,", flags=0x%04x, len_f=%.1f, len_f_delta=%.1f, v=%.1f, v_delta=%.1f, brightn.=%.1f, brightn.delta=%1f"
					, evt->flags, evt->len_factor, evt->len_factor_delta, evt->speed, evt->acceleration, evt->brightness, evt->brightness_delta);
			httpd_resp_send_chunk(req, buf, l);

			if ( evt->what_list) {
				snprintf(buf, sz_buf, "\n  what=");
				httpd_resp_send_chunk(req, buf, l);

				for ( T_EVT_WHAT *w=evt->what_list; w; w=w->nxt) {
					snprintf(buf, sz_buf,"\n    id=%d, type=%d/%s, pos=%d, len=%d",
							w->id, w->type, WT2TEXT(w->type), w->pos, w->len
					);
					httpd_resp_send_chunk(req, buf, l);
				}

			} else {
				snprintf(buf, sz_buf,"\n  no what list");
				httpd_resp_send_chunk(req, buf, l);
			}

			if (evt->evt_time_list) {
				snprintf(buf, sz_buf,"\n  time events:");
				httpd_resp_send_chunk(req, buf, l);

				for (T_EVT_TIME *tevt = evt->evt_time_list; tevt; tevt=tevt->nxt) {
					snprintf(buf, sz_buf,"\n    id=%d, starttime=%llu, type=%d/%s, val=%.2f",
							tevt->id, tevt->starttime, tevt->type, ET2TEXT(tevt->type), tevt->value);
					httpd_resp_send_chunk(req, buf, l);

				}
			} else {
				snprintf(buf, sz_buf,"\n  no time events.");
				httpd_resp_send_chunk(req, buf, l);
			}
		}
	}
	httpd_resp_send_chunk(req, "\n", l);

	release_eventlist_lock();
}

static void get_handler_data_status(httpd_req_t *req, bool changed, run_status_type status) {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    if ( changed ) {
         cJSON_AddTrueToObject(root, "changed");
     } else {
         cJSON_AddFalseToObject(root, "changed");
     }
    cJSON_AddStringToObject(root, "status", RUN_STATUS_TYPE2TEXT(status));
    cJSON_AddNumberToObject(root, "scene_time", get_scene_time());
    cJSON_AddNumberToObject(root, "timer_period", get_event_timer_period());
    cJSON_AddNumberToObject(root, "free_heap_size",esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "minimum_free_heap_size",esp_get_minimum_free_heap_size());

    cJSON *fs_size = cJSON_AddObjectToObject(root,"filesystem");
    size_t total,used;
    storage_info(&total,&used);
    cJSON_AddNumberToObject(fs_size, "total", total);
    cJSON_AddNumberToObject(fs_size, "used", used);

    const char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_send_chunk(req, resp, strlen(resp));
	httpd_resp_send_chunk(req, "\n", 1);

    free((void *)resp);
    cJSON_Delete(root);
}

static void get_handler_data_status_current(httpd_req_t *req) {
	get_handler_data_status(req, false, get_scene_status());
}

static void get_handler_data_scene_status(httpd_req_t *req, run_status_type new_status) {
	run_status_type old_status = get_scene_status();
	if ( old_status != new_status) {
		old_status = set_scene_status(new_status);
	}
	get_handler_data_status(req, old_status != new_status, new_status);
}

static void get_handler_data_clear(httpd_req_t *req) {
	char resp_str[64];
	get_handler_data_scene_status(req, RUN_STATUS_STOPPED);

	if (event_list_free() == ESP_OK) {
		snprintf(resp_str,sizeof(resp_str),"event list cleared");
	} else {
		snprintf(resp_str,sizeof(resp_str),"clear event list failed");
	}
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
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

	char resp_str[64];
	snprintf(resp_str, sizeof(resp_str),"BLANK done\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

}

static void get_handler_data_delete(httpd_req_t *req) {
	char resp_str[255];
	char *buf;
	size_t buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char *paramname="id";
			char val[256];
			if (httpd_query_key_value(buf, paramname, val, sizeof(val)) == ESP_OK) {
				// doit
				int id = atoi(val);
				esp_err_t rc=delete_event_by_id(id);
				switch(rc) {
				case ESP_OK:
					snprintf(resp_str, sizeof(resp_str),"event %d deleted\n", id);
					break;
				case ESP_ERR_NOT_FOUND:
					snprintf(resp_str, sizeof(resp_str),"event %d not found\n", id);
					break;
				default:
					snprintf(resp_str, sizeof(resp_str),"delete event %d FAILED\n", id);
				}
			} else {
				snprintf(resp_str, sizeof(resp_str),"ERROR: missing query parameter 'id=<nn>'\n");
			}
		}
		free(buf);
	} else {
		ESP_LOGI(__func__,"buf_len == 0");
	}

	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
}

static void get_handler_data_config(httpd_req_t *req) {
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

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    // system informations
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);

    cJSON_AddNumberToObject(root, "numleds", gConfig.numleds);
    cJSON_AddStringToObject(root, "autoplayfile", gConfig.autoplayfile);

    if ( gConfig.flags & CFG_AUTOPLAY ) {
        cJSON_AddTrueToObject(root, "autoplay");
    } else {
        cJSON_AddFalseToObject(root, "autoplay");
    }

    if ( gConfig.flags & CFG_SHOW_STATUS ) {
        cJSON_AddTrueToObject(root, "showstatus");
    } else {
        cJSON_AddFalseToObject(root, "showstatus");
    }

    if ( gConfig.flags & CFG_WITH_WIFI ) {
        cJSON_AddTrueToObject(root, "with_wifi");
    } else {
        cJSON_AddFalseToObject(root, "with_wifi");
    }

    cJSON_AddNumberToObject(root, "cycle", gConfig.cycle);

    cJSON *fs_size = cJSON_AddObjectToObject(root,"filesystem");
    size_t total,used;
    storage_info(&total,&used);
    cJSON_AddNumberToObject(fs_size, "total", total);
    cJSON_AddNumberToObject(fs_size, "used", used);


    // led strip configuration
    const char *resp = cJSON_PrintUnformatted(root);
    ESP_LOGI(__func__,"resp=%s", resp?resp:"nix");
	httpd_resp_send_chunk(req, resp, strlen(resp));
	httpd_resp_send_chunk(req, "\n", 1);

    free((void *)resp);
    cJSON_Delete(root);


	if ( restart_needed ) {
		// End response
		httpd_resp_send_chunk(req, NULL, 0);

		ESP_LOGI(__func__,"Restarting in a second...\n");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	    ESP_LOGI(__func__, "Restarting now.\n");
	    fflush(stdout);
	    esp_restart();
	}
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

	//snprintf(resp_str, sizeof(resp_str),"path='%s' todo %d\n", path, pt->todo);
	//httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

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
	case HP_DELETE:
		get_handler_data_delete(req);
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
	case HP_ADD:
	case HP_SET:
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

    bool overwrite = true;
    switch(pt->todo) {
    case HP_ADD:
    	overwrite=false;
    	break;
    case HP_SET:
    	overwrite=true;
    	break;
    default:
		snprintf(resp_str, sizeof(resp_str),"path='%s' GET only\n", path);
		httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
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

    char errmsg[64];
    esp_err_t res = decode_json4event(buf, overwrite, errmsg, sizeof(errmsg));
    if (res != ESP_OK) {
        snprintf(resp_str,sizeof(resp_str),"Decoding data failed: %s\n",errmsg);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, resp_str);
        return ESP_FAIL;
    }

	snprintf(resp_str,sizeof(resp_str),"DONE\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;
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
