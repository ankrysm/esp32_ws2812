/*
 * paint_pixel.h
 *
 *  Created on: 28.07.2022
 *      Author: andreas
 */

#ifndef MAIN_PAINT_PIXEL_H_
#define MAIN_PAINT_PIXEL_H_

#define SEC_SPEC_MASK 0x00FF
#define SEC_FLAG_MASK 0xFF00

typedef enum {
	COL_SEC_CONSTANT     = 0x0000,
	COL_SEC_LINEAR       = 0x0001, // up/down: switch color-from, color-to
	COL_SEC_EXP          = 0x0002,
	COL_SEC_RAINBOW      = 0x0003,
	COL_SEC_SPARKLE      = 0x0004,
	// flags
	//COL_SPEC_WRAP        = 0x0100
} col_sec_spec;

typedef enum {
	LVL_SEC_CONSTANT     = 0x0001,
	LVL_SEC_LINEAR       = 0x0002,
	LVL_SEC_EXP          = 0x0003,
	LVL_SEC_SPARKLE      = 0x0004,
	// flags
	//LVL_SEC_INIT_NEEDED  = 0x0100
} lvl_sec_spec;

typedef enum {
	TIM_LVL_SEC_CONSTANT    = 0x0000, // time independend
	TIM_LVL_SEC_LINEAR      = 0x0001, // linear from one level to another
	TIM_LVL_SEC_EXP         = 0x0002, // exponential from one level to another
	TIM_LVL_SEC_RANDOM      = 0x0003, // random between two levels slow or fast
	// flags
	TIM_LVL_SEC_INITIALIZED = 0x0100, 
	TIM_LVL_SEC_ENDED       = 0x0200, // nothing to do anymore
	TIM_LVL_SEC_BOUNCE      = 0x0400, // lvl1 -> lvl2 -> lvl1 ...., default: lvl1 -> lvl2, lvl1 -> lvl2
	TIM_LVL_SEC_CLEAR_AT_END= 0x0800  // set to black if timing ends
} tim_lvl_sec_spec;

typedef enum {
	TIM_COL_SEC_CONSTANT   = 0x0000, // time independend
	TIM_COL_SEC_LINEAR     = 0x0001, // linear from one color to another
	TIM_COL_SEC_EXP        = 0x0002, // exponential from one color to another
	TIM_SEC_RAINBOW        = 0x0003, // changes the start H Parameter from rainbow HSV
	
	// flags
	TIM_COL_SEC_INITIALIZED = 0x0100, 
	TIM_COL_SEC_ENDED       = 0x0200, // nothing to do anymore
	TIM_COL_SEC_BOUNCE      = 0x0400, // lvl1 -> lvl2 -> lvl1 ...., default: lvl1 -> lvl2, lvl1 -> lvl2
	TIM_COL_SEC_CLEAR_AT_END= 0x0800  // set to black if timing ends
} tim_col_sec_spec;

typedef enum {
	LED_SEC_NO_WRAP         = 0x0000, // cut after end
	LED_SEC_WRAP            = 0x0001, // wrap around 
	LED_SEC_BOUNCE          = 0x0002  // bounce to start
} led_sec_spec;

// forward
//typedef struct COLOR_SECTION T_COLOR_SECTION;
//typedef struct LEVEL_SECTION T_LEVEL_SECTION;

/*******************
 section structures
********************/

typedef struct LEVEL_SECTION {
	lvl_sec_spec spec;
	int32_t len;  // length of the segment
	
	// special parameter
	int32_t lvl1; // location based level in percent
	int32_t lvl2; // optional second value in percent

	// for list:
	struct LEVEL_SECTION *nxt;
} T_LEVEL_SECTION;


//  number of sparkles = 5
//  width = 3: .|X|.
//  .|X|.       .|X|.           .|X|.     .|X|.     .|X|.
// |x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|x|
//   |                 |                             |
//  min pos        center pos                    max pos
// timing: if a sparkle disappeared create a new one
typedef struct SPARKLE_SECTION {
	uint32_t n; // number of sparkles
	uitn32_t width;
	uint32_t center_pos; // for moving effects
	uint32_t min_pos; // for minimum sparkle position
	uitn32_t max_pos; // fro maximum sparkle position
	uint32_t *sp_pos; // array for sparkle positions
	uint32_t *sp_time; // array display time of the sparkles
} T_SPARKLE_SECTION;

typedef struct COLOR_SECTION {
	col_sec_spec spec;
	uint32_t len; // Length of the section

	// special parameter
	void *para;
	int32_t startpos;
	T_COLOR_HSV hsv1;
	T_COLOR_HSV hsv2;
	
	// for build a list
	struct COLOR_SECTION *nxt;
} T_COLOR_SECTION;

typedef struct TIMING_LVL_SECTION {
	tim_lvl_sec_spec spec;
	uint32_t lvl1; // in percent
	uint32_t lvl2; // in percent
	uint32_t speed; // percent per second
	uint32_t repeat; // 0 - forever
	uint64_t duration; // in ms
	// working data
	uint32_t w_repeat;
	double w_f; // factor for everything
	double w_df; // another factor for everything
} T_TIMING_LVL_SECTION;


//// Timing sections

typedef struct TIMING_COL_SECTION {
	tim_col_sec_spec spec;
	uint32_t speed; // percent per second
	uint32_t repeat; // 0 - forever
	
	// color definitions as HSV makes it easier to calculate new colors
	T_COLOR_HSV hsv1;
	T_COLOR_HSV hsv2;
	
	// working data
	uint32_t w_repeat;
	double w_f_h; // h factor for everything
	double w_f_s;
	double w_f_v;
	double w_df_h; // deltas
	double w_df_s; 
	double w_df_v; 
	
} T_TIMING_COL_SECTION;

// main definition
typedef struct LED_SECTION {
	led_sec_spec spec;
	uint32_t len; // Length of the section
	int32_t startpos;
	
	// working data
	int32_t sec_pos;
	int32_t d_sec_pos;
	
	// level + color sections
	T_LEVEL_SECTION *lvl_sec_list; // list of lvl sections
	T_COLOR_SECTION *col_sec_list; // list of color sections
	
	// level + color timing sections
	T_TIMING_LVL_SECTION *tim_lvl_sec_list; // timing level section list
	T_TIMING_COL_SECTION *tim_col_sec_list; // timing color section list
} T_LED_SECTION;

// function prototypes
//typedef esp_err_t (paint_level) (T_LEVEL_SECTION*);
//typedef esp_err_t (paint_color) (T_COLOR_SECTION*, int32_t, int32_t);



#endif /* MAIN_PAINT_PIXEL_H_ */
