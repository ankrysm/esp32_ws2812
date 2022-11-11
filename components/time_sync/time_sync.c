/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "time_sync.h"

/* Timer interval once every day (24 Hours) */
#define TIME_PERIOD (86400000000ULL)

static char *s_storage_namespace=NULL; // "storage"

void initialize_sntp(void)
{
    ESP_LOGI(__func__, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.windows.com");
    sntp_setservername(1, "pool.ntp.org");
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

static esp_err_t obtain_time(void)
{
    /**
     * NTP server address could be aquired via DHCP,
     * see LWIP_DHCP_GET_NTP_SRV menuconfig option
     */
#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);
#endif

    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(__func__, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    if (retry == retry_count) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t fetch_and_store_time_in_nvs(void *args)
{
    initialize_sntp();
    if (obtain_time() != ESP_OK) {
        return ESP_FAIL;
    }

    nvs_handle_t my_handle;
    esp_err_t err;

    time_t now;
    time(&now);

    //Open
    err = nvs_open(s_storage_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        goto exit;
    }

    //Write
    err = nvs_set_i64(my_handle, "timestamp", now);
    if (err != ESP_OK) {
        goto exit;
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        goto exit;
    }

    nvs_close(my_handle);
    sntp_stop();

exit:
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Error updating time in nvs");
    } else {
        ESP_LOGI(__func__, "Updated time in NVS");
    }
    return err;
}

esp_err_t update_time_from_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open(s_storage_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(__func__, "Error opening NVS");
        goto exit;
    }

    int64_t timestamp = 0;

    err = nvs_get_i64(my_handle, "timestamp", &timestamp);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(__func__, "Time not found in NVS. Syncing time from SNTP server.");
        if (fetch_and_store_time_in_nvs(NULL) != ESP_OK) {
            err = ESP_FAIL;
        } else {
            err = ESP_OK;
        }
    } else if (err == ESP_OK) {
        struct timeval get_nvs_time;
        get_nvs_time.tv_sec = timestamp;
        settimeofday(&get_nvs_time, NULL);
    }

exit:
    nvs_close(my_handle);
    return err;
}


esp_err_t init_time_service(char *storage_namespace) {

	s_storage_namespace = strdup(storage_namespace);

	esp_err_t res;
	if (esp_reset_reason() == ESP_RST_POWERON) {
		ESP_LOGI(__func__, "Updating time from NVS");
		if ( (res=update_time_from_nvs()) != ESP_OK) {
			ESP_LOGE(__func__, "Updating time from NVS failed");
			return res;
		}
	}

	const esp_timer_create_args_t nvs_update_timer_args = {
			.callback = (void *)&fetch_and_store_time_in_nvs,
	};

	esp_timer_handle_t nvs_update_timer;
	if ( (res = esp_timer_create(&nvs_update_timer_args, &nvs_update_timer))!= ESP_OK) {
		ESP_LOGE(__func__, "esp_timer_create failed");
		return res;
	}
	if ((res = esp_timer_start_periodic(nvs_update_timer, TIME_PERIOD)) != ESP_OK) {
		ESP_LOGE(__func__, "esp_timer_start_periodic failed");
		return res;
	}
	ESP_LOGI(__func__,"time service established");
	return ESP_OK;
}

void get_current_timestamp(char *tbuf, size_t sz_tbuf) {
	char tformat[32];

	struct timeval tv;
    struct tm timeinfo = { 0 };

	gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);
	snprintf(tformat, sizeof(tformat), "%%Y-%%m-%%d %%H:%%M:%%S.%06ld", tv.tv_usec);
	strftime(tbuf, sz_tbuf, tformat, &timeinfo);
}

/*
 * examples
 *  Berlin: CET-1CEST,M3.5.0,M10.5.0/3
 *  Japan: JST
 *
 */
void set_timezone(char *tz) {
    setenv("TZ", tz, 1);
    tzset();
}
