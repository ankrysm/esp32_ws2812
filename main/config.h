/*
 * config.h
 *
 *  Created on: 12.06.2022
 *      Author: andreas
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define STORAGE_NAMESPACE "storage"
#define STORAGE_KEY_CONFIG "config"
#define STORAGE_KEY_WIFI_CONFIG_SSID "wifi_ssid"
#define STORAGE_KEY_WIFI_CONFIG_PW "wifi_pw"


// flag values
#define CFG_AUTOPLAY 0x0001
#define CFG_REPEAT   0x0002

#define LEN_SCENEFILE 32

typedef struct {
	uint32_t flags;
	uint32_t numleds;
	uint32_t cycle; // Timer cycle in ms
	char scenefile[LEN_SCENEFILE];
} T_CONFIG;

typedef struct {
	char *ssid;
	char *pw;
} T_WIFI_CONFIG;

// prototypes
esp_err_t store_config();
esp_err_t store_wifi_config(char *ssid, char *pw);
esp_err_t init_storage();
char *config2txt(char *txt, size_t sz);

#endif /* CONFIG_H_ */
