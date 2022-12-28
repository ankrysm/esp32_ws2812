/*
 * process_bmp.h
 *
 *  Created on: Nov 17, 2022
 *      Author: andreas
 */

#ifndef MAIN_PROCESS_BMP_H_
#define MAIN_PROCESS_BMP_H_

#define LEN_RD_BUF 3*1000 // 3 bytes for n leds

typedef struct PROCESS_BMP {
	uint32_t read_buffer_len; // data length in read_buffer
	uint8_t *read_buffer; // link to the actual read buffer
	uint32_t rd_mempos; // read position in read_buffer

	int bufno; // which buffer has data 1 or 2, depends on HAS_DATA-bit

	uint8_t buf[LEN_RD_BUF]; // TODO with calloc
	uint8_t last_buf[LEN_RD_BUF]; // TODO with calloc
	int bytes_per_line;

	uint32_t line_count;
	uint32_t missing_lines_count;

} T_PROCESS_BMP;

#endif /* MAIN_PROCESS_BMP_H_ */
