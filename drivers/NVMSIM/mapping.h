#ifndef MAPPING_H
#define MAPPING_H

typedef uint32_t word_t;
/* The length of a "Bit",equal to int size*/
#define BIT_WIDTH_IN_BYTES (sizeof(int))
#define BIT_WIDTH_IN_BITS (BIT_WIDTH_IN_BYTES << 3)
/* the position in "int" Bitmap */
#define INT_OFFSET(pos) (pos / (BIT_WIDTH_IN_BITS))

// page-maping and use block offset
#define PAGE_MAPPING

/* Mapping Table*/
word_t *MapTable;
// index Table
word_t *IndexTable;
/**
 * Mapping Table parameters
 */
#define KEY_LENGTH (sizeof(word_t) << 3)

#define SECTORS_SIZE_SHIFT (23)
#define TOTAL_SECTORS (1 << SECTORS_SIZE_SHIFT)

#define PAGE_SIZE_SHIFT (5)
#define PAGE_SIZE (1 << PAGE_SIZE_SHIFT)

#define TOTAL_PAGES (TOTAL_SECTORS >> PAGE_SIZE_SHIFT)
#define TOTAL_PAGES_SHIFT (SECTORS_SIZE_SHIFT - PAGE_SIZE_SHIFT)

#define TOTAL_FLAGS_SHIFT (KEY_LENGTH - SECTORS_SIZE_SHIFT)

// get page number by block number
#define BLOCK_TO_PAGE(lbn) (lbn / PAGE_SIZE)
/**
 * Not used
 * 
 * key helper function
 * 18 + 5 + 9
 * lfn+ lbn+flag
 
word_t logic_page_num(word_t key){
    return (key >> 14);
}
word_t logic_block_num(word_t key){
    key <<= (32 - 14);
    key >>= 27;
    return key;
}

word_t flag_key(word_t key){
    key &= 0x1FF;
    return key;
}
Not Used*/

/**
 * key helper function
 * 18+14
 * physical-page-number + access information
 */
// get the physical-page-number by key bug mapping not page
#define PHYSICAL_PAGE_NUM(key) (key >> 14)

// get the flag(access information)
#define ACCESS_TIME(key) (key & 0x3fff)

// make newkey by pbn and access time
#define MAKE_KEY(pbn,num) ((pbn<<14)+num)

// get block offset within page
/*
word_t block_offset(word_t lbn)
{
    word_t block_begin, offset;
    block_begin = INT_OFFSET(lbn) * PAGE_SIZE;
    offset = lbn - block_begin;

    return offset;
}*/

/**
 * init map table by logic block size
 */
int init_maptable(word_t size);

/**
 * update maptable by index(logic-page-number) and key
 */
int update_maptable(word_t index, word_t key);

/**
 * get key from maptable by index(logic-block-number)
 */
word_t get_maptable(word_t lbn);

/**
 * for map: 
 *   make the map from lbn->pbn
 *   return access time for lbn
 */
int map_maptable(word_t lbn,word_t pbn);

/**
 * demap:
 *  clear the physical-block-number
 *  but reserve the access time
 * 
*/
int demap_maptable(word_t lbn);

/**
 * print the mapping 
 *  bug too big
 */
int print_maptable(word_t lbn);

/**
 * init
 */
int init_indextable(word_t size);

/**
 *index_indextable:
 *      return the first index equals zero
*/
int index_indextable();

/**
 * insert:
 *    insert
*/
int inset_indextable();

/***
 *  clear
 */
int clear_indextable();
#endif
