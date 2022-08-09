/*
 * paint_pixel.c
 *
 *  Created on: 28.07.2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"


// ****painting functions

//     LED-strip
//        ^
//        |
//      level sections
//        ^
//        |
//      color sections
//        ^
//        |
//      timing (change parameter for colors and level depending on the time)
//        ^
//        |
//      moving (change location and/or length depending on the time)
//
//              sec_pos                                                           sec_len
//                 +--->                                                        <----+
// leds:           |x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|
//                 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// level sections: | lvl sec 1   |   lvl sec 2       |    lvl sec 3      |
//                 +-------------+-----------+-------+-------------------+-----+
// color sections: |    col sec 1            |          col sec2               |
//                 +-------------------------+---------------------------------+

//              sec_pos                                                           sec_len
//                 +--->                                                        <----+
//                 |                                                                 |->wrap to sec_pos or
//                 |                                         count down to sec_pos <-|
// leds:           |x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|
//                 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---------+
// level sections: | lvl sec 1   |   lvl sec 2       |    lvl sec 3      | lvl sec 4           |
//                 +-------------+-----------+-------+-------------------+-----+---------------+
// color sections: |    col sec 1            |          col sec2               |   col sec 3   |
//                 +-------------------------+---------------------------------+---------------+

/******************************************************************************************
typedef struct PAINT_STRUC {
	double col_h, col_s, col_v;
	double col_dh, col_ds, col_dv;
	double lvl_f, lvl_df;
	int32_t led_pos;
	uint32_t flags;
} T_PAINT_STRUC;


void led_sec_init_col_sec(T_PAINT_STRUC *w, T_COLOR_SECTION *w_col_sec) {
	col_sec_spec w_col_spec;
	if (w_col_sec) {

		// Start with HSV1
		w->col_h = 1.0 * w_col_sec->hsv1.h;
		w->col_s = 1.0 * w_col_sec->hsv1.s;
		w->col_v = 1.0 * w_col_sec->hsv1.v;
		w->col_dh = w->col_ds = w->col_dv = 0.0;

		w_col_spec = w_col_sec->spec;
		switch ( w_col_spec  & SEC_SPEC_MASK) {
		case COL_SEC_CONSTANT:
			break;

		case COL_SEC_LINEAR:
			w->col_dh = 1.0 * (w_col_sec->hsv2.h - w_col_sec->hsv1.h) / w_col_sec->len;
			w->col_ds = 1.0 * (w_col_sec->hsv2.s - w_col_sec->hsv1.s) / w_col_sec->len;
			w->col_dv = 1.0 * (w_col_sec->hsv2.v - w_col_sec->hsv1.v) / w_col_sec->len;
			break;

		case COL_SEC_EXP:
			// TODO change from linear (this) to exp:
			w->col_dh = 1.0 * (w_col_sec->hsv2.h - w_col_sec->hsv1.h) / w_col_sec->len;
			w->col_ds = 1.0 * (w_col_sec->hsv2.s - w_col_sec->hsv1.s) / w_col_sec->len;
			w->col_dv = 1.0 * (w_col_sec->hsv2.v - w_col_sec->hsv1.v) / w_col_sec->len;
			break;

		case COL_SEC_RAINBOW:
			// saturation always 100%
			w->col_s = 100.0;

			// hue over 360 degrees
			w->col_dh = 360.0 / w_col_sec->len;
			// saturation and value constant
			w->col_ds = 0.0;
			w->col_dv = 0.0;
			break;

		case COL_SEC_SPARKLE:
			sparkle_color_init(w_col_sec);
			break;

		default:
			ESP_LOGE(__func__, "NYI %d", w_col_spec);
			// todo some defaults
		}
	//} else {
	//	w_col_spec = COL_SEC_CONSTANT;
	}
}


void led_sec_init_lvl_sec(T_PAINT_STRUC *w, T_LEVEL_SECTION *w_lvl_sec) {
	lvl_sec_spec w_lvl_spec;
	if (w_lvl_sec) {
		w_lvl_spec = w_lvl_sec->spec;
		switch(w_lvl_spec & SEC_SPEC_MASK) {
		case LVL_SEC_CONSTANT:
			// w_f as constant factor
			w->lvl_f = 1.0 * w_lvl_sec->lvl1 / 100.0;
			w->lvl_df = 0.0; // not used
			break;
		case LVL_SEC_LINEAR:
			// w_lvl_df as constant increment,
			// example: if lvl2-lvl1 = 100 % and len = 4
			// use 25% 50% 75% 100%, not start with 0%
			w->lvl_df = 1.0 *(w_lvl_sec->lvl2 - w_lvl_sec->lvl1) / w_lvl_sec->len;
			w->lvl_f = w_lvl_df;
			break;
		case LVL_SEC_EXP:
			// w_f as increasing factor, doubled each cycle
			// example: if lvl2 - lvl1 is 100% and len is 4
			// use 12,5%, 25%, 50% 100%
			w->lvl_f = 1.0 / pow(2.0, (w_lvl_sec->len-1));
			w->lvl_df = 0.0; // not used
			break;
		case LVL_SEC_SPARKLE:
			// TODO
			w->lvl_f = 1.0;
			w->lvl_df = 0.0;
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", w_lvl_sec->spec);
			w->lvl_f = 1.0;
			w->lvl_df = 0.0;
		}
	} else {
		w_lvl_spec =  LVL_SEC_CONSTANT;
		w->lvl_f = 1.0;
		w->lvl_df = 0.0;
	}
}

void led_sec_main(T_LED_SECTION *led_sec) {

	const int32_t max_cnt = 999999;

	const uint32_t init_lvl_sec = 0x0001;
	const uint32_t init_col_sec = 0x0002;

	T_PAINT_STRUC w;

	w.flags = init_lvl_sec | init_col_sec;

	// first level section
	T_LEVEL_SECTION *w_lvl_sec=led_sec->lvl_sec_list; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : max_cnt;
//	lvl_sec_spec w_lvl_spec = LVL_SEC_CONSTANT;

	// first color section
	T_COLOR_SECTION *w_col_sec = led_sec->col_sec_list;
	int32_t col_sec_cnt = w_col_sec ? w_col_sec->len : max_cnt;
	//col_sec_spec w_col_spec = COL_SEC_CONSTANT;

	// d_sec_pos should be -1 or +1 not 0
	if ( led_sec->d_sec_pos == 0)
		led_sec->d_sec_pos = 1;


	int32_t cnt = 0; //
	w.led_pos = led_sec->sec_pos; // position on the strip
	int32_t end_pos = led_sec->startpos + led_sec->len;

	// transient values for level section
	w.lvl_f = w.lvl_df = 0.0;

	// transient values for color section
	//T_COLOR_HSV w_col_hsv1={.h=0, .s=0, .v=0};
	//T_COLOR_HSV w_col_hsv2={.h=0, .s=0, .v=0};
	//double w_col_h, w_col_s, w_col_v;
	//double w_col_dh, w_col_ds, w_col_dv;
	w.col_h = w.col_s = w.col_v = w.col_dh = w.col_ds = w.col_dv = 0.0;

	// *** main loop ***
	for ( cnt = 0; cnt < led_sec->len; cnt++) {
		// ******************************************
		// 1. *** init? ***

		// *** color section init ***
		// initiate color values: w.col_h, w.col_s, w.col_v
		//           differences: w.col_dh, w.col_ds, w.col_dv
		if ( w.flags & init_col_sec ) {
			// init color section
			led_sec_init_col_sec(&w, w_col_sec);
			w.flags &= ~init_col_sec;
		}
		// *** level section init ***
		// initiate level factors: w_lvl_f
		//            differences: w_lvl_df
		if ( w.flags & init_lvl_sec ) {
			// init level section
			led_sec_init_lvl_sec(&w, w_lvl_sec);
			w.flags &= ~init_lvl_sec;
		}
		// *** End of init ****************************************
		// ********************************************************

		// 2. *** make color and level for one pixel
		// color from color section with brightness from level section
		// the color for a single pixel
		T_COLOR_HSV hsv={.h=0, .s=0, .v=0};
		T_COLOR_RGB rgb={.r=0,.g=0,.b=0};
		hsv.h = w.col_h;
		hsv.s = w.col_s;
		hsv.v = w.col_v;

		c_hsv2rgb(&hsv, &rgb);
		rgb.r = rgb.r * w.lvl_f;
		rgb.g = rgb.g * w.lvl_f;
		rgb.b = rgb.b * w.lvl_f;

		// 3. *** set the color
		if ( w.led_pos >=0 && w.led_pos < get_numleds()) {
			strip_set_pixel(w.led_pos, &rgb);
		}

		// 4. **** calculate color parameter for next cycle
		// ********************************************************

		switch ( w_col_spec  & SEC_SPEC_MASK) {
		case COL_SEC_CONSTANT:
			break;

		case COL_SEC_LINEAR:
			w.col_h += w.col_dh;
			w.col_s += w.col_ds;
			w.col_v += w.col_dv;
			break;

		case COL_SEC_EXP:
			// TODO another calculation for real exponential values instead of this linear needed
			w.col_h += w.col_dh;
			w.col_s += w.col_ds;
			w.col_v += w.col_dv;
			break;

		case COL_SEC_RAINBOW:
			w.col_h += w.col_dh;
			w.col_s += w.col_ds;
			w.col_v += w.col_dv;
			break;

		case COL_SEC_SPARKLE:
			// choose random position for a spot and a random color between hsv1 and hsv2
			// with a random life time
			sparkle_color_get(w_col_sec, led_pos);
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", w_lvl_sec->spec);
		}

		// * 5. *** calculate level parameter for next
		switch(w_lvl_spec & SEC_SPEC_MASK) {
		case LVL_SEC_CONSTANT:
			// w_f as constant factor
			break;

		case LVL_SEC_LINEAR:
			w_lvl_f += w_lvl_df;
			break;

		case LVL_SEC_EXP:
			w_lvl_f = 2 * w_lvl_f;
			break;

		case LVL_SEC_SPARKLE:
			// TODO
			w_lvl_f = 1.0;
			w_lvl_df = 0.0;
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", w_lvl_sec->spec);
		}

		// 6. *** next position
		led_pos+= led_sec->d_sec_pos;

		// 7. *** check for section border and strategy
		int stop_working = 0;
		if ( led_pos >= end_pos ) {
			switch (led_sec->spec & SEC_SPEC_MASK ) {
			case LED_SEC_NO_WRAP:
				stop_working = 1;
				// stop
				break;
			case LED_SEC_WRAP:
				// continue with first position
				led_pos = led_sec->startpos;
				break;
			case LED_SEC_BOUNCE:
				// stop working but change direction of pos
				led_sec->d_sec_pos = - led_sec->d_sec_pos;
				break;
			default:
				ESP_LOGE(__func__, "led_sec_spec %d NYI", led_sec->spec);
			}

		} else if (led_pos < led_sec->startpos) {
			switch (led_sec->spec & SEC_SPEC_MASK ) {
			case LED_SEC_NO_WRAP:
				stop_working = 1;
				// stop
				break;
			case LED_SEC_WRAP:
				// continue with first position
				led_pos = led_sec->startpos + led_sec->len;
				break;
			case LED_SEC_BOUNCE:
				// stop working but change direction of pos
				led_sec->d_sec_pos = - led_sec->d_sec_pos;
				break;
			default:
				ESP_LOGE(__func__, "led_sec_spec %d NYI", led_sec->spec);
			}
		}
		if ( stop_working ) {
			break;
		}

		// 8. *** check for a next section ***
		// level section
		if ( cnt >= lvl_sec_cnt) {
			// take the next level section
			w_lvl_sec = w_lvl_sec->nxt;
			if ( w_lvl_sec) {
				lvl_sec_cnt += w_lvl_sec->len;
			} else {
				lvl_sec_cnt = max_cnt; // not reached
			}
			flags |= init_lvl_sec;
		}
		// color section
		if ( cnt >= col_sec_cnt) {
			// take the next level section
			w_col_sec = w_col_sec->nxt;
			if ( w_col_sec) {
				col_sec_cnt += w_col_sec->len;
			} else {
				col_sec_cnt = max_cnt; // not reached
			}
			flags |= init_col_sec;
		}

	} // for
}

//  number of sparkles = 5
//  width = 3: .|X|.
//  .|X|.       .|X|.           .|X|.     .|X|.     .|X|.
// |x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|
//   |                 |                             |
//  min pos        center pos                    max pos
// timing: if a sparkle disappeared create a new one
void sparkle_color_init(T_COLOR_SECTION *col_sec) {
	if ( ! col_sec->para ) {
		ESP_LOGE(__func__, "data section needed");
		return;
	}
	T_SPARKLE_SECTION *sec = (T_SPARKLE_SECTION*) col_sec->para;
	if ( sec->n < 1 ) {
		ESP_LOGE(__func__, "number of sparkles not set");
		return;
	}
	uint32_t diff;
	diff = sec->max_pos - sec->min_pos;
	// check for new sparkles - depends on time
	for( int i=0; i<sec->n; i++) {
		if ( sec->sp_time[i] & SPARKLE_TIME_MASK > 0)
			continue; // this slot is running

		// create a new sparkle at a position
		uint32_t t = get_random(sec->sp_start_delay, sec->sp_time_vary);
		sec->sp_time[i] = t | SPARKLE_IDLE;
		sec->sp_pos[i] = get_random(sec->min_pos, sec->pos_width);
	}
}

// get color for a position
void sparkle_color_get(T_COLOR_SECTION *col_sec, uint32_t pos, T_COLOR_HSV *hsv) {
	if ( ! col_sec->para ) {
		ESP_LOGE(__func__, "data section needed");
		return;
	}
	T_SPARKLE_SECTION *sec = (T_SPARKLE_SECTION*) col_sec->para;
	if ( sec->n < 1 ) {
		ESP_LOGE(__func__, "number of sparkles not set");
		return;
	}


	for( int i=0; i<sec->n; i++) {
		if ( sec->sp_time[i] & SPARKLE_TIME_MASK > 0)
			continue; // this slot is running


		uint32_t ph = sec->sp_time[i] & SPARKLE_PHASE_MASK;
		switch ( ph ) {
		case SPARKLE_START_UP:
			break;
		case SPARKLE_START:
			break;
		case SPARKLE_MID:
			break;
		case SPARKLE_END:
			break;
		}
	}
}

*************************************************************************/


/************************************************************************
 * Level funktions
 ************************************************************************/

/*
// constant
static void lvl_sec_constant(T_LEVEL_SECTION *lvlsec,  T_COLOR_RGB *rgb) {
	double f = lvlsec->w_f * lvlsec->w_timf;
	rgb->r = f * rgb->r;
	rgb->g = f * rgb->g;
	rgb->b = f * rgb->b;
}

// linear transition
static void lvl_sec_linear(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	double f = lvlsec->w_f * lvlsec->w_timf;
	rgb->r = f * rgb->r;
	rgb->g = f * rgb->g;
	rgb->b = f * rgb->b;

	lvlsec->w_f +=lvlsec->w_df;
}

// exponential transition
static void lvl_sec_exp(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	double f = lvlsec->w_f * lvlsec->w_timf;
	rgb->r = f * rgb->r;
	rgb->g = f * rgb->g;
	rgb->b = f * rgb->b;

	lvlsec->w_f  = 2.0 * lvlsec->w_f;
}


static void lvl_sec_sparkle(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	rgb->r = lvlsec->w_f * rgb->r;
	rgb->g = lvlsec->w_f * rgb->g;
	rgb->b = lvlsec->w_f * rgb->b;
}
*/

/*

// ******* main level paint function ******************
// ******* called for a single pixel at positoin lvl_sec_pos
static void lvl_sec_main(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos, T_COLOR_RGB *rgb) {

	T_COLOR_RGB lrgb = {.r=0, .g=0, .b=0};
	if ( rgb ) {
		if ( lvlsec ) {
			double f = lvlsec->w_f;
			switch ( lvlsec->spec ) {
			case LVL_SEC_CONSTANT:
				//lvl_sec_constant(lvlsec, rgb);
				break;
			case LVL_SEC_LINEAR:
				//lvl_sec_linear(lvlsec, rgb);
				lvlsec->w_f +=lvlsec->w_df;
				break;
			case LVL_SEC_EXP:
				//lvl_sec_exp(lvlsec, rgb);
				lvlsec->w_f  = 2.0 * lvlsec->w_f;
				break;
			case LVL_SEC_SPARKLE:
				//lvl_sec_sparkle(lvlsec, rgb);
				// TODO
				break;
			default:
				ESP_LOGE(__func__, "NYI %d", lvlsec->spec);
			}
			f = f * lvlsec->w_timf;
			lrgb.r = rgb->r * f;
		}
	}

	if ( lvl_sec_pos >= 0 && lvl_sec_pos < get_numleds()) {
		// only in the visible range
		strip_set_pixel(lvl_sec_pos, &lrgb);
	}
}

*/
// ***** init functions

/*
static void lvl_sec_constant_init(T_LEVEL_SECTION *lvlsec, int32_t w_pos) {
	// w_f as constant factor
	lvlsec->w_f = 1.0 * lvlsec->lvl1 / 100.0;
	lvlsec->w_df = 0.0; // not used
}

static void lvl_sec_linear_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	if (lvlsec->len <1) {
		lvlsec->w_f = 0.0;
		ESP_LOGE(__func__,"lvlsec->len <1!");
		return;
	}
	// w_df as constant increment,
	// if lvl2-lvl1 = 100 % and len = 4
	// use 25% 50% 75% 100%, not start with 0%
	lvlsec->w_df = 1.0 *(lvlsec->lvl2 - lvlsec->lvl1) / lvlsec->len;
	lvlsec->w_f = lvlsec->w_df;
}

static void lvl_sec_exp_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {

	lvlsec->w_df = 0.0; // not used

	if (lvlsec->len <1){
		lvlsec->w_f = 0.0;
		ESP_LOGE(__func__,"lvlsec->len <1!");
		return;
	}

	// w_f as increasing factor, doubled each cycle
	// if lvl2 - lvl1 is 100% and len is 4
	// use 12,5%, 25%, 50% 100%
	lvlsec->w_f = 1.0 / pow(2.0, (lvlsec->len-1));
}

static void lvl_sec_sparkle_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	lvlsec->w_f = 1.0; // todo
}
*/

/*
// ***** main init function  *****
// ***** called for a range  *****
static void lvl_sec_main_init(T_LEVEL_SECTION *lvl_sec_list, int32_t lvl_sec_pos) {
	if ( !lvl_sec_list ) {
		ESP_LOGE(__func__,"missing lvl_sec_list!");
		return;
	}

//	int32_t pos = lvl_sec_pos;
	for (T_LEVEL_SECTION *sec = lvl_sec_list; sec; sec = sec->nxt) {
		sec->w_f = 1.0;
		sec->w_df = 0.0;
		sec->w_timf = 1.0; // will be change in timing control

		if (sec->len <1) {
			// non visible
			ESP_LOGI(__func__,"lvlsec->len <1!");
			continue;
		}
		switch ( sec->spec ) {
		case LVL_SEC_CONSTANT:
			//lvl_sec_constant_init(sec, pos);
			// w_f as constant factor
			sec->w_f = 1.0 * sec->lvl1 / 100.0;
			break;

		case LVL_SEC_LINEAR:
			//lvl_sec_linear_init(sec, pos);
			// w_df as constant increment,
			// example: if lvl2-lvl1 = 100 % and len = 4
			// use 25% 50% 75% 100%, not start with 0%
			sec->w_df = 1.0 *(sec->lvl2 - sec->lvl1) / sec->len;
			sec->w_f = sec->w_df;
			break;

		case LVL_SEC_EXP:
			//lvl_sec_exp_init(sec, pos);
			// w_f as increasing factor, doubled each cycle
			// if lvl2 - lvl1 is 100% and len is 4
			// use 12,5%, 25%, 50% 100%
			sec->w_f = 1.0 / pow(2.0, (sec->len-1));
			break;

		case LVL_SEC_SPARKLE:
			// TODO lvl_sec_sparkle_init(sec, pos);
			break;

		default:
			ESP_LOGE(__func__, "NYI %d", sec->spec);
		}
//		pos += sec->len;
	}
}

*/
/************************************************************************
 * color section funktions
 ************************************************************************/

/*
// constant color in a range
static void col_sec_constant(T_COLOR_SECTION *col_sec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	T_LEVEL_SECTION *w_lvl_sec=col_sec->lvl_sec_list; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	int32_t endpos = w_pos + w_len;
	for ( int pos = w_pos; pos < col_sec->len; pos++) {

		lvl_sec_main( w_lvl_sec, pos, &(col_sec->w_rgb1));

		pos++;
		if ( pos >= endpos ) {
			if (col_sec->spec & COL_SPEC_WRAP){
				pos = w_pos; // from the beginning of the strip
			} else {
				break;
			}
		}

		lvl_sec_cnt--;
		if ( w_lvl_sec && lvl_sec_cnt <=0 ) {
			// next lvl section if exists
			w_lvl_sec = w_lvl_sec->nxt;
			lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;
		}
	}
}
*/

/*
// sets a color transition from rgb1 at wpos to rgb2 at w_pos + w_len
// if the wrap flag is set it starts from w_pos
static void col_sec_linear(T_COLOR_SECTION *col_sec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	// because there are more then one level section, positions must be checkagainst level section len,
	// when reached the end take then next section
	T_LEVEL_SECTION *w_lvl_sec=col_sec->lvl_sec_list; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	double dr,dg,db,r,g,b;
	T_COLOR_RGB *rgb1 = &(col_sec->w_rgb1);
	T_COLOR_RGB *rgb2 = &(col_sec->w_rgb2);

	r = rgb1->r;
	g = rgb1->g;
	b = rgb1->b;
	dr = 1.0 *(rgb2->r - rgb1->r) / w_len;
	dg = 1.0 *(rgb2->g - rgb1->g) / w_len;
	db = 1.0 *(rgb2->b - rgb1->b) / w_len;

	int32_t pos = w_pos;
	int32_t endpos = w_pos + w_len;
	for (int i=0; i < col_sec->len; i++) {
		r += dr;
		g += dg;
		b += db;
		/ *
		ESP_LOGI(__func__, "fade_in %d at %d, delta=%d: drgb=%d/%d/%d d=%.4f/%.4f/%.4f rgb=%.2f/%.2f/%.2f"
				,i, *pos, delta_pos
				,(rgb2->r - rgb1->r),(rgb2->g - rgb1->g),(rgb2->b - rgb1->b)
				,dr,dg,db
				,r,g,b
				);
		 * /
		T_COLOR_RGB rgb;
		rgb.r = r; rgb.g = g; rgb.b = b;

		lvl_sec_main(w_lvl_sec, pos, &rgb);

		if ( pos > 0 && pos < get_numleds()) {
			// only in the visible range
			strip_set_pixel(pos, &rgb);
		}

		pos++;
		if ( pos >= endpos ) {
			if (col_sec->spec & COL_SPEC_WRAP){
				pos = w_pos; // from the beginning of the strip
			} else {
				break;
			}
		}

		lvl_sec_cnt--;
		if ( w_lvl_sec && lvl_sec_cnt <=0 ) {
			// all positions used, go to the next level section
			w_lvl_sec = w_lvl_sec->nxt;
			lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;
		}
	}
}
*/

/*
static void col_sec_exp(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	// TODO
}
*/

/*
// based on HSV
static void col_sec_rainbow(T_COLOR_SECTION *col_sec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	// because there are more then one level section, positions must be checkagainst level section len,
	// when reached the end take then next section
	T_LEVEL_SECTION *w_lvl_sec=col_sec->lvl_sec_list; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	int32_t pos = w_pos;
	int32_t endpos = w_pos + w_len;
	for (int i=0; i < col_sec->len; i++) {
		T_COLOR_RGB rgb;
		T_COLOR_HSV hsv;
		hsv.h =

		lvl_sec_main(w_lvl_sec, pos, &rgb);

		if ( pos > 0 && pos < get_numleds()) {
			// only in the visible range
			strip_set_pixel(pos, &rgb);
		}

		pos++;
		if ( pos >= endpos ) {
			if (col_sec->spec & COL_SPEC_WRAP){
				pos = w_pos; // from the beginning of the strip
			} else {
				break;
			}
		}

		lvl_sec_cnt--;
		if ( w_lvl_sec && lvl_sec_cnt <=0 ) {
			// all positions used, go to the next level section
			w_lvl_sec = w_lvl_sec->nxt;
			lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;
		}

	}
}

*/

/*
static void col_sec_sparkle(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	// TODO
}
*/


/*
// ***** main color function ******
// location dependend - sets the colors in a given range
static void col_sec_main(T_LED_SECTION *col_sec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	// because there are more then one level section, positions must be checkagainst level section len,
	// when reached the end take then next section
	T_LEVEL_SECTION *w_lvl_sec=col_sec->lvl_sec_list; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	double dr,dg,db,r,g,b;
	T_COLOR_RGB *rgb1 = &(col_sec->w_rgb1);
	T_COLOR_RGB *rgb2 = &(col_sec->w_rgb2);

	r = rgb1->r;
	g = rgb1->g;
	b = rgb1->b;
	dr = 1.0 *(rgb2->r - rgb1->r) / w_len;
	dg = 1.0 *(rgb2->g - rgb1->g) / w_len;
	db = 1.0 *(rgb2->b - rgb1->b) / w_len;

	int32_t pos = w_pos;
	int32_t endpos = w_pos + w_len;
	for (int i=0; i < col_sec->len; i++) {
		r += dr;
		g += dg;
		b += db;
		/ *
		ESP_LOGI(__func__, "fade_in %d at %d, delta=%d: drgb=%d/%d/%d d=%.4f/%.4f/%.4f rgb=%.2f/%.2f/%.2f"
				,i, *pos, delta_pos
				,(rgb2->r - rgb1->r),(rgb2->g - rgb1->g),(rgb2->b - rgb1->b)
				,dr,dg,db
				,r,g,b
				);
		 * /
		T_COLOR_RGB rgb;
		rgb.r = r; rgb.g = g; rgb.b = b;

		// -------------------------
		switch (colsec->spec) {
		case COL_SEC_CONSTANT:
			col_sec_constant(colsec, w_pos, w_len);
			break;
		case COL_SEC_LINEAR:
			col_sec_linear(colsec, w_pos, w_len);
			break;
		case COL_SEC_EXP:
			col_sec_linear(colsec, w_pos, w_len);
			//col_sec_exp(colsec, w_pos, w_len);
			break;
		case COL_SEC_RAINBOW:
			col_sec_rainbow(colsec, w_pos, w_len);
			break;
		case COL_SEC_SPARKLE:
			col_sec_sparkle(colsec, w_pos, w_len);
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", colsec->spec);



		// --------------------------

		lvl_sec_main(w_lvl_sec, pos, &rgb);

		if ( pos > 0 && pos < get_numleds()) {
			// only in the visible range
			strip_set_pixel(pos, &rgb);
		}

		pos++;
		if ( pos >= endpos ) {
			if (col_sec->spec & COL_SPEC_WRAP){
				pos = w_pos; // from the beginning of the strip
			} else {
				break;
			}
		}

		lvl_sec_cnt--;
		if ( w_lvl_sec && lvl_sec_cnt <=0 ) {
			// all positions used, go to the next level section
			w_lvl_sec = w_lvl_sec->nxt;
			lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;
		}
	}


	switch (colsec->spec) {
	case COL_SEC_CONSTANT:
		col_sec_constant(colsec, w_pos, w_len);
		break;
	case COL_SEC_LINEAR:
		col_sec_linear(colsec, w_pos, w_len);
		break;
	case COL_SEC_EXP:
		col_sec_linear(colsec, w_pos, w_len);
		//col_sec_exp(colsec, w_pos, w_len);
		break;
	case COL_SEC_RAINBOW:
		col_sec_rainbow(colsec, w_pos, w_len);
		break;
	case COL_SEC_SPARKLE:
		col_sec_sparkle(colsec, w_pos, w_len);
		break;
	default:
		ESP_LOGE(__func__, "NYI %d", colsec->spec);
	}
}

*/

/************************************************************************
 * timing functions
 ************************************************************************/

/*
// **************** Level timing functions *****************************
static void tim_lvl_sec_constant(T_COLOR_SECTION *col_sec) {
	/ * is not necessary
	T_TIMING_LVL_SECTION *s = col_sec->tim_lvl_sec;
	if (!((s->spec) & TIM_LVL_SEC_INITIALIZED)) {
		// assign values only once
		if ( col_sec->lvl_sec) {
			for (T_LEVEL_SECTION *lsec = col_sec->lvl_sec; lsec; lsec = lsec->nxt) {
				lsec->lvl1 = s->lvl1;
				lsec->lvl2 = s->lvl2;
			}
		}
		// mark as initialized
		s->spec |= TIM_LVL_SEC_INITIALIZED;
	}
	 * /
}
*/


/*
static void tim_lvl_sec_linear(T_COLOR_SECTION *col_sec) {

	T_TIMING_LVL_SECTION *s = col_sec->tim_lvl_sec;
	if ( ! ((s->spec) & TIM_LVL_SEC_INITIALIZED)) {
		uint64_t p = get_event_timer_period(); // in ms
		if ( p == 0 ) {
			ESP_LOGE(__func__, "no timer period set");
			return;
		}
		// speed in (% / sec) converted to (% / ms)
		s->w_df = 1.0 *(s->lvl2 - s->lvl1) / (p * (s->speed / 1000.0));
		if ( s->w_df < 0 ) {
			s->w_f = 1.0;
		} else {
			s->w_f = 0.0;
		}
		s->spec |= TIM_LVL_SEC_INITIALIZED;
	}

	// sets the value
	if ( col_sec->lvl_sec) {
		for (T_LEVEL_SECTION *lsec = col_sec->lvl_sec; lsec; lsec = lsec->nxt) {
			lsec->w_timf = s->w_f;
		}
	}

	// get next value
	uint8_t ended = false;
	if ( !(s->spec & TIM_LVL_SEC_ENDED )) {
		s->w_f += s->w_df;
		if ( s->w_f < 0.0 ) {
			s->w_f = 0.0;
			ended = true;
		} else if ( s->w_f > 1.0 ) {
			s->w_f = 1.0;
			ended = true;
		}
	}

	if ( ended ) {
		// do repeat
		if ( s->repeat > 0 ) {
			(s->w_repeat)--;
			if ( s->w_repeat > 0) {
				ended = false;
			}
		}
		if ( ended ) {
			s->spec |= TIM_LVL_SEC_ENDED;
		} else {
			// start a new cycle
			if (s->spec & TIM_LVL_SEC_BOUNCE) {
				s->w_df = -1.0 * s->w_df;
			} else {
				if ( s->w_df < 0 ) {
					s->w_f = 1.0;
				} else {
					s->w_f = 0.0;
				}
			}
		}
	}
}

*/



/*
// ***************** color timing functions ************************************
static void tim_col_sec_constant(T_COLOR_SECTION *col_sec) {
	// nothing todo
}

// transition from color1 to color2 and back (if bounced)
static void tim_col_sec_linear(T_COLOR_SECTION *col_sec) {

	T_TIMING_COL_SECTION *cs = col_sec->tim_col_sec;
	if ( ! ((cs->spec) & TIM_COL_SEC_INITIALIZED)) {
		uint64_t p = get_event_timer_period(); // in ms
		if ( p == 0 ) {
			ESP_LOGE(__func__, "no timer period set");
			return;
		}

		// from hsv1 to hsv2 with the given percent speed
		// speed in (% / sec) converted to (% / ms)
		cs->w_df_h = 1.0 *(cs->hsv2.h - cs->hsv1.h) / (p * (cs->speed / 1000.0));
		if ( cs->w_df_h < 0.0 ) {cs->w_f_h = 1.0;} else {cs->w_f_h = 0.0;}

		cs->w_df_s = 1.0 *(cs->hsv2.s - cs->hsv1.s) / (p * (cs->speed / 1000.0));
		if ( cs->w_df_s < 0.0 ) {cs->w_f_s = 1.0;} else {cs->w_f_s = 0.0;}

		cs->w_df_v = 1.0 *(cs->hsv2.v - cs->hsv1.v) / (p * (cs->speed / 1000.0));
		if ( cs->w_df_v < 0.0 ) {cs->w_f_v = 1.0;} else {cs->w_f_v = 0.0;}

		cs->spec |= TIM_COL_SEC_INITIALIZED;
	}

	// sets the value
	if ( col_sec->lvl_sec) {
		for (T_LEVEL_SECTION *lsec = col_sec->lvl_sec; lsec; lsec = lsec->nxt) {
			lsec->w_timf = s->w_f;
		}
	}

	// get next value
	uint8_t ended = false;
	if ( !(s->spec & TIM_LVL_SEC_ENDED )) {
		s->w_f += s->w_df;
		if ( s->w_f < 0.0 ) {
			s->w_f = 0.0;
			ended = true;
		} else if ( s->w_f > 1.0 ) {
			s->w_f = 1.0;
			ended = true;
		}
	}

	if ( ended ) {
		// do repeat
		if ( s->repeat > 0 ) {
			(s->w_repeat)--;
			if ( s->w_repeat > 0) {
				ended = false;
			}
		}
		if ( ended ) {
			s->spec |= TIM_LVL_SEC_ENDED;
		} else {
			// start a new cycle
			if (s->spec & TIM_LVL_SEC_BOUNCE) {
				s->w_df = -1.0 * s->w_df;
			} else {
				if ( s->w_df < 0 ) {
					s->w_f = 1.0;
				} else {
					s->w_f = 0.0;
				}
			}
		}
	}
}

//
// *************** main timing function ***************************************
void tim_sec_main(T_COLOR_SECTION *col_sec, int32_t w_pos, int32_t w_len) {

	lvl_sec_main_init(col_sec->lvl_sec, w_pos);

	if ( col_sec->tim_lvl_sec) {
		T_TIMING_LVL_SECTION *ls = col_sec->tim_lvl_sec;
		tim_lvl_sec_spec spec = ls->spec & 0xFF; // ignore flags
		// calculate level timings
		switch ( spec ) {
		case TIM_LVL_SEC_CONSTANT:
			tim_lvl_sec_constant(col_sec);
			break;
		case TIM_LVL_SEC_LINEAR:
			tim_lvl_sec_linear(col_sec);
			break;
		case TIM_LVL_SEC_EXP:
			// TODO create a special exp function
			tim_lvl_sec_linear(col_sec);
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", spec);
		}
	}

	if ( col_sec->tim_col_sec) {
		T_TIMING_COL_SECTION *cs = col_sec->tim_col_sec;
		tim_col_sec_spec spec = cs->spec;
		// TODO calculate color timings
		switch(spec) {
		case TIM_COL_SEC_CONSTANT:
			// nothing todo
			tim_col_sec_constant(col_sec);
			break;
		case TIM_COL_SEC_LINEAR:
			tim_col_sec_linear(col_sec);
			break;
		case TIM_COL_SEC_EXP:
			// TODO separate exp function needed
			tim_col_sec_linear(col_sec);
			break;
		case TIM_SEC_RAINBOW:
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", spec);
		}
	}


	col_sec_main(col_sec, w_pos, w_len);
}
*/

