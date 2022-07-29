/*
 * location_based_events.c
 *
 *  Created on: 22.07.2022
 *      Author: ankrysm
 */


#include "esp32_ws2812.h"

//extern T_CONFIG gConfig;
// extern size_t s_numleds;


/******************************************************
 * decoding text strings for location based events
 * called from create_events.c
 ******************************************************/

/**
 * a constant color over a range
 */
esp_err_t decode_effect_solid(T_LOC_EVENT *evt, uint32_t len, T_COLOR_HSV *hsv) {
	memset(evt,0,sizeof(T_LOC_EVENT));

	evt->type = LOC_EVENT_SOLID;
	evt->len = len;
	c_hsv2rgb( hsv, &(evt->rgb1));

    ESP_LOGI(__func__,"len=%d hsv=%d/%d/%d rgb=%d/%d/%d",
    		evt->len, hsv->h, hsv->s, hsv->v, evt->rgb1.r, evt->rgb1.g, evt->rgb1.b
    );

    return ESP_OK;

}


/**
 * a constant color with fade in and fade out
 */
esp_err_t decode_effect_smooth(
		T_LOC_EVENT *evt,
		uint32_t len,
		uint32_t fade_in,
		uint32_t fade_out,
		T_COLOR_HSV *hsv1,
		T_COLOR_HSV *hsv2,
		T_COLOR_HSV *hsv3
) {
	memset(evt,0,sizeof(T_LOC_EVENT));
	evt->type = LOC_EVENT_SMOOTH;
	evt->flags = fade_in_lin | fade_out_lin;
	evt->len = len;
	evt->fade_in = fade_in;
	evt->fade_out= fade_out;
	c_hsv2rgb( hsv1, &(evt->rgb1));
	c_hsv2rgb( hsv2, &(evt->rgb2));
	c_hsv2rgb( hsv3, &(evt->rgb3));
    ESP_LOGI(__func__,"len=%d, fade_in=%d, fade_out=%d, 1: hsv=%d/%d/%d rgb=%d/%d/%d, 2: hsv=%d/%d/%d rgb=%d/%d/%d, 3: hsv=%d/%d/%d rgb=%d/%d/%d",
    		evt->len, evt->fade_in, evt->fade_out,
			hsv1->h, hsv1->s, hsv1->v, evt->rgb1.r, evt->rgb1.g, evt->rgb1.b,
			hsv2->h, hsv2->s, hsv2->v, evt->rgb2.r, evt->rgb2.g, evt->rgb2.b,
			hsv3->h, hsv3->s, hsv3->v, evt->rgb3.r, evt->rgb3.g, evt->rgb3.b
    );
    return ESP_OK;
}

/******************************************************
 * process location based events
 ******************************************************/

/*
 * solid
 */
static esp_err_t process_effect_solid(T_EVENT *evt) {

	T_LOC_EVENT *levt = &(evt->loc_event);
	T_MOV_EVENT *mevt = &(evt->mov_event);

	int32_t start = evt->mov_event.w_pos;

    //ESP_LOGI(__func__,"start=%d, len=%d rgb=%d/%d/%d", start, levt->len, levt->rgb1.r, levt->rgb1.g, levt->rgb1.b);

	int32_t delta_pos = 1;
	int32_t pos = start;
	for ( int i = 0; i < levt->len; i++) {
		if ( pos >= 0 && pos < s_numleds) {
			strip_set_pixel(pos, &(levt->rgb1));
		    //ESP_LOGI(__func__,"i=%d: pos=%d", i, pos);
		}
		calc_pos(mevt, &pos, &delta_pos);
	}

	//strip_set_color(start, start + evt->len - 1, levt->rgb1.r, levt->rgb1.g, levt->rgb1.b);

	return ESP_OK;
}

/*
 * smooth, with different fade types
 */
static void process_fade_lin(
		int32_t *pos,
		T_MOV_EVENT *mevt,
		uint32_t fade_len,
		T_COLOR_RGB *rgb1, // from
		T_COLOR_RGB *rgb2  // to
) {
	if ( fade_len < 1)
		return;

	double dr,dg,db,r,g,b;
	int32_t delta_pos = 1; // 1 or -1

	r = rgb1->r;
	g = rgb1->g;
	b = rgb1->b;
	dr = 1.0 *(rgb2->r - rgb1->r) / fade_len;
	dg = 1.0 *(rgb2->g - rgb1->g) / fade_len;
	db = 1.0 *(rgb2->b - rgb1->b) / fade_len;

	for (int i=0; i < fade_len; i++) {
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
		c_checkrgb(&rgb, rgb1, rgb2);

		if ( *pos > 0 && *pos < s_numleds) {
			strip_set_pixel(*pos, &rgb);
		}
		calc_pos(mevt, pos, &delta_pos);
	}
}


static void process_fade_exp(
		int32_t *pos,
		T_MOV_EVENT *mevt,
		uint32_t fade_len,
		T_COLOR_RGB *rgb1, // from
		T_COLOR_RGB *rgb2  // to
) {
	if ( fade_len < 1)
		return;

	double dr,dg,db,dr21,dg21,db21, r,g,b;

	double dd = 1.0 / pow(2.0, fade_len);
	int32_t delta_pos = 1; // 1 or -1

	r = rgb1->r;
	g = rgb1->g;
	b = rgb1->b;
	dr21 = 1.0 * (rgb2->r - rgb1->r);
	dg21 = 1.0 * (rgb2->g - rgb1->g);
	db21 = 1.0 * (rgb2->b - rgb1->b);

	for (int i=0; i < fade_len; i++) {
		dr = dr21 * dd;
		dg = dg21 * dd;
		db = db21 * dd;

		r += dr;
		g += dg;
		b += db;
		/*
		ESP_LOGI(__func__, "fade_in %d at %d, delta=%d: dd=%.6f drgb=%d/%d/%d d=%.4f/%.4f/%.4f rgb=%.2f/%.2f/%.2f" //rgb=%d/%d/%d"
				,i, *pos, delta_pos, dd
				,(rgb2->r - rgb1->r),(rgb2->g - rgb1->g),(rgb2->b - rgb1->b)
				,dr,dg,db
				,r,g,b//,rgb.r, rgb.g, rgb.b
				);
		 */
		T_COLOR_RGB rgb;
		rgb.r = r; rgb.g = g; rgb.b = b;
		c_checkrgb(&rgb, rgb1, rgb2);

		if ( *pos > 0 && *pos < s_numleds) {
			strip_set_pixel(*pos, &rgb);
		}
		calc_pos(mevt, pos, &delta_pos);

		dd = dd*2.0;
	}
}

/*
 * smooth main
 */
static esp_err_t process_effect_smooth(T_EVENT *evt) {

	T_LOC_EVENT *levt = &(evt->loc_event);
	T_MOV_EVENT *mevt = &(evt->mov_event);

	int32_t start = evt->mov_event.w_pos;

	/*
	ESP_LOGI(__func__,"start=%d, len=%d, fade_in=%d, fade_out=%d, 1: rgb=%d/%d/%d, 2: rgb=%d/%d/%d, 3: rgb=%d/%d/%d",
    		start, levt->len, levt->fade_in, levt->fade_out,
			levt->rgb1.r, levt->rgb1.g, levt->rgb1.b,
			levt->rgb2.r, levt->rgb2.g, levt->rgb2.b,
			levt->rgb3.r, levt->rgb3.g, levt->rgb3.b
    );
    */

	int32_t pos = start;

	if ( (levt->flags ) & fade_in_exp ) {
		process_fade_exp(&pos, mevt, levt->fade_in, &(levt->rgb1), &(levt->rgb2));
	} else {
		process_fade_lin(&pos, mevt, levt->fade_in, &(levt->rgb1), &(levt->rgb2));
	}

	int32_t len = levt->len - levt->fade_in - levt->fade_out;
	uint32_t l_fade_out = levt->fade_out;
	if ( len < 0) {
		// fade out parameter to big. ignore it
		len = levt->len - levt->fade_in;
		l_fade_out=0;
	}

	int32_t delta_pos = 1;
	for ( int i = 0; i < len; i++) {
		if ( pos > 0 && pos < s_numleds) {
			strip_set_pixel(pos, &(levt->rgb2));
		}
		calc_pos(mevt, &pos, &delta_pos);
	}
	//ESP_LOGI(__func__, "middle %d-%d: rgb=%d/%d/%d",pos, pos+len, levt->rgb2.r, levt->rgb2.g, levt->rgb2.b);

	if ( (levt->flags ) & fade_out_exp ) {
		process_fade_exp(&pos, mevt, l_fade_out, &(levt->rgb2), &(levt->rgb3));
	} else {
		process_fade_lin(&pos, mevt, l_fade_out, &(levt->rgb2), &(levt->rgb3));
	}
	return ESP_OK;
}

/******************************************************
 * process location based events, main entry
 ******************************************************/
esp_err_t process_loc_event( T_EVENT *evt) {
	switch(evt->loc_event.type) {
	case LOC_EVENT_SOLID:
		return process_effect_solid(evt);
	case LOC_EVENT_SMOOTH:
		return process_effect_smooth(evt);
	default:
		ESP_LOGW(__func__, "%d NYI", evt->loc_event.type);
	}
	return ESP_FAIL;
}

void loc_event2string(T_LOC_EVENT *evt, char *buf, size_t sz_buf) {
	switch(evt->type) {
	case LOC_EVENT_SOLID:
		snprintf(buf,sz_buf,"'solid' len=%d rgb=%d/%d/%d",
				evt->len, evt->rgb1.r, evt->rgb1.g, evt->rgb1.b
		);
		break;
	case LOC_EVENT_SMOOTH:
		snprintf(buf,sz_buf,"'smooth' len=%d, fade_in=%d, fade_out=%d, rgb1=%d/%d/%d, rgb2=%d/%d/%d, rgb3=%d/%d/%d",
				evt->len, evt->fade_in, evt->fade_out,
				evt->rgb1.r, evt->rgb1.g, evt->rgb1.b,
				evt->rgb2.r, evt->rgb2.g, evt->rgb2.b,
				evt->rgb3.r, evt->rgb3.g, evt->rgb3.b
		);
		break;
	default:
		snprintf(buf,sz_buf, "%d NYI", evt->type);
	}

}

