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


typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
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
// new handler
static esp_err_t get_handler_strip_config(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;
	int restart_needed = 0;
	int store_config_needed = 0;


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
				// stop playing when cacle changed
				scenes_stop();
				set_timer_cycle(gConfig.cycle);
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


/**
 * play scene
 * Parameter: cmd=run|stop|pause, file=fname
 */
static esp_err_t get_handler_scene(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;

	extern T_CONFIG gConfig;

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

	run_status_type old_status = RUN_STATUS_IDLE;
	run_status_type new_status = RUN_STATUS_IDLE;

	char resp_str[255];

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);

			//                   0     1
			char *paramnames[]={"cmd","add",""};
			for (int i=0; strlen(paramnames[i]); i++) {
				char param[256];
				if (httpd_query_key_value(buf, paramnames[i], param, sizeof(param)) != ESP_OK) {
					continue;
				}
				switch(i) {
				case 0: // cmd
				{
					if ( param[0] == 'r' ) {
						new_status = RUN_STATUS_RUNNING;
					} else if (param[0] == 's' ) {
						new_status = RUN_STATUS_STOPPED;
					} else if (param[0] == 'p' ) {
						new_status = RUN_STATUS_PAUSED;
					} else if (param[0] == 'l' ) {
						// list events
						{
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
									snprintf(resp_str,sizeof(resp_str),"event %d\n", evt->lfd);
									httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

									loc_event2string(&(evt->loc_event), buf, sizeof(buf));
									snprintf(resp_str,sizeof(resp_str),"  loc_evt=%s\n", buf);
									httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

									mov_event2string(&(evt->mov_event), buf, sizeof(buf));
									snprintf(resp_str,sizeof(resp_str),"  mov_evt=%s\n", buf);
									httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
								}
							}
							release_eventlist_lock();
						}
					} else if (param[0] == 'c' ) {
						// clear
						new_status = RUN_STATUS_STOPPED;
						if (event_list_free() == ESP_OK) {
							snprintf(resp_str,sizeof(resp_str),"event list cleared");
						} else {
							snprintf(resp_str,sizeof(resp_str),"clear event list failed");
						}
						httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
					}

					if (new_status != RUN_STATUS_IDLE) {
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
				case 1: // add
				{
					// expected typ,parameter,parameter ...
					// type 'solid' parameter: startpixel, #pixel, h,s,v
					// type 'smooth' parameter: startpixel, #pixel, fade in pixel, fade out pixel, start-h,s,v, middle-h,s,v, end-h,s,v
					// effects with different types (moving, location based etc.)
					// can be in a list separated bei ';'
					T_EVENT evt;
					if (decode_effect_list(param, &evt) == ESP_OK) {
						ESP_LOGI(__func__,"done: %s\n", param);
						if ( event_list_add(&evt) == ESP_OK) {
							ESP_LOGI(__func__,"event '%s' stored", param);
						} else {
							ESP_LOGE(__func__,"could not store event '%s'", param);
						}
					} else {
						ESP_LOGW(__func__,"decode FAILED: %s\n", param);
					}

				}
				break;

				default:
					snprintf(resp_str,sizeof(resp_str),"%d NYI",i);
					httpd_resp_send_chunk(req, resp_str, strlen(resp_str));
				}
			}
			free(buf);
		}
	}
	snprintf(resp_str,sizeof(resp_str),"DONE\n");
	httpd_resp_send_chunk(req, resp_str, strlen(resp_str));

	// End response
	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;
}


static esp_err_t get_handler_reset(httpd_req_t *req)
{
	//size_t buf_len;

	// clear nvs
	nvs_flash_erase();
	scenes_stop();

	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");

	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

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

    // config
    httpd_uri_t strip_setup = {
        .uri       = "/api/v1/config",
        .method    = HTTP_GET,
        .handler   = get_handler_strip_config,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_setup);

    httpd_uri_t reset_uri = {
        .uri       = "/api/v1/reset",
        .method    = HTTP_GET,
        .handler   = get_handler_reset,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &reset_uri);

    httpd_uri_t status_uri = {
        .uri       = "/api/v1/scene",
        .method    = HTTP_GET,
        .handler   = get_handler_scene,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &status_uri);

    /*
    httpd_uri_t strip_setcolor = {
        .uri       = "/api/v1/setcolor",
        .method    = HTTP_GET,
        .handler   = get_handler_strip_setcolor,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_setcolor);
	*/
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
