/*
* module.c
* 
*
* Description:
*            The block driver for NVM
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <fs.h>
#include <linux/blkdev.h> // new version for cmd_type
#include <err.h>


#include "ramDevice.h"
/**
 * 
 * -------Module Parameters-------
 * nvm_num_devices
 *      The maximum number of PCM devices
 * 
 * nvmdevices_mutex:
 *      The mutex guarding the list of devices
 */
static int nvm_num_devices = 1;

module_param(nvm_num_devices, int, 0)
    //MODULE_PARM_DESC()

 static DEFINE_MUTEX(nvmdevices_mutex);

/**
 * 
 * -------Module Define-------
 * nvm_devices_name:
 *      The name of device
 */
#define NVM_DEVICES_NAME "NVMSIM"

/**
 * 
 * The driver function for NVM Block Drivers
 * 
 * Contains :
 *          1. nvm_init()
 *          2. nvm_exit()
 * 
 * */

/**
 * nvm_init()
 * 
 * Description:
 *             Initialize a module
 */

static int __init nvm_init(void)
{

    // register a block device
    if (register_blkdev(NVM_MAJOR,NVM_DEVICES_NAME)<0){
        err = -EIO;
    }

    // initialize a request queue

    // initialize a gendisk
}

/**
 * nvm_exit()
 * 
 * Description:
 *             Deinitialize a module
 */
static int __init nvm_init(void)
{
}

/**
 * NVM Module declarations
 */
module_init(nvm_init);
module_exit(nvm_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(PCMSIM_MAJOR);
MODULE_ALIAS("pcm")
