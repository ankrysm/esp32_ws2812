/*
 * create_events.c
 *
 *  Created on: 25.06.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"


/**
 * different types of effects can be in list, separated by ';'
 * /
esp_err_t decode_effect_list(char *param, T_EVENT *event ) {

	memset(event, 0, sizeof(T_EVENT));

	char *str = strdup(param);
	char *typ, *tok, *ltok, *l, *ll;
	esp_err_t ret = ESP_OK;

	const size_t maxp = 32;
	int32_t p[maxp];

	for (ltok=strtok_r(str,";", &ll); ltok; ltok=strtok_r(NULL,";", &ll)) {

		memset(p,0, sizeof(p));

		typ=strtok_r(ltok,",",&l);
		int n=0;
		for ( n=0; n<maxp; n++) {
			if ( ! (tok=strtok_r(NULL,",", &l)))
				break;
			p[n] = atoi(tok);
			ESP_LOGI(__func__, "parameter %d = %d (%s)", n, p[n], tok);
		}
		ESP_LOGI(__func__, "n=%d", n);

		T_COLOR_HSV hsv1, hsv2, hsv3;
		uint32_t len;


		// ******* location based **********
		if ( !strcmp(typ, "solid")) {
			// 0   1 2 3
			// len,h,s,v
			len = p[0] > 0 ? p[0] : get_numleds() -1;
			hsv1.h = p[1];
			hsv1.s = p[2];
			hsv1.v = p[3];

			// fix from 0 is default
			decode_effect_fix(&(event->mov_event), 0);

			ret = decode_effect_solid(
					&(event->loc_event),
					len, //length
					&hsv1
			);

		} else if ( !strcmp(typ, "smooth")) {
			// 0   1      2        3  4  5  6  7  8  9  10 11
			// len,fad_in,fade_out,h1,s1,v1,h2,s2,v2,h3,s3,v3
			// or
			// 0   1      2        3  4  5
			// len,fad_in,fade_out,h2,s2,v2
			if ( n < 3) {
				ESP_LOGI(__func__,"not enough parameters");
				ret = ESP_FAIL;
			} else {
				len = p[0] > 0 ? p[0] : get_numleds() -1;
				if ( n<=7 ) {
					hsv1.h = hsv1.s = hsv1.v = 0;
					hsv2.h = p[3];  hsv2.s = p[4];   hsv2.v = p[5];
					hsv3.h = hsv3.s = hsv3.v = 0;
				} else {
					hsv1.h = p[3];  hsv1.s = p[4];   hsv1.v = p[5];
					hsv2.h = p[6];  hsv2.s = p[7];   hsv2.v = p[8];
					hsv3.h = p[9]; hsv3.s = p[10];  hsv3.v = p[11];
				}
				// fix from 0 is default
				decode_effect_fix(&(event->mov_event), 0);
				ret = decode_effect_smooth(
						&(event->loc_event),
						len, //length
						p[1], // fade_in
						p[2], // fade out
						&hsv1, // start hsv
						&hsv2, // middle hsv
						&hsv3  // end hsv
				);
			}

		// ********* movements **********
		} else if ( !strcmp(typ, "rotate")) {
			// 0     1   2     3   4        5  6   7
			// start,len,speed,dir,[startpos][,bh,bgs,bgv]]
			int32_t start = p[0];
			len = p[1] > 0 ? p[1] : get_numleds() - start - 1;
			// bg_hsv
			int32_t startpos = p[4];
			hsv1.h = p[5];  hsv1.s = p[6];   hsv1.v = p[7];

			decode_effect_rotate(
					&(event->mov_event),
					start,
					len,
					startpos,
					p[2], // speed in ms per tick
					p[3],  // direction
					&hsv1
					);
		} else if ( !strcmp(typ, "fix")) {
			// start
			decode_effect_fix(
					&(event->mov_event),
					p[0] // start
			)
					;
		} else {
			ESP_LOGI(__func__, "cannot process '%s'", param);
			ret = ESP_FAIL;
			break;
		}
	} // for
	free(str);
	return ret;
}
// */

/*
void create_solid(
		int32_t pos, // start position
		int32_t len, // numer of leds, -1 = strip len
		T_COLOR_RGB *fg_color, // foreground color
		T_COLOR_RGB *bg_color, // background color
		int32_t inset, // number of edge leds
		int32_t outset, // number of edge leds
		int32_t t_start, // in ms
		int32_t duration, // in ms
		int32_t fadein_time, // in ms
		int32_t fadeout_time, // in ms
		int32_t repeats, // -1 forever
		T_MOVEMENT *movement
) {
	ESP_LOGI(__func__, "start");

	T_EVENT *evt = calloc(1,sizeof(T_EVENT));
	evt->status = SCENE_IDLE;
	evt->type = EVT_SOLID;
	evt->pos = pos;
	evt->len = len;
//	evt->t_start = t_start;
//	evt->duration = duration;
//	evt->t_fade_in = fadein_time;
//	evt->t_fade_out = fadeout_time;
//	evt->repeats = repeats;
//	evt->flags_origin = flags;

	if ( bg_color) {
		evt->bg_color=calloc(1,sizeof(T_COLOR_RGB));
		memcpy(evt->bg_color, bg_color, sizeof(T_COLOR_RGB));
	}

	T_SOLID_DATA *data = calloc(1, sizeof(T_SOLID_DATA));
	evt->data = data;
	// if no color specified assume white
	data->fg_color.r = fg_color? fg_color->r : 255;
	data->fg_color.g = fg_color? fg_color->g : 255;
	data->fg_color.b = fg_color? fg_color->b : 255;
	data->inset = inset;
	data->outset = outset;

	event_list_add(evt);

}
*/

/*
void create_blank(
		int32_t pos, // start position
		int32_t len, // numer of leds, -1 = strip len
		int32_t t_start, // in ms
		int32_t duration, // in ms
		int32_t repeats, // -1 forever
		T_MOVEMENT *movement
) {
	ESP_LOGI(__func__, "start");

	T_EVENT *evt = calloc(1,sizeof(T_EVENT));
	evt->type = EVT_BLANK;
	evt->status = SCENE_IDLE;
	evt->pos = pos;
	evt->len = len;
	evt->t_start = t_start;
	evt->duration = duration;
	evt->repeats = repeats;
//	evt->flags_origin = flags;
	event_list_add(evt);

}
*/

/*
void create_noops(
		int32_t t_start // in ms
) {
	ESP_LOGI(__func__, "start");

	T_EVENT *evt = calloc(1,sizeof(T_EVENT));
	evt->type = EVT_NOOP;
	evt->status = SCENE_IDLE;
	evt->t_start = t_start;
//	evt->flags_origin = flags;
	event_list_add(evt);

}
*/

/*
T_MOVEMENT *create_movement(
		int32_t pos, // start movement positiin
		int32_t len, // length of moving area, -1 = strip len
		float speed, // leds per second
		int32_t pause, // pause between move cycles in ms
		int32_t repeats, // -1 forever
		movement_type_type type
){
	ESP_LOGI(__func__, "start");
	T_MOVEMENT *mv = calloc(1, sizeof(T_MOVEMENT));
	mv->pos = pos;
	mv->len = len;
	mv->speed = speed;
	mv->pause = pause;
	mv->repeats = repeats;
	mv->type = type;
	return mv;
}
*/
