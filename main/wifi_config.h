/*
 * wifi_config.h
 *
 *  Created on: 21.06.2022
 *      Author: ankrysm
 */

#ifndef MAIN_WIFI_CONFIG_H_
#define MAIN_WIFI_CONFIG_H_

#define STORAGE_WIFI_NAMESPACE "wifi"
#define STORAGE_WIFI_KEY_SSID "wifi_ssid"
#define STORAGE_WIFI_KEY_PW "wifi_pw"

typedef enum {
	WIFI_IDLE, // wifi connection will be initiated
	WIFI_TRY_CONNECT, // try to connect with known credentials
	WIFI_TRY_SMART_CONFIG, // try connect with smartconfig
	WIFI_CONNECTED,// wifi connect to AP successful
	WIFI_CONNECTION_FAILED, // connect failed
} wifi_status_type;

void initialise_wifi();
esp_err_t waitforConnect();
wifi_status_type wifi_connect_status();
char *wifi_connect_status2text(wifi_status_type status);


#endif /* MAIN_WIFI_CONFIG_H_ */
