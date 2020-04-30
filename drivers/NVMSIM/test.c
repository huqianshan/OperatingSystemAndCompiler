#include <linux/kernel.h>
#include <linux/module.h>

#include "bitmap.h"
#include "infotable.h"
#include "maptable.h"

#define DEVICES_NAME "test"

static int __init test_init(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Init\n", __FUNCTION__, __LINE__);
  destroy_maptable(init_maptable(map_table_size));
  return 0;
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Exit\n", __FUNCTION__, __LINE__);
  return;
}

module_init(test_init);
module_exit(test_exit);
