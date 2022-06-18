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
#include "config.h"


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int WIFI_CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1; // end of smart config
static const int WIFI_FAIL_BIT = BIT2;

//static const char *__func__ = "smartconfig";

static int s_retry_num = 0;
static int s_max_retry_num = 10;

//int isConnected = 0;
//int isConnectFail=0;


/**
 * ##########################
 * smart config functions
 * ##########################
 */


// Background task for smart config, waiting for connection
//static void smartconfig_task(void * parm)
//{
//    EventBits_t uxBits;
//    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
//    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
//    while (1) {
//        uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
//        if(uxBits & WIFI_CONNECTED_BIT) {
//            ESP_LOGI(__func__, "WiFi Connected to ap");
//        }
//        if(uxBits & ESPTOUCH_DONE_BIT) {
//            ESP_LOGI(__func__, "smartconfig over");
//            esp_smartconfig_stop();
//            vTaskDelete(NULL);
//        }
//    }
//}

static void smartconfig_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
 //       xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
        ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(__func__, "Scan done");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(__func__, "Found channel");

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(__func__, "Got SSID and password");

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

        // got the right connection parameter TODO store it
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
//            isConnectFail = 1;
//            isConnected = 0;
        }
        ESP_LOGI(__func__,"connect to the AP fail");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(__func__, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
//        isConnected = 1;
//        isConnectFail = 0;
    }
}

static esp_err_t waitforConnect() {
	EventBits_t uxBits;
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, true, false, portMAX_DELAY);
        if(uxBits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(__func__, "WiFi Connected to known ap");
            return ESP_OK;
        }
        if(uxBits & WIFI_FAIL_BIT) {
            ESP_LOGI(__func__, "connection failed over");
            return ESP_FAIL;
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(__func__, "end of smart config");
            return ESP_FAIL;
        }
    }

//	const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
//
//	while(!isConnected && !isConnectFail) {
//		ESP_LOGI(__func__,"try connect to the AP %d/%d/%d", s_retry_num, isConnected, isConnectFail);
//		vTaskDelay(xDelay);
//	}

    // not reached
	return ESP_FAIL; //isConnected ? ESP_OK : ESP_FAIL;
}

static esp_err_t initialise_know_wifi() {

	extern T_WIFI_CONFIG gWifiConfig;

	if ( !gWifiConfig.ssid || !strlen((char *)gWifiConfig.ssid)) {
	    ESP_LOGI(__func__, "no stored Wifi-Connection.");
		return ESP_FAIL;
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

    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, gWifiConfig.ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, gWifiConfig.pw, sizeof(wifi_config.sta.password));

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



static esp_err_t initialise_smartconfig_wifi() {
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | ESPTOUCH_DONE_BIT);

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smartconfig_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
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

	esp_err_t res = initialise_know_wifi();
	if (res == ESP_OK) {
		ESP_LOGI(__func__, "connected to known Wifi.");
		return res;
	}

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
