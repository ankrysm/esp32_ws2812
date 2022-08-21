/*
 * create_demo.c
 *
 *  Created on: 26.06.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

/*
void start_demo1() {

	T_COLOR_RGB c = {32,0,0};
	create_solid(
			1,    // start position
			20,   // len
			&c,   // fg color
			NULL, // bg_color
			5,    // inset
			5,    // outset
			1000, // t_start
			5000, // duration
			1000, // fade in time
			1000, // fade out time
			-1,
			NULL    // movement
			);

/ *	create_blank(
			1,     // start position
			-1,    // len
			0,     // t_start
			0,     // duration
			0,     // once
			NULL   // movement
			);
* /

	c.r=64;
	c.g=64;
	c.b=0;
	create_solid(
			40,     // start position
			15,     // len
			&c,     // fg color
			NULL,   // bg_color
			3,      // inset
			3,      // outset
			3000,   // t_start
			2000,   // duration
			200,    // fade in time
			500,    // fade out time
			3,
			NULL    // movement
	);

	/ *
	create_blank(
			1,       // start position
			-1,      // len
			12000,  // t_start
			0,       // duration
			0,       // once
			NULL     // no movement
			);
	* /
}
// */

void build_demo2(
		T_COLOR_RGB *fg_color // foreground color
) {



	T_EVENT *evt = calloc(1,sizeof(T_EVENT));

	// * wait 3 secs,
	// * at position 10 paint 15 pixel in red for 5 secs then finished
	// * brightness 10 %

	evt->id=1000;
	evt->pos = 10;
	evt->brightness = 0.0;

	T_EVT_WHAT *w = calloc(1,sizeof(T_EVT_WHAT));
	w->id = 2001;
	w->pos=0;
	w->len=15;
	w->type = WT_COLOR;
	T_NAMED_RGB_COLOR *nc = color4name("red");
	w->para.hsv.h = nc->hsv.h;
	w->para.hsv.s = nc->hsv.s;
	w->para.hsv.v = nc->hsv.v;

	evt->what_list = w;

	T_EVT_TIME *tevt1 = calloc(1,sizeof(T_EVT_TIME));
	tevt1->id =1001;
	tevt1->type = ET_SET_BRIGHTNESS;
	tevt1->starttime = 3000; // 3 sec, in ms
	tevt1->value = 0.1;

	T_EVT_TIME *tevt2 = calloc(1,sizeof(T_EVT_TIME));
	tevt1->nxt = tevt2;
	tevt2->id =1002;
	tevt2->type = ET_CLEAR;
	tevt2->starttime = 8000; // 3 sec, in ms
	tevt2->value = 0.0;

	evt->evt_time_list = tevt1;

	event_list_add(evt);



	/*
	char *params[]={
			"smooth,10,4,4,210,100,10;rotate,0,0,100,1,0",
			"solid,1,0,100,10;rotate,0,0,100,-1,10",
			"solid,1,240,100,10;rotate,0,0,100,1,40",
			"solid,2,120,100,10;rotate,0,0,50,-1,30",
			""
	};
*/
	/*
	for(int i=0; strlen(params[i]); i++) {
		T_EVENT evt;
		char *param=params[i];
		if (decode_effect_list(param, &evt) == ESP_OK) {
			ESP_LOGI(__func__,"done: %s\n", param);
			if ( event_list_add(&evt) == ESP_OK) {
				ESP_LOGI(__func__,"event '%s' stored", param);
			} else {
				ESP_LOGE(__func__,"could not store event '%s'", param);
			}
		} else {
			ESP_LOGW(__func__,"decode FAILED: %s\n", param);
		}
	}
	*/
	/*
	int pos = 1;
	int len = 20;

	// build then event
	T_EVENT *evt = calloc(1, sizeof(T_EVENT));
	event_list_add(evt);

	evt->status = SCENE_IDLE;
	evt->type = EVT_SOLID;
	evt->pos = pos;
	evt->len = len;

	// if no color specified assume white
	evt->fg_color.r = fg_color? fg_color->r : 255;
	evt->fg_color.g = fg_color? fg_color->g : 255;
	evt->fg_color.b = fg_color? fg_color->b : 255;


	// location based effects
	T_SCENE_PARAMETER_LIST *l_br = calloc(1,sizeof(T_SCENE_PARAMETER_LIST));
	evt->l_brightness = l_br;

	T_SCENE_PARAMETER *sp1,*sp2, *sp3;

	sp1 = calloc(1, sizeof(T_SCENE_PARAMETER));
	l_br->sp_list = sp1;

	/ * Brightness
	   /------\
	  /        \
	 /          \
	* /
	// ramp up
	sp1->v_start = 0;
	sp1->v_end = 100;
	sp1->length = 20; // in percent
	sp1->type_t = TR_EXPONENTIAL;
	sp1->type_a = 0; // doesn't matter here
	sp1->type_m = 0; // doen't matter here

	// constant middle
	sp2 = calloc(1, sizeof(T_SCENE_PARAMETER));
	sp1->nxt = sp2;

	sp1->v_start = 100;
	sp1->v_end = 100;
	sp1->length = 40; // in percent
	sp1->type_t = 0; // doesn't matter here
	sp1->type_a = 0; // doesn't matter here
	sp1->type_m = 0; // doesn't matter here

	sp3 = calloc(1, sizeof(T_SCENE_PARAMETER));
	sp2->nxt = sp3;

	// ramp down
	sp1->v_start = 100;
	sp1->v_end = 0;
	sp1->length = 40; // in percent
	sp1->type_t = TR_EXPONENTIAL;
	sp1->type_a = 0;// doesn't matter here
	sp1->type_m = 0; // doesn't matter here

*/
}








