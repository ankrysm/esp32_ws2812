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
#include "esp_ota_ops.h"
#include "common_util.h"

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

void sha256totext(const uint8_t *image_hash, char *text, size_t sz_text)
{
	if ( sz_text < HASH_TEXT_LEN ) {
		snprintf(text, sz_text, "sz_text too small: %lu, expected: %lu", sz_text, HASH_TEXT_LEN);
		ESP_LOGE(__func__, "%s", text);
		return;
	}
    memset(text, 0, sz_text);
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&(text[i * 2]), "%02x", image_hash[i]);
    }
}

void get_sha256_of_running_partition(char *text, size_t sz_text)
{
    uint8_t sha_256[HASH_LEN] = { 0 };

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    sha256totext(sha_256, text, sz_text);
}

void get_sha256_of_bootloader_partition(char *text, size_t sz_text)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    sha256totext(sha_256, text, sz_text);

}
