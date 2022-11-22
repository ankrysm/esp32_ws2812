/*
 * common_util.h
 *
 *  Created on: 20.09.2022
 *      Author: ankrysm
 */

#ifndef COMPONENTS_COMMON_UTILS_COMMON_UTIL_H_
#define COMPONENTS_COMMON_UTILS_COMMON_UTIL_H_

int32_t get_random(int32_t min, uint32_t diff);
uint32_t crc32b(const uint8_t arr[], size_t sz);
int snprintfapp(char *txt, size_t sz_txt, char *fmt, ... );

//#define LOG_MEM(f,c) {ESP_LOGI(f, "MEMORY(%d): free_heap_size=%lu, min=%lu", c, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());}
#define LOG_MEM(f,c) {}

#endif /* COMPONENTS_COMMON_UTILS_COMMON_UTIL_H_ */
