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
#include "common_util.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      13

static size_t s_numleds=0;
static size_t s_size_led_strip_pixels = 0;
static uint8_t *led_strip_pixels=NULL;

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
        .mem_block_symbols = 512, //64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 1, // 4, // set the number of transactions that can be pending in the background
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

    ESP_LOGI(__func__, "LED-strip with %d pixel, init done.", s_numleds);
}

void led_strip_refresh() {
	//ESP_LOGI(__func__, "Start");
	// Flush RGB values to LEDs
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, s_size_led_strip_pixels, &tx_config));
	esp_err_t ret;
	if ( (ret=rmt_tx_wait_all_done(led_chan, 100)) != ESP_OK){
		ESP_LOGE(__func__, "rmt_tx_wait_all_done failed %d", ret);
	}
}

void led_strip_firstled(int red, int green, int blue) {
	uint8_t firstled[3];
	//gbr
	firstled[0]=green & 0xFF;
	firstled[1]=red & 0xFF;
	firstled[2]=blue & 0xFF;
	ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, firstled, sizeof(firstled), &tx_config));

}

void led_strip_clear() {
	memset(led_strip_pixels, 0, s_size_led_strip_pixels);
}

void led_strip_set_pixel(int32_t idx, uint8_t r, uint8_t g, uint8_t b) {
	if ( idx < 0 || idx >= s_numleds) {
		return;
	}

	uint32_t pos = 3 * idx;
	led_strip_pixels[pos++] = g;
	led_strip_pixels[pos++] = r;
	led_strip_pixels[pos]   = b;

}

/**
 * copies 'nbuf' bytes from 'buf' to led strip memory at position 'idx'
 */
void led_strip_memcpy(int32_t idx, uint8_t *buf, uint32_t nbuf) {

	uint32_t pos = 3 * idx;
	if ( pos + nbuf >  s_numleds * 3) {
		nbuf = s_numleds * 3 - pos;
	}
	memcpy(&(led_strip_pixels[pos]), buf, nbuf);
}

bool is_led_strip_initialized() {
	return led_strip_pixels ? true : false;
}

size_t get_numleds() {
	return s_numleds;
}

esp_err_t set_numleds(uint32_t numleds) {
	if ( numleds < 1 || numleds > 1000) {
		ESP_LOGE(__func__, "new numleds (%d) out of range 1 .. 999", numleds);
		return ESP_FAIL;
	}
	if ( led_strip_pixels)
		free(led_strip_pixels);

	s_numleds = numleds;
    s_size_led_strip_pixels = 3 * s_numleds;
    led_strip_pixels = calloc(s_size_led_strip_pixels, sizeof(uint8_t));
    if ( !led_strip_pixels) {
    	ESP_LOGE(__func__,"could not allocate %d bytes", s_size_led_strip_pixels);
    	return ESP_FAIL;
    }
    ESP_LOGI(__func__, "LED-strip with new size %d pixel", s_numleds);

	return ESP_OK;
}

uint32_t get_led_strip_data_hash() {
	return crc32b(led_strip_pixels, s_size_led_strip_pixels);
}

/**
 * pseudo graphics
 */
void led_strip_demo(char *msg){
	char txt[1024];
	uint32_t pos=0;
	snprintf(txt,sizeof(txt),"#### LED(%s):<", msg?msg:"");
	for (int i=0; i<get_numleds(); i++) {
		uint32_t g= led_strip_pixels[pos++]; // g
		uint32_t r= led_strip_pixels[pos++]; // r
		uint32_t b= led_strip_pixels[pos++]; //b
		uint32_t s = g+r+b;
		if ( s>0) {
			if (g>r && g>b)
				strlcat(txt,"G",sizeof(txt));
			else if(r>g && r>b)
				strlcat(txt,"R",sizeof(txt));
			else if(b>r && b>g)
				strlcat(txt,"B",sizeof(txt));
			else strlcat(txt,"X",sizeof(txt));
		} else {
			strlcat(txt,".",sizeof(txt));
		}
	}
	strlcat(txt,">",sizeof(txt));
	ESP_LOGI(__func__, "%s", txt);
}


