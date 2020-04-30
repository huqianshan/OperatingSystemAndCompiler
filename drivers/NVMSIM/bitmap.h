#ifndef BIT_MAP_H
#define BIT_MAP_H

#include "nvmconfig.h"

/**
 * Bitmap Macro
 */

#define ONE ((word_t)1)
/* The length of a "Bit",equal to int size*/

#define BIT_WIDTH_IN_BITS (BIT_WIDTH_IN_BYTES << 3)
/* the position in "Bit"*/
#define BIT_OFFSET(pos) (pos % (BIT_WIDTH_IN_BITS))
/* the position in "int" Bitmap */
#define INT_OFFSET(pos) (pos / (BIT_WIDTH_IN_BITS))
/* Set the pos in Bitmap to one */
#define SET_BITMAP(pos, BitMap) \
    (BitMap[INT_OFFSET(pos)] |= (ONE << BIT_OFFSET(pos)))
/* Clear the pos in Bitmap to zero */
#define CLEAR_BITMAP(pos, BitMap) \
    (BitMap[INT_OFFSET(pos)] &= (~(ONE << BIT_OFFSET(pos))))
/* find the pos postion if is equal to One*/
#define BOOL(pos, BitMap) \
    ((BitMap[INT_OFFSET(pos)] & (ONE << BIT_OFFSET(pos))) != 0)

/**
 * Bitmap function Define
 *
*/
// Num is the total numbers of item.
word_t *init_bitmap(word_t num);

/**
 * Query for the free block in page
 * return physical block number
 *  pos: lbn
 *  return: pbn; 0 for not found
 *  len: total item size
 */
word_t query_bitmap(word_t *bitmap, word_t pos, word_t len);

/**
 * retur ncount of number of 1's in word
 */
word_t bitCount(word_t x);
// len: size in word_t
word_t pageCount(word_t *bitmap, word_t len);
/**
 *  print first len bits of the bitmap
 */
void printb_bitmap(word_t *bitmap, word_t len);
// len: size in total item
void print_bitmap(word_t *bitmap, word_t len);

/**
 * print summary information of bitmap
 */
void print_summary_bitmap(word_t *bitmap, word_t len);
word_t total_bitmap(word_t *bitmap);

void destroy_bitmap(word_t *bitmap);
#endif