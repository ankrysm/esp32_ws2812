/*
 * bmp.h
 *
 *  Created on: Sep 13, 2022
 *      Author: andreas
 * see
 * https://de.wikipedia.org/wiki/Windows_Bitmap
 * https://docs.microsoft.com/en-us/previous-versions//dd183376(v=vs.85)?redirectedfrom=MSDN
 *
 */

#ifndef COMPONENTS_BMP_BMP_H_
#define COMPONENTS_BMP_BMP_H_

#include "https_get.h"

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

// instead windows.h
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;

#define SZ_BUFFER 4096 //8192

// sets by working process
#define BMP_BIT_BUFFER_PROCESSED   0x01
#define BMP_BIT_STOP_WORKING       0x02

// sets by https-callback process
#define BMP_BIT_BUFFER1_HAS_DATA   0x10
#define BMP_BIT_BUFFER2_HAS_DATA   0x20
#define BMP_BIT_NO_MORE_DATA       0x40

typedef enum {
	BRP_IDLE, // 0
	BRP_CONNECT, // 1
	BRP_GOT_FILE_HEADER, // 2
	BRP_GOT_INFO_HEADER, // 3
	BRP_GOT_SKIPPED_DATA, // 4
	BRP_GOT_FIRST_DATA_BUFFER1, // 5
	BRP_GOT_DATA_BUFFER1, // 6
	BRP_GOT_DATA_BUFFER2 // 7
} t_bmp_read_phase;

#define BRP2TXT(c) ( \
		c==BRP_IDLE ? "IDLE" :  \
		c==BRP_GOT_FILE_HEADER ? "GOT_FILE_HEADER" :  \
		c==BRP_GOT_INFO_HEADER ? "GOT_INFO_HEADER" : \
		c==BRP_GOT_SKIPPED_DATA ? "GOT_SKIPPED_DATA" : \
		c==BRP_GOT_FIRST_DATA_BUFFER1 ? "GOT_FIRST_DATA_BUFFER1" : \
		c==BRP_GOT_DATA_BUFFER1 ? "GOT_DATA_BUFFER1" : \
		c==BRP_GOT_DATA_BUFFER2 ? "GOT_DATA_BUFFER2" : "UNKNOWN" )



// header must be packed to map the read data exactly to the memory
// we had to check the byte order but ESP32 matches, no byte swap needed

// first block: file header
//          <42><4D><36><20><1C><00><00><00><00><00><36><00><00><00><28><00>
// 00000000  42  4d  36  20  1c  00  00  00  00  00  36  00  00  00  28  00  |BM6 ......6...(.|
//           t   t   s   s   s   s
typedef struct tagBITMAPFILEHEADER {
  char  bfType1; // must be 'BM'
  char  bfType2;
  DWORD bfSize; // The size, in bytes, of the bitmap file (unreliable).
  WORD  bfReserved1; // must be 0
  WORD  bfReserved2; // must be 0
  DWORD bfOffBits; // The offset, in bytes, from the beginning of the BITMAPFILEHEADER structure to the bitmap bits.
} __attribute__((packed)) BITMAPFILEHEADER;

// second block: info header
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize; // The number of bytes required by the structure.
  LONG  biWidth; // The width of the bitmap, in pixels.
  LONG  biHeight; // The height of the bitmap, in pixels. positive values bottom up, negative values top down
  WORD  biPlanes;  // The number of planes for the target device. always 1
  WORD  biBitCount; // The number of bits-per-pixel.
                    // 0, 1*, 4*, 8*, 16, 24, 32 ( *=indexed )
  DWORD biCompression; // 0 - RGB uncompressed, 1 RLE8, 2 RLE4, 3 BITFIELDS
  DWORD biSizeImage; // can be 0 if uncompressed data
  LONG  biXPelsPerMeter; // resolution, mostly 0
  LONG  biYPelsPerMeter; // resolution mostly 0
  DWORD biClrUsed; // number of used color table entries
  DWORD biClrImportant; //
}  __attribute__((packed))  BITMAPINFOHEADER;

// third block: data

typedef struct {
	// FreeRTOS event group to signal when something is read or something is processed
	EventGroupHandle_t s_bmp_event_group_for_reader; // read process (it's me) waits for ...PROCESSED or STOP_WORKING
	EventGroupHandle_t s_bmp_event_group_for_worker; // worker process waits for ...HAS_DATA, ...NO_MORE_DATA

	BITMAPFILEHEADER bmpFileHeader;
	BITMAPINFOHEADER bmpInfoHeader;

	uint8_t *read_buffer1; // alternated used read buffer
	uint8_t *read_buffer2;
	size_t sz_read_buffer; // size of read buffers
	uint32_t read_buffer_length; // read data len

	uint8_t *buffer; // buffer for skipped data
	uint32_t total_length; // total length of read data
	t_bmp_read_phase bmp_read_phase; // working phase
	uint32_t buf_len_expected; // amount of expected data from webserver

	char errmsg[256];
	bool has_new_errmsg;


} T_BMP_WORKING;



// prototypes
void bmp_set_extended_log(int p_extended_log);
void bmp_init();
void bmp_init_data(T_BMP_WORKING *data);
void bmp_free_buffers(T_BMP_WORKING *data);
uint32_t get_bytes_per_pixel(T_BMP_WORKING *data);
uint32_t get_bytes_per_line(T_BMP_WORKING *data);
void clear_ux_bits(T_BMP_WORKING *data);

EventBits_t get_ux_bits(T_BMP_WORKING *data, TickType_t xTicksToWait);
void set_ux_quit_bits(T_BMP_WORKING *data, EventBits_t uxQuitBits);

uint32_t get_read_length(T_BMP_WORKING *data);
uint8_t *get_read_buffer(T_BMP_WORKING *data, int bufnr);

int https_callback_bmp_processing(T_HTTPS_CLIENT_SLOT *slot, uint8_t **buf, uint32_t *buf_len);

#endif /* COMPONENTS_BMP_BMP_H_ */
