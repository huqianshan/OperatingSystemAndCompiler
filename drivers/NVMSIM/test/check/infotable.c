#include "infotable.h"

/**
 * Access Information Table Implementation
*/

word_t *init_infotable(word_t size)
{
    word_t v_size;
    word_t *tem;

    v_size = size * (sizeof(word_t));
    tem = vzalloc(v_size);

    if (tem == NULL)
    {
        printk(KERN_ERR "NVMSIM: [%s(%d)]: init InfoTable size: %u failed  \n",
               __FUNCTION__, __LINE__, size);
    }
    else
    {
        printk(KERN_INFO
               "NVMSIM: [%s(%d)]: init InfoTable size: %u success addr: %x\n",
               __FUNCTION__, __LINE__, size, tem);
    }
    return tem;
}

int update_infotable(word_t *InfoTable, word_t lbn)
{
    // exceed the info table range
    if (lbn >= info_table_size)
    {
        return 1;
    }

    InfoTable[lbn] += 1;
    return 0;
}

word_t get_infotable(word_t *InfoTable, word_t lbn)
{
    // exceed the info table range
    if (lbn >= info_table_size || InfoTable == NULL)
    {
        return 0;
    }

    return InfoTable[lbn];
}

int destroy_infotable(word_t *InfoTable)
{
    if (InfoTable != NULL)
    {
        vfree(InfoTable);
        printk(KERN_INFO "NVM_SIM [%s(%d)]: free Infotable \n", __FUNCTION__,
               __LINE__);
        return 0;
    }
    return 1;
}

void print_infotable(word_t *InfoTable, word_t size, word_t step)
{
    if (size >= info_table_size)
    {
        return;
    }
    word_t i, tem;
    printk(KERN_INFO "NVM_SIM [%s(%d)]: Information Table Summary \n",
           __FUNCTION__, __LINE__);
    printk(KERN_INFO "   LBN    AccessTime\n", __FUNCTION__, __LINE__);
    for (i = 0; i < size; i += step)
    {
        tem = get_infotable(InfoTable, i);
        if (tem != 0)
        {
            printk(KERN_INFO " %6u  %6u \n", i, tem);
        }
    }
}