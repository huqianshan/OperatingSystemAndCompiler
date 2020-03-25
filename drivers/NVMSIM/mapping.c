#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//#include "bit_map.h"
#include "mapping.h"

int init_maptable(word_t size)
{
    //word_t page_num = size / PAGE_SIZE;
    word_t page_num = size + 1;
    word_t *tem = calloc(page_num, BIT_WIDTH_IN_BYTES);

    if (tem == NULL)
    {
        printf("malloc for maptable failed pagenum:%u\n", page_num);
        return -4;
    }
    MapTable = tem;
    printf("Maptable address: 0x%x table size: %u\n", MapTable, page_num);
    return 0;
}

int init_indextable(word_t size)
{
    word_t page_num = size / PAGE_SIZE;
    word_t *tem = calloc(page_num, BIT_WIDTH_IN_BITS);

    if (tem == NULL)
    {
        printf("malloc for indexTable failed pagenum:%u\n", page_num);
        return -4;
    }
    IndexTable = tem;
    printf("Indextable address: 0x%x table size: %u\n", MapTable, page_num);
    return 0;
}

int update_maptable(word_t index, word_t key)
{
    // check for the valid parameter index bug
    if (index > (TOTAL_PAGES))
    {
        printf("update for maptable failed index %u too big\n", index);
        return -6;
    }
    // maybe make sure the data recovery here
    MapTable[index] = key;
    return 0;
}

word_t get_maptable(word_t lbn)
{
    // check
    if (lbn > (TOTAL_SECTORS) || lbn < 0)
    {
        printf("get for maptable failed index %u too big\n", lbn);
        return -7; // bug return type bug
    }
    // get key by logical page number
    return MapTable[lbn];
}

int map_maptable(word_t lbn, word_t pbn)
{
    word_t key, num, newkey;
    // check for return value
    key = get_maptable(lbn);
    // if key=0 indicates lbn not mapping
    num = ACCESS_TIME(key) + 1;

#ifdef PAGE_MAPPING
    //Bug check for overflow
    newkey = MAKE_KEY(pbn, num);
    if (update_maptable(lbn, newkey) != 0)
    {
        printf("update accesstime in maptable  failed\n");
        return -9;
    }
    printf("Map: lbn   pbn   key   num   newkey\n");
    printf("     %-6u%-6u%-6u%-6u%-6u\n", lbn, pbn, key, num, newkey);
    return num;
#endif
    return 0;
}

int demap_maptable(word_t lbn)
{
    word_t key = get_maptable(lbn);
    word_t access_num = ACCESS_TIME(key);

    if (update_maptable(lbn, access_num) != 0)
    {
        printf("Demap in maptable failed lbn:accessTime [%u]:[%u]\n",
               lbn, access_num);
        return -7;
    }
    return 0;
}

int print_maptable(word_t lbn)
{
    word_t i, tem, pbn;
    printf("------------Mapping Table-----------------\n");
    printf("    lbn      pbn      accesstime            \n");
    for (i = 0; i <= lbn; i++)
    {
        tem = get_maptable(i);
        pbn = PHYSICAL_PAGE_NUM(tem);
        if (pbn != 0)
        // bug ,=0->0 not show ;solved, pbn begin with [1,size]
        {
            printf("%8u%8u%8u\n", i, pbn, ACCESS_TIME(tem));
        }
    }
}

int index_indextable()
{
    if (IndexTable == NULL)
    {
        printf("can't find index,indextable is null\n");
        return -8;
    }
    word_t i = 0;
    word_t tem = IndexTable[i];
    while (tem != 0)
    {
        i++;
        tem = IndexTable[i];
    }
    return i;
}

word_t extract_maptable(word_t table_size, word_t **arr,word_t **index)
{

    word_t n = 0;
    word_t tem, pbn;
    // begin with 1 bug
    for (int i = 0; i < table_size; i++)
    {
        tem = get_maptable(i);
        pbn = PHYSICAL_PAGE_NUM(tem);
        if (pbn != 0)
        {
            n++;
        }
    }
    printf("function %s line %d var n:%u\n", __FUNCTION__, __LINE__, n);
    word_t *tem_arr = malloc(sizeof(uint32_t) * n);
    word_t *new_arr = malloc(sizeof(uint32_t) * n);
    word_t j = 0;
    for (int i = 0; i < table_size; i++)
    {
        tem = get_maptable(i);
        pbn = PHYSICAL_PAGE_NUM(tem);
        if (pbn != 0)
        {
            tem_arr[j] = tem;
            new_arr[j] = i;
            j++;
        }
    }
    *arr = tem_arr;
    *index = new_arr;
    printf("func arr %x index %x\n", *arr,*index);
    return n;
}

