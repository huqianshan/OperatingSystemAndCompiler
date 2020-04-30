#include "maptable.h"

/**
 *  MapTable functions
 */
word_t *init_maptable(word_t size)
{
    word_t table_num;
    word_t *tem;
    unsigned long v_size;

    table_num = size + 1;
    v_size = (unsigned long)table_num * sizeof(word_t);
    tem = vzalloc(v_size);
    if (tem == NULL)
    {
        printk(KERN_ERR "NVMSIM: %s(%d) init maptable size: %u failed  \n",
               __FUNCTION__, __LINE__, table_num);
    }
    else
    {
        printk(KERN_INFO "NVMSIM: %s(%d) init MapTable size: %u success addr: %x\n",
               __FUNCTION__, __LINE__, table_num, tem);
    }
    return tem;
};

int update_maptable(word_t *map_table, word_t lbn, word_t pbn)
{
    if (lbn > map_table_size || lbn < 0)
    {
        printk(KERN_ERR "NVMSIM: [%s(%d)] Error index %u exceed %u,update maptble failed\n",
               __FUNCTION__, __LINE__, lbn, map_table_size);
        return 0;
    }
    map_table[lbn] = pbn;
    return 1;
};

word_t get_maptable(word_t *map_table, word_t lbn)
{
    if (lbn > map_table_size || (lbn < 0))
    {
        printk(KERN_ERR "NVMSIM: [%s(%d)] Error index %u exceed %u,gete maptble failed\n",
               __FUNCTION__, __LINE__, lbn, map_table_size);
        return 0; //return -EINVAL; bug
    }
    return map_table[lbn];
};

word_t map_table(word_t *map_table, word_t lbn, word_t pbn)
{
    word_t raw_pbn;
    // check for return value
    raw_pbn = get_maptable(map_table, lbn);
    // if pbn=0 indicates lbn not mapping
    if ((update_maptable(map_table, lbn, pbn) == 0))
    {
        printk(KERN_ERR "NVMSIM: [%s(%d)] Error Map lbn %u for pbn %u failed\n",
               __FUNCTION__, __LINE__, lbn, pbn);
        return 0;
    }
    return raw_pbn;
};

int demap_maptable(word_t *map_table, word_t lbn)
{
    word_t raw_pbn;
    // check for return value
    raw_pbn = get_maptable(map_table, lbn);
    if (raw_pbn == 0)
    {
        return 0;
    }
    // if pbn=0 indicates lbn not mapping
    if ((update_maptable(map_table, lbn, 0) == 0))
    {
        printk(KERN_ERR "NVMSIM: [%s(%d)] Error DeMap lbn %u failed its raw pbn %u n",
               __FUNCTION__, __LINE__, lbn, raw_pbn);
        return 0;
    }
    return raw_pbn;
}

void print_maptable(word_t *map_table, word_t lbn)
{
    word_t i, pbn;
    printk(KERN_INFO
           "NVMSIM: %s(%d)\n------------Mapping Table-----------------\n",
           __FUNCTION__, __LINE__);
    printk(KERN_INFO "    lbn      pbn                 \n");
    for (i = 0; i <= 64; i++)
    {
        pbn = get_maptable(map_table, i);

        if (pbn != 0)
        // bug ,=0->0 not show ;solved, pbn begin with [1,size]
        {
            printk(KERN_INFO "%8u%8u\n", i, pbn);
        }
    }

    for (i = 0; i <= lbn; i += 5000)
    {
        pbn = get_maptable(map_table, i);
        if (pbn != 0)
        // bug ,=0->0 not show ;solved, pbn begin with [1,size]
        {
            printk(KERN_INFO "%8u%8u\n", i, pbn);
        }
    }
};

void destroy_maptable(word_t *map_table)
{
    if (map_table != NULL)
    {
        vfree(map_table);
        printk(KERN_ERR "NVMSIM: [%s(%d)] Destroy Maptable\n",
               __FUNCTION__, __LINE__);
    }
};