/*
 * timer_events.c
 *
 * handle timer events
 *
 *  Created on: 24.06.2022
 *      Author: ankrysm
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"


esp_timer_handle_t s_periodic_timer;

static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
}

void init_timer_events(int delta_ms) {
	ESP_LOGI(__func__, "Start, delta=%d ms", delta_ms);

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &s_periodic_timer));

	// start timer, time inm microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_periodic_timer, delta_ms*1000));
}

