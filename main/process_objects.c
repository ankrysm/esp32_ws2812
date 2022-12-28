/*
 * process_objects.c
 *
 *  Created on: Oct 21, 2022
 *      Author: andreas
 */

#include "esp32_ws2812.h"

/**
 * paint objects
 */
void process_object(T_TRACK_ELEMENT *ele) {

	//if ( extended_logging)
	//	ESP_LOGI(__func__, "startetd evtgrp '%s', flags 0x%x, sts=%d, pos=%d", evtgrp->id, evtgrp->w_flags, evtgrp->status, evtgrp->w_pos);

	if ( ele->w_flags & EVFL_WAIT) {
		return; // do nothing, wait
	}

	T_DISPLAY_OBJECT *obj = ele->w_object; //NULL;

//	if ( strlen(ele->w_object_oid)) {
//		obj = find_object4oid(ele->w_object_oid);
//		if ( !obj) {
//			ESP_LOGW(__func__, "ele.id='%s', no object found for oid='%s'", ele->id, ele->w_object_oid);
//		}
//	}
	if (! obj) {
		if ( ele->w_flags & EVFL_CLEARPIXEL) {
			ele->w_flags &= ~EVFL_CLEARPIXEL; // reset the flag
			uint32_t n = get_numleds();
			ESP_LOGI(__func__,"clear pixel without object: whole strip: blank, numleds=%u", n);
			T_COLOR_RGB bk={.r=0,.g=0,.b=0};
			strip_set_range(0, n - 1, &bk);
			return;
		}

		ESP_LOGI(__func__, "nothing to paint");
		return;
	}

	int32_t startpos, endpos;
	T_COLOR_RGB rgb = {.r=0,.g=0,.b=0};
	T_COLOR_RGB rgb2 = {.r=0,.g=0,.b=0};
	T_COLOR_HSV hsv;
	double dh;

	// whole length of all sections
	int32_t len = 0;
	for (T_DISPLAY_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
		len += data->len;
	}
	double flen = len * ele->w_len_factor;

	if ( flen < 1.0 ) {
		return; // nothing left to display
	}

	if ( ele->w_flags & EVFL_CLEARPIXEL) {
		ele->w_flags &= ~EVFL_CLEARPIXEL; // reset the flag
		startpos = floor(ele->w_pos);
		endpos   = ceil(ele->w_pos + len);
		strip_set_range(startpos, endpos, &rgb);
		ESP_LOGI(__func__, "clear pixel %d .. %d", startpos, endpos);
		return;
	}

	// display range length:
	//   len/2 - lenf/2 = (len-lenf)/2
	//
	// len   = 10 10   10      11
	// lenf  =  2  3    1      11
	// pos   =  4  3.5  4.5     5.5
	// start =  4  3    4       0
	// end   =  6  6    5      11
	//
	// 0123456789
	// xxxxxxxxxx
	//     XX

	startpos = ele->w_pos;
	endpos   = ele->w_pos + len;
	int pos = startpos;
	if ( ele->delta_pos > 0 ) {
		pos = startpos;
	} else {
		pos = endpos;
	}
	double f = ele->w_brightness;

	int32_t dstart = startpos + floor(len - flen)/2.0;
	int32_t dend = ceil(dstart + flen);

	//ESP_LOGI(__func__, "pos=%d..%d, +%d d=%d..%d", startpos, endpos, evt->delta_pos, dstart, dend);
	double r,g,b;

	// for color transition
	double df,dr,dg,db;

	bool ende = false;
	for (T_DISPLAY_OBJECT_DATA *data = obj->data; data; data=data->nxt) {
		if ( data->type == OBJT_BMP ) {
			// special handling for bmp processing
			process_object_bmp(pos, ele, f);
			pos += ele->delta_pos * data->len;
			if ( pos < startpos || pos > endpos) {
				ende = true;
				break; // done.
			}

		} else {

			// other object painted pixelwise
			for ( int data_pos=0; data_pos < data->len; data_pos++) {
				switch (data->type) {
				case OBJT_COLOR:
					if ( data_pos == 0 ) {
						// initialize
						c_hsv2rgb(&(data->para.hsv), &rgb);
						r = f*rgb.r;
						g = f*rgb.g;
						b = f*rgb.b;
						rgb.r = r;
						rgb.g = g;
						rgb.r = r;
					}
					break;
				case OBJT_COLOR_TRANSITION: // linear from one color to another
					// if lvl2-lvl1 = 100 % and len = 4
					// use 25% 50% 75% 100%, not start with 0%
					if (data_pos ==0) {
						df = 1.0 / data->len;
						c_hsv2rgb(&(data->para.tr.hsv_from), &rgb);
						c_hsv2rgb(&(data->para.tr.hsv_to), &rgb2);
						dr = 1.0 *(rgb2.r - rgb.r) * df;
						dg = 1.0 *(rgb2.g - rgb.g) * df;
						db = 1.0 *(rgb2.b - rgb.b) * df;
						r=rgb.r;
						g=rgb.g;
						b=rgb.b;
						/*ESP_LOGI(__func__,"color transition rgb %d/%d/%d -> %d/%d/%d, drgb=%.1f/%.1f/%.1f, f=%.2f "
							,rgb.r, rgb.g, rgb.b
							,rgb2.r, rgb2.g, rgb2.b
							,dr,dg,db
							,f
							);*/
					}
					r+=dr;
					g+=dg;
					b+=db;
					rgb.r = f * r;
					rgb.g = f * g;
					rgb.b = f * b;
					/*ESP_LOGI(__func__,"pos=%d, color transition rgb %d/%d/%d"
						,pos,rgb.r, rgb.g, rgb.b
						);*/
					break;

				case OBJT_RAINBOW:
					if ( data_pos==0) {
						// init
						hsv.h=0;
						hsv.s=100;
						hsv.v=100;
						dh = 360.0 / data->len;
					}
					c_hsv2rgb(&hsv, &rgb);
					rgb.r = f * rgb.r;
					rgb.g = f * rgb.g;
					rgb.b = f * rgb.b;
					hsv.h += dh;
					if ( hsv.h > 360)
						hsv.h=360;
					break;

				case OBJT_SPARKLE:
					break;

				case OBJT_BMP: // should not occure here
					break;

				default:
					ESP_LOGW(__func__,"what type %d NYI", data->type);
				}

				//ESP_LOGI(__func__,"paint id=%d, type=%d, len=%d, pos=%d, dpos=%d, startpos=%d, endpos=%d, dstart=%d, dend=%d",
				//		w->id, w->type, w->len, pos, evt->delta_pos, startpos, endpos, dstart, dend);

				if ( pos >=0 && pos < get_numleds() && pos >= dstart && pos <= dend) {
					c_checkrgb_abs(&rgb);
					strip_set_pixel(pos, &rgb);
				}

				pos += ele->delta_pos;
				if ( pos < startpos || pos > endpos) {
					ende = true;
					break; // done.
				}
			}

		}
		if ( ende)
			break;
	}
	// ESP_LOGI(__func__, "ENDE");
}


