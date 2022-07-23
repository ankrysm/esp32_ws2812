/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"
#include "config.h"
#include "color.h"
*/
#include "esp32_ws2812.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define LED_STRIP_PIN 13

static led_strip_t *gVstrip = NULL;

static uint32_t s_numleds;

int strip_initialized() {
	return gVstrip ? 1 : 0;
}

/**
 * initial init
 */
esp_err_t strip_init(int numleds) {
	if (numleds <1 || numleds>1000) {
		ESP_LOGE(__func__, "%s: numleds %d out of range", __func__, numleds);
		return ESP_FAIL;
	}

	s_numleds = numleds;
	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_STRIP_PIN, RMT_TX_CHANNEL);
	// set counter clock to 40MHz
	config.clk_div = 2;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

	//gVnumleds = numleds; // initial value
	// install ws2812 driver
	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(numleds, (led_strip_dev_t) config.channel);
	gVstrip = led_strip_new_rmt_ws2812(&strip_config);
	if (!gVstrip) {
		ESP_LOGE(__func__, "install WS2812 driver failed");
		return ESP_FAIL;
	}
	// Clear LED strip (turn off all LEDs)
	ESP_ERROR_CHECK(gVstrip->clear(gVstrip, 100));
    ESP_LOGI(__func__, "init done with numleds %d",numleds );

    return ESP_OK;

}

/**
 * sets the color for a pixel range
 */
void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue) {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	if ( start_idx >= s_numleds || end_idx < start_idx || end_idx >= s_numleds) {
		ESP_LOGE(__func__, "%s: idx %d - %d out of range", __func__, start_idx, end_idx);
		return;
	}
	for(int i = start_idx; i <= end_idx; i++) {
		ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, i, red, green, blue));
	    //ESP_LOGI(__func__, "%s: set pixel @%d %d/%d/%d", __func__, i,red,green,blue );
	}
}

void strip_set_color_rgb(uint32_t start_idx, uint32_t end_idx, T_COLOR_RGB *rgb) {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	if ( start_idx >= s_numleds || end_idx < start_idx || end_idx >= s_numleds) {
		ESP_LOGE(__func__, "%s: idx %d - %d out of range", __func__, start_idx, end_idx);
		return;
	}
	for(int i = start_idx; i <= end_idx; i++) {
		ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, i, rgb->r, rgb->g, rgb->b));
	}
}

/**
 * set the color of a single pixel
 */
void strip_set_pixel(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue) {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	if ( idx >= s_numleds) {
		ESP_LOGE(__func__, "%s: idx %d out of range", __func__, idx);
		return;
	}
	ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, idx, red, green, blue));
}

void strip_set_pixel_rgb(uint32_t idx, T_COLOR_RGB *rgb) {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	if ( idx >= s_numleds) {
		ESP_LOGE(__func__, "%s: idx %d out of range", __func__, idx);
		return;
	}
	ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, idx, rgb->r, rgb->g, rgb->b));
}

/**
 * lvl between 0.0 and 1.0
 */
void strip_set_pixel_lvl(uint32_t idx, uint32_t red, uint32_t green, uint32_t blue, double lvl) {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	if ( idx >= s_numleds) {
		ESP_LOGE(__func__, "%s: idx %d out of range", __func__, idx);
		return;
	}
	int32_t r,g,b;
	double f = lvl < 0.0 ? 0.0 : lvl > 1.0 ? 1.0 : lvl;
	r = red * f;
	g = green * f;
	b = blue * f;

	ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, idx, r, g, b));
}

/**
 * clears the strip
 */
void strip_clear()  {
	ESP_ERROR_CHECK(gVstrip->clear(gVstrip, 100));
}

/**
 * flush the pixel buffer to the strip
 */
void strip_show() {
	if (!strip_initialized()) {
		ESP_LOGE(__func__, "%s: not initalized", __func__);
		return;
	}
	ESP_ERROR_CHECK(gVstrip->refresh(gVstrip, 100));
}

/**
 * rotates the strip by one led left or right
 */
void strip_rotate(int32_t dir)  {
	ESP_ERROR_CHECK(gVstrip->rotate(gVstrip, dir));
}

/**
 * sets the color of the first led
 */
void firstled(int red, int green, int blue) {
	int pos = 0;
	strip_set_color(pos, pos, red, green, blue);
	strip_show();
}

uint32_t strip_get_numleds() {
	return s_numleds;
}

