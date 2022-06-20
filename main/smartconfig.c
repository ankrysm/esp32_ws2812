/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
//#include "config.h"

void firstled(int red, int green, int blue);

#define STORAGE_WIFI_NAMESPACE "wifi"
#define STORAGE_KEY_WIFI_CONFIG_SSID "wifi_ssid"
#define STORAGE_KEY_WIFI_CONFIG_PW "wifi_pw"


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int WIFI_CONNECTED_BIT = BIT0; // connection successfully
static const int ESPTOUCH_DONE_BIT = BIT1; // end of smart config
static const int WIFI_FAIL_BIT = BIT2; // connection to configured access point failed
static const int WIFI_SMART_CONFIG_RUNNING = BIT3;
static const int WIFI_NO_CONNECTION_STORED = BIT4;

static const int ALL_WIFI_BITS = WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT | WIFI_FAIL_BIT | WIFI_SMART_CONFIG_RUNNING |WIFI_NO_CONNECTION_STORED;

static int s_retry_num = 0;
static int s_max_retry_num = 10;



static esp_err_t store_wifi_config(char *ssid, char *pw) {
	nvs_handle_t my_handle;

    ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'",(ssid?ssid:"<null>"),(pw?pw:"<null>") );

	esp_err_t ret = nvs_open(STORAGE_WIFI_NAMESPACE, NVS_READWRITE, &my_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_open() failed (%s)", esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_set_str(my_handle, STORAGE_KEY_WIFI_CONFIG_SSID , ssid ? ssid : "");
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_str(%s) failed (%s)", STORAGE_KEY_WIFI_CONFIG_SSID, esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_set_str(my_handle, STORAGE_KEY_WIFI_CONFIG_PW , pw ? pw : "");
	if (ret != ESP_OK) {
		ESP_LOGE(__func__, "nvs_set_str(%s) failed (%s)", STORAGE_KEY_WIFI_CONFIG_PW, esp_err_to_name(ret));
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
	char *keyname=STORAGE_KEY_WIFI_CONFIG_SSID;
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

	keyname=STORAGE_KEY_WIFI_CONFIG_PW;
	size = sz_pw;
	ret = nvs_get_str(my_handle, keyname, NULL, &size);
	if ( ret == ESP_OK ) {
		// known
		nvs_get_str(my_handle, keyname, pw, &size);
		ESP_LOGI(__func__, "getting '%s' successful sz=%d: '%s'",keyname, size, pw );

	} else if (ret == ESP_ERR_NVS_NOT_FOUND) {
		// nothing stored, store an emtpy string
		ESP_LOGI(__func__, "nothing stored for '%s'", keyname);
		pw = strdup("");
	}

	ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'", ssid, pw );

	// close handle immediately, if it's necessary to open it again, it will be done later
	nvs_close(my_handle);
	return ESP_OK;

}


/**
 * ##########################
 * smart config functions
 * ##########################
 */


// Background task for smart config, waiting for connection
static void smartconfig_task(void * parm)
{
	ESP_LOGI(__func__, "started.");
	EventBits_t uxBits;
	while (1) {
		uxBits = xEventGroupWaitBits(s_wifi_event_group, ALL_WIFI_BITS, true, false, portMAX_DELAY);
		ESP_LOGI(__func__, "uxBits = 0x%x", uxBits);

		if(uxBits & WIFI_CONNECTED_BIT) {
			ESP_LOGI(__func__, "WiFi Connected to ap");
		}
		if(uxBits & WIFI_FAIL_BIT) {
			ESP_LOGE(__func__, "WiFi Connect to ap failed");
		}
		if(uxBits & ESPTOUCH_DONE_BIT) {
			ESP_LOGI(__func__, "smartconfig over");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
}

static void smartconfig_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
	EventBits_t uxBits = xEventGroupGetBits(s_wifi_event_group);
	int smartconfig_running = uxBits & WIFI_SMART_CONFIG_RUNNING;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {

        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);

        if (uxBits & WIFI_NO_CONNECTION_STORED ) {
			firstled(16,16,0); // yellow
			xEventGroupSetBits(s_wifi_event_group, WIFI_SMART_CONFIG_RUNNING);

        	ESP_LOGI(__func__,"[0x%03X] start smart config",uxBits);
        	ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
        	smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        	ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
        } else {
        	ESP_LOGI(__func__,"[0x%03X] start connect with stored data", uxBits);
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

    	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    	if (s_retry_num < s_max_retry_num) {
    		// connect while smart config or real config
    		esp_wifi_connect();
    		s_retry_num++;
    		ESP_LOGI(__func__, "[0x%03X] retry %d/%d to connect to the AP",uxBits, s_retry_num,s_max_retry_num);
    	} else {
    		// if connecion failed: Start smart config
    		if ( ! smartconfig_running) {
    			xEventGroupSetBits(s_wifi_event_group, WIFI_SMART_CONFIG_RUNNING);
    			firstled(0,0,16); // blue

    			ESP_LOGI(__func__,"[0x%03X] connect to the AP fail, start smart config",uxBits);
    			esp_wifi_disconnect();
    			esp_smartconfig_stop();

    			esp_wifi_connect();

    			ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    			smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    			ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

    		} else {
    			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    			ESP_LOGI(__func__,"[0x%03X] connect to the AP fail",uxBits);
    			firstled(16,0,0); // red
    		}
    	}



    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    	int bits = WIFI_CONNECTED_BIT;
        if ( smartconfig_running) {
        	bits |=ESPTOUCH_DONE_BIT;
        }
        xEventGroupSetBits(s_wifi_event_group, bits);
		firstled(0,16,0); // green

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
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

// ----------------------------------------

/**
 * ###############################
 * normal wifi connect functions
 * ###############################
 */
/*
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < s_max_retry_num) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(__func__, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(__func__,"connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(__func__, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
*/


static esp_err_t waitforConnect() {
	EventBits_t uxBits;
	ESP_LOGI(__func__, "started.");
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, true, false, portMAX_DELAY);
    	ESP_LOGI(__func__, "xEventGroupWaitBits return %d", uxBits);
       if(uxBits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(__func__, "WiFi Connected to known ap");
            return ESP_OK;
        }
        if(uxBits & WIFI_FAIL_BIT) {
            ESP_LOGI(__func__, "connection failed over");
            return ESP_FAIL;
        }
    }

    // not reached
	return ESP_FAIL; //isConnected ? ESP_OK : ESP_FAIL;
}


/*
static esp_err_t initialise_know_wifi() {

	char ssid[64];
	char pw[64];
	get_wifi_config(ssid, sizeof(ssid), pw, sizeof(pw));

	if ( !strlen((char *)ssid)) {
	    ESP_LOGI(__func__, "no stored Wifi-Connection.");
		return ESP_ERR_NOT_FOUND;
	}

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | ESPTOUCH_DONE_BIT);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

//    wifi_config_t wifi_config = {
//        .sta = {
//            .ssid = gWifiConfig.ssid, // TODO  from storage
//            .password = gWifiConfig.pw, // TODO from storage
//            // Setting a password implies station will connect to all security modes including WEP/WPA.
//            // However these modes are deprecated and not advisable to be used. Incase your Access point
//            // doesn't support WPA2, these mode can be enabled by commenting below line
//			.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
//        },
//    };

    ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'",(ssid?ssid:"<null>"),(pw?pw:"<null>") );

    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, gWifiConfig.ssid, sizeof(wifi_config.sta.ssid));
    if (gWifiConfig.pw) {
    	memcpy(wifi_config.sta.password, gWifiConfig.pw, sizeof(wifi_config.sta.password));
    }


    // Setting a password implies station will connect to all security modes including WEP/WPA.
    // However these modes are deprecated and not advisable to be used. Incase your Access point
    // doesn't support WPA2, these mode can be enabled by commenting below line
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(__func__, "started.");

    return waitforConnect();
}
*/


static esp_err_t initialise_smartconfig_wifi() {
	firstled(16,16,16);
	ESP_LOGI(__func__, "started.");
	char ssid[64];
	char pw[64];
	get_wifi_config(ssid, sizeof(ssid), pw, sizeof(pw));
    ESP_LOGI(__func__, "stored Wifi-Connection '%s', '%s'", ssid, pw);



	xEventGroupClearBits(s_wifi_event_group, ALL_WIFI_BITS);

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );



    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    if ( strlen((char *)ssid)) {
    	ESP_LOGI(__func__, "activate stored Wifi-Connection.");

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
        xEventGroupSetBits(s_wifi_event_group, WIFI_NO_CONNECTION_STORED);
    }

    ESP_ERROR_CHECK( esp_wifi_start() );

    return waitforConnect();

}


esp_err_t initialise_wifi()
{
	ESP_LOGI(__func__, "started.");

	ESP_ERROR_CHECK(esp_netif_init());
	s_wifi_event_group = xEventGroupCreate();
	//ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

	// try to connect to known WLAN
	// if it failes, start smart config

	esp_err_t res; // = initialise_know_wifi();
//	if (res == ESP_OK) {
//		ESP_LOGI(__func__, "connected to known Wifi.");
//		return res;
//	}
//
//	if ( res == ESP_FAIL ) {
//		// not able to connect with stored data
//		ESP_LOGI(__func__, "wifi stop");
//		esp_wifi_stop();
//	}

	ESP_LOGI(__func__, "try smart config");
	res =  initialise_smartconfig_wifi();
	if (res == ESP_OK) {
		ESP_LOGI(__func__, "smart config successfull.");
		return res;
	}
	ESP_LOGI(__func__, "smart config failed.");
	return res;


	//    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );
	//    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler, NULL) );
	//    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );
	//
	//    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	//    ESP_ERROR_CHECK( esp_wifi_start() );
}


//void app_main(void)
//{
//    ESP_ERROR_CHECK( nvs_flash_init() );
//    initialise_wifi();
//}
