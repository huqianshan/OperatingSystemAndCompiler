#ifndef MAPPING_H
#define MAPPING_H

// page-maping and use block offset
#define PAGE_MAPPING

/* Mapping Table*/
word_t *MapTable;

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
*/

/**
 * key helper function
 * 18+14
 * physical-page-number + access information
 */
// get the physical-page-number by key
#define PHYSICAL_PAGE_NUM(key) (key >> 14)

// get the flag(access information)
#define ACCESS_TIME(key) (key & 0x3ffff)

word_t block_offset(word_t lbn)
{
    word_t block_begin, block_offset;
    block_begin = INT_OFFSET(lbn) * PAGE_SIZE;
    block_offset = lbn - block_begin;

    return block_offset;
}

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
int get_maptable(word_t lbn);

/** 
 * for read: 
 * just get the key by logical-block-number
 * return physical-block-number
 */
word_t read_maptable(word_t lbn);

/**
 * for write: 
 *  return the physical-block-number;
 *  update the bitmap
 *  increase access time in flag of key
 */

int write_maptable(word_t lbn);

#endif
