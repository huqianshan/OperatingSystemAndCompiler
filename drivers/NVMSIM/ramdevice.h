/*
 * ramdisk.h
 * NVM Simulator: RAM backed block device driver
 * 
 * 
 */


#ifndef __NVMSIM_RAMDISK_H
#define __NVMSIM_RAMDISK_H

//#include "nvm.h"

#define NVMSIM_MAJOR		231
#define SECTOR_SHIFT		9
#define PAGE_SECTORS_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS		(1 << PAGE_SECTORS_SHIFT)


/**
 * The simulated NVM device with a RAM disk backing store
 */
struct nvmsim_device {
	
	/// The device number
	int nvmsim_number;

	/// The capacity in sectors
	unsigned nvmsim_capacity;

	/// The backing data store
	void* nvmsim_data;

	/// The lock protecting the data store
	spinlock_t nvmsim_lock;

	/// Request queue
	struct request_queue* nvmsim_queue;

	/// Disk
	struct gendisk* nvmsim_disk;

	/// The collection of lists the device belongs to
	struct list_head nvmsim_list;

	/// Additional NVM model data
	struct nvm_model* nvmsim_model;
};


/**
 * Allocate the NVM device
 */
struct nvmsim_device* nvmsim_alloc(int index, unsigned capacity_mb);

/**
 * Free a NVM device
 */
void nvmsim_free(struct nvmsim_device *nvmsim);

#endif