/*
 * ramDevice.h
 * NVM Simulator: RAM backed block device driver
 * 
 * 
 */

#ifndef __NVMSIM_RAMDISK_H
#define __NVMSIM_RAMDISK_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/major.h>
#include <linux/genhd.h> // gendisk
#include <linux/bio.h>
#include <linux/blkdev.h> // blk_queue_xx
#include <linux/fs.h>   // block_device
#include <linux/hdreg.h>  // hd_geometry
#include <linux/blk_types.h>
#include <linux/bvec.h>

#include "nvm.h"
#include "mem.h"

//#include "nvmconfig.h"

#define NVM_MAJOR 231
#define SECTOR_SHIFT 9
#define PAGE_SECTORS_SHIFT (PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS (1 << PAGE_SECTORS_SHIFT)

#define HARDSECT_SIZE (512)
#define KERNEL_SECT_SIZE (512)

#define MB_PER_BYTES_SHIFT (20)
#define MB_PER_SECTOR_SHIFT (11)

#define PARTION_PER_DISK (1)

#define NVMDEV_MEM_MAX_SECTORS (8)
#define NVM_RAMDISK_ONLY (1)



/**
 * The simulated NVM device with  RAM 
 */
struct nvm_device
{

	int nvmdev_number;					// The device number
	unsigned long nvmdev_capacity;		// The capacity in sectors BUG should in bytes?
	u8 *nvmdev_data;					// The backing data store
	spinlock_t nvmdev_lock;				// The lock protecting the data store
	struct request_queue *nvmdev_queue; /// Request queue
	struct gendisk *nvmdev_disk;		/// Disk

	/// The collection of lists the device belongs to
	struct list_head nvmdev_list;

	/// Additional NVM model data
	struct nvm_model *nvmdev_model;
};

/**
 * Allocate and free the NVM device
 */
struct nvm_device *nvm_alloc(int index, unsigned capacity_mb);

void nvm_free(struct nvm_device *device);

/**
 * Binder requet to queue
 * 
 */

static void nvm_make_request(struct request_queue *q, struct bio *bio);

/**
 *  nvmdev_do_bvec
 * 			Process a single request
 */
static int nvm_do_bvec(struct nvm_device *device, struct page *page,
					   unsigned int len, unsigned int off, int rw, sector_t sector);

/** 
 * Copy n bytes to from the NVM to dest starting at the given sector
 */
void __always_inline copy_from_nvm(void *dest, struct nvm_device *device,
								   sector_t sector, size_t n);

void __always_inline copy_to_nvm(struct nvm_device *device,
								 const void *src, sector_t sector, size_t n);

/**
 * Perform I/O control
 */
static int nvm_ioctl(struct block_device *bdev, fmode_t mode,
					 unsigned int cmd, unsigned long arg);

static int nvm_disk_getgeo(struct block_device *bdev,
						   struct hd_geometry *geo);
#endif