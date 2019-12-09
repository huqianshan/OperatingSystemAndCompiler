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
	device->nvmdev_capacity = capacity_mb << MB_PER_SECTOR_SHIFT; // in MB
	spin_lock_init(&device->nvmdev_lock);

	// Allocate the NVM model

	//TODO 1
	device->nvmdev_model = pcm_model_allocate(device->nvmdev_capacity);
	if (!device->nvmdev_model)
		goto out_free_struct;

	// Allocate the backing store

	device->nvmdev_data = vmalloc(capacity_mb << MB_PER_BYTES_SHIFT);
	if (!device->nvmdev_data)
		goto out_free_model;

	// Allocate the block request queue by blk_alloc_queue without I/O scheduler

	device->nvmdev_queue = blk_alloc_queue(GFP_KERNEL);
	if (!device->nvmdev_queue)
		goto out_free_dev;

	// register nvmdev_queue,
	blk_queue_make_request(device->nvmdev_queue, nvmdev_make_request);

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
	disk->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
	sprintf(disk->disk_name, "pcm%d", index);
	set_capacity(disk, capacity_mb <<MB_PER_SECTOR_SHIFT);

	return device;

	// Cleanup on error

out_free_queue:
	blk_cleanup_queue(device->nvmdev_queue);
out_free_dev:
	vfree(device->nvmdev_data);
out_free_model:
	pcm_model_free(device->nvmdev_model);
out_free_struct:
	kfree(device);
out:
	return NULL;
}

/**
 * Process pending requests from the queue
 */
static int nvm_make_request(struct request_queue *q, struct bio *bio)
{
	// bio->bi_bdev has been discarded
	//struct block_device *bdev = bio->bi_bdev;
	//struct pcmsim_device *pcmsim = bdev->bd_disk->private_data;

	struct gendisk *disk = bio->bi_disk;
	struct nvm_device *nvm_dev = disk->private_data;
	int rw;
	struct bio_vec *bvec;
	sector_t sector;
	int i;
	int err = -EIO;
	unsigned capacity;

	// Check the device capacity
	// bi_sector,bi_size has moved to bio->bi_iter.bi_sector
	// TODO the judge condition
	sector = bio->bi_iter.bi_sector;
	capacity = get_capacity(disk);
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

	bio_for_each_segment(bvec, bio, i)
	{
		unsigned int len = bvec->bv_len;
		err = nvmdev_do_bvec(nvm_dev, bvec->bv_page, len, bvec->bv_offset, rw, sector);
		if (err){

		}
			break;
		sector += len >> SECTOR_SHIFT;
	}

	// Cleanup

out:
	bio_endio(bio, err);

	return 0;
}


/**
 * Process a single request
 */
static int nvmdev_do_bvec(struct nvm_device *pcmsim, struct page *page,
                          unsigned int len, unsigned int off, int rw, sector_t sector)
{
	void *mem;
	int err = 0;

	mem = kmap_atomic(page);
	if (rw == READ) {
		copy_from_pcmsim(mem + off, pcmsim, sector, len);
		flush_dcache_page(page);
	}
	else {
		flush_dcache_page(page);
		copy_to_pcmsim(pcmsim, mem + off, sector, len);
	}
	kunmap_atomic(mem);

	return err;
}


/**
 * Copy n bytes to from the PCM to dest starting at the given sector
 */
void __always_inline copy_from_pcmsim(void *dest, struct nvm_device *pcmsim,
									  sector_t sector, size_t n)
{
	const void *pcm;
#ifndef PCMSIM_RAMDISK_ONLY
	unsigned l, o;
#endif

	pcm = pcmsim->nvmdev_data + (sector << SECTOR_SHIFT);

#ifndef PCMSIM_RAMDISK_ONLY
	o = 0;
	while (n > 0) {
		l = n;
		// TODO MAYBE BUGGY AND ERROR
		// o is useless?
		if (l > NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT) 
			l = NVMDEV_MEM_MAX_SECTORS << SECTOR_SHIFT;
		pcm_read(pcmsim->nvmdev_model, ((char*) dest) + o, ((const char*) pcm) + o, l, sector);
		n -= l;
	}
#else
	memory_copy(dest, pcm, n);
#endif
}

/**
 * PCM block device operations
 */
static struct block_device_operations nvmdev_fops = {
	.owner        = THIS_MODULE,
//	.locked_ioctl = pcmsim_ioctl,
};