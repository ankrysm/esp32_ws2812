/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
//#include "protocol_examples_common.h"
//#if CONFIG_EXAMPLE_WEB_DEPLOY_SD
//#include "driver/sdmmc_host.h"
//#endif
#include "esp_chip_info.h"
#include "local.h"
#include "config.h"
#include "timer_events.h"
#include "led_strip_proto.h"

#define MDNS_INSTANCE "esp home web server"


#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

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


static uint32_t trufal(char *txt) {
	if ( !strcmp(txt,"1") || !strcasecmp(txt,"true") || !strcasecmp(txt,"t")) {
		return 1;
	} else {
		return 0;
	}
}
/* Send HTTP response with the contents of the requested file */
/*
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(__func__, "Failed to open file : %s", filepath);
        // Respond with 500 Internal Server Error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        // Read file in chunks into the scratch buffer
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(__func__, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            // Send the buffer contents as HTTP response chunk
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(__func__, "File sending failed!");
                // Abort sending file
                httpd_resp_sendstr_chunk(req, NULL);
                // Respond with 500 Internal Server Error
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    // Close file after sending complete
    close(fd);
    ESP_LOGI(__func__, "File sending complete");
    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}
*/

/* Simple handler for light brightness control */
/*
static esp_err_t light_brightness_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        // Respond with 500 Internal Server Error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int red = cJSON_GetObjectItem(root, "red")->valueint;
    int green = cJSON_GetObjectItem(root, "green")->valueint;
    int blue = cJSON_GetObjectItem(root, "blue")->valueint;
    ESP_LOGI(__func__, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}
*/
/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for getting temperature data */
/*
static esp_err_t temperature_data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "raw", esp_random() % 20);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
*/

/*
static int process_commands(char *buf) {
	char *l,*t, *p1, *p2, *p3, *p4, *p5, *ll;
	const char *tr=";\r\n";
	for(t=strtok_r(buf,tr,&l); t; t=strtok_r(NULL,tr,&l)) {
		p1=p2=p3=p4=p5=NULL;

		int initialized =  strip_initialized();

		p1=strtok_r(t,", ", &ll);
		if ( !strcasecmp(p1,"c")) {
			if ( !initialized) {
				ESP_LOGE(__func__, "cmd '%s' not initialized",p1);
				return -1;
			}
			strip_clear();
			ESP_LOGI(__func__, "cmd '%s' (clear)", p1);

		} else if ( !strcasecmp(p1,"i")) {
			// init i,numleds;
			p2=strtok_r(NULL,", ", &ll);
			int numleds = atoi(p2);
	    	strip_setup(numleds);

			ESP_LOGI(__func__, "cmd '%s' (init %d)", p1, numleds);

		} else if ( !strcasecmp(p1,"p")) {
			// p,pos,r,g,b
			if ( !initialized) {
				ESP_LOGE(__func__, "cmd '%s' not initialized",p1);
				return -1;
			}
			do {
				if ( !(p2=strtok_r(NULL,", ", &ll))) break;
				if ( !(p3=strtok_r(NULL,", ", &ll))) break;
				if ( !(p4=strtok_r(NULL,", ", &ll))) break;
				if ( !(p5=strtok_r(NULL,", ", &ll))) break;
			} while(0);
			int pos = p2 ? atoi(p2) : 0;
			int red = p3 ? atoi(p3) : 0;
			int green = p4 ? atoi(p4) : 0;
			int blue = p5 ? atoi(p5) : 0;
			strip_set_color(pos, pos, red, green, blue);
			ESP_LOGI(__func__, "cmd '%s' (%d,%d,%d,%d)", p1, pos,red,green,blue);

		} else if ( !strcasecmp(p1,"h")) {
			// p,pos,h,s,v
			if ( !initialized) {
				ESP_LOGE(__func__, "cmd '%s' not initialized",p1);
				return -1;
			}
			do {
				if ( !(p2=strtok_r(NULL,", ", &ll))) break;
				if ( !(p3=strtok_r(NULL,", ", &ll))) break;
				if ( !(p4=strtok_r(NULL,", ", &ll))) break;
				if ( !(p5=strtok_r(NULL,", ", &ll))) break;
			} while(0);
			int pos = p2 ? atoi(p2) : 0;
			int hue = p3 ? atoi(p3) : 0;
			int sat = p4 ? atoi(p4) : 0;
			int val = p5 ? atoi(p5) : 0;
			uint32_t red;
			uint32_t green;
			uint32_t blue;
			led_strip_hsv2rgb(hue, sat, val, &red, &green, &blue);

			strip_set_color(pos, pos, red, green, blue);
			//ESP_LOGI(__func__, "cmd '%s' (%d,%d,%d,%d)", p1, pos,red,green,blue);

		} else if ( !strcasecmp(p1,"r")) {
			// rotate
			// r,n  -n < 0 oder > 0 die Richtung und stepweite
			if ( !initialized) {
				ESP_LOGE(__func__, "cmd '%s' not initialized",p1);
				return -1;
			}
			do {
				if ( !(p2=strtok_r(NULL,", ", &ll))) break;
			} while(0);
			int32_t dir = p2 ? atoi(p2) : 0;
			strip_rotate(dir);

		} else {
			ESP_LOGI(__func__, "ignored cmd => '%s'", p1);
		}
	}
 	strip_show();
 	return 0;
}
*/

/*
static esp_err_t post_handler_strip_file(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        // Respond with 500 Internal Server Error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    //ESP_LOGI(__func__, "%s: received %d total %d bytes", __func__, cur_len, total_len);

    buf[total_len] = '\0';
    int res = process_commands(buf);
    if ( res) {
    	httpd_resp_sendstr(req, "NOT INITIALIZED\n");
    } else {
    	httpd_resp_sendstr(req, "POST-Request successfully\n");
    }
    return ESP_OK;
}
*/


/* An HTTP GET handler */

/*
static esp_err_t get_handler_strip_setup(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;


    // Read URL query string length and allocate memory for length + 1,
    //  extra byte for null termination
    int numleds=0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(__func__, "Found URL query => %s", buf);
            char param[32];
            // Get value of expected key from query string
            if (httpd_query_key_value(buf, "n", param, sizeof(param)) == ESP_OK) {
                 ESP_LOGI(__func__, "Found URL query parameter => n=%s", param);
                 numleds = atoi(param);
             }
        }
        free(buf);
    }

    // TODO do_led1();
    char resp_str[64];
    if ( strip_initialized()) {
        snprintf(resp_str, sizeof(resp_str),"INITIALIZED\n");
    } else {
    	strip_setup(numleds);
        snprintf(resp_str, sizeof(resp_str),"done. numleds=%d\n", numleds);
    }

    // Response-String
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
*/

/*
static esp_err_t get_handler_strip_rotate(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;


    // Read URL query string length and allocate memory for length + 1,
    //  extra byte for null termination
    int32_t dir=0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(__func__, "Found URL query => %s", buf);
            char param[32];
            // Get value of expected key from query string
            if (httpd_query_key_value(buf, "d", param, sizeof(param)) == ESP_OK) {
                 ESP_LOGI(__func__, "Found URL query parameter => d=%s", param);
                 dir = atoi(param);
             }
        }
        free(buf);
    }

    char resp_str[64];
    if ( !strip_initialized()) {
        snprintf(resp_str, sizeof(resp_str),"NOT INITIALIZED\n");
    } else {
    	strip_rotate(dir);
    	strip_show();
        snprintf(resp_str, sizeof(resp_str),"done: %d\n", dir);
    }

    // Response-String
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
*/


static esp_err_t get_handler_strip_setcolor(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    // Read URL query string length and allocate memory for length + 1,
    // extra byte for null termination
    uint32_t red=0;
    uint32_t green=0;
    uint32_t blue=0;
    uint32_t start_idx=1;
    uint32_t end_idx=2; //strip_numleds()-1;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(__func__, "Found URL query => %s", buf);
            char param[32];
            char *paramname = "range";
            char *tok1, *tok2, *tok3, *s, *l;

            // Get value of expected key from query string
            if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
                 ESP_LOGI(__func__, "Found URL query parameter => %s=%s", paramname, param);
                 // expected "start,end"
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

            paramname = "rgb";
            if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
                 ESP_LOGI(__func__, "Found URL query parameter => %s=%s", paramname, param);
                 // expected "start,end"
                 s=strdup(param);
                 tok1=strtok_r(s,   ",", &l);
                 tok2=strtok_r(NULL,",", &l);
                 tok3=tok2 ? strtok_r(NULL,",", &l) : NULL;
                 red = atoi(tok1);
                 if (tok2) {
                	 green = atoi(tok2);
                 }
                 if (tok3) {
                	 blue = atoi(tok3);
                 }
                 free(s);
            }

            paramname = "hsv";
            if (httpd_query_key_value(buf, paramname, param, sizeof(param)) == ESP_OK) {
                 ESP_LOGI(__func__, "Found URL query parameter => %s=%s", paramname, param);
                 // expected "start,end"
                 s=strdup(param);
                 tok1=strtok_r(s,   ",", &l);
                 tok2=strtok_r(NULL,",", &l);
                 tok3=tok2 ? strtok_r(NULL,",", &l) : NULL;
                 uint32_t hh,ss,vv;
                 hh=ss=vv=0;
                 hh = atoi(tok1);
                 if (tok2) {
                	 ss = atoi(tok2);
                 }
                 if (tok3) {
                	 vv = atoi(tok3);
                 }
                 free(s);
                 led_strip_hsv2rgb(hh, ss, vv, &red, &green, &blue);

            }

        }
        free(buf);
    }

    char resp_str[64];
    if ( !strip_initialized()) {
        snprintf(resp_str, sizeof(resp_str),"NOT INITIALIZED\n");
    } else {
    	strip_set_color(start_idx, end_idx, red, green, blue);
    	strip_show();
        snprintf(resp_str, sizeof(resp_str),"done: %d-%d rgb=%d/%d/%d\n", start_idx, end_idx, red,green,blue);
    }
    ESP_LOGI(__func__,"response=%s", resp_str);

    // Send response with custom headers and body set as the
    // string passed in user context
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

	extern T_CONFIG gConfig;

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char param[32];
			// Get value of expected key from query string
			if (httpd_query_key_value(buf, "autoplay", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: autoplay=%s", param);
				gConfig.flags &= !CFG_AUTOPLAY;
				gConfig.flags |= trufal(param);
			}
			if (httpd_query_key_value(buf, "scenefile", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: scenefile=%s", param);
				snprintf(gConfig.scenefile, sizeof(gConfig.scenefile), "%s", param);
			}
			if (httpd_query_key_value(buf, "repeat", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: repeat=%s", param);
				gConfig.flags &= !CFG_REPEAT;
				gConfig.flags |= trufal(param);
			}
			if (httpd_query_key_value(buf, "numleds", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: numleds=%s", param);
				int numleds = atoi(param);
				gConfig.numleds = numleds;

				// stop playing when numleds changed
				scenes_stop();
				strip_setup(numleds);
			}
			if (httpd_query_key_value(buf, "cycle", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: cycle=%s", param);
				gConfig.cycle = atoi(param);
				// stop playing when cacle changed
				scenes_stop();
				set_timer_cycle(gConfig.cycle);
			}
		}
		free(buf);

		store_config();
	}

	// TODO do_led1();
	char resp_str[255];


//	if ( !strip_initialized()) {
//		snprintf(resp_str, sizeof(resp_str),"NOT INITIALIZED\n");
//	} else {
//		strip_set_color(start_idx, end_idx, red, green, blue);
//		strip_show();
//		snprintf(resp_str, sizeof(resp_str),"done: %d-%d rgb=%d/%d/%d\n", start_idx, end_idx, red,green,blue);
//	}

	// Set some custom headers
	//httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	//httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	// Send response with custom headers and body set as the
	// string passed in user context
	//const char* resp_str = (const char*) "done\n";

	config2txt(resp_str, sizeof(resp_str));
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

	// After sending the HTTP response the old HTTP request
	// headers are lost. Check if HTTP request headers can be read now.
//	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
//		ESP_LOGI(__func__, "Request headers lost");
//	}

	return ESP_OK;
}



static esp_err_t get_handler_status_do(httpd_req_t *req, run_status_type new_status)
{
	char*  buf;
	size_t buf_len;

	extern T_CONFIG gConfig;

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

	run_status_type old_status = SCENES_NOTHING;

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = malloc(buf_len);
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			ESP_LOGI(__func__, "Found URL query => %s", buf);
			char param[64];
			// Get value of expected key from query string
			if (httpd_query_key_value(buf, "scenefile", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(__func__, "query parameter: scenefile=%s", param);
				new_status = SCENES_RUNNING;
				// TODO: new scene file
			}
		}
		free(buf);
	}

	char resp_str[255];

	if (new_status != SCENES_NOTHING) {
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


	// Send response with custom headers and body set as the
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

	return ESP_OK;
}

static esp_err_t get_handler_run(httpd_req_t *req) {
	return get_handler_status_do(req, SCENES_RUNNING);
}

static esp_err_t get_handler_stop(httpd_req_t *req) {
	return get_handler_status_do(req, SCENES_STOPPED);
}

static esp_err_t get_handler_pause(httpd_req_t *req) {
	return get_handler_status_do(req, SCENES_PAUSED);
}

static esp_err_t get_handler_restart(httpd_req_t *req) {
	return get_handler_status_do(req, SCENES_RESTART);
}

static esp_err_t get_handler_status(httpd_req_t *req) {
	return get_handler_status_do(req, SCENES_NOTHING);
}

static esp_err_t get_handler_reset(httpd_req_t *req)
{
//	char*  buf;
	size_t buf_len;

//	extern T_CONFIG gConfig;

	// Read URL query string length and allocate memory for length + 1,
	// extra byte for null termination

//	buf_len = httpd_req_get_url_query_len(req) + 1;
//	if (buf_len > 1) {
//		buf = malloc(buf_len);
//		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
//			ESP_LOGI(__func__, "Found URL query => %s", buf);
//			char param[32];
//			// Get value of expected key from query string
//			if (httpd_query_key_value(buf, "autoplay", param, sizeof(param)) == ESP_OK) {
//				ESP_LOGI(__func__, "query parameter: autoplay=%s", param);
//				gConfig.flags &= !CFG_AUTOPLAY;
//				gConfig.flags |= trufal(param);
//			}
//			if (httpd_query_key_value(buf, "scenefile", param, sizeof(param)) == ESP_OK) {
//				ESP_LOGI(__func__, "query parameter: scenefile=%s", param);
//				snprintf(gConfig.scenefile, sizeof(gConfig.scenefile), "%s", param);
//			}
//			if (httpd_query_key_value(buf, "repeat", param, sizeof(param)) == ESP_OK) {
//				ESP_LOGI(__func__, "query parameter: repeat=%s", param);
//				gConfig.flags &= !CFG_REPEAT;
//				gConfig.flags |= trufal(param);
//			}
//			if (httpd_query_key_value(buf, "numleds", param, sizeof(param)) == ESP_OK) {
//				ESP_LOGI(__func__, "query parameter: numleds=%s", param);
//				gConfig.numleds = atoi(param);
//				strip_setup(gConfig.numleds);
//			}
//			if (httpd_query_key_value(buf, "cycle", param, sizeof(param)) == ESP_OK) {
//				ESP_LOGI(__func__, "query parameter: cycle=%s", param);
//				gConfig.cycle = atoi(param);
//			}
//		}
//		free(buf);
//	}

	// clear nvs
	nvs_flash_erase();
	scenes_stop();

	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str),"RESET done\n");


//	if ( !strip_initialized()) {
//		snprintf(resp_str, sizeof(resp_str),"NOT INITIALIZED\n");
//	} else {
//		strip_set_color(start_idx, end_idx, red, green, blue);
//		strip_show();
//		snprintf(resp_str, sizeof(resp_str),"done: %d-%d rgb=%d/%d/%d\n", start_idx, end_idx, red,green,blue);
//	}

	// Set some custom headers
	//httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
	//httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

	// Send response with custom headers and body set as the
	// string passed in user context
	//const char* resp_str = (const char*) "done\n";

	//config2txt(resp_str, sizeof(resp_str));
	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

	// After sending the HTTP response the old HTTP request
	// headers are lost. Check if HTTP request headers can be read now.
//	if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
//		ESP_LOGI(__func__, "Request headers lost");
//	}

	return ESP_OK;
}


static httpd_handle_t server = NULL;

esp_err_t start_rest_server(const char *base_path)
{
	if ( !base_path ) {
		ESP_LOGE(__func__, "base_path missing");
		return ESP_FAIL;
	}

    //REST_CHECK(base_path, "wrong base path", err);

	rest_server_context_t *rest_context = (rest_server_context_t*) calloc(1, sizeof(rest_server_context_t));
    //REST_CHECK(rest_context, "No memory for rest context", err);
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
    //REST_CHECK
    if ( httpd_start(&server, &config) != ESP_OK ) {
    	ESP_LOGE(__func__, "Start server failed");
        free(rest_context);
        return ESP_FAIL;
    }

    // Install URI Handler
    // URI handler for fetching system info
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    // vconfig
    httpd_uri_t strip_setup = {
        .uri       = "/api/v1/config",
        .method    = HTTP_GET,
        .handler   = get_handler_strip_config, // get_handler_strip_setup,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_setup);

    httpd_uri_t reset_uri = {
        .uri       = "/api/v1/reset",
        .method    = HTTP_GET,
        .handler   = get_handler_reset, // get_handler_strip_setup,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &reset_uri);

    httpd_uri_t status_uri = {
        .uri       = "/api/v1/status",
        .method    = HTTP_GET,
        .handler   = get_handler_status,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &status_uri);

    /*
    httpd_uri_t run_uri = {
        .uri       = "/api/v1/run",
        .method    = HTTP_GET,
        .handler   = get_handler_run,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &run_uri);

    httpd_uri_t pause_uri = {
         .uri       = "/api/v1/pause",
         .method    = HTTP_GET,
         .handler   = get_handler_pause,
         .user_ctx  = rest_context
     };
     httpd_register_uri_handler(server, &pause_uri);

     httpd_uri_t stop_uri = {
         .uri       = "/api/v1/stop",
         .method    = HTTP_GET,
         .handler   = get_handler_stop,
         .user_ctx  = rest_context
     };
     httpd_register_uri_handler(server, &stop_uri);

     httpd_uri_t restart_uri = {
         .uri       = "/api/v1/restart",
         .method    = HTTP_GET,
         .handler   = get_handler_restart,
         .user_ctx  = rest_context
     };
     httpd_register_uri_handler(server, &restart_uri);
	*/


    httpd_uri_t strip_setcolor = {
        .uri       = "/api/v1/setcolor",
        .method    = HTTP_GET,
        .handler   = get_handler_strip_setcolor,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_setcolor);

    /**
    httpd_uri_t strip_rotate = {
        .uri       = "/api/v1/rotate",
        .method    = HTTP_GET,
        .handler   = get_handler_strip_rotate,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_rotate);

    httpd_uri_t strip_file = {
        .uri       = "/api/v1/file",
        .method    = HTTP_POST,
        .handler   = post_handler_strip_file,
        .user_ctx  = rest_context
    };
    httpd_register_uri_handler(server, &strip_file);
*/
    return ESP_OK;
//err_start:
//    free(rest_context);
//err:
//    return ESP_FAIL;
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

//	ESP_ERROR_CHECK(example_connect());
	ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
	//ESP_LOGI(__func__, "ohne HTTP Server start");

}

void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

void initialise_netbios() {
	netbiosns_init();
	netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);
}
