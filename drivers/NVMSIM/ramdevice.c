/*
 * ramDevice.C
 * NVM Simulator: RAM backed block device driver
 * 
 * Description:
 *            For the device of NVM block driver
 * 
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/major.h>

#include "ramDevice.h"
