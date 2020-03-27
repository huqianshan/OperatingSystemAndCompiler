#ifndef __BIT_MAP_H
#define __BIT_MAP_H

/**
 * A literal 1 is a regular, signed int, which raises two problems: 
 * it isn't guaranteed to be 32 bits, and 1 << 31 is undefined behavior 
 * (because of the sign bit). All literal 1s 
 * in the above code should be replaced with the
 *  unsigned 32-bit (word_t) 1.
 */
typedef uint32_t word_t;
word_t ONE = 1;
int *BitMap;

/* The length of a "Bit",equal to int size*/
#define BIT_WIDTH_IN_BYTES (sizeof(int))
#define BIT_WIDTH_IN_BITS (BIT_WIDTH_IN_BYTES << 3)

/* the position in "Bit"*/
#define BIT_OFFSET(pos) (pos % (BIT_WIDTH_IN_BITS))

/* the position in "int" Bitmap */
#define INT_OFFSET(pos) (pos / (BIT_WIDTH_IN_BITS))

/* Set the pos in Bitmap to one */
#define SET_BITMAP(pos) \
    (BitMap[INT_OFFSET(pos)] |= (ONE << BIT_OFFSET(pos)))

/* Clear the pos in Bitmap to zero */
#define CLEAR_BITMAP(pos) \
    (BitMap[INT_OFFSET(pos)] &= (~(ONE << BIT_OFFSET(pos))))

/* find the pos postion if is equal to One*/
#define BOOL(pos) (BitMap[INT_OFFSET(pos)] & (ONE << BIT_OFFSET(pos)))

/**
 *  Num is the total numbers of item. 
 */
int init_bitmap(int num);

/**
 * Query for the free block in page
 * return physical block number
 *  pos: lbn
 *  return: pbn
 */
word_t query_bitmap(word_t pos);

/**
 * retur ncount of number of 1's in word
 */
word_t bitCount(word_t x);

/**
 *  print first len bits of the bitmap 
 */
void printb(int len);

/**
 * print summary information of bitmap
 */
void prints(int len);



/** 
 * Access Information Table
 *  
 *  Entry: Logic Sectors Number(23bits) Value: int 9 bits,equal to 1 int
 */
word_t *AIT;

/**
 * Page: Page: 32 Sectors (1 int size)
 */
#define PAGE_SIZE (32)
#define KEY_TABLE(pos) (pos / PAGE_SIZE)

/**
 * init Access Infor Table by sectors numbers
 *      all set to zero
 */
int init_ait(word_t size);

/**
 * update_ait(pos)
 * add sectors pos value,AIT[page(1)]+=1;
 */
int update_ait(word_t pos);

word_t get_ait(word_t pos);

void print_ait(int len);

word_t set_helper(word_t lbn);
word_t read_helper(word_t lbn);
void clear_helper(word_t lbn);


word_t wear_leavel(word_t size);

#endif