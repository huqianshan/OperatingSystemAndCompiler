#include "bitmap.h"
/**
 * BitMap function implementation
 */

word_t *init_bitmap(word_t num)
{
    word_t size, *tem;
    unsigned long v_size;
    size = (num / BIT_WIDTH_IN_BITS) + 1;
    v_size = (unsigned long)size * sizeof(word_t);
    tem = vzalloc(v_size);
    if (tem == NULL)
    {
        printk(KERN_ERR "NVMSIM: %s(%d) init BitMap size: %u failed  \n",
               __FUNCTION__, __LINE__, size);
    }
    else
    {
        printk(KERN_INFO "NVMSIM: %s(%d) init BitMap size: %u success addr: %x\n",
               __FUNCTION__, __LINE__, size, tem);
    }
    return tem;
}

word_t query_bitmap(word_t *bitmap, word_t pos){
    return 0;
};

word_t bitCount(word_t x)
{
    word_t c = 0;
    word_t v = x;
    c = (v & 0x55555555) + ((v >> 1) & 0x55555555);
    c = (c & 0x33333333) + ((c >> 2) & 0x33333333);
    c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
    c = (c & 0x00FF00FF) + ((c >> 8) & 0x00FF00FF);
    c = (c & 0x0000FFFF) + ((c >> 16) & 0x0000FFFF);
    return c;
}

void printb_bitmap(word_t *bitmap, word_t len)
{
    word_t i;
    word_t tem;
    printk(KERN_INFO "\nBitMap Information\n");
    for (i = 0; i < len; i++)
    {
        tem = BOOL(i, bitmap);
        printk(KERN_INFO "%u", tem);

        if ((i + 1) % (BIT_WIDTH_IN_BITS * 2) == 0)
        {
            printk(KERN_INFO "\n");
        }
        else if ((i + 1) % BIT_WIDTH_IN_BITS == 0)
        {
            printk(KERN_INFO "    ");
        }
        else if ((i + 1) % 8 == 0)
        {
            printk(KERN_INFO "  ");
        }
        else if ((i + 1) % 4 == 0)
        {
            printk(KERN_INFO " ");
        }
    }
}
void print_summary_bitmap(word_t *bitmap, word_t len)
{
    word_t size = (len / BIT_WIDTH_IN_BITS) + 1; // ceilq

    // calcaulate every usage of int in bitmap
    word_t tem = 0;
    word_t i, total = 0;
    for (i = 0; i < size; i++)
    {
        tem = bitCount(bitmap[i]);
        total += tem;
    }
    printk(KERN_INFO "\nUsed bits: %d     Free bits %d \n", total, len - total);
}