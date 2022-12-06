/**
 * HTTP Restful API Server - MAIN service
 * based on esp-idf examples
 */


#include "esp32_ws2812.h"

#define MAX_CONTENTLEN 10240


typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)


static httpd_handle_t server = NULL;

// from global_data.c
extern T_HTTP_PROCCESSING_TYPE http_processing[];

static void http_help(httpd_req_t *req) {
	char resp_str[255];
	snprintf(resp_str, sizeof(resp_str), "path - description\n");
	httpd_resp_sendstr_chunk(req, resp_str);

	for  (int i=0; http_processing[i].todo != HP_END_OF_LIST; i++) {
		snprintf(resp_str, sizeof(resp_str),"'%s%s' - %s%s\n",
				http_processing[i].path,
				(http_processing[i].flags & HPF_PATH_FROM_URL ? "<fname>" : ""),
				http_processing[i].help,
				(http_processing[i].flags & HPF_POST ? ", requires POST data" : "")
		);
		httpd_resp_sendstr_chunk(req, resp_str);
	}

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
	memset(resp_str, 0, sizeof(resp_str));

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

    LOG_MEM(__func__,1);
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

    LOG_MEM(__func__,2);

    esp_err_t res = ESP_FAIL;

    switch(pt->todo) {
    case HP_LOAD:
    	res = post_handler_load(req, buf);
    	break;

    case HP_FILE_STORE:
    	res = post_handler_file_store(req, buf, fname, sizeof(fname));
    	break;

    case HP_CONFIG_SET:
    	res = post_handler_config_set(req, buf);
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

	LOG_MEM(__func__,3);
	return res;
}

static esp_err_t get_handler_main(httpd_req_t *req)
{
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	char path[256];
	get_path_from_uri(req->uri, path, sizeof(path));
	ESP_LOGI(__func__,"uri='%s', path='%s'", req->uri, path);

	char resp_str[64];
	esp_err_t res = ESP_OK;

	T_HTTP_PROCCESSING_TYPE *pt = get_http_processing(path);
	if (!pt ) {
		// try web site
		res = get_handler_html(req);
		return res;
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

	case HP_LIST_ERR:
		get_handler_list_err(req);
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
		get_handler_scene_new_status(req, RUN_STATUS_STOP_AND_BLANK);
		break;

	case HP_ASK:
		get_handler_scene_new_status(req, RUN_STATUS_ASK);
		break;

	case HP_CONFIG_GET:
		get_handler_config(req,"");
		break;

	case HP_RESET:
		get_handler_restart(req);
		break;

	case HP_CFG_RESET:
		get_handler_reset(req);
		break;

	case HP_CFG_OTA_CHECK:
		get_handler_ota_check(req);
		break;

	case HP_CFG_OTA_UPDATE:
		get_handler_ota_update(req);
		break;

	case HP_CFG_OTA_STATUS:
		get_handler_ota_status(req, "");
		break;

	case HP_CLEAR_ERR:
		get_handler_clear_err(req);
		break;

	case HP_TEST:
		get_handler_test(req, fname, sizeof(fname));
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



static esp_err_t start_rest_server(const char *base_path)
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

void initialise_mdns(void) {
}

void initialise_netbios() {
}
