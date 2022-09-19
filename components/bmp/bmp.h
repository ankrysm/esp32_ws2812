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

// instead windows.h
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;

typedef struct tagBITMAPFILEHEADER {
  WORD  bfType; // must be 'BM'
  DWORD bfSize; // The size, in bytes, of the bitmap file (unreliable).
  WORD  bfReserved1; // must be 0
  WORD  bfReserved2; // must be 0
  DWORD bfOffBits; // The offset, in bytes, from the beginning of the BITMAPFILEHEADER structure to the bitmap bits.
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize; // The number of bytes required by the structure.
  LONG  biWidth; // The width of the bitmap, in pixels.
  LONG  biHeight; // The height of the bitmap, in pixels. positive values bottom up, negative values top down 
  WORD  biPlanes;  // The number of planes for the target device. always 1
  WORD  biBitCount; // The number of bits-per-pixel.
                    // 0, 1*, 4*, 8*, 16, 32 ( *=indexed )
  DWORD biCompression; // 0 - RGB uncompressed, 1 RLE8, 2 RLE4, 3 BITFIELDS
  DWORD biSizeImage; // can be 0 if uncompressed data
  LONG  biXPelsPerMeter; // resolution, mostly 0
  LONG  biYPelsPerMeter; // resolution mostly 0
  DWORD biClrUsed; // number of used color table entries
  DWORD biClrImportant; //
} BITMAPINFOHEADER; 

#endif /* COMPONENTS_BMP_BMP_H_ */
