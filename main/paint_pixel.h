/*
 * paint_pixel.h
 *
 *  Created on: 28.07.2022
 *      Author: andreas
 */

#ifndef MAIN_PAINT_PIXEL_H_
#define MAIN_PAINT_PIXEL_H_

typedef enum {
	COL_SEC_CONSTANT     = 0x0000,
	COL_SEC_LINEAR       = 0x0001, // up/down: switch color-from, color-to
	COL_SEC_EXP          = 0x0002,
	COL_SEC_RAINBOW      = 0x0003,
	COL_SEC_SPARKLE      = 0x0004,
	// flags
	COL_SPEC_INIT_NEEDED = 0x0100,
	COL_SPEC_WRAP        = 0x0200
} col_sec_spec;

typedef enum {
	LVL_SEC_CONSTANT     = 0x0001,
	LVL_SEC_LINEAR       = 0x0002,
	LVL_SEC_EXP          = 0x0003,
	LVL_SEC_SPARKLE      = 0x0004,
	// flags
	LVL_SEC_INIT_NEEDED  = 0x0100
} lvl_sec_spec;

// forward
//typedef struct COLOR_SECTION T_COLOR_SECTION;
//typedef struct LEVEL_SECTION T_LEVEL_SECTION;


typedef struct LEVEL_SECTION {
	int32_t lvl1; // in percent
	int32_t lvl2; // optional second value in percent
	lvl_sec_spec spec;
	int32_t len;  // length of a segment
	// working
	double w_f; // factor for everything
	// for list:
	struct LEVEL_SECTION *nxt;
} T_LEVEL_SECTION;


typedef struct COLOR_SECTION {
	col_sec_spec spec;
	uint32_t len; // Length of the section
	T_COLOR_RGB *colortab; // List of colors
	size_t colortab_len; // number of colors in colortab
	T_LEVEL_SECTION *lvl_sec;
	//T_LEVEL_SECTION *w_lvl_sec; // working section
} T_COLOR_SECTION;

// function prototypes
//typedef esp_err_t (paint_level) (T_LEVEL_SECTION*);
//typedef esp_err_t (paint_color) (T_COLOR_SECTION*, int32_t, int32_t);



#endif /* MAIN_PAINT_PIXEL_H_ */
