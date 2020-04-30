
#include <linux/slab.h>
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

word_t query_bitmap(word_t *bitmap, word_t pos, word_t len)
{
    if (!bitmap || pos < 0 || pos > len)
        return 0;

    word_t i, rtn;
    rtn = 0;
    for (i = pos; i < len; i++)
    {
        if (BOOL(i, bitmap) == 0)
        {
            rtn = i;
            goto out;
        }
    }

    for (i = 0; i < pos; i++)
    {
        if (BOOL(i, bitmap) == 0)
        {
            rtn = i;
            goto out;
        }
    }
    rtn = 0;
out:
    return rtn;
};

word_t bitCount(word_t x)
{
    word_t c, v;
    c = 0;
    v = x;
    c = (v & 0x55555555) + ((v >> 1) & 0x55555555);
    c = (c & 0x33333333) + ((c >> 2) & 0x33333333);
    c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
    c = (c & 0x00FF00FF) + ((c >> 8) & 0x00FF00FF);
    c = (c & 0x0000FFFF) + ((c >> 16) & 0x0000FFFF);
    return c;
}

word_t pageCount(word_t *bitmap, word_t len)
{
    if (bitmap == NULL)
    {
        return 0;
    }
    word_t i, total;
    total = 0;
    for (i = 0; i < len; i++)
    {
        if (bitmap[i] != 0)
        {
            total++;
        }
    }
    return total;
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

void print_bitmap(word_t *bitmap, word_t len)
{
    char *buffer;
    buffer = kmalloc(64, GFP_KERNEL);
    printk(KERN_INFO "\n     BitMap Information\n");

    word_t rows, j, tem, pos;
    pos = 0;
    for (j = 0; j < len; j++)
    {
        tem = BOOL(j, bitmap);
        buffer[pos++] = tem + '0';
        if ((j + 1) % (BIT_WIDTH_IN_BITS) == 0)
        {
            buffer[pos + 1] = '\n';
            printk(KERN_INFO "%s", buffer);
            pos = 0;
        }
        else if ((j + 1) % 4 == 0)
        {
            buffer[pos++] = ' ';
            buffer[pos++] = ' ';
        }
    }
    if (pos != 0)
    {
        printk(KERN_INFO "%s", buffer);
    }
    kfree(buffer);
}

word_t total_bitmap(word_t *bitmap)
{
    word_t size, tem, i, total;
    
    total = 0;
    size = (bit_table_size / BIT_WIDTH_IN_BITS) + 1;
    for (i = 0; i < size; i++)
    {
        tem = bitCount(bitmap[i]);
        total += tem;
    }
    return total;
}

void print_summary_bitmap(word_t *bitmap, word_t len)
{
    word_t total;
    total = total_bitmap(bitmap);
    printk(KERN_INFO "\nUsed bits: %d     Free bits %d \n", total, len - total);
}

void destroy_bitmap(word_t *bitmap)
{
    if (bitmap)
    {
        vfree(bitmap);
        printk(KERN_INFO "%s [%s(%d)]: free Bitmap \n", __FILE__, __FUNCTION__,
               __LINE__);
    }
}