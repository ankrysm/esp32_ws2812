/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
//#define MAIN
#include "esp32_ws2812.h"


int main_flags=0;


//uint64_t timmi_dt =22;

//extern T_CONFIG gConfig;
//extern uint32_t cfg_flags;
extern uint32_t cfg_trans_flags;
extern uint32_t cfg_numleds;
extern uint32_t cfg_cycle;
//extern char *cfg_autoplayfile;

void firstled(int red, int green, int blue) ;

//static uint64_t timtim=0;
//extern T_EVENT *s_event_list;

/*
void timmi() {
	if (obtain_eventlist_lock() != ESP_OK) {
		ESP_LOGE(__func__, "couldn't get lock on eventlist");
		return;
	}
	strip_clear();

	if ( !s_event_list) {
		firstled(16,0,16);
		release_eventlist_lock();
		timtim=0;
		return;
	}

	/// play scenes
	timtim += timmi_dt;
	int n_paint=0;
	// first check: is something to paint?
	for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
		// first: move
		process_move_events(evt,timmi_dt);
		if ( evt->isdirty) {
			n_paint++;
			evt->isdirty=0;
		}
		// next: time events
		// TODO
	}

	if ( n_paint > 0) {
		// i have something to paint
		for ( T_EVENT *evt= s_event_list; evt; evt = evt->nxt) {
			process_loc_event(evt);
		}
		//ESP_LOGI(__func__, "strip_show");
		strip_show();
	}
	release_eventlist_lock();
}
*/

/*
void timmi_task(void *para) {
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	TickType_t xDelay = 50 / portTICK_PERIOD_MS;

	while(1) {
		//printf("xxx\n");
		timmi();
		vTaskDelay(xDelay);
	}

}
*/

void some_useful_informations() {
	ESP_LOGI(__func__, "LEN_PATH_MAX=%d", LEN_PATH_MAX);
	ESP_LOGI(__func__,"sizeof int=%u, int32_t=%u, int64_t=%u, float=%u, double=%u", sizeof(int), sizeof(int32_t), sizeof(int64_t), sizeof(float), sizeof(double));

}

void app_main() {

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
	    ESP_ERROR_CHECK(init_time_service(STORAGE_NAMESPACE));
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
	// */

	/*
	TaskHandle_t  Core1TaskHnd ;
	xTaskCreatePinnedToCore(timmi_task,"CPU_1",10000,NULL,1,&Core1TaskHnd,1);
	*/


	// load autostart file if specified
	load_autostart_file();

	// if configured and possible start the autostart scene
	scenes_autostart();

	// main loop
	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	xDelay = 50000 / portTICK_PERIOD_MS;
	while(1) {
		vTaskDelay(xDelay);
	}
}
