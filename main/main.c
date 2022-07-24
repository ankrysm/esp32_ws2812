/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp32_ws2812.h"


int main_flags=0;

extern T_CONFIG gConfig;

void firstled(int red, int green, int blue) ;

void app_main() {

	// init storage and get/initalize config
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(init_storage());

	// init led-strip
	ESP_ERROR_CHECK(strip_init(gConfig.numleds));


	firstled(32, 32, 32);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	initialise_mdns();
	initialise_netbios();

	initialise_wifi();
	// esp_err_t res =	waitforConnect();

	TickType_t xDelay = 500 / portTICK_PERIOD_MS;

	wifi_status_type done = 0;
	while(done==0) {
		//printf("xxx\n");
		vTaskDelay(xDelay);
		wifi_status_type s = wifi_connect_status();
		ESP_LOGI(__func__, "connection status=%d(%s)", s, wifi_connect_status2text(s));
		switch (s) {
		case WIFI_IDLE:
			firstled(16,16,16);
			break;
		case WIFI_TRY_CONNECT:
			firstled(16,16,0);
			break;
		case WIFI_TRY_SMART_CONFIG:
			firstled(0,0,16);
			break;
		case WIFI_CONNECTED:
			firstled(0,16,0);
			done=s;
			break;
		case WIFI_CONNECTION_FAILED:
			firstled(16,0,0);
			done=s;
			break;
		default:
			done = 99;
		}
	}


	if ( done == WIFI_CONNECTED ) {
		init_restservice();
		gConfig.flags |=CFG_WITH_WIFI;
		// green
		firstled(0,32,0);
		ESP_LOGI(__func__, "with WIFI");
	} else {
		gConfig.flags &= ~CFG_WITH_WIFI;
		// red
		firstled(32,0,0);
		ESP_LOGI(__func__, "without WIFI");
	}
	char buf[128];
	config2txt(buf, sizeof(buf));
	ESP_LOGI(__func__, "config=%s",buf);

	// start timer
	init_timer_events(50); // TODO config time

	//start_demo1();
	T_COLOR_RGB fg_color ={32,0,0};
	build_demo2(
			 &fg_color // foreground color
	) ;


	xDelay = 50000 / portTICK_PERIOD_MS;

	while(1) {
		//printf("xxx\n");
		vTaskDelay(xDelay);
	}
}
