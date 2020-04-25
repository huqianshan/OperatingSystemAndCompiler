/**
 * NVMSIM related configuration
 */

#ifndef __NVMCONFIG_H
#define __NVMCONFIG_H

#include <linux/printk.h>
#include <linux/vmalloc.h>

typedef unsigned int word_t;
/*
word_t map_table_size = 262144;
word_t bit_table_size = 8388608;
word_t info_table_size = 262144;
*/
#define map_table_size (8388608)
#define bit_table_size (8388608)
#define info_table_size (262144)

#define BIT_WIDTH_IN_BYTES (sizeof(word_t))

#define INFO_PAGE_SIZE_SHIFT (5)
#define INFO_PAGE_SIZE (1 << INFO_PAGE_SIZE_SHIFT)
#endif
