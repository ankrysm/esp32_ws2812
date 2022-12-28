/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
//#define MAIN
#include "esp32_ws2812.h"


int main_flags=0;

extern uint32_t cfg_trans_flags;
extern uint32_t cfg_numleds;
extern uint32_t cfg_cycle;
extern char *cfg_timezone;

// from config.c
extern char sha256_hash_boot_partition[];
extern char sha256_hash_run_partition[];

void firstled(int red, int green, int blue) ;


void some_useful_informations() {
	ESP_LOGI(__func__, "LEN_PATH_MAX=%d", LEN_PATH_MAX);
	ESP_LOGI(__func__,"sizeof int=%u, int32_t=%u, int64_t=%u, float=%u, double=%u, time_t=%u",
			sizeof(int), sizeof(int32_t), sizeof(int64_t), sizeof(float), sizeof(double), sizeof(time_t));
}

void app_main() {

	init_logging(ESP_LOG_VERBOSE);

	global_data_init();

	some_useful_informations();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//get_random(1,1000);

	// init storage and get/initalize config
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(init_storage());
	ESP_ERROR_CHECK(load_config());

	// init led-strip
	led_strip_init(cfg_numleds);
	strip_clear();
	strip_show(true);
	firstled(16, 16, 16);

	bmp_init();
	// log config
	char buf[256];
	config2txt(buf, sizeof(buf));
	ESP_LOGI(__func__, "config=%s",buf);

	// init timer
	init_timer_events();
	set_event_timer_period(cfg_cycle);


	// initialize networking
	TickType_t xDelay = 500 / portTICK_PERIOD_MS;

	ESP_ERROR_CHECK(esp_netif_init());

	initialise_mdns();
	initialise_netbios();

	initialise_wifi();



	xDelay = 500 / portTICK_PERIOD_MS;

	wifi_status_type done_with_status = 0;
	while(done_with_status==0) {
		vTaskDelay(xDelay);
		wifi_status_type s = wifi_connect_status();
		ESP_LOGI(__func__, "connection status=%d(%s)", s, wifi_connect_status2text(s));
		switch (s) {
		case WIFI_IDLE:
			firstled(16,16,16); // white
			break;
		case WIFI_TRY_CONNECT:
			firstled(16,16,0); // yellow
			break;
		case WIFI_TRY_SMART_CONFIG:
			firstled(0,0,16); // blue
			break;
		case WIFI_CONNECTED:
			firstled(0,16,0); // green
			done_with_status=s;
			break;
		case WIFI_CONNECTION_FAILED:
			firstled(16,0,0); // red
			done_with_status = s;
			break;
		default:
			done_with_status = 99;
		}
	}

	if ( done_with_status == WIFI_CONNECTED ) {
		// init time service needed for https requests
	    if (init_time_service() != ESP_OK) {
	    	log_err(__func__, "could not initialise internet time service");
	    }
		init_restservice();
		cfg_trans_flags |=CFG_WITH_WIFI;
		// green
		firstled(0,32,0);
		ESP_LOGI(__func__, "with WIFI");
	} else {
		cfg_trans_flags &= ~CFG_WITH_WIFI;
		// red
		firstled(32,0,0);
		ESP_LOGI(__func__, "without WIFI");
	}

	get_sha256_partition_hashes();

	// load autostart file if specified
	load_autostart_file();

	// if configured and possible start the autostart scene
	scenes_autostart();

	// main loop
    set_timezone(cfg_timezone);
    log_current_time();

	log_info(__func__, "main started");
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());

	xDelay = 50000 / portTICK_PERIOD_MS;
	while(1) {
		vTaskDelay(xDelay);
	}
}
