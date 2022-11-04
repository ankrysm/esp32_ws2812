/*
 * esp32_ws2812_basic.h
 *
 *  Created on: 23.07.2022
 *      Author: ankrysm
 */

#ifndef MAIN_ESP32_WS2812_BASIC_H_
#define MAIN_ESP32_WS2812_BASIC_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_http_server.h"
#include "esp_vfs.h"
#include "esp_random.h"
#include "cJSON.h"
#include "esp_chip_info.h"
#include "driver/gpio.h"
//#include "driver/rmt.h"
#include "esp_system.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_random.h"
#include "bootloader_random.h"


//#include "mdns.h"
#include "esp_smartconfig.h"
//#include "mdns.h"
#include "lwip/apps/netbiosns.h"

#include "time_sync.h"
#include "bmp.h"
#include "https_get.h"

#define LEN_PATH_MAX (ESP_VFS_PATH_MAX+128)
#define N_TRACKS 16

#endif /* MAIN_ESP32_WS2812_BASIC_H_ */
