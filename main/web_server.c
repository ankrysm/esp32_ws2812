/*
 * web_server.c
 *
 *  Created on: 31.10.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

typedef struct WEBRESSOURCE {
	char *url;
	char *content_type;
	const unsigned char *start;
	const unsigned char *end;
	size_t sz;
} T_WEBRESSOURCE;

#define CONTENT_TOKEN "<!-- MAIN -->"

// from global_data.c
extern T_EVENT_CONFIG event_config_tab[];
extern T_HTTP_PROCCESSING_TYPE http_processing[];
extern T_EVENT_CONFIG event_config_tab[];
extern T_OBJECT_ATTR_CONFIG object_attr_config_tab[];
extern T_OBJECT_CONFIG object_config_tab[];

extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
extern const unsigned char ledstrip_jpg_start[] asm("_binary_ledstrip_jpg_start");
extern const unsigned char ledstrip_jpg_end[]   asm("_binary_ledstrip_jpg_end");
extern const unsigned char stylesheet_css_start[] asm("_binary_stylesheet_css_start");
extern const unsigned char stylesheet_css_end[]   asm("_binary_stylesheet_css_end");
extern const unsigned char main_page_html_start[] asm("_binary_main_page_html_start");
extern const unsigned char main_page_html_end[]   asm("_binary_main_page_html_end");
extern const unsigned char main_content_html_start[] asm("_binary_main_content_html_start");
extern const unsigned char main_content_html_end[]   asm("_binary_main_content_html_end");
extern const unsigned char settings_content_html_start[] asm("_binary_settings_content_html_start");
extern const unsigned char settings_content_html_end[]   asm("_binary_settings_content_html_end");
extern const unsigned char files_content_html_start[] asm("_binary_files_content_html_start");
extern const unsigned char files_content_html_end[]   asm("_binary_files_content_html_end");

extern const unsigned char example_json_start[] asm("_binary_example_json_start");
extern const unsigned char example_json_end[]   asm("_binary_example_json_end");
extern const unsigned char example_config_json_start[] asm("_binary_example_config_json_start");
extern const unsigned char example_config_json_end[]   asm("_binary_example_config_json_end");

T_WEBRESSOURCE webressources[] = {
		{"/favicon.ico", "image/x-icon", favicon_ico_start, favicon_ico_end, 0 },
		{"/ledstrip.jpg","image/jpg", ledstrip_jpg_start, ledstrip_jpg_end, 0 },
		{"/stylesheet.css", "text/css", stylesheet_css_start,stylesheet_css_end, 0},
		{"","", NULL, NULL, 0}
};

static esp_err_t get_handler_main_ressource(httpd_req_t *req) {
	for (int i=0; webressources[i].start; i++) {
		if ( !strcmp(req->uri, webressources[i].url)) {
			size_t sz = webressources[i].end - webressources[i].start;
			httpd_resp_set_type(req,  webressources[i].content_type);
		    httpd_resp_send(req, (const char *)webressources[i].start, sz);
			return ESP_OK;
		}
	}
	return ESP_ERR_NOT_FOUND;
}

esp_err_t get_handler_main_header(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {

	size_t sz = (main_page_html_end - main_page_html_start);

    unsigned char *content_ptr = memmem(main_page_html_start, sz, CONTENT_TOKEN, strlen(CONTENT_TOKEN));
    if ( !content_ptr) {
    	snprintf(errmsg, sz_errmsg, "couldn't find pattern for content in data");
    	return ESP_FAIL;
    }

    // Send HTML file header
    sz = (content_ptr - main_page_html_start);
    httpd_resp_send_chunk(req, (const char *)main_page_html_start, sz);

    return ESP_OK;
}

esp_err_t get_handler_main_footer(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {

	size_t sz = (main_page_html_end - main_page_html_start);

	unsigned char *content_ptr = memmem(main_page_html_start, sz, CONTENT_TOKEN, strlen(CONTENT_TOKEN));
	if ( !content_ptr) {
		snprintf(errmsg, sz_errmsg, "couldn't find pattern for content in data");
		return ESP_FAIL;
	}
	content_ptr += strlen(CONTENT_TOKEN);
	sz = main_page_html_end - content_ptr;
	httpd_resp_send_chunk(req, (const char *)content_ptr, sz);

	return ESP_OK;

}

static esp_err_t get_handler_index_html(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {

	esp_err_t res;
	if ( (res = get_handler_main_header(req, errmsg, sz_errmsg)) != ESP_OK)
		return res;

	// HTML content
    const size_t sz = (main_content_html_end - main_content_html_start);
    httpd_resp_send_chunk(req, (const char *)main_content_html_start, sz);

	return get_handler_main_footer(req, errmsg, sz_errmsg);
}

static esp_err_t get_handler_settings_html(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {
	esp_err_t res;
	if ( (res = get_handler_main_header(req, errmsg, sz_errmsg)) != ESP_OK)
		return res;

	// HTML content
    const size_t sz = (settings_content_html_end - settings_content_html_start);
    httpd_resp_send_chunk(req, (const char *)settings_content_html_start, sz);

     return get_handler_main_footer(req, errmsg, sz_errmsg);

}
static esp_err_t get_handler_files_html(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {
	esp_err_t res;
	if ( (res = get_handler_main_header(req, errmsg, sz_errmsg)) != ESP_OK)
		return res;

	// HTML content
    const size_t sz = (files_content_html_end - files_content_html_start);
    httpd_resp_send_chunk(req, (const char *)files_content_html_start, sz);

     return get_handler_main_footer(req, errmsg, sz_errmsg);

}

static void get_handler_help_html_api_reference(httpd_req_t *req) {
	char txt[256];
	// caption
	strlcpy(txt, "<table>\n" \
			"<caption>API reference</caption>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// header
	memset(txt, 0, sizeof(txt));
	strlcat(txt,"<tr>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"path", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"description", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"</tr>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// content
	for  (int i=0; http_processing[i].todo != HP_END_OF_LIST; i++) {
		snprintf(txt, sizeof(txt),"<tr><td>%s%s</td><td>%s%s</td></tr>\n",
				http_processing[i].path,
				(http_processing[i].flags & HPF_PATH_FROM_URL ? "\"fname\"" : ""),
				http_processing[i].help,
				(http_processing[i].flags & HPF_POST ? ", requires POST data" : "")
		);
		httpd_resp_sendstr_chunk(req, txt);
	}

	// trailer
	strlcpy(txt, "</table>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);
}

static void get_handler_help_html_event_syntax(httpd_req_t *req) {
	char txt[256];
	// caption
	strlcpy(txt, "<table>\n" \
			"<caption>event syntax</caption>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// header
	memset(txt, 0, sizeof(txt));
	strlcat(txt,"<tr>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"json syntax", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"description", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"</tr>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// content
	for (int i=0; event_config_tab[i].evt_type != ET_NONE; i++) {
		int evtcfg = event_config_tab[i].evt_para_type & 0x0F;
		bool optional_para = event_config_tab[i].evt_para_type & EVT_PARA_OPTIONAL;
		switch (evtcfg) {
		case EVT_PARA_NONE:
			snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\"}</td><td>%s%s</td></tr>\n",
					event_config_tab[i].name,
					event_config_tab[i].help,
					(optional_para ? "(optional)":""));
			break;
		case EVT_PARA_NUMERIC:
			snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\", \"value\":&lt;%s&gt;}</td><td>%s%s</td></tr>\n",
					event_config_tab[i].name,
					event_config_tab[i].parahelp,
					event_config_tab[i].help,
					(optional_para ? "(optional)":""));
			break;
		case EVT_PARA_STRING:
			snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\", \"value\":\"&lt;%s&gt;\" }</td><td>%s%s</td></tr>\n",
					event_config_tab[i].name,
					event_config_tab[i].parahelp,
					event_config_tab[i].help,
					(optional_para ? "(optional)":""));
			break;
		}
		httpd_resp_sendstr_chunk(req, txt);
	}

	// trailer
	strlcpy(txt, "</table>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);
}

static void get_handler_help_html_object_parameter(httpd_req_t *req) {
	char txt[256];
	// caption
	strlcpy(txt, "<table>\n" \
			"<caption>object parameter</caption>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// header
	memset(txt, 0, sizeof(txt));
	strlcat(txt,"<tr>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"attribute name", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"description", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"</tr>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// content
	for  (int i=0; object_attr_config_tab[i].type != OBJATTR_EOT; i++) {
		snprintf(txt, sizeof(txt),"<tr><td>\"%s\"</td><td>%s</td></tr>\n",
				object_attr_config_tab[i].name,
				object_attr_config_tab[i].help
		);
		httpd_resp_sendstr_chunk(req, txt);
	}

	// trailer
	strlcpy(txt, "</table>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);
}

static void get_handler_help_html_object_definitions(httpd_req_t *req) {
	char txt[256];
	// caption
	strlcpy(txt, "<table>\n" \
			"<caption>object definition</caption>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// header
	memset(txt, 0, sizeof(txt));
	strlcat(txt,"<tr>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"\"type\"", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"parameter group 1", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"parameter group 2", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"<th>", sizeof(txt)); strlcat(txt,"description", sizeof(txt)); strlcat(txt,"</th>", sizeof(txt));
	strlcat(txt,"</tr>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);

	// content
	for  (int i=0; object_config_tab[i].type != OBJT_EOT; i++) {
		char g1[64];
		char g2[64];


		memset(g1,0,sizeof(g1));
		memset(g2,0,sizeof(g2));
		object_attr_group2text(object_config_tab[i].attr_group1, g1, sizeof(g1));
		object_attr_group2text(object_config_tab[i].attr_group2, g2, sizeof(g2));
		snprintf(txt, sizeof(txt),"<tr><td>\"%s\"</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
				object_config_tab[i].name,
				g1, g2,
				object_config_tab[i].help
		);
		httpd_resp_sendstr_chunk(req, txt);
	}

	// trailer
	strlcpy(txt, "</table>\n", sizeof(txt));
	httpd_resp_sendstr_chunk(req,txt);
}

static void get_handler_help_html_examples(httpd_req_t *req) {
	const size_t sz_example_json = (example_json_end - example_json_start);
	const size_t sz_example_config_json = (example_config_json_end - example_config_json_start);

	httpd_resp_sendstr_chunk(req,"<p>Example of a displayed scene</p>\n" );
	httpd_resp_sendstr_chunk(req, "<pre><code class=\"language-json\">\n");
	httpd_resp_send_chunk(req, (const char *)example_json_start, sz_example_json);
	httpd_resp_sendstr_chunk(req,"</code></pre>\n");

	httpd_resp_sendstr_chunk(req,"<p>Example of a configuration file</p>\n" );
	httpd_resp_sendstr_chunk(req, "<pre><code class=\"language-json\">\n");
	httpd_resp_send_chunk(req, (const char *)example_config_json_start, sz_example_config_json);
	httpd_resp_sendstr_chunk(req,"</code></pre>\n");

}


static esp_err_t get_handler_help_html(httpd_req_t *req, char *errmsg, size_t sz_errmsg) {
	esp_err_t res = ESP_OK;


	// Send HTML file header
	if ( (res = get_handler_main_header(req, errmsg, sz_errmsg)) != ESP_OK)
		return res;

	// HTML content
	// ***** start of article
	httpd_resp_sendstr_chunk(req,"<article>");

	httpd_resp_sendstr_chunk(req,"<h2>Help</h2>" );

	httpd_resp_sendstr_chunk(req,"<h3>API Description</h3>" );

	/// **** common descriptions *****
	httpd_resp_sendstr_chunk(req,"<p>Configurations and scenes are described in json files with sections</p>\n");
	httpd_resp_sendstr_chunk(req,"<p>\"objects\" - what to display, colors and section length</p>\n");
	httpd_resp_sendstr_chunk(req,"<p>\"events\" - how to arrange and modify displayed objects, move them, blink ... </p>\n");
	httpd_resp_sendstr_chunk(req,"<p>\"tracks\" - arrange events, repeat them </p>\n");

	// ****** Table: API reference **********
	get_handler_help_html_api_reference(req);

	// ***** Table: "Event syntax" *************
	get_handler_help_html_event_syntax(req);

	// ****** Table: object parameter **********
	get_handler_help_html_object_parameter(req);

	// ****** Table: object definitions **********
	get_handler_help_html_object_definitions(req);

	// ******* examples **********************
	httpd_resp_sendstr_chunk(req,"<h3>Examples</h3>\n" );
	get_handler_help_html_examples(req);

	// TODO add script section to provide onBodyLoad

	// ***** end of article
	httpd_resp_sendstr_chunk(req,"</article>");
    return get_handler_main_footer(req, errmsg, sz_errmsg);
}

/**
 * GET-Handler start page
 */
esp_err_t get_handler_html(httpd_req_t *req)
{

	esp_err_t res = ESP_OK;
	char errmsg[256];
	memset(errmsg, 0, sizeof(errmsg));
	ESP_LOGI(__func__, "GET '%s'", req->uri);

	res = get_handler_main_ressource(req);
	if ( res == ESP_OK)
		return res; // it was a ressource

	if (strcmp(req->uri,"/") == 0 || ! strlen(req->uri)) {
		res = get_handler_index_html(req, errmsg, sizeof(errmsg));
	} else if (strcmp(req->uri, "/index.html") == 0) {
		res = get_handler_index_html(req, errmsg, sizeof(errmsg));

	} else if (strstr(req->uri, "/settings.html") == req->uri) {
		res = get_handler_settings_html(req, errmsg, sizeof(errmsg));

	} else if (strstr(req->uri, "/settings.html") == req->uri) {
		res = get_handler_settings_html(req, errmsg, sizeof(errmsg));

	} else if (strstr(req->uri, "/files.html") == req->uri) {
		res = get_handler_files_html(req, errmsg, sizeof(errmsg));

	} else if (strstr(req->uri, "/help.html") == req->uri) {
		res = get_handler_help_html(req, errmsg, sizeof(errmsg));

	} else {
		// not found
		res = ESP_ERR_NOT_FOUND;
	}

	switch (res) {
	case ESP_OK:
		break;
	case ESP_ERR_NOT_FOUND:
		snprintf(errmsg,sizeof(errmsg),"nothing found");
		ESP_LOGE(__func__, "%s", errmsg);
		snprintfapp(errmsg, sizeof(errmsg), "\n");
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, errmsg);
		break;
	default:
		ESP_LOGE(__func__, "%s", errmsg);
		snprintfapp(errmsg, sizeof(errmsg), "\n");
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, errmsg);
	}

	httpd_resp_send_chunk(req, NULL, 0);
	return res;

}
