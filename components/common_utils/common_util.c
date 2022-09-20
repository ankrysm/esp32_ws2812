/*
 * util.c
 *
 *  Created on: 05.08.2022
 *      Author: andreas
 */


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"
//#include "bootloader_random.h"

int32_t get_random(int32_t min, uint32_t diff) {
	if ( diff == 0)
		return min;

	int64_t drr;
	if (diff == UINT32_MAX)
		drr= esp_random();
	else
		drr = (int64_t)(diff)*esp_random() / UINT32_MAX ;
	return min + drr;
}

uint32_t crc32b(const uint8_t arr[], size_t sz) {
    // Source: https://stackoverflow.com/a/21001712
    unsigned int byte, crc, mask;
    int i = 0, j;
    crc = 0xFFFFFFFF;
    for(i=0; i<sz; i++) {
        byte = arr[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}

// like snprintf, but appends then new data
int snprintfapp(char *txt, size_t sz_txt, char *fmt, ... ) {
	va_list	ap;
	int len = strlen(txt);
	va_start(ap, fmt);
	int res=vsnprintf(&txt[len], sz_txt - len, fmt, ap);
	va_end(ap);
	return res;
}
