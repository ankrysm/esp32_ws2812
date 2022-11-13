/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "esp32_ws2812_basic.h"
#include "wifi_config.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
// status bits for connect process
static const int WIFI_BIT_CONNECT_WITH_KNOWN_CREDETIALS = BIT0;
static const int WIFI_BIT_CONNECT_WITH_SMART_CONFIG = BIT1;
static const int WIFI_BIT_CONNECTED = BIT2;
static const int WIFI_BIT_CONNECT_FAILED = BIT3;

// bits for configuration of connect process
static const int WIFI_BIT_NO_CONNECTION_STORED = BIT4;
static const int WIFI_BIT_SMART_CONFIG_RUNNING = BIT5;

static volatile wifi_status_type s_wifi_connect_status = WIFI_IDLE;

static const int WIFI_BITS_ALL = WIFI_BIT_CONNECT_WITH_KNOWN_CREDETIALS | \
		WIFI_BIT_CONNECT_WITH_SMART_CONFIG | \
		WIFI_BIT_CONNECTED | \
		WIFI_BIT_CONNECT_FAILED | \
		WIFI_BIT_NO_CONNECTION_STORED | \
		WIFI_BIT_SMART_CONFIG_RUNNING;

static const int WIFI_BIT_DONE = WIFI_BIT_CONNECTED | WIFI_BIT_CONNECT_FAILED;

static int s_retry_num = 0;
static int s_max_retry_num = 5;

/**
 * store the wifi config in the nvs
 */
static esp_err_t store_wifi_config(char *ssid, char *pw) {
	nvs_handle_t my_handle;

    ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'",(ssid?ssid:"<null>"),(pw?pw:"<null>") );

	esp_err_t ret = nvs_open(STORAGE_WIFI_NAMESPACE, NVS_READWRITE, &my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_set_str(my_handle, STORAGE_WIFI_KEY_SSID , ssid ? ssid : "");
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_str(%s) failed (%s)", STORAGE_WIFI_KEY_SSID, esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_set_str(my_handle, STORAGE_WIFI_KEY_PW , pw ? pw : "");
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_str(%s) failed (%s)", STORAGE_WIFI_KEY_PW, esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_commit(my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_commit() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	nvs_close(my_handle);

    ESP_LOGI(__func__, "done.");

	return ESP_OK;

}

/**
 * retrieve the wifi config from nvs
 */
static esp_err_t get_wifi_config(char *ssid, size_t sz_ssid, char *pw, size_t sz_pw) {

	esp_err_t ret;

	memset(ssid, 0, sz_ssid);
	memset(pw, 0, sz_pw);

	nvs_handle_t my_handle;
	ret = nvs_open(STORAGE_WIFI_NAMESPACE, NVS_READWRITE, &my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	// get WIFI config
	char *keyname=STORAGE_WIFI_KEY_SSID;
	size_t size = sz_ssid;
	ret = nvs_get_str(my_handle, keyname, NULL, &size);
	if ( ret == ESP_OK ) {
		// known
		nvs_get_str(my_handle, keyname, ssid, &size);
		ESP_LOGI(__func__, "getting '%s' successful sz=%d: '%s'",keyname, size, ssid );

	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
		// nothing stored
		ESP_LOGI(__func__, "nothing stored for '%s'", keyname);
		ssid = strdup("");
	}

	keyname=STORAGE_WIFI_KEY_PW;
	size = sz_pw;
	ret = nvs_get_str(my_handle, keyname, NULL, &size);
	if ( ret == ESP_OK ) {
		// known
		nvs_get_str(my_handle, keyname, pw, &size);
		ESP_LOGI(__func__, "getting '%s' successful sz=%d: '%s'",keyname, size, "*****"); //pw );

	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
		// nothing stored, store an emtpy string
		ESP_LOGI(__func__, "nothing stored for '%s'", keyname);
		pw = strdup("");
	}

	ESP_LOGI(__func__, "stored Wifi-Connection ssid='%s', pw='%s'", ssid, "****" ); //pw );

	// close handle immediately, if it's necessary to open it again, it will be done later
	nvs_close(my_handle);
	return ESP_OK;

}


/**
 * ##########################
 * smart config functions
 * ##########################
 */


// Background task for connect process
static void smartconfig_task(void * parm)
{
	ESP_LOGI(__func__, "started.");
	EventBits_t uxBits;
	while (1) {
		uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_BITS_ALL, true, false, portMAX_DELAY);
		ESP_LOGI(__func__, "uxBits = 0x%x", uxBits);

		if(uxBits & WIFI_BIT_CONNECTED) {
			ESP_LOGI(__func__, "WiFi Connected to ap");
			s_wifi_connect_status = WIFI_CONNECTED;
		}
		if(uxBits & WIFI_BIT_CONNECT_FAILED) {
			ESP_LOGE(__func__, "WiFi Connect to ap failed");
			s_wifi_connect_status = WIFI_CONNECTION_FAILED;
		}
		if(uxBits & WIFI_BIT_DONE) {
			ESP_LOGI(__func__, "wifi init done");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
}

// event handler for connect process
static void smartconfig_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	EventBits_t uxBits = xEventGroupGetBits(s_wifi_event_group);
	int smartconfig_running = uxBits & WIFI_BIT_SMART_CONFIG_RUNNING;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
    	s_retry_num = 0;
    	s_wifi_connect_status = WIFI_TRY_CONNECT;

        if (uxBits & WIFI_BIT_NO_CONNECTION_STORED ) {
        	// no credentials stored, start with smartconfig
			xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_SMART_CONFIG_RUNNING);
			s_wifi_connect_status = WIFI_TRY_SMART_CONFIG;
	    	//xEventGroupClearBits(s_wifi_event_group, WIFI_TRY_TO_CONNECT_BIT);

        	ESP_LOGI(__func__,"[0x%03X] start smart config",uxBits);

        	ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
        	smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        	ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

        } else {
        	// try to connect with known credentials
        	//xEventGroupSetBits(s_wifi_event_group, WIFI_TRY_TO_CONNECT_BIT);
        	ESP_LOGI(__func__,"[0x%03X] start connect with stored data", uxBits);
            esp_wifi_connect();
        }

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

    	xEventGroupClearBits(s_wifi_event_group, WIFI_BIT_CONNECTED);

    	if (s_retry_num < s_max_retry_num) {
    		// connect failed while smart config or with known credentials doesn't matter
    		esp_wifi_connect();
    		s_retry_num++;
    		ESP_LOGI(__func__, "[0x%03X] retry %d/%d to connect to the AP",uxBits, s_retry_num,s_max_retry_num);

    	} else {
    		// if connecion failed: Start smart config if not running
    		if ( ! smartconfig_running) {
    			xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_CONNECT_WITH_SMART_CONFIG);
    			s_wifi_connect_status = WIFI_TRY_SMART_CONFIG;
    			ESP_LOGI(__func__,"[0x%03X] connect to the AP fail, start smart config", uxBits);

    			esp_wifi_disconnect();
    			esp_smartconfig_stop();

    			esp_wifi_connect();

    			ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    			smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    			ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

    		} else {
    			// too many tries, give up
    			xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_CONNECT_FAILED);
    			ESP_LOGI(__func__,"[0x%03X] connect to the AP fail",uxBits);
    		}
    	}

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_CONNECTED);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(__func__, "[0x%03X] Scan done", uxBits);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(__func__, "[0x%03X] Found channel",uxBits);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(__func__, "[0x%03X] Got SSID and password",uxBits);

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));

        ESP_ERROR_CHECK(store_wifi_config((char *)ssid, (char*)password));

        //ESP_LOGI(__func__, "SSID:%s", ssid);
        //ESP_LOGI(__func__, "PASSWORD:%s", password);

        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(__func__, "RVD_DATA:");
            for (int i=0; i<33; i++) {
                printf("%02x ", rvd_data[i]);
            }
            printf("\n");
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );

        // got the right connection parameter
        // reconnect..
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_CONNECT_FAILED);
    }
}

// ----------------------------------------

/**
 * waits until a final state: connected or not
 */
esp_err_t waitforConnect() {
	EventBits_t uxBits;
	ESP_LOGI(__func__, "started.");
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_BIT_DONE, false, false, portMAX_DELAY);
    	ESP_LOGI(__func__, "xEventGroupWaitBits return %d", uxBits);
       if(uxBits & WIFI_BIT_CONNECTED) {
            ESP_LOGI(__func__, "WiFi Connected to known ap");
            return ESP_OK;
        }
        if(uxBits & WIFI_BIT_CONNECT_FAILED) {
            ESP_LOGI(__func__, "connection failed");
            return ESP_FAIL;
        }
    }

    // not reached
	return ESP_FAIL;
}

/**
 * check status of connect
 */
wifi_status_type wifi_connect_status() {
	return s_wifi_connect_status;
}

char *wifi_connect_status2text(wifi_status_type status)  {
	switch (status) {
	case WIFI_IDLE: return "IDLE";
	case WIFI_TRY_CONNECT: return "TRY_CONNECT";
	case WIFI_TRY_SMART_CONFIG: return "TRY SMART CONFIG";
	case WIFI_CONNECTED: return "CONNECTED";
	case WIFI_CONNECTION_FAILED: return "FAILED";
	default: return "???";
	}
}


void initialise_wifi()
{
	ESP_LOGI(__func__, "started.");

	ESP_ERROR_CHECK(esp_netif_init());
	s_wifi_event_group = xEventGroupCreate();

	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

	xEventGroupClearBits(s_wifi_event_group, WIFI_BITS_ALL);

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

	char ssid[64];
	char pw[64];
	get_wifi_config(ssid, sizeof(ssid), pw, sizeof(pw));

	ESP_LOGI(__func__, "use stored Wifi-Connection '%s'", ssid);

    if ( strlen((char *)ssid)) {
    	// try stored connection
    	wifi_config_t wifi_config;
    	bzero(&wifi_config, sizeof(wifi_config_t));
    	memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    	memcpy(wifi_config.sta.password, pw, sizeof(wifi_config.sta.password));
    	// Setting a password implies station will connect to all security modes including WEP/WPA.
    	// However these modes are deprecated and not advisable to be used. Incase your Access point
    	// doesn't support WPA2, these mode can be enabled by commenting below line
    	wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    } else {
    	// nothing stored, set a marker to start smart config
        xEventGroupSetBits(s_wifi_event_group, WIFI_BIT_NO_CONNECTION_STORED);
    }

    ESP_ERROR_CHECK( esp_wifi_start() );

}

