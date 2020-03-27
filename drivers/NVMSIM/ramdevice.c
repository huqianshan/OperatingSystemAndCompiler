/*
 * ramDevice.C
 * NVM Simulator: RAM based block device driver
 * 
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/major.h>
#include <linux/genhd.h> // gendisk
#include <linux/bio.h>
#include <linux/blkdev.h> // blk_queue_xx
#include <linux/fs.h>     // block_device
#include <linux/hdreg.h>  // hd_geometry
#include <linux/blk_types.h>
#include <linux/bvec.h>
#include <asm/io.h>

#include "mem.h"
#include "ramdevice.h"

/**
 * 
 * -------Module Parameters-------
 * nvm_num_devices
 *      The maximum number of NVM devices
 * 
 * nvmdevices_mutex:
 *      The mutex guarding the list of devices
 */
static int nvm_num_devices = 1;
module_param(nvm_num_devices, int, 0);

/**
 * Size of each NVM disk in MB
 */
int nvm_capacity_mb = 4096;
int nvm_capacity_mb_shift = 12;
uint64_t g_highmem_phys_addr = 0x100000000; /* beginning of the reserved phy mem space (bytes)*/
module_param(nvm_capacity_mb, int, 0);
MODULE_PARM_DESC(nvm_capacity_mb, "Size of each NVM disk in MB");
//word_t map_table_size= (nvm_capacity_mb << (MB_PER_BYTES_SHIFT - SECTOR_SHIFT - MAP_PER_SECTORS_SHIFT));
word_t map_table_size = 262144;
word_t bit_table_size = 8388608;
/**
 * The list and mutex of NVM devices
 */

static LIST_HEAD(nvm_list_head);
static DEFINE_MUTEX(nvm_devices_mutex);

/**
 * nvm_devices_name:
 *      The name of device
 */
#define NVM_DEVICES_NAME "nvm"

/**
 * NVM block device operations
 */
static const struct block_device_operations nvmdev_fops = {
    .owner = THIS_MODULE,
    .getgeo = nvm_disk_getgeo,
};

/**
 *  Maptable functions
 */
word_t *init_maptable(word_t size)
{

    word_t table_num = size + 1;
    unsigned long v_size = (unsigned long)table_num * sizeof(word_t);
    word_t *tem = vmalloc(v_size);

    if (tem == NULL)
    {
        printk(KERN_ERR "NVMSIM: %s(%d) init maptable size: %u failed  \n",
               __FUNCTION__, __LINE__, table_num);
    }
    else
    {
        memset(tem, 0, v_size);
        printk(KERN_INFO "NVMSIM: %s(%d) init maptbale size: %u success addr: %x\n",
               __FUNCTION__, __LINE__, table_num, tem);
    }
    return tem;
}

int update_maptable(word_t *map_table, word_t index, word_t key)
{
    if (index > map_table_size)
    {
        printk(KERN_ERR "NVMSIM: %s(%d) index %u exceed %u,update maptble failed\n",
               __FUNCTION__, __LINE__, index, map_table_size);
        return 0;
    }
    map_table[index] = key;
    return 1;
}

word_t get_maptable(word_t *map_table, word_t lbn)
{
    if (lbn > map_table_size || (lbn < 0))
    {
        printk(KERN_ERR "NVMSIM: %s(%d) index %u exceed %u,gete maptble failed\n",
               __FUNCTION__, __LINE__, lbn, map_table_size);
    }
    return map_table[lbn];
}

int map_table(word_t *map_table, word_t lbn, word_t pbn)
{
    word_t key, num, newkey;
    // check for return value
    key = get_maptable(map_table, lbn);
    // if key=0 indicates lbn not mapping
    num = ACCESS_TIME(key) + 1;

    newkey = MAKE_KEY(pbn + 1, num);
    if ((update_maptable(map_table, lbn, newkey) == 0))
    {
        return -1;
    }
    return num;
}

int demap_maptable(word_t *map_table, word_t lbn)
{
    word_t key = get_maptable(map_table, lbn);
    word_t access_num = ACCESS_TIME(key);

    if (update_maptable(map_table, lbn, access_num) <= 0)
    {
        return 0;
    }
    return 1;
}
void print_maptable(word_t *map_table, word_t lbn)
{
    word_t i, tem, pbn;
    printk(KERN_INFO "NVMSIM: %s(%d)\n------------Mapping Table-----------------\n", __FUNCTION__, __LINE__);
    printk(KERN_INFO "    lbn      pbn      accesstime            \n");
    for (i = 0; i <= lbn; i++)
    {
        tem = get_maptable(map_table, i);
        pbn = PHY_SEC_NUM(tem);
        if (i % 1000 == 0 && pbn != 0)
        // bug ,=0->0 not show ;solved, pbn begin with [1,size]
        {
            printk(KERN_INFO "%8u%8u%8u\n", i, pbn, ACCESS_TIME(tem));
        }
    }
}
word_t extract_maptbale(word_t *map_table, word_t table_size, word_t **arr, word_t **index)
{
    word_t n = 0;
    word_t tem, pbn;
    // begin with 1 bug
    int i = 0;
    for (i; i < table_size; i++)
    {
        tem = get_maptable(map_table, i);
        pbn = PHY_SEC_NUM(tem);
        if (pbn != 0)
        {
            n++;
        }
    }
    word_t *tem_arr = vmalloc(sizeof(word_t) * n);
    word_t *new_arr = vmalloc(sizeof(word_t) * n);
    word_t j = 0;
    i = 0;
    for (i; i < table_size; i++)
    {
        tem = get_maptable(map_table, i);
        pbn = PHY_SEC_NUM(tem);
        if (pbn != 0)
        {
            tem_arr[j] = tem;
            new_arr[j] = i;
            j++;
        }
    }
    *arr = tem_arr;
    *index = new_arr;
    return n;
}

/**
 * BitMap function
 */

word_t *init_bitmap(word_t num)
{
    word_t size = (num / BIT_WIDTH_IN_BITS) + 1; // ceil
    word_t *tem = vmalloc(size * sizeof(word_t));
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
word_t query_bitmap(word_t *bitmap, word_t pos);
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
    word_t size = (len / BIT_WIDTH_IN_BITS) + 1; //ceilq

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
/**
 * Allocate the NVM device
 * 	  1. nvm_alloc() : allocates disk and driver 
 *    2. nvm_highmem_map() : make mapping for highmem physical address by ioremap()
 */
void *nvm_highmem_map(void)
{
    // https://patchwork.kernel.org/patch/3092221/
    if ((g_highmem_virt_addr = ioremap_cache(g_highmem_phys_addr, g_highmem_size)))
    {

        g_highmem_curr_addr = g_highmem_virt_addr;
        printk(KERN_INFO "NVMSIM: high memory space remapped (offset: %llu MB, size=%lu MB)\n",
               BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
        return g_highmem_virt_addr;
    }
    else
    {
        printk(KERN_ERR "NVMSIM: %s(%d) %llu Bytes:%x failed remapping high memory space (offset: %llu MB size=%llu MB)\n",
               __FUNCTION__, __LINE__, g_highmem_phys_addr, g_highmem_virt_addr, BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
        return NULL;
    }
}

void nvm_highmem_unmap(void)
{
    /* de-remap the high memory from kernel address space */
    if (g_highmem_virt_addr)
    {
        iounmap(g_highmem_virt_addr);
        g_highmem_virt_addr = NULL;
        printk(KERN_INFO "NVMSIM: unmapping high mem space (offset: %llu MB, size=%lu MB)is unmapped\n",
               BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
    }
    return;
}

static void *hmalloc(uint64_t bytes)
{
    void *rtn = NULL;

    /* check if there is still available reserve high memory space */
    if (bytes <= PMBD_HIGHMEM_AVAILABLE_SPACE)
    {
        rtn = g_highmem_curr_addr;
        g_highmem_curr_addr += bytes;
    }
    else
    {
        printk(KERN_ERR "NVMSIM: %s(%d) - no available space (< %llu bytes) in reserved high memory\n",
               __FUNCTION__, __LINE__, bytes);
    }
    return rtn;
}

struct nvm_device *nvm_alloc(int index, unsigned capacity_mb)
{
    struct nvm_device *device;
    struct gendisk *disk;

    // Allocate the device
    device = kzalloc(sizeof(struct nvm_device), GFP_KERNEL);
    if (!device)
        goto out;
    device->nvmdev_number = index;
    device->nvmdev_capacity = capacity_mb << MB_PER_SECTOR_SHIFT; // in Sectors
    spin_lock_init(&device->nvmdev_lock);
    // maptable bitmap init
    spin_lock_init(&device->map_lock);
    spin_lock_init(&device->bit_lock);
    device->MapTable = init_maptable(map_table_size);
    if (device->MapTable == NULL)
    {
        printk(KERN_ERR "NVMSIM: %s(%d): maptable space allocation %u failed\n",
               __FUNCTION__, __LINE__, map_table_size);
        goto out_free_struct;
    }
    else
    {
        printk(KERN_INFO "NVMSIM: %s(%d): maptable space allocation %u Success\n",
               __FUNCTION__, __LINE__, map_table_size);
    }

    device->BitMap = init_bitmap(bit_table_size);
    if (device->BitMap == NULL)
    {
        printk(KERN_ERR "NVMSIM: %s(%d): BitMap space allocation %u failed\n",
               __FUNCTION__, __LINE__, map_table_size);
        goto out_free_struct;
    }
    else
    {
        printk(KERN_INFO "NVMSIM: %s(%d): BitMap space allocation %u Success\n",
               __FUNCTION__, __LINE__, map_table_size);
    }
    // vmaloc allocate in size bytes
    if (NVM_USE_HIGHMEM())
    {
        device->nvmdev_data = hmalloc(device->nvmdev_capacity << SECTOR_SHIFT);
    }
    else
    {
        device->nvmdev_data = vmalloc(device->nvmdev_capacity << SECTOR_SHIFT);
    }

    if (device->nvmdev_data != NULL)
    {
#if 0
		/* FIXME: No need to do this. It's slow, system could be locked up */
		memset(pmbd->mem_space, 0, pmbd->sectors * pmbd->sector_size);
#endif
        printk(KERN_INFO "NVMSIM: %s(%d):  created [%lu : %llu MBs]\n", __FUNCTION__, __LINE__,
               (unsigned long)device->nvmdev_data, SECTORS_TO_MB(device->nvmdev_capacity));
    }
    else
    {
        printk(KERN_ERR "NVMSIM: %s(%d): NVM space allocation failed\n", __FUNCTION__, __LINE__);
        goto out_free_struct;
    }

    // Allocate the block request queue by blk_alloc_queue without I/O scheduler
    device->nvmdev_queue = blk_alloc_queue(GFP_KERNEL);
    if (!device->nvmdev_queue)
    {
        goto out_free_dev;
    }
    // register nvmdev_queue,
    blk_queue_make_request(device->nvmdev_queue, (make_request_fn *)nvm_make_request);

    //blk_queue_max_hw_sectors(device->nvmdev_queue, 255);//set max sectors for a request for this queue

    //set logical block size for the queue
    blk_queue_logical_block_size(device->nvmdev_queue, HARDSECT_SIZE);
    // Allocate the disk device /* cannot be partitioned */
    device->nvmdev_disk = alloc_disk(PARTION_PER_DISK);
    disk = device->nvmdev_disk;
    if (!disk)
        goto out_free_queue;
    disk->major = NVM_MAJOR;
    disk->first_minor = index;
    disk->fops = &nvmdev_fops;
    disk->private_data = device;
    disk->queue = device->nvmdev_queue;
    //disk->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
    sprintf(disk->disk_name, "nvm%d", index);

    // in sectors
    set_capacity(disk, capacity_mb << MB_PER_SECTOR_SHIFT);
    return device;

    // Cleanup on error
out_free_queue:
    blk_cleanup_queue(device->nvmdev_queue);
out_free_dev:
    vfree(device->nvmdev_data);
out_free_struct:
    kfree(device);
out:
    return NULL;
}

/**
 * Free a NVM device
 */
void nvm_free(struct nvm_device *device)
{
    put_disk(device->nvmdev_disk);
    blk_cleanup_queue(device->nvmdev_queue);

    if (device->nvmdev_data != NULL)
    {
        if (NVM_USE_HIGHMEM())
        {
            hfree(device->nvmdev_data);
        }
        else
        {
            vfree(device->nvmdev_data);
        }
    }

    if (device->MapTable != NULL)
    {
        print_maptable(device->MapTable, map_table_size);
        vfree(device->MapTable);
        printk(KERN_INFO "NVMSIM: %s(%d): maptable space free %u success\n",
               __FUNCTION__, __LINE__, map_table_size);
    }

    if (device->BitMap != NULL)
    {
        printb_bitmap(device->BitMap, 128);
        print_summary_bitmap(device->BitMap, bit_table_size);
        vfree(device->BitMap);
        printk(KERN_INFO "NVMSIM: %s(%d): BitMap space free %u success\n",
               __FUNCTION__, __LINE__, bit_table_size);
    }
    kfree(device);
}

/**
 * Process pending requests from the queue
 */
static void nvm_make_request(struct request_queue *q, struct bio *bio)
{
    // bio->bi_bdev has been discarded
    //struct block_device *bdev = bio->bi_bdev;
    //struct nvm_device *device = bdev->bd_disk->private_data;
    struct nvm_device *nvm_dev = bio->bi_disk->private_data;

    int rw;
    int err = -EIO;
    sector_t sector;
    unsigned capacity;

    struct bio_vec bvec;
    struct bvec_iter iter;

    // Check the device capacity
    // bi_sector,bi_size has moved to bio->bi_iter.bi_sector
    // TODO the judge condition and out information

    // bi_iter.bi_size is the number of remained bi_vec
    // logcial block number lbn
    sector = bio->bi_iter.bi_sector;
    capacity = get_capacity(bio->bi_disk);
    if (sector + (bio->bi_iter.bi_size >> SECTOR_SHIFT) > capacity)
        goto out;

    // Get the request vector
    rw = bio_data_dir(bio);
    //printk(KERN_INFO "NVMSIM: %s(%d) sector: %lu rw: %d\n",
    //	   __FUNCTION__, __LINE__, sector, rw);
    // Perform each part of a request
    if (rw == WRITE)
    {
        word_t lbn = (sector >> MAP_PER_SECTORS_SHIFT);
        word_t num = map_table(nvm_dev->MapTable, lbn, lbn);
        if (num != -1)
        {
            printk(KERN_INFO "maptbale secotr: %u lbn %u times %u success\n",
                   sector, lbn, num);
        }
        word_t bit_sec = sector + 1;
        if (BOOL(bit_sec, nvm_dev->BitMap) == 0)
        {
            SET_BITMAP(bit_sec, nvm_dev->BitMap);
            printk(KERN_INFO "BitMap sector %u actual %u\n", sector, bit_sec);
        }
    }
    bio_for_each_segment(bvec, bio, iter)
    {
        unsigned int len = bvec.bv_len;
        // Every biovec means a SEGMENT of a PAGE
        err = nvm_do_bvec(nvm_dev, bvec.bv_page, len, bvec.bv_offset, rw, sector);
        if (err)
        {
            printk(KERN_WARNING "Transfer data in Segments %x of address %x failed\n", len, bvec.bv_page);
            break;
        }
        sector += len >> SECTOR_SHIFT;
        //printk(KERN_INFO "NVMSIM: %s(%d) sector: %lu rw: %d bvec.bv_len :%u bvec.bv_page :%x bvec.bv_offset %u\n",
        //	   __FUNCTION__, __LINE__, sector, rw, len, bvec.bv_page, bvec.bv_offset);
    }
out:
    bio_endio(bio);
    return;
}

/**
 * Process a single request
 */
static int nvm_do_bvec(struct nvm_device *device, struct page *page,
                       unsigned int len, unsigned int off, int rw, sector_t sector)
{
    void *mem;
    int err = 0;

    // map to kernel address
    mem = kmap_atomic(page);
    if (rw == READ)
    {
        copy_from_nvm(mem + off, device, sector, len);
        flush_dcache_page(page); // if D-cache aliasing is not an issue
    }
    else
    {
        flush_dcache_page(page);
        copy_to_nvm(device, mem + off, sector, len);
    }
    kunmap_atomic(mem);

    return err;
}

/**
 * Copy n bytes to from the NVM to dest starting at the given sector
 */
void __always_inline copy_from_nvm(void *dest, struct nvm_device *device,
                                   sector_t sector, size_t n)
{
    const void *nvm;
    nvm = device->nvmdev_data + (sector << SECTOR_SHIFT);
    memory_copy(dest, nvm, n);
}

void __always_inline copy_to_nvm(struct nvm_device *device,
                                 const void *src, sector_t sector, size_t n)
{
    void *nvm;
    nvm = device->nvmdev_data + (sector << SECTOR_SHIFT);
    memory_copy(nvm, src, n);
}

/**
 * Perform I/O control
 */
static int nvm_ioctl(struct block_device *bdev, fmode_t mode,
                     unsigned int cmd, unsigned long arg)
{
    return -ENOTTY;
}

static int nvm_disk_getgeo(struct block_device *bdev,
                           struct hd_geometry *geo)
{
    // bug to check size and geo argu
    long size;
    struct nvm_device *dev = bdev->bd_disk->private_data;

    size = dev->nvmdev_capacity * (HARDSECT_SIZE / KERNEL_SECT_SIZE);
    geo->cylinders = (size & ~0x3f) >> 6;
    geo->heads = 4;
    geo->sectors = 16;
    geo->start = 4;
    return 0;
}

/**
 * The driver function for NVM Block Drivers
 * 
 * Contains :
 *          1. nvm_init()
 *          2. nvm_exit()
 * */

static int __init nvm_init(void)
{
    int i;
    struct nvm_device *device, *next;

    g_highmem_size = (u64)(nvm_capacity_mb) << MB_PER_BYTES_SHIFT;
    printk(KERN_INFO "NVMSIM: [%s() %d] g_highmem_size %llu bytes g_highmemaddr %llu ; nvm_capacity_mb %llu MB \n",
           __FUNCTION__, __LINE__, g_highmem_size, g_highmem_phys_addr, nvm_capacity_mb, LLONG_MAX);

    // remap the highmem physical address
    if (NVM_USE_HIGHMEM())
    {
        if (nvm_highmem_map() == NULL)
            return -ENOMEM;
    }

    // register a block device number
    if (register_blkdev(NVM_MAJOR, NVM_DEVICES_NAME) != 0)
    {
        printk(KERN_INFO "The device major number %d is occupied\n", NVM_MAJOR);
        return -EIO;
    }

    // allocate block device and gendisk
    for (i = 0; i < nvm_num_devices; i++)
    {
        device = nvm_alloc(i, nvm_capacity_mb);
        if (!device)
            goto out_free;
        // initialize a request queue
        list_add_tail(&device->nvmdev_list, &nvm_list_head);
    }

    // Register block devices's gendisk
    list_for_each_entry(device, &nvm_list_head, nvmdev_list)
    {
        add_disk(device->nvmdev_disk);
    }
    printk(KERN_INFO "NVMSIM [%s() %d]: module loaded\n", __FUNCTION__, __LINE__);
    return 0;

out_free:
    list_for_each_entry_safe(device, next, &nvm_list_head, nvmdev_list)
    {
        list_del(&device->nvmdev_list);
        nvm_free(device);
    }
    unregister_blkdev(NVM_MAJOR, NVM_DEVICES_NAME);
    return -ENOMEM;
}

/**
 * Delete a device
 */
static void nvm_del_one(struct nvm_device *device)
{
    list_del(&device->nvmdev_list);
    del_gendisk(device->nvmdev_disk);
    nvm_free(device);
}

/**
 * Deinitalize a module
 */
static void __exit nvm_exit(void)
{
    unsigned long range;
    struct nvm_device *nvmsim, *next;

    range = nvm_num_devices ? nvm_num_devices : 1UL << (MINORBITS - 1);

    list_for_each_entry_safe(nvmsim, next, &nvm_list_head, nvmdev_list)
    {
        nvm_del_one(nvmsim);
    }

    blk_unregister_region(MKDEV(NVM_MAJOR, 0), range);
    unregister_blkdev(NVM_MAJOR, NVM_DEVICES_NAME);
    printk(KERN_ERR "NVMSIM: %s(%d) -Exited\n",
           __FUNCTION__, __LINE__);
}

/**
 * NVM Module declarations
 */
module_init(nvm_init);
module_exit(nvm_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(NVM_MAJOR);
MODULE_AUTHOR("HuQianshan <hujinlei1999@qq.com>");
MODULE_ALIAS("NVM");
MODULE_VERSION("0.1");