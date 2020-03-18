#include<stdio.h>
#include<stdlib.h>

#include "bit_map.h"
#include "mapping.h"

int  init_maptable(word_t size){
    word_t page_num = size / PAGE_SIZE;
    word_t *tem = calloc(page_num, sizof(word_t));

    if(tem==NULL){
        printf("malloc for maptable failed pagenum:%u\n", page_num);
        return -4;
    }
    MapTable = tem;
      printf("Maptable address: 0x%x table size: %u\n", MapTable, page_num);
  return 0;
}

int update_maptable(word_t index,word_t key){
    // check for the valid parameter
    if(index>(TOTAL_PAGES)){
        printf("update for maptable failed index %u too big\n", index);
        return -6;
    }

    // maybe make sure the data recovery here
    MapTable[index] = key;
    return 0;
}


word_t get_maptable(word_t lbn){
    // check
    if(lbn>(TOTAL_SECTORS)||lbn<=0){
        printf("get for maptable failed index %u too big\n", lbn);
        return -7;// bug return type bug
    }
    // get key by logical page number
    return MapTable[BLOCK_TO_PAGE(lbn)];
}

word_t read_maptable(word_t lbn){
    word_t key,phy_page_n,phy_block_n;

    // check for return value
    key = get_maptable(lbn);
    phy_page_n = PHYSICAL_PAGE_NUM(key);

    //calcaulate the offset of phy_block
#ifdef PAGE_MAPPING
    phy_block_n = phy_page_n * PAGE_SIZE + block_offset(lbn);
    return phy_block_n;
#endif

    return 0;
}

int write_maptable(word_t lbn){
    word_t key,phy_page_n,phy_block_n;

    // check for return value
    key = get_maptable(lbn);
    // if key=0 indicates lbn not mapping
    phy_page_n = PHYSICAL_PAGE_NUM(key);

    //calcaulate the offset of phy_block
#ifdef PAGE_MAPPING
    phy_block_n = phy_page_n * PAGE_SIZE + block_offset(lbn);

    // update bitmap
    SET_BITMAP(phy_block_n);
    // add access time
    word_t newkey = key + 1;
    if(update_maptable(phy_page_n, newkey)!=0){
        prinf("update accesstime in maptable  failed\n");
        return -9;
    }

    return phy_block_n;
#endif

    return 0;
}    

