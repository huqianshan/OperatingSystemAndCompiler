/*
 * ramDevice.C
 * NVM Simulator: RAM backed block device driver
 * 
 * Description:
 *            For the device of NVM block driver
 * 
 */

#include "ramdevice.h"

/**
 * NVM block device operations
 */
static const struct block_device_operations nvmdev_fops = {
	.owner = THIS_MODULE,
	.getgeo = nvm_disk_getgeo,
}
;

/**
 * Allocate the NVM device
 */
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

	// Allocate the NVM model

	//TODO 1 MAY BE BUG
	device->nvmdev_model = nvm_model_allocate(device->nvmdev_capacity);
	if (!device->nvmdev_model)
		goto out_free_struct;

	// Allocate the backing store

	// BUG MAY BE WRONG IN SIZE ;ASSUME vmaloc allocate in size bytes
	device->nvmdev_data = vmalloc(device->nvmdev_capacity << SECTOR_SHIFT);
	if (!device->nvmdev_data)
	{
		printk(KERN_WARNING "vmalloc for %d bytes has been failed\n", device->nvmdev_capacity << SECTOR_SHIFT);
		goto out_free_model;
	}
	

	// Allocate the block request queue by blk_alloc_queue without I/O scheduler

	device->nvmdev_queue = blk_alloc_queue(GFP_KERNEL);
	if (!device->nvmdev_queue)
	{
		goto out_free_dev;
	}
	// register nvmdev_queue,
	blk_queue_make_request(device->nvmdev_queue, (make_request_fn *)nvm_make_request);

	//set max sectors for a request for this queue
	blk_queue_max_hw_sectors(device->nvmdev_queue, 255);

	//set logical block size for the queue
	blk_queue_logical_block_size(device->nvmdev_queue, 512);

	// the QUEUE_ORDERED_TAG HAS BEEN REMOVED
	//http://www.jeepxie.net/article/505053.html
	// TODO BUGGY
	//blk_queue_ordered(device->nvmdev_queue, QUEUE_ORDERED_TAG, NULL);
	//blk_queue_flush(device->nvmdev_queue,) also depreasd
	blk_queue_write_cache(device->nvmdev_queue, 0, 0);

	//THIS ATTRIBUTE HAS BEEN SET TO DEFAUTLY
	//blk_queue_bounce_limit(device->nvmdev_queue, BLK_BOUNCE_ANY);
	/*blk_queue_max_sectors (device->nvmdev_queue, 1024);*/

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
out_free_model:
	nvm_model_free(device->nvmdev_model);
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
	nvm_model_free(device->nvmdev_model);
	if (device->nvmdev_data != NULL)
		vfree(device->nvmdev_data);
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
	sector = bio->bi_iter.bi_sector;
	capacity = get_capacity(bio->bi_disk);
	if (sector + (bio->bi_iter.bi_size >> SECTOR_SHIFT) > capacity)
		goto out;

	// Get the request vector
	// bio_rw and READA has been removed
	// https://patchwork.kernel.org/patch/9173331/
	// bio_data_dir == (op_is_write(bio_op(bio)) ? WRITE : READ) 1 0
	/*rw = bio_rw(bio);
	if (rw == READA)
		rw = READ;*/
	rw = bio_data_dir(bio);

	// Perform each part of a request
	bio_for_each_segment(bvec, bio, iter)
	{
		unsigned int len = bvec.bv_len;
		// Every biovec means a SEGMENT of a PAGE
		err = nvm_do_bvec(nvm_dev, bvec.bv_page, len, bvec.bv_offset, rw, sector);
		if (err){
			printk(KERN_WARNING "Transfer data in Segments %x of address %x failed\n", len, bvec.bv_page);
			break;
		}
		sector += len >> SECTOR_SHIFT;
		//TODO BUT MAY BUG secotr not update
	}
out:
	bio_endio(bio);
	return ;
}

/**
 * Process a single request
 */
static int nvm_do_bvec(struct nvm_device *device, struct page *page,
					   unsigned int len, unsigned int off, int rw, sector_t sector)
{
	void *mem;
	int err = 0;

	mem = kmap_atomic(page);
	if (rw == READ)
	{
		copy_from_nvm(mem + off, device, sector, len);
		// if D-cache aliasing is not an issue
		flush_dcache_page(page);
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
#ifndef NVM_RAMDISK_ONLY
	unsigned l, o;
#endif

	nvm = device->nvmdev_data + (sector << SECTOR_SHIFT);

#ifndef NVM_RAMDISK_ONLY
	o = 0;
	while (n > 0)
	{
		l = n;
		// TODO MAYBE BUGGY AND ERROR
		// o is useless?
		if (l > NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT)
			l = NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT;
		nvm_read(device->nvmdev_model, ((char *)dest) + o, ((const char *)nvm) + o, l, sector);
		n -= l;
	}
#else
	memory_copy(dest, nvm, n);
#endif
}

void __always_inline copy_to_nvm(struct nvm_device *device,
								 const void *src, sector_t sector, size_t n)
{
	void *nvm;
#ifndef NVM_RAMDISK_ONLY
	unsigned l, o;
#endif

	nvm = device->nvmdev_data + (sector << SECTOR_SHIFT);

#ifndef NVM_RAMDISK_ONLY
	o = 0;
	while (n > 0)
	{
		l = n;
		if (l > NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT)
			l = NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT;
		nvm_write(device->nvmdev_model, ((char *)nvm) + o, ((const char *)src) + o, l, sector);
		n -= l;
	}
#else
	memory_copy(nvm, src, n);
#endif
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
						   struct hd_geometry *geo){

	long size;
	struct nvm_device *dev = bdev->bd_disk->private_data;

	size = dev->nvmdev_capacity * (HARDSECT_SIZE / KERNEL_SECT_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 4;
	return 0;
}