/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      13

size_t s_numleds=0;
size_t s_size_led_strip_pixels = 0;
uint8_t *led_strip_pixels=NULL;

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static rmt_transmit_config_t tx_config = {
		.loop_count = 0, // no transfer loop
};


void led_strip_init(uint32_t numleds)
{
    s_numleds= numleds;
	if ( numleds <1 || numleds > 1000) {
		s_numleds = 60;
		ESP_LOGE(__func__,"numleds %d out of rang 1..1000, set to default", numleds);
	}


    ESP_LOGI(__func__, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(__func__, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(__func__, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    s_size_led_strip_pixels = 3 * s_numleds;
    led_strip_pixels = calloc(s_size_led_strip_pixels, sizeof(uint8_t));

    ESP_LOGI(__func__, "LED-Strip with %d Pixel, init done.", s_numleds);
}

void led_strip_refresh() {
	ESP_LOGI(__func__, "Start");
	// Flush RGB values to LEDs
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));

}

void led_strip_firstled(int red, int green, int blue) {
	uint8_t firstled[3];
	//gbr
	firstled[0]=green;
	firstled[1]=blue;
	firstled[2]=red;
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, firstled, sizeof(firstled), &tx_config));

}


