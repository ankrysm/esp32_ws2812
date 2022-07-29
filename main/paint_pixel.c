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
//      level (location based)
//        ^
//        |
//      colors (location based)
//        ^
//        |


/************************************************************************
 * Level funktions
 ************************************************************************/

// constant
static void lvl_sec_constant(T_LEVEL_SECTION *lvlsec,  T_COLOR_RGB *rgb) {
	rgb->r = lvlsec->w_f * rgb->r;
	rgb->g = lvlsec->w_f* rgb->g;
	rgb->b = lvlsec->w_f * rgb->b;
}

// linear transition
static void lvl_sec_linear(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	rgb->r = lvlsec->w_f * rgb->r;
	rgb->g = lvlsec->w_f * rgb->g;
	rgb->b = lvlsec->w_f * rgb->b;
}

// exponential transition
static void lvl_sec_exp(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	rgb->r = lvlsec->w_f * rgb->r;
	rgb->g = lvlsec->w_f* rgb->g;
	rgb->b = lvlsec->w_f * rgb->b;

	lvlsec->w_f  = 2.0* lvlsec->w_f;
}


static void lvl_sec_sparkle(T_LEVEL_SECTION *lvlsec, T_COLOR_RGB *rgb) {
	rgb->r = lvlsec->w_f * rgb->r;
	rgb->g = lvlsec->w_f* rgb->g;
	rgb->b = lvlsec->w_f * rgb->b;
}

// ******* main level paint function ******************
// ******* called for a single pixel at positoin lvl_sec_pos
static void lvl_sec_main(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos, T_COLOR_RGB *rgb) {

	if ( lvlsec ) {
		switch ( lvlsec->spec ) {
		case LVL_SEC_CONSTANT:
			lvl_sec_constant(lvlsec, rgb);
			break;
		case LVL_SEC_LINEAR:
			lvl_sec_linear(lvlsec, rgb);
			break;
		case LVL_SEC_EXP:
			lvl_sec_exp(lvlsec, rgb);
			break;
		case LVL_SEC_SPARKLE:
			lvl_sec_sparkle(lvlsec, rgb);
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", lvlsec->spec);
		}
	}
	if ( lvl_sec_pos >= 0 && lvl_sec_pos < s_numleds) {
		// only in the visible range
		strip_set_pixel(lvl_sec_pos, rgb);
	}
}


// ***** init functions
static void lvl_sec_constant_init(T_LEVEL_SECTION *lvlsec, int32_t w_pos) {
	// w_f as constant factor
	lvlsec->w_f = 1.0 * lvlsec->lvl1 / 100.0;
}

static void lvl_sec_linear_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	if (lvlsec->len <1) {
		lvlsec->w_f = 0.0;
		ESP_LOGE(__func__,"lvlsec->len <1!");
		return;
	}
	// w_f as constant increment
	lvlsec->w_f = 1.0 *(lvlsec->lvl2 - lvlsec->lvl1) / lvlsec->len;
}

static void lvl_sec_exp_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	if (lvlsec->len <1){
		lvlsec->w_f = 0.0;
		ESP_LOGE(__func__,"lvlsec->len <1!");
		return;
	}

	// w_f as increasing factor
	lvlsec->w_f = 1.0 / pow(2.0, lvlsec->len);
}

static void lvl_sec_sparkle_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	lvlsec->w_f = 1.0; // todo
}

// ***** main init function  *****
// ***** called for a range  *****
static void lvl_sec_main_init(T_LEVEL_SECTION *lvlsec, int32_t lvl_sec_pos) {
	if ( !lvlsec )
		return;

	int32_t pos = lvl_sec_pos;
	for (T_LEVEL_SECTION *sec = lvlsec; sec; sec = sec->nxt) {
		switch ( sec->spec ) {
		case LVL_SEC_CONSTANT:
			lvl_sec_constant_init(sec, pos);
			break;
		case LVL_SEC_LINEAR:
			lvl_sec_linear_init(sec, pos);
			break;
		case LVL_SEC_EXP:
			lvl_sec_exp_init(sec, pos);
			break;
		case LVL_SEC_SPARKLE:
			lvl_sec_sparkle_init(sec, pos);
			break;
		default:
			ESP_LOGE(__func__, "NYI %d", sec->spec);
		}
		pos += sec->len;
	}
}

/************************************************************************
 * Color funktions
 ************************************************************************/

// constant color in a range
static void col_sec_constant(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	if ( colsec->colortab_len < 1 ) {
		ESP_LOGE(__func__, "not enough colors");
		return;
	}

	T_LEVEL_SECTION *w_lvl_sec=colsec->lvl_sec; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	//int32_t pos = w_pos;
	int32_t endpos = w_pos + w_len;
	for ( int pos = w_pos; pos < colsec->len; pos++) {

		T_COLOR_RGB rgb;
		memcpy(&rgb, &(colsec->colortab[0]), sizeof(T_COLOR_RGB));

		lvl_sec_main( w_lvl_sec, pos, &rgb);

		pos++;
		if ( pos >= endpos ) {
			if (colsec->spec & COL_SPEC_WRAP){
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

// sets a color transition from rgb1 at wpos to rgb2 at w_pos + w_len
// if the wrap flag is set it starts from w_pos
static void col_sec_linear(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	if ( w_len < 1)
		return;

	if ( colsec->colortab_len < 2 ) {
		ESP_LOGE(__func__, "not enough colors");
		return;
	}

	T_LEVEL_SECTION *w_lvl_sec=colsec->lvl_sec; // start with this
	int32_t lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;

	double dr,dg,db,r,g,b;
	//int32_t delta_pos = 1; // 1 or -1
	T_COLOR_RGB rgb1 = colsec->colortab[0];
	T_COLOR_RGB rgb2 = colsec->colortab[1];

	r = rgb1.r;
	g = rgb1.g;
	b = rgb1.b;
	dr = 1.0 *(rgb2.r - rgb1.r) / w_len;
	dg = 1.0 *(rgb2.g - rgb1.g) / w_len;
	db = 1.0 *(rgb2.b - rgb1.b) / w_len;

	int32_t pos = w_pos;
	int32_t endpos = w_pos + w_len;
	for (int i=0; i < colsec->len; i++) {
		r += dr;
		g += dg;
		b += db;
		/*
		ESP_LOGI(__func__, "fade_in %d at %d, delta=%d: drgb=%d/%d/%d d=%.4f/%.4f/%.4f rgb=%.2f/%.2f/%.2f"
				,i, *pos, delta_pos
				,(rgb2->r - rgb1->r),(rgb2->g - rgb1->g),(rgb2->b - rgb1->b)
				,dr,dg,db
				,r,g,b
				);
		 */
		T_COLOR_RGB rgb;
		rgb.r = r; rgb.g = g; rgb.b = b;

		//c_checkrgb(&rgb, &rgb1, &rgb2);

		lvl_sec_main(w_lvl_sec, pos, &rgb);

		if ( pos > 0 && pos < s_numleds) {
			// only in the visible range
			strip_set_pixel(pos, &rgb);
		}

		pos++;
		if ( pos >= endpos ) {
			if (colsec->spec & COL_SPEC_WRAP){
				pos = w_pos; // from the beginning of the strip
			} else {
				break;
			}
		}

		lvl_sec_cnt--;
		if ( w_lvl_sec && lvl_sec_cnt <=0 ) {
			w_lvl_sec = w_lvl_sec->nxt;
			lvl_sec_cnt = w_lvl_sec ? w_lvl_sec->len : 0;
		}
	}
}

static void col_sec_exp(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	// TODO
}

static void col_sec_rainbow(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	// TODO
}

static void col_sec_sparkle(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {
	// TODO
}

// ***** main color function ******
void col_sec_main(T_COLOR_SECTION *colsec, int32_t w_pos, int32_t w_len) {

	lvl_sec_main_init(colsec->lvl_sec, w_pos);

	switch (colsec->spec) {
	case COL_SEC_CONSTANT:
		col_sec_constant(colsec, w_pos, w_len);
		break;
	case COL_SEC_LINEAR:
		col_sec_linear(colsec, w_pos, w_len);
		break;
	case COL_SEC_EXP:
		col_sec_exp(colsec, w_pos, w_len);
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

