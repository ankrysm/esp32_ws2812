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

void initialize_sntp(void)
{
    ESP_LOGI(__func__, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "time.windows.com");
    sntp_setservername(0, "pool.ntp.org");
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
    const int retry_count = 30;
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
    return ESP_OK;
}


esp_err_t init_time_service() {
    initialize_sntp();
    if (obtain_time() != ESP_OK) {
        return ESP_FAIL;
    }

	esp_err_t res;
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
	time_t now;
	time(&now);
	get_time4(now, tbuf, sz_tbuf);
	ESP_LOGI(__func__, "The current date/time is: %s", tbuf);
}

void get_time4(time_t now, char *tbuf, size_t sz_tbuf) {
	struct tm timeinfo;
	char tformat[64];
	char *tz = getenv("TZ");
	snprintf(tformat, sizeof(tformat),"%s/%%A, %%F %%T", tz?tz:"not set");
	localtime_r(&now, &timeinfo);
	strftime(tbuf, sz_tbuf, tformat, &timeinfo);
}

void get_shorttime4(time_t now, char *tbuf, size_t sz_tbuf) {
	struct tm timeinfo;
	localtime_r(&now, &timeinfo);
	strftime(tbuf, sz_tbuf, "%F %T", &timeinfo);
}

/*
 * examples
 *  Berlin: CET-1CEST,M3.5.0,M10.5.0/3
 *  Japan: JST-9
 *
 */
void set_timezone(char *tz) {

	if ( tz == NULL|| !strlen(tz)) {
		ESP_LOGW(__func__, "no time zone specified");
		return;
	}
	ESP_LOGI(__func__, "new tz is '%s'", tz);
    setenv("TZ", tz, 1);
    tzset();

    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(__func__, "The current date/time for '%s' is: %s", tz, strftime_buf);

}
