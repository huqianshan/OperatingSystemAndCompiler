/*
* module.c
* Description:
*            The block driver for NVM
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/blkdev.h> // new version for cmd_type
//#include <vmalloc.h>
//#include <bio.h>

#include "ramdevice.h"
#include "mem.h"
#include "nvm.h"

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
 
#ifndef __LP64__
    static int nvm_capacity_mb = 128;
#else
    
#endif
*/
int nvm_capacity_mb = 1024;

module_param(nvm_capacity_mb, int, 0);
MODULE_PARM_DESC(nvm_capacity_mb, "Size of each NVM disk in MB");

/**
 * The list and mutex of NVM devices
 */
static LIST_HEAD(nvm_list_head);
static DEFINE_MUTEX(nvm_devices_mutex);

/**
 * 
 * -------Module Define-------
 * nvm_devices_name:
 *      The name of device
 */
#define NVM_DEVICES_NAME "nvm"


/**
 * Initialize one device
 */
static struct nvm_device* nvm_init_one(dev_t i)
{
    struct nvm_device *device;

    list_for_each_entry(device, &nvm_list_head, nvmdev_list)
    {
        if (device->nvmdev_number == i)
            goto out;
    }

    device = nvm_alloc(i, nvm_capacity_mb);
    if (device)
    {
        add_disk(device->nvmdev_disk);
        list_add_tail(&device->nvmdev_list, &nvm_list_head);
    }
out:
    return device;
}



/**
 * Probe a device
 */
static struct kobject *nvm_probe(dev_t dev, int *part, void *data)
{
    struct nvm_device *device;
    struct kobject *kobj;

    mutex_lock(&nvm_devices_mutex);
    device = nvm_init_one((unsigned int)dev & MINORMASK);
    kobj = device ? get_disk(device->nvmdev_disk) : ERR_PTR(-ENOMEM);
    mutex_unlock(&nvm_devices_mutex);

    *part = 0;
    return kobj;
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
    unsigned long range;
    struct nvm_device *device, *next;
    // register a block device number
    if (register_blkdev(NVM_MAJOR, NVM_DEVICES_NAME) != 0)
    {
        printk(KERN_INFO "The device major number %d is occupied\n", NVM_MAJOR);
        return -EIO;
    }

    // allocate block device

    for (i = 0; i < nvm_num_devices; i++)
    {
        device = nvm_alloc(i, nvm_capacity_mb);
        if (!device)
            goto out_free;
        // initialize a request queue
        list_add_tail(&device->nvmdev_list, &nvm_list_head);
    }

    // Register block devices
    list_for_each_entry(device, &nvm_list_head, nvmdev_list)
    {
        add_disk(device->nvmdev_disk);
    }
    range = nvm_num_devices ? nvm_num_devices : 1UL << (MINORBITS - 1);
    blk_register_region(MKDEV(NVM_MAJOR, 0), range, THIS_MODULE, nvm_probe, NULL, NULL);

    printk(KERN_INFO "nvm: module loaded\n");
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
}

/**
 * NVM Module declarations
 */
module_init(nvm_init);
module_exit(nvm_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(NVM_MAJOR);
//MODULE_ALIAS("nvm")
