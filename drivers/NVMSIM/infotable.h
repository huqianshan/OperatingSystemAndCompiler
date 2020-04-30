#ifndef INFO_TABLE_H
#define INFO_TABLE_H

#include "nvmconfig.h"

word_t *init_infotable(word_t size);

// lbn is phy-block-num, infortable key is lbn>> INFO_PAGE_SIZE_SHIFT
int update_infotable(word_t *InfoTable, word_t lbn);

word_t get_infotable(word_t *InfoTable, word_t lbn);

int destroy_infotable(word_t *InfoTable);

void print_infotable(word_t *InfoTable, word_t size, word_t step);

#endif