#ifndef SYNCER_H
#define SYNCER_H

#include "nvmconfig.h"
#include "infotable.h"
#include "bitmap.h"
#include "maptable.h"

void *auto_malloc(word_t size);
void auto_free(void *p, word_t size);
word_t minium(word_t *arr, word_t n);
word_t maxium(word_t *arr, word_t n);
word_t *bigK_index(word_t *arr, word_t n, word_t k);
word_t *smallK_index(word_t *arr, word_t n, word_t k);

word_t extract_big_small(word_t *bitmap, word_t *infotable, word_t page_count, word_t n, word_t k,
                         word_t **big_arr, word_t **small_arr);
word_t extract_small(word_t *bitmap, word_t *infotable, word_t page_count, word_t n, word_t k, word_t **arr);

word_t wear_level(word_t *bitmap, word_t *infotable, word_t *maptable, word_t *lpn, word_t size);

word_t exchange_pbi(word_t *bitmap, word_t *infotable, word_t *maptable, word_t lbn, word_t pbn);
#endif