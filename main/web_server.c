/*
 * web_server.c
 *
 *  Created on: 31.10.2022
 *      Author: ankrysm
 */

#include "esp32_ws2812.h"

#define INDEX_HTML "index.html"
#define SETTINGS_HTML "settings.html"
#define HELP_HTML "help.html"

#define HTML_TITLE "LED-strip-server"
#define HTML_HEADER "<!DOCTYPE html><html><meta charset=\"UTF-8\">" \
	"<head><title>"HTML_TITLE"</title></head>\n" \
	"<body>\n" \
	"<header>\n" \
	"<link rel=\"stylesheet\" href=\"stylesheet.css\">\n" \
	"<img src=\"logo.png\" alt=\"logo\">\n" \
	"<p>Welcome to the LED strip control service</p>\n" \
	"</header>\n" \
	"<nav>\n" \
		"<p><a href=\""INDEX_HTML"\">main</a></p>\n" \
		"<p><a href=\""SETTINGS_HTML"\">configuration</a></p>\n" \
		"<p><a href=\""HELP_HTML"\">help</a></p>\n" \
	"</nav>\n" \
	"<main\n>"


#define HTML_FOOTER  \
	"\n</main>\n" \
	"<footer>\n" \
	"<p> <a href=\"https://github.com/ankrysm/esp32_ws2812\">source code</a></p>\n" \
	"</footer>\n" \
	"</body>\n" \
	"</html>\n"

#define STYLESHEET_CSS \
	"table, th, td, caption {" \
	"border: thin solid #a0a0a0;" \
	"}" \
	"table {" \
		"border-collapse: collapse;" \
		"border-spacing: 0;" \
		"border-width: thin 0 0 thin;" \
		"margin: 0 0 1em;" \
		"table-layout: auto;" \
		"max-width: 100%;" \
	"}" \
	"th, td {" \
		"font-weight: normal;" \
		"text-align: left;" \
	"}" \
	"th, caption {" \
		"background-color: #f1f3f4;" \
		"font-weight: 700;" \
	"}"

// from global_data.c
extern T_EVENT_CONFIG event_config_tab[];
extern T_HTTP_PROCCESSING_TYPE http_processing[];
extern T_EVENT_CONFIG event_config_tab[];
extern T_OBJECT_ATTR_CONFIG object_attr_config_tab[];
extern T_OBJECT_CONFIG object_config_tab[];

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t sz = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, sz);
    return ESP_OK;
}

static esp_err_t logo_get_handler(httpd_req_t *req)
{
    extern const unsigned char ledstrip_png_start[] asm("_binary_ledstrip_png_start");
    extern const unsigned char ledstrip_png_end[]   asm("_binary_ledstrip_png_end");
    const size_t sz = (ledstrip_png_end - ledstrip_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)ledstrip_png_start, sz);
    return ESP_OK;
}

static esp_err_t stylesheet_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, STYLESHEET_CSS, strlen(STYLESHEET_CSS));
    return ESP_OK;
}

esp_err_t get_handler_index_html(httpd_req_t *req) {
	esp_err_t res = ESP_OK;

    // Send HTML file header
    httpd_resp_sendstr_chunk(req, HTML_HEADER);

    // HTML content
    httpd_resp_sendstr_chunk(req,
    		"<h2>Main</h2>" );

    // Footer
    httpd_resp_sendstr_chunk(req, HTML_FOOTER);
    // finished
    httpd_resp_send_chunk(req, NULL, 0);

    return res;
}

esp_err_t get_handler_settings_html(httpd_req_t *req) {
	esp_err_t res = ESP_OK;

	   // Send HTML file header
	    httpd_resp_sendstr_chunk(req, HTML_HEADER);

	    // HTML content
	    httpd_resp_sendstr_chunk(req,
				"<h2>Settings</h2>" );

	    // Footer
	    httpd_resp_sendstr_chunk(req, HTML_FOOTER);
	    // finished
	    httpd_resp_send_chunk(req, NULL, 0);

	return res;
}


esp_err_t get_handler_help_html(httpd_req_t *req) {
	esp_err_t res = ESP_OK;
	char txt[1024];

	   // Send HTML file header
	    httpd_resp_sendstr_chunk(req, HTML_HEADER);

	    // HTML content
	    httpd_resp_sendstr_chunk(req,"<h2>Help</h2>" );

	    // ****** Table: API reference **********
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

	    // ***** Table: "Event syntax" *************
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
	    		snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\"}</td><td>%s%s</td></tr>",
	    				event_config_tab[i].name,
						event_config_tab[i].help,
	    				(optional_para ? "(optional)":""));
	    		break;
	    	case EVT_PARA_NUMERIC:
	    		snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\", \"value\":&lt;numeric value&gt;}</td><td>%s%s</td></tr>",
	    				event_config_tab[i].name,
						event_config_tab[i].help,
	    				(optional_para ? "(optional)":""));
	    		break;
	    	case EVT_PARA_STRING:
	    		snprintf(txt, sizeof(txt), "<tr><td>{\"type\":\"%s\", \"value\":\"&lt;string value&gt;\" }</td><td>%s%s</td></tr>",
	    				event_config_tab[i].name,
						event_config_tab[i].help,
	    				(optional_para ? "(optional)":""));
	    		break;
	    	}
			httpd_resp_sendstr_chunk(req, txt);
	    }

	    // trailer
	    strlcpy(txt, "</table>\n", sizeof(txt));
	    httpd_resp_sendstr_chunk(req,txt);

	    // ****** Table: object parameter **********
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

	    // ****** Table: object definitions **********
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

	    // Footer
	    httpd_resp_sendstr_chunk(req, HTML_FOOTER);
	    // finished
	    httpd_resp_send_chunk(req, NULL, 0);

	return res;
}

/**
 * GET-Handler start page
 */
esp_err_t get_handler_html(httpd_req_t *req)
{
    ESP_LOGI(__func__, "GET '%s'", req->uri);

	if (strcmp(req->uri,"/") == 0 || ! strlen(req->uri)) {
		return get_handler_index_html(req);
	} else if (strcmp(req->uri, "/"INDEX_HTML) == 0) {
		return get_handler_index_html(req);

	} else if (strstr(req->uri, "/"SETTINGS_HTML) == req->uri) {
		return get_handler_settings_html(req);

	} else if (strstr(req->uri, "/"HELP_HTML) == req->uri) {
		return get_handler_help_html(req);

	} else if (strcmp(req->uri, "/favicon.ico") == 0) {
		return favicon_get_handler(req);
	} else if (strcmp(req->uri, "/logo.png") == 0) {
		return logo_get_handler(req);
	} else if (strcmp(req->uri, "/stylesheet.css") == 0) {
		return stylesheet_get_handler(req);
	}
	ESP_LOGE(__func__, "%s: no handler for '%s'", __func__, req->uri);
	return ESP_FAIL;

}
