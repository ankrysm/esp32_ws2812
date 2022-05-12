/* RMT example -- RGB LED Strip

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"

static const char *TAG = "ledstrip";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define LED_STRIP_PIN 13

#define EXAMPLE_CHASE_SPEED_MS (200)

static int gVnumleds = 0;
static led_strip_t *gVstrip = NULL;

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}



int strip_initialized() {
	return gVstrip ? 1 : 0;
}

int strip_numleds() {
	return gVnumleds;
}

/**
 * initial init
 */
void strip_init(int numleds) {
	if (numleds <1 || numleds>1000) {
		ESP_LOGE(TAG, "%s: numleds %d out of range", __func__, numleds);

	}

	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_STRIP_PIN, RMT_TX_CHANNEL);
	// set counter clock to 40MHz
	config.clk_div = 2;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

	gVnumleds = numleds; // initial value
	// install ws2812 driver
	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(gVnumleds, (led_strip_dev_t) config.channel);
	gVstrip = led_strip_new_rmt_ws2812(&strip_config);
	if (!gVstrip) {
		ESP_LOGE(TAG, "%s: install WS2812 driver failed", __func__);
	}
	// Clear LED strip (turn off all LEDs)
	ESP_ERROR_CHECK(gVstrip->clear(gVstrip, 100));
    ESP_LOGI(TAG, "%s: init done with numleds %d", __func__, gVnumleds );

}

/**
 * change numleds
 */
void strip_setup(int numleds) {
	// TODO later, no init only
	if ( numleds == gVnumleds) {
	    ESP_LOGI(TAG, "%s: new numleds equals old numleds %d", __func__, numleds );
	    return;
	}

	if ( ! gVstrip) {
		strip_init(numleds);
	    ESP_LOGI(TAG, "%s: setup done with numleds %d", __func__, gVnumleds );
	} else {
	    ESP_LOGE(TAG, "%s: NYI setup with numleds %d", __func__, numleds );
	}
}

void strip_set_color(uint32_t start_idx, uint32_t end_idx, uint32_t red, uint32_t green, uint32_t blue) {
	if (!strip_initialized()) {
		ESP_LOGE(TAG, "%s: not initalized", __func__);
		return;
	}
	if ( start_idx >= gVnumleds || end_idx < start_idx || end_idx >= gVnumleds) {
		ESP_LOGE(TAG, "%s: idx %d - %d out of range", __func__, start_idx, end_idx);
		return;
	}
	for(int i = start_idx; i <= end_idx; i++) {
		ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, i, red, green, blue));
	    //ESP_LOGI(TAG, "%s: set pixel @%d %d/%d/%d", __func__, i,red,green,blue );
	}
}

void strip_clear()  {
	ESP_ERROR_CHECK(gVstrip->clear(gVstrip, 100));
}

void strip_show() {
	if (!strip_initialized()) {
		ESP_LOGE(TAG, "%s: not initalized", __func__);
		return;
	}
	ESP_ERROR_CHECK(gVstrip->refresh(gVstrip, 100));
}

void strip_rotate(int32_t dir)  {
	ESP_ERROR_CHECK(gVstrip->rotate(gVstrip, dir));
}


void led_strip_main(void)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint16_t hue = 0;
	uint16_t start_rgb = 0;

	//   int derpin=13;
	int numleds=12;

	strip_setup(numleds);

	//    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(derpin, //CONFIG_EXAMPLE_RMT_TX_GPIO,
	//    		RMT_TX_CHANNEL);
	//    // set counter clock to 40MHz
	//    config.clk_div = 2;
	//
	//    ESP_ERROR_CHECK(rmt_config(&config));
	//    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
	//
	//    // install ws2812 driver
	//    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(numleds //CONFIG_EXAMPLE_STRIP_LED_NUMBER
	//    		, (led_strip_dev_t)config.channel);
	//    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
	//    if (!strip) {
	//        ESP_LOGE(TAG, "install WS2812 driver failed");
	//    }
	//    // Clear LED strip (turn off all LEDs)
	//    ESP_ERROR_CHECK(strip->clear(strip, 100));
	// Show simple rainbow chasing pattern

	ESP_LOGI(TAG, "LED Rainbow Chase Start");
	int base=0;
	while (true) {
		for (int j = 0; j < numleds; j++) {
			if ( (j+base) % 3 == 0) {
				// Build RGB values
				hue = (j+base) * 360 / numleds + start_rgb;
				led_strip_hsv2rgb(hue, 100, 50, &red, &green, &blue);
			} else {
				led_strip_hsv2rgb(hue, 100, 10, &red, &green, &blue);
			}
			// Write RGB values to strip driver
			strip_set_color(j, j, red, green, blue);
			//ESP_ERROR_CHECK(gVstrip->set_pixel(gVstrip, j, red, green, blue));
		}
		base = (base+1) % numleds;

		// Flush RGB values to LEDs
		strip_show();
		//ESP_ERROR_CHECK(gVstrip->refresh(gVstrip, 100));

		vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
		//strip->clear(strip, 50);
		//vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
		start_rgb += 60;
	}
}
