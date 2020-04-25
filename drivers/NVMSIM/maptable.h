#ifndef MAP_TABLE_H
#define MAP_TABLE_H

#include "nvmconfig.h"

word_t *init_maptable(word_t size);
int update_maptable(word_t *map_table, word_t lbn, word_t pbn);
word_t get_maptable(word_t *map_table, word_t lbn);
word_t map_table(word_t *map_table, word_t lbn, word_t pbn);
int demap_maptable(word_t *map_table, word_t lbn);
void print_maptable(word_t *map_table, word_t lbn);
void destroy_maptable(word_t *map_table);

#endif