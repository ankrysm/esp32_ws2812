/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#define MAIN
#include "esp32_ws2812.h"


int main_flags=0;


//uint64_t timmi_dt =22;

extern T_CONFIG gConfig;

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

void app_main() {

	// init storage and get/initalize config
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(init_storage());

	// init led-strip
	led_strip_init(gConfig.numleds);
	//ESP_ERROR_CHECK(strip_init(gConfig.numleds));


	firstled(32, 32, 32);
	TickType_t xDelay = 500 / portTICK_PERIOD_MS;

	// *
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	initialise_mdns();
	initialise_netbios();

	initialise_wifi();
	// esp_err_t res =	waitforConnect();

	xDelay = 500 / portTICK_PERIOD_MS;

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
	// */
	char buf[128];
	config2txt(buf, sizeof(buf));
	ESP_LOGI(__func__, "config=%s",buf);

	// init timer
	init_timer_events();
	set_event_timer_period(gConfig.cycle);

	//start_demo1();
	T_COLOR_RGB fg_color ={32,0,0};
	build_demo2(
			 &fg_color // foreground color
	) ;

	/*
	TaskHandle_t  Core1TaskHnd ;
	xTaskCreatePinnedToCore(timmi_task,"CPU_1",10000,NULL,1,&Core1TaskHnd,1);
	*/

	ESP_LOGI(__func__,"running on core %d",xPortGetCoreID());
	xDelay = 50000 / portTICK_PERIOD_MS;
	//int logcnt =0;
	while(1) {
		//printf("xxx\n");
		/*
		int64_t t_start = esp_timer_get_time();

		timmi();
		int64_t t_end = esp_timer_get_time();
		if ( logcnt== 0 ) {
			ESP_LOGI(__func__,"processing time %lld mikrosec", (t_end - t_start));
			logcnt=20;
		}
		logcnt--;
		 */
		vTaskDelay(xDelay);
	}
}
