#include "syncer.h"

void *auto_malloc(word_t size)
{
    unsigned long max_num;
    max_num = (unsigned long)KZALLOC_MAX_BYTES;
    void *p;
    if (size < max_num)
    {
        p = kzalloc(size, GFP_KERNEL);
        if (p == NULL)
        {
            printk(KERN_ERR "NVM_SIM %s%d ERROR: for size %lu autoalloc in kzalloc\n",
                   __FUNCTION__, __LINE__, size);
        }
    }
    else
    {
        p = vzalloc(size);
        printk(KERN_INFO "NVM_SIM [%s(%d)]: Vmalloc for size %lu at %p\n",
               __FUNCTION__, __LINE__, size, p);
        if (p == NULL)
        {
            printk(KERN_ERR "NVM_SIM %s%d ERROR: for size %lu autoalloc in vmalloc\n",
                   __FUNCTION__, __LINE__, size);
        }
    }
    return p;
}

void auto_free(void *p, word_t size)
{
    unsigned long max_num;

    max_num = KZALLOC_MAX_BYTES;
    if (size < max_num)
    {
        printk(KERN_INFO "NVM_SIM [%s(%d)]: kfree size %u p %p\n", __FUNCTION__, __LINE__, size, p);
        kfree(p);
    }
    else
    {
        printk(KERN_INFO "NVM_SIM [%s(%d)]: vmfree size %u p %p\n", __FUNCTION__,
               __LINE__, size, p);
        vfree(p);
    }
}

word_t minium(word_t *arr, word_t n)
{
    if (arr == NULL)
    {
        //bug
        return 0;
    }
    word_t i, min, index;
    min = arr[0];
    index = 0;
    for (i = 0; i < n; i++)
    {
        if (min > arr[i])
        {
            min = arr[i];
            index = i;
        }
    }
    return index;
}

word_t maxium(word_t *arr, word_t n)
{
    if (arr == NULL)
    {
        //bug
        return 0;
    }
    word_t i, max, index;
    max = arr[0];
    index = 0;
    for (i = 0; i < n; i++)
    {
        if (max < arr[i])
        {
            max = arr[i];
            index = i;
        }
    }
    return index;
}

word_t *bigK_index(word_t *arr, word_t n, word_t k)
{
    if (arr == NULL || n <= 0 || k > n || k <= 0)
    {
        goto out;
    }
    word_t *key, *addr, i, min_index, min;
    // tempory store the key
    key = (word_t *)auto_malloc(sizeof(word_t) * k);
    // store the maxium k index in arr
    addr = (word_t *)auto_malloc(sizeof(word_t) * k);
    if ((!key) || (!addr))
    {
        goto free_out;
    }
    for (i = 0; i < k; i++)
    {
        key[i] = arr[i];
        addr[i] = i;
    }

    min_index = minium(key, k);
    min = key[min_index];
    for (i = k; i < n; i++)
    {
        if (arr[i] > min)
        {
            key[min_index] = arr[i];
            addr[min_index] = i;
            min_index = minium(key, k);
            min = key[min_index];
        }
    }

    auto_free(key, sizeof(word_t) * k);
    return addr;
free_out:
    auto_free(key, sizeof(word_t) * k);
    auto_free(addr, sizeof(word_t) * k);
out:
    return NULL;
}

word_t *smallK_index(word_t *arr, word_t n, word_t k)
{
    if (arr == NULL || n <= 0 || k > n || k <= 0)
    {
        goto out;
    }
    word_t *key, *addr, i, max_index, max;
    key = (word_t *)auto_malloc(sizeof(word_t) * k);
    addr = (word_t *)auto_malloc(sizeof(word_t) * k);
    if ((!key) || (!addr))
    {
        goto free_out;
    }
    for (i = 0; i < k; i++)
    {
        key[i] = arr[i];
        addr[i] = i;
    }

    max_index = maxium(key, k);
    max = key[max_index];
    for (i = k; i < n; i++)
    {
        if (arr[i] < max)
        {
            key[max_index] = arr[i];
            addr[max_index] = i;
            max_index = maxium(key, k);
            max = key[max_index];
        }
    }
    auto_free(key, sizeof(word_t) * k);
    return addr;

free_out:
    auto_free(key, sizeof(word_t) * k);
    auto_free(addr, sizeof(word_t) * k);
out:
    return NULL;
}

//  n is the bitmap size in word_t
word_t extract_big(word_t *bitmap, word_t *infotable, word_t n, word_t k, word_t **arr)
{
    if ((!infotable) || (!bitmap))
    {
        goto err;
    }
    word_t page_count;
    page_count = pageCount(bitmap, n);
    printk("page_count %u\n", page_count);

    word_t i, j, avg;
    word_t *marray, *barray;
    j = 0;
    avg = 0;
    // key is logic-page-number
    barray = auto_malloc(page_count * sizeof(word_t));
    //
    marray = auto_malloc(page_count * sizeof(word_t));
    if (!barray || !marray)
    {
        auto_free(marray, page_count * sizeof(word_t));
        auto_free(barray, page_count * sizeof(word_t));
        goto err;
    }
    for (i = 0; i < n; i++)
    {
        if (bitmap[i] != 0)
        {
            barray[j] = i;
            marray[j] = infotable[barray[j]];
            avg += marray[j];
            j++;
        }
    }
    if (j != page_count)
    {
        printk("j %u page_count %u\n", j, page_count);
        goto err;
    }
    word_t *earray;
    earray = bigK_index(marray, page_count, k);
    if (!earray)
        goto err;
    word_t *rarray;
    rarray = auto_malloc(k * sizeof(word_t));
    for (i = 0; i < k; i++)
    {
        rarray[i] = barray[earray[i]];
    }
    *arr = rarray;
    auto_free(barray, page_count * sizeof(word_t));
    auto_free(marray, page_count * sizeof(word_t));
    auto_free(earray, k * sizeof(word_t));

    avg = avg / page_count;
    return avg;
    ;
err:
    return 0;
}

/*
word_t extract_big(word_t *infotable, word_t n, word_t k, word_t **arr)
{
    if ((!infotable))
    {
        goto err;
    }

    word_t *karray;
    karray = bigK_index(infotable, n, k);
    if (!karray)
        goto err;
    *arr = karray;

    return 1;
err:
    return 0;
}*/

word_t wear_level(word_t *bitmap, word_t *infotable, word_t *maptable, word_t *lpn, word_t size)
{
    if (!lpn)
        return 0;
    word_t i, j;
    word_t lbn_begin, lbn_end;
    word_t pbn, next_pbn;
    for (i = 0; i < size; i++)
    {
        lbn_begin = lpn[i] * BIT_WIDTH_IN_BITS;
        lbn_end = (lpn[i] + 1) * BIT_WIDTH_IN_BITS;
        for (j = lbn_begin; j < lbn_end; j++)
        {
            if (BOOL(j, bitmap))
            {
                pbn = maptable[j];
                if (pbn == 0)
                {
                    continue;
                }
                //bug
                next_pbn = query_bitmap(bitmap, pbn, bit_table_size);
                printk("lpn %u lbn %u pbn %u next_free_pbn %u",
                       lpn[i], j, pbn, next_pbn);

                exchange_pbi(bitmap, infotable, maptable, j, next_pbn);
            }
        }
    }
}

word_t exchange_pbi(word_t *bitmap, word_t *infotable, word_t *maptable, word_t lbn, word_t pbn)
{

    CLEAR_BITMAP(lbn, bitmap);
    SET_BITMAP(pbn, bitmap);

    demap_maptable(maptable, lbn);
    map_table(maptable, lbn, pbn);

    // transfer data

    update_infotable(infotable, pbn >> INFO_PAGE_SIZE_SHIFT);
}