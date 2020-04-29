/*
 * ramDevice.C
 * NVM Simulator: RAM based block device driver
 */
#include <asm/io.h>
#include <linux/bio.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h> // blk_queue_xx
#include <linux/bvec.h>
#include <linux/delay.h> // msleep
#include <linux/fs.h>    // block_device
#include <linux/genhd.h> // gendisk
#include <linux/hdreg.h> // hd_geometry
#include <linux/init.h>
#include <linux/jiffies.h> // jiffies
#include <linux/kernel.h>
#include <linux/kthread.h> // kthreads
#include <linux/major.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h> //proc
#include <linux/sched.h>   // task_struct

#include "nvmconfig.h"
#include "mem.h"
#include "ramdevice.h"
#include "infotable.h"
#include "bitmap.h"
#include "maptable.h"
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
/* beginning of the reserved phy mem space (bytes)*/
uint64_t g_highmem_phys_addr = 0x100000000;
module_param(nvm_capacity_mb, int, 0);
MODULE_PARM_DESC(nvm_capacity_mb, "Size of each NVM disk in MB");
// word_t map_table_size= (nvm_capacity_mb << (MB_PER_BYTES_SHIFT - SECTOR_SHIFT
// - MAP_PER_SECTORS_SHIFT));

/**
 * The list and mutex of NVM devices
 */

static LIST_HEAD(nvm_list_head);
static DEFINE_MUTEX(nvm_devices_mutex);

/* /proc file system entry */
static struct proc_dir_entry *proc_nvm = NULL;
static struct proc_dir_entry *proc_nvmstat = NULL;
static struct proc_dir_entry *proc_nvmcfg = NULL;

struct file_operations proc_nvmstat_ops = {
    .read = nvm_proc_nvmstat_read,
    .owner = THIS_MODULE,
};

struct file_operations proc_nvmcfg_ops = {
    .read = nvm_proc_nvmcfg_read,
    .owner = THIS_MODULE,
};

struct file_operations proc_nvmdev_ops = {
    .read = nvm_proc_devstat_read,
    .owner = THIS_MODULE,
};
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


static void nvm_auto_free(void *p, word_t size)
{
  auto_free(p, size * (sizeof(word_t)));
}

word_t extract_maptable(word_t *map_table, word_t table_size, word_t **arr,
                        word_t **index)
{
  word_t n, i, j, tem, pbn;
  word_t *tem_arr, *new_arr;
  n = 0;
  // Check begin with 1 bug
  // get map_table size n
  for (i = 0; i < table_size; i++)
  {
    tem = get_maptable(map_table, i);
    pbn = PHY_SEC_NUM(tem);
    if (pbn != 0)
    {
      n++;
    }
  }
  if (n == 0)
  {
    goto out;
  }

  tem_arr = (word_t *)auto_malloc(sizeof(word_t) * n);
  new_arr = (word_t *)auto_malloc(sizeof(word_t) * n);
  if ((!tem_arr) || (!new_arr))
  {
    auto_free(tem_arr, sizeof(word_t) * n);
    auto_free(new_arr, sizeof(word_t) * n);
    n = 0;
    goto out;
  }
  j = 0;
  for (i = 0; i < table_size; i++)
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

  printk(KERN_INFO
         "NVM_SIM [%s(%d)]: In Extracted function arr:%p index: %p  n: %u\n",
         __FUNCTION__, __LINE__, tem_arr, new_arr, j);
  *arr = tem_arr;
  *index = new_arr;
out:
  return n;
}


/*
 **************************************************************************
 * /proc file system entries
 **************************************************************************
 */

static int nvm_proc_create(void)
{
  proc_nvm = proc_mkdir("nvm", 0);
  if (proc_nvm == NULL)
  {
    printk(KERN_ERR "NVM: %s(%d): cannot create /proc/nvm\n", __FUNCTION__,
           __LINE__);
    return -ENOMEM;
  }

  proc_nvmstat = proc_create("nvmstat", S_IRUGO, proc_nvm, &proc_nvmstat_ops);
  if (proc_nvmstat == NULL)
  {
    remove_proc_entry("nvmstat", proc_nvm);
    printk(KERN_ERR "nvm: cannot create /proc/nvm/nvmstat\n");
    return -ENOMEM;
  }
  printk(KERN_INFO "nvm: /proc/nvm/nvmstat created\n");

  proc_nvmcfg = proc_create("nvmcfg", S_IRUGO, proc_nvm, &proc_nvmcfg_ops);
  if (proc_nvmcfg == NULL)
  {
    remove_proc_entry("nvmcfg", proc_nvm);
    printk(KERN_ERR "nvm: cannot create /proc/nvm/nvmcfg\n");
    return -ENOMEM;
  }
  printk(KERN_INFO "nvm: /proc/nvm/nvmcfg created\n");

  return 0;
}

static int nvm_proc_destroy(void)
{
  remove_proc_entry("nvmcfg", proc_nvm);
  printk(KERN_INFO "nvm: /proc/nvm/nvmcfg is removed\n");

  remove_proc_entry("nvmstat", proc_nvm);
  printk(KERN_INFO "nvm: /proc/nvm/nvmstat is removed\n");

  remove_proc_entry("nvm", 0);
  printk(KERN_INFO "nvm: /proc/nvm is removed\n");
  return 0;
}

/**
 * Allocate the NVM device
 * 	  1. nvm_alloc() : allocates disk and driver
 *    2. nvm_highmem_map() : make mapping for highmem physical address by
 * ioremap()
 */
void *nvm_highmem_map(void)
{
  // https://patchwork.kernel.org/patch/3092221/
  if ((g_highmem_virt_addr =
           ioremap_cache(g_highmem_phys_addr, g_highmem_size)))
  {
    g_highmem_curr_addr = g_highmem_virt_addr;
    printk(
        KERN_INFO
        "NVMSIM: high memory space remapped (offset: %llu MB, size=%lu MB)\n",
        BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
    return g_highmem_virt_addr;
  }
  else
  {
    printk(KERN_ERR
           "NVMSIM: %s(%d) %llu Bytes:%x failed remapping high memory space "
           "(offset: %llu MB size=%llu MB)\n",
           __FUNCTION__, __LINE__, g_highmem_phys_addr, g_highmem_virt_addr,
           BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
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
    printk(KERN_INFO
           "NVMSIM: unmapping high mem space (offset: %llu MB, size=%lu MB)is "
           "unmapped\n",
           BYTES_TO_MB(g_highmem_phys_addr), BYTES_TO_MB(g_highmem_size));
  }
  return;
}

/* device->device_stat */
static int nvm_stat_alloc(NVM_DEVICE_T *device)
{
  int err = 0;
  device->nvm_stat = (NVM_STAT_T *)kzalloc(sizeof(NVM_STAT_T), GFP_KERNEL);
  if (device->nvm_stat)
  {
    spin_lock_init(&(device->nvm_stat->stat_lock));
    printk(KERN_INFO "nvm: %s(%d): NVM Stat space allocation Success\n",
           __FUNCTION__, __LINE__);
  }
  else
  {
    printk(KERN_ERR "nvm: %s(%d): NVM space allocation failed\n", __FUNCTION__,
           __LINE__);
    err = -ENOMEM;
  }
  return err;
}

static int nvm_stat_free(NVM_DEVICE_T *device)
{
  if (device->nvm_stat)
  {
    kfree(device->nvm_stat);
    device->nvm_stat = NULL;
  }
  return 0;
}

/*
 * Allocate/free memory backstore space for nvm devices
 */
static int nvm_mem_space_alloc(NVM_DEVICE_T *device)
{
  int err = 0;
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
    printk(KERN_INFO "NVMSIM: %s(%d):  created [%lu : %llu MBs]\n",
           __FUNCTION__, __LINE__, (unsigned long)device->nvmdev_data,
           SECTORS_TO_MB(device->nvmdev_capacity));
  }
  else
  {
    printk(KERN_ERR "NVMSIM: %s(%d): NVM space allocation failed\n",
           __FUNCTION__, __LINE__);
    err = -ENOMEM;
  }
  return err;
}

static int nvm_mem_space_free(NVM_DEVICE_T *device)
{
  if (device->nvmdev_data)
  {
    if (NVM_USE_HIGHMEM())
    {
      hfree(device->nvmdev_data);
    }
    else
    {
      vfree(device->nvmdev_data);
    }
    device->nvmdev_data = NULL;
  }
  return 0;
}

static int nvm_pbi_space_alloc(NVM_DEVICE_T *device)
{
  /* maptable bitmap init*/
  spin_lock_init(&device->map_lock);
  spin_lock_init(&device->bit_lock);
  spin_lock_init(&device->info_lock);

  device->MapTable = init_maptable(map_table_size);
  if (device->MapTable == NULL)
  {
    goto out;
  }

  device->BitMap = init_bitmap(bit_table_size);
  if (device->BitMap == NULL)
  {
    goto out;
  }

  device->InfoTable = init_infotable(info_table_size);
  if (device->InfoTable == NULL)
  {
    goto out;
  }

  /*
  map_table(device->MapTable, 0, 1);
  map_table(device->MapTable, 0, 1);
  map_table(device->MapTable, 0, 1);

  map_table(device->MapTable, 1, 2);
  map_table(device->MapTable, 1, 2);
  map_table(device->MapTable, 1, 2);
  map_table(device->MapTable, 1, 2);

  map_table(device->MapTable, 2, 3);
  map_table(device->MapTable, 2, 3);
  print_maptable(device->MapTable, map_table_size);

  word_t begin, end, i, j;
  unsigned long long seed, tem;
  begin = 2;
  end = 1000;
  for (i = begin; i < end - 2; i++)
  {
    seed = (jiffies * i) % 1000 + 1;
    //seed = 2;
    for (j = 1; j < seed; j++)
    {
      map_table(device->MapTable, i, i + 1);
      update_infotable(device->InfoTable, i);
      SET_BITMAP(i, device->BitMap);
    }
  }*/

  return 0;
out:
  return -ENOMEM;
}

static int nvm_pbi_space_free(NVM_DEVICE_T *device)
{
  if (device->MapTable != NULL)
  {
    print_maptable(device->MapTable, map_table_size);
    vfree(device->MapTable);
    printk(KERN_INFO "NVMSIM: %s(%d): maptable space free %u success\n",
           __FUNCTION__, __LINE__, map_table_size);
  }
  /*
  if (device->ExtractedIndexMapTable != NULL) {
    auto_free(device->ExtractedIndexMapTable, device->ExtractedSize);
  }
  if (device->ExtractedPbnMapTable != NULL) {
    auto_free(device->ExtractedPbnMapTable, device->ExtractedSize);
  }*/

  if (device->BitMap != NULL)
  {
    // printb_bitmap(device->BitMap, 128);
    print_summary_bitmap(device->BitMap, bit_table_size);
    vfree(device->BitMap);
    printk(KERN_INFO "NVMSIM: %s(%d): BitMap space free %u success\n",
           __FUNCTION__, __LINE__, bit_table_size);
  }

  if (device->InfoTable != NULL)
  {
    print_infotable(device->InfoTable, 100, 10);
    destroy_infotable(device->InfoTable);
    printk(KERN_INFO "NVM_SIM [%s(%d)]: Destroy InfoTable success\n",
           __FUNCTION__, __LINE__);
  }
  return 0;
}

ssize_t nvm_proc_nvmstat_read(struct file *filp, char *buf, size_t count,
                              loff_t *offp)
{
  int ret;
  char *messages = NULL;
  if (*offp > 0)
  {
    ret = 0;
    goto out;
  }
  else
  {
    messages = kzalloc(PROC_CHAR_SIZZE * sizeof(char), GFP_KERNEL);
    int len;
    sprintf(messages, "%s %d Hello World \n", __FUNCTION__, __LINE__);
    len = strlen(messages);
    if (count > len)
    {
      count = len;
    }

    if (copy_to_user(buf, messages, count))
    {
      ret = -EFAULT;
      goto free_out;
    }
    *offp += count;
    ret = count;
  }

free_out:
  if (messages)
  {
    kfree(messages);
  }
out:
  return ret;
}

ssize_t nvm_proc_nvmcfg_read(struct file *filp, char *buf, size_t count,
                             loff_t *offp)
{
  int ret;
  char *messages = NULL;
  if (*offp > 0)
  {
    ret = 0;
    goto out;
  }
  else
  {
    messages = kzalloc(PROC_CHAR_SIZZE * sizeof(char), GFP_KERNEL);
    int len;
    sprintf(messages, "%s %d Hello World \n", __FUNCTION__, __LINE__);
    len = strlen(messages);
    if (count > len)
    {
      count = len;
    }

    if (copy_to_user(buf, messages, count))
    {
      ret = -EFAULT;
      goto free_out;
    }
    *offp += count;
    ret = count;
  }

free_out:
  if (messages)
  {
    kfree(messages);
  }
out:
  return ret;
}

ssize_t nvm_proc_devstat_read(struct file *filp, char *buf, size_t count,
                              loff_t *offp)
{
  /**
   * What is missing from the preceding list is the case of “there is no data,
   * but it may arrive later.”
   * In this case, the read system call should block
   */
  int ret;
  char *messages = NULL;
  if (*offp > 0)
  {
    ret = 0;
    goto out;
  }
  else
  {
    messages = kzalloc(PROC_CHAR_SIZZE * sizeof(char), GFP_KERNEL);
    int len;
    sprintf(messages, "%s %d Hello World \n", __FUNCTION__, __LINE__);
    len = strlen(messages);
    if (count > len)
    {
      count = len;
    }

    if (copy_to_user(buf, messages, count))
    {
      ret = -EFAULT;
      goto free_out;
    }
    *offp += count;
    ret = count;
  }

free_out:
  if (messages)
  {
    kfree(messages);
  }
out:
  return ret;
}

static int nvm_proc_devstat_create(NVM_DEVICE_T *device)
{
  /*  create a /proc/nvm/<dev> entry  */
  int err = 0;
  device->proc_devstat =
      proc_create(device->nvm_name, S_IRUGO, proc_nvm, &proc_nvmdev_ops);
  if (device->proc_devstat == NULL)
  {
    remove_proc_entry(device->nvm_name, proc_nvm);
    printk(KERN_ERR "NVMSIM: cannot create /proc/nvm/%s\n", device->nvm_name);
    err = -ENOMEM;
    goto out;
  }
  printk(KERN_INFO "NVMSIM: /proc/nvm/%s created\n", device->nvm_name);
out:
  return err;
}
static int nvm_proc_devstat_destroy(NVM_DEVICE_T *device)
{
  remove_proc_entry(device->nvm_name, proc_nvm);
  printk(KERN_INFO "nvm: /proc/nvm/%s removed\n", device->nvm_name);
  return 0;
}

static int nvm_buffer_space_alloc(NVM_DEVICE_T *device)
{
  int err = 0;
  device->ExtractedPbnMapTable = NULL;
  device->ExtractedIndexMapTable = NULL;
  device->ExtractedSize = 0;
  device->tail = NULL;
  device->head = NULL;
  device->head_tail_size = 0;
  device->flag = 0;
  spin_lock_init(&device->syncer_lock);
  spin_lock_init(&device->flush_lock);
  err = nvm_syncer_init(device);

  return err;
}

static int nvm_buffer_space_free(NVM_DEVICE_T *device)
{
  nvm_syncer_stop(device);
  if (device->head_tail_size)
  {
    nvm_auto_free(device->head, device->head_tail_size);
    nvm_auto_free(device->tail, device->head_tail_size);
  }
  if (device->ExtractedSize)
  {
    nvm_auto_free(device->ExtractedPbnMapTable, device->ExtractedSize);
    nvm_auto_free(device->ExtractedIndexMapTable, device->ExtractedSize);
  }
  return 0;
}

static int nvm_create(NVM_DEVICE_T *device)
{
  int err = 0;
  /* allocate statistics info */
  if ((err = nvm_stat_alloc(device)) < 0)
    goto error;

  /* allocate memory space */
  if ((err = nvm_mem_space_alloc(device)) < 0)
    goto error;

  /* allocate block info space */
  if ((err = nvm_pbi_space_alloc(device)) < 0)
    goto error;

  /* allocate buffer space */
  if ((err = nvm_buffer_space_alloc(device)) < 0)
    goto error;

  /* create a /proc/device/<dev> entry*/
  if ((err = nvm_proc_devstat_create(device)) < 0)
    goto error;
error:
  return err;
}

static int nvm_destroy(NVM_DEVICE_T *device)
{
  /* free /proc entry */
  nvm_proc_devstat_destroy(device);

  /* free buffer space */
  nvm_buffer_space_free(device);

  /* free block info space */
  nvm_pbi_space_free(device);

  /* free memory backstore space */
  nvm_mem_space_free(device);

  /* free statistics data */
  nvm_stat_free(device);

  printk(KERN_INFO "nvm: /dev/%s is destroyed (%llu MB)\n", device->nvm_name,
         SECTORS_TO_MB(device->nvmdev_capacity));
  return 0;
}

static uint64_t nvm_device_is_idle(NVM_DEVICE_T *device)
{
  // check size type
  unsigned long last_jiffies, now_jiffies;
  uint64_t interval = 0;

  now_jiffies = jiffies;
  NVM_DEV_GET_ACCESS_TIME(device, last_jiffies);
  interval = jiffies_to_msecs(now_jiffies - last_jiffies);

  if (NVM_DEV_IS_IDLE(interval))
  {
    printk(KERN_INFO
           "NVM_SIM [%s(%d)] Test Get Into Dev Idle last_jiffies %u "
           "now_jiffies %u "
           "interval %llu\n",
           __FUNCTION__, __LINE__, last_jiffies, now_jiffies, interval);
    return interval;
  }
  else
  {
    printk(KERN_INFO
           "NVM_SIM %s%d Test Dec Not Idle Test last_jiffies %u now_jiffies %u "
           "interval %llu\n",
           __FUNCTION__, __LINE__, last_jiffies, now_jiffies, interval);
    return 0;
  }
}

static int nvm_syncer_init(NVM_DEVICE_T *device)
{
  int err = 0;
  struct task_struct *tsk = NULL;
  tsk = kthread_run(nvm_syncer_worker, (void *)device, "nsyncer");
  if (IS_ERR(tsk))
  {
    printk(KERN_ERR "NVMSIM: initializing syncer threads failed\n");
    // FIXME bug return err num
    err = -ENOMEM;
    goto out;
  }
  device->syncer = tsk;
  printk(KERN_INFO "NVMSIM: [%s(%d)] inin syncer threads at %p success\n",
         __FUNCTION__, __LINE__, tsk);
out:
  return err;
}

static int nvm_syncer_stop(NVM_DEVICE_T *device)
{
  if (device->syncer)
  {
    kthread_stop(device->syncer);
    device->syncer = NULL;
    printk(KERN_INFO "NVMSIM:%s:%d buffer syncer stopped\n", __FUNCTION__,
           __LINE__);
  }
  return 0;
}

static int nvm_syncer_worker(void *device)
{
  set_user_nice(current, 0);
  NVM_DEVICE_T *deviceT = (NVM_DEVICE_T *)device;

  do
  {
    /* we start flushing, if
     * (1) the num of dirty blocks hits the high watermark, or
     * (2) the device has been idle for a while */

    spin_lock(&deviceT->syncer_lock);
    /*
    if (nvm_device_is_idle(deviceT) && nvm_block_above_level(deviceT)) {
      if (deviceT->flag == 1) {
        nvm_block_check_flush(device);

        print_maptable(deviceT->MapTable, map_table_size);
        word_t *p,t;
        t = map_table_size * 4;
        p = auto_malloc(t);
        printk(KERN_INFO "%u\n", t,p);
        auto_free(p,t);
      }
    }*/
    spin_unlock(&deviceT->syncer_lock);
    /*
  word_t *p;
  p = NULL;
  auto_free(p, 3<<20);*/
    /*
    word_t size;
    size = 2048 << KB_SHIFT;
    p = auto_malloc(size);
      printk(KERN_INFO"NVM_SIM [%s(%d)]: size %d p %p\n",
    __FUNCTION__,__LINE__,size,p);
    auto_free(p,size);
    */
    /*
    p = kzalloc(1<<17, GFP_KERNEL);
    if(p){
      printk(KERN_INFO"NVM_SIM [%s(%d)]: PAGE_SHIFT %d p %p\n",
      __FUNCTION__,__LINE__,PAGE_SHIFT,p);
      kfree(p);
    }*/
    /*
    p = vmalloc(1 << 20);
  if(p!=NULL){
    printk(KERN_INFO"NVM_SIM [%s(%d)]: size %d p %p\n",
    __FUNCTION__,__LINE__,1<<20,p);
    vfree(p);
  }*/
    /*
    word_t *arr, *index, *big_arr_index;
    word_t size, i;
    arr = NULL;
    index = NULL;
    size = extract_maptable(deviceT->MapTable, map_table_size, &arr, &index);
    printk(KERN_INFO
           "NVM_SIM [%s(%d)]: ExtractedSize %u Good Here arr %p index %p\n",
           __FUNCTION__, __LINE__, size, arr, index);
    for (i = 0; i < size; i += 9000) {
      printk(KERN_INFO "NVM_SIM [%s(%d)]: lbn:%u key:%u pbn:%u access:%u\n",
             __FUNCTION__, __LINE__, index[i], arr[i], PHY_SEC_NUM(arr[i]),
             ACCESS_TIME(arr[i]));
    }
    */
    /*
   // find minmum item index in arr of size n
   word_t min_index,*tarr,j;
   tarr = auto_malloc(size * sizeof(word_t));
   for (j = 0; j < size; j++) {
     tarr[j] = ACCESS_TIME(arr[j]);
     printk(KERN_INFO "NVM_SIM [%s(%d)]: debug tarr %d:%u:%u\n", __FUNCTION__,
            __LINE__, j, index[j], tarr[j]);
   }
   min_index = minium(tarr, size);
   printk(KERN_INFO"NVM_SIM [%s(%d)]: minium of arr index %d items %u\n",
   __FUNCTION__,__LINE__,min_index,arr[min_index]);

   big_arr_index = bigK_index(tarr, size, 1);
   printk(KERN_INFO "NVM_SIM [%s(%d)]: big_index:%p big_index[0]:%u pbn:%u\n",
          __FUNCTION__, __LINE__, big_arr_index,
  big_arr_index[0],tarr[big_arr_index[0]]); auto_free(big_arr_index, 1);

   auto_free(arr, size);
   auto_free(index, size);*/

    /* go to sleep */
    // FIXME CHECK FOR DEADLOCK maybe bug
    // set_current_state(TASK_INTERRUPTIBLE);
    printk(KERN_INFO "NVM_SIM [%s(%d)] sleep %lu\n", __FUNCTION__, __LINE__,
           10);
    ssleep(10);
    // schedule_timeout(500);
    // printk(KERN_INFO "NVM_SIM [%s(%d)] Wake up after %lu\n", __FUNCTION__,
    //       __LINE__, NVM_AFTER_FLUSH_SLEEPTIME);
    // set_current_state(TASK_RUNNING);
  } while (!kthread_should_stop());
  return 0;
}

int nvm_block_above_level(NVM_DEVICE_T *device)
{
  // check flag
  if (nvm_check_head_tail(device) != 0)
  {
    device->flag = 1;
    return 1;
  }
  device->flag = 0;
  return 0;
};

int nvm_get_extracted_maptable(NVM_DEVICE_T *device)
{
  word_t n;
  if (device->MapTable == NULL)
  {
    n = 0;
    goto out;
  }
  word_t *arr, *index;
  arr = NULL;
  index = NULL;
  n = extract_maptable(device->MapTable, map_table_size, &arr, &index);
  /*
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Test extracted n %u arr %p index %p \n",
         __FUNCTION__, __LINE__, n, arr, index);*/
  if ((!n) || (!arr) || (!index))
  {
    n = 0;
    goto out;
  }
  device->ExtractedPbnMapTable = arr;
  device->ExtractedIndexMapTable = index;
  device->ExtractedSize = n;
out:
  return n;
}

void nvm_free_extracted_maptable(NVM_DEVICE_T *device)
{
  nvm_auto_free(device->ExtractedPbnMapTable, device->ExtractedSize);
  nvm_auto_free(device->ExtractedIndexMapTable, device->ExtractedSize);
  device->ExtractedSize = 0;
  device->ExtractedIndexMapTable = NULL;
  device->ExtractedPbnMapTable = NULL;
}

int nvm_get_sorted_head_tail_maptale(NVM_DEVICE_T *device)
{
  // assume extractedmaptable not null
  word_t *arr, *index, *tarr;
  word_t m, n, j;
  arr = device->ExtractedPbnMapTable;
  index = device->ExtractedIndexMapTable;
  n = device->ExtractedSize;
  tarr = NULL;
  if ((!n) || (!arr) || (!index))
  {
    n = 0;
    goto end;
  }

  // make an arry with key value of accesstime not key(pbn+accesstime)
  tarr = (word_t *)auto_malloc(n * sizeof(word_t));
  if (tarr == NULL)
  {
    n = 0;
    goto end;
  }
  for (j = 0; j < n; j++)
  {
    tarr[j] = ACCESS_TIME(arr[j]);
  }

  word_t k, *bigK, *smallK;
  k = (n / SORTED_BASE) + 1;

  bigK = bigK_index(tarr, n, k);
  smallK = smallK_index(tarr, n, k);
  if ((!bigK) || (!smallK))
  {
    if (tarr != NULL)
    {
      auto_free(tarr, n * sizeof(word_t));
    }
    return 0;
  }
  printk(
      KERN_INFO
      "NVM_SIM [%s(%d)]: Test Find bigK %p first %u lbn: %u pbn: %u acc: %u\n",
      __FUNCTION__, __LINE__, bigK, bigK[0], index[bigK[0]],
      PHY_SEC_NUM(arr[bigK[0]]), ACCESS_TIME(arr[bigK[0]]));
  printk(KERN_INFO
         "NVM_SIM [%s(%d)]: Test Find smallK %p first %u lbn: %u pbn: %u acc: "
         "%u\n",
         __FUNCTION__, __LINE__, smallK, smallK[0], index[smallK[0]],
         PHY_SEC_NUM(arr[smallK[0]]), ACCESS_TIME(arr[smallK[0]]));
  device->head_tail_size = k;
  device->head = bigK;
  device->tail = smallK;
  n = k;
out:
  if (tarr != NULL)
  {
    auto_free(tarr, n * sizeof(word_t));
  }
end:
  return n;
}

void nvm_free_sorted_head_tail_maptale(NVM_DEVICE_T *device)
{
  nvm_auto_free(device->head, device->head_tail_size);
  nvm_auto_free(device->tail, device->head_tail_size);
  device->head_tail_size = 0;
  device->head = NULL;
  device->tail = NULL;
}

int nvm_block_check_flush(NVM_DEVICE_T *device)
{
  if (device->flag == 0)
  {
    goto out;
  }
  word_t i, k, n, tem, bindex, sindex, blbn, slbn, bkey, skey;
  word_t *bigK, *smallK, *index, *arr, *MapTable;

  MapTable = device->MapTable;
  bigK = device->head;
  smallK = device->tail;
  index = device->ExtractedIndexMapTable;
  arr = device->ExtractedPbnMapTable;
  n = device->ExtractedSize;
  k = device->head_tail_size;
  if ((!MapTable) || (!bigK) || (!smallK) || (!index) || (!arr) || (!n) ||
      (!k))
  {
    goto out;
  }
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Flush k %u n:%u base:%u\n", __FUNCTION__,
         __LINE__, k, n, SORTED_BASE);
  for (i = 0; i < k; i++)
  {
    // bindex one of the big k index
    bindex = bigK[i];
    blbn = index[bindex];
    bkey = arr[bindex];

    sindex = smallK[i];
    slbn = index[sindex];
    skey = arr[sindex];

    printk(KERN_INFO
           "[%s(%d)] Flush: blbn<->slbn %u<->%u pbn: %u<->%u access %u<->%u\n",
           __FUNCTION__, __LINE__, blbn, slbn, PHY_SEC_NUM(bkey),
           PHY_SEC_NUM(skey), ACCESS_TIME(bkey), ACCESS_TIME(skey));

    // map_table(MapTable, blbn, PHY_SEC_NUM(skey));
    update_maptable(MapTable, blbn, skey);
    // transfer data
    // map_table(MapTable, slbn, PHY_SEC_NUM(bkey));
    update_maptable(MapTable, slbn, bkey);
  }

out:
  device->flag = 0;

  nvm_free_sorted_head_tail_maptale(device);
  nvm_free_extracted_maptable(device);
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Remove buffer Infomation %u %u\n",
         __FUNCTION__, __LINE__, device->head_tail_size, device->ExtractedSize);
  return k;
}

int nvm_check_head_tail(NVM_DEVICE_T *device)
{
  if (nvm_get_extracted_maptable(device) == 0)
  {
    goto free_extracted;
  }

  if (nvm_get_sorted_head_tail_maptale(device) == 0)
  {
    goto free_head_tail;
  }
  printk(KERN_INFO "NVMSIM [%s(%d)] Debug keys %p index %p size %u\n",
         __FUNCTION__, __LINE__, device->ExtractedPbnMapTable,
         device->ExtractedIndexMapTable, device->ExtractedSize);
  return 1;
free_head_tail:
  nvm_free_sorted_head_tail_maptale(device);
free_extracted:
  nvm_free_extracted_maptable(device);
out:
  return 0;
}

static void *hmalloc(uint64_t bytes)
{
  void *rtn = NULL;

  /* check if there is still available reserve high memory space */
  if (bytes <= NVM_HIGHMEM_AVAILABLE_SPACE)
  {
    rtn = g_highmem_curr_addr;
    g_highmem_curr_addr += bytes;
  }
  else
  {
    printk(KERN_ERR
           "NVMSIM: %s(%d) - no available space (< %llu bytes) in reserved "
           "high memory\n",
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

  // Allocate the block request queue by blk_alloc_queue without I/O scheduler
  device->nvmdev_queue = blk_alloc_queue(GFP_KERNEL);
  sprintf(device->nvm_name, "nvm%d", (index));

  if (!device->nvmdev_queue)
  {
    goto out_free_dev;
  }
  // register nvmdev_queue,
  blk_queue_make_request(device->nvmdev_queue,
                         (make_request_fn *)nvm_make_request);
  // set max sectors for a request for this queue
  blk_queue_max_hw_sectors(device->nvmdev_queue, 1024);
  // set logical block size for the queue
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
  // disk->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
  strcpy(disk->disk_name, device->nvm_name);
  // in sectors
  set_capacity(disk, capacity_mb << MB_PER_SECTOR_SHIFT);

  /* allocate PM space */
  if (nvm_create(device) < 0)
    goto out_free_queue;
  return device;

  // Cleanup on error
out_free_queue:
  blk_cleanup_queue(device->nvmdev_queue);
out_free_dev:
  // vfree(device->nvmdev_data);
  nvm_mem_space_free(device);
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
  nvm_destroy(device);
  kfree(device);
}

// set helper function
int set_helper(struct nvm_device *device, sector_t sector, word_t len)
{
  word_t sector_begin, sector_end, lbn_begin, lbn_end;
  sector_begin = sector;
  sector_end = sector + (len >> SECTOR_SHIFT);
  lbn_begin = sector_begin >> MAP_PER_SECTORS_SHIFT;
  lbn_end = sector_end >> MAP_PER_SECTORS_SHIFT;

  /*
  printk(KERN_INFO
         "NVM_SIM [%s(%d)]:set helper sector_begin %u %u lbn_begin %u %u\n",
         __FUNCTION__, __LINE__, sector_begin, sector_end, lbn_begin, lbn_end);
  */
  word_t i, rtn;
  // Access Info Table
  //spin_lock(&device->syncer_lock);
  for (i = lbn_begin; i <= lbn_end; i++)
  {
    if (update_infotable(device->InfoTable, i))
    { 
      
      printk(KERN_ERR "NVM_SIM [%s(%d)]: InforTable update %u failed \n",
             __FUNCTION__, __LINE__, i);
    }
  }
  //spin_unlock(&device->syncer_lock);
  // set bitmap and maptable
  for (i = sector_begin; i < sector_end; i++)
  {
    if (BOOL(i, device->BitMap) == 0)
    {
      SET_BITMAP(i, device->BitMap);
      // pbn just equal lbn+1
      rtn = map_table(device->MapTable, i, i + 1);
      if ((signed)rtn != -1)
      {
        /*
      printk(KERN_INFO "MapTable secotr: [%u:%u] lbn %u times %u success\n",
             sector_begin, sector_end, i, rtn);*/
      }
      /*
      printk(KERN_INFO "BitMap sector %u actual in MapTable %u\n", i,
             (i >> MAP_PER_SECTORS_SHIFT) + 1);*/
    }
  }
  return 1;
}

/**
 * Process pending requests from the queue
 */
static void nvm_make_request(struct request_queue *q, struct bio *bio)
{
  // bio->bi_bdev has been discarded
  // struct block_device *bdev = bio->bi_bdev;
  // struct nvm_device *device = bdev->bd_disk->private_data;
  NVM_DEVICE_T *nvm_dev = bio->bi_disk->private_data;
  NVM_STAT_T *nvm_stat = nvm_dev->nvm_stat;

  int rw, num_sectors;
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
  num_sectors = bio_sectors(bio);
  /*
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Debug bio_sector(bio) : %d\n",
         __FUNCTION__, __LINE__, num_sectors);*/
  if (sector + (bio->bi_iter.bi_size >> SECTOR_SHIFT) > capacity)
  {
    printk(KERN_ERR "NVM_SIM %s%d Test %u Exceed size %u", __FUNCTION__,
           __LINE__, sector + (bio->bi_iter.bi_size >> SECTOR_SHIFT), capacity);
    goto out;
  }

  // Get the request vector
  rw = bio_data_dir(bio);
  /* update rw */
  // if (rw != READ && rw != WRITE)
  // panic("NVMSIM: %s(%d) found request not read or write either\n",
  // __FUNCTION__, __LINE__);

  /* update the access time*/
  NVM_DEV_UPDATE_ACCESS_TIME(nvm_dev);

  // printk(KERN_INFO "NVMSIM: %s(%d) sector: %lu rw: %d\n",
  //	   __FUNCTION__, __LINE__, sector, rw);
  // Perform each part of a request
  bio_for_each_segment(bvec, bio, iter)
  {
    unsigned int len = bvec.bv_len;
    // Every biovec means a SEGMENT of a PAGE
    err = nvm_do_bvec(nvm_dev, bvec.bv_page, len, bvec.bv_offset, rw, sector);
    if (err)
    {
      printk(KERN_WARNING "Transfer data in Segments %x of address %x failed\n",
             len, bvec.bv_page);
      break;
    }
    sector += len >> SECTOR_SHIFT;
    // printk(KERN_INFO "NVMSIM: %s(%d) sector: %lu rw: %d bvec.bv_len :%u
    // bvec.bv_page :%x bvec.bv_offset %u\n",
    //	   __FUNCTION__, __LINE__, sector, rw, len, bvec.bv_page,
    // bvec.bv_offset);
  }
out:
  bio_endio(bio);

  /* update statistics data */
  spin_lock(&nvm_stat->stat_lock);
  if (rw == READ)
  {
    nvm_stat->num_requests_read++;
    nvm_stat->num_sectors_read += num_sectors;
  }
  else
  {
    nvm_stat->num_requests_write++;
    nvm_stat->num_sectors_write += num_sectors;
  }
  /*
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Test in End %u\n", __FUNCTION__, __LINE__,
         nvm_stat->num_sectors_write);
  */
  spin_unlock(&nvm_stat->stat_lock);
  return;
}

/**
 * Process a single request
 */
static int nvm_do_bvec(struct nvm_device *device, struct page *page,
                       unsigned int len, unsigned int off, int rw,
                       sector_t sector)
{
  void *mem;
  int err = 0;

  if (rw == WRITE)
  {
    set_helper(device, sector, len);
  }
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

void __always_inline copy_to_nvm(struct nvm_device *device, const void *src,
                                 sector_t sector, size_t n)
{
  void *nvm;
  nvm = device->nvmdev_data + (sector << SECTOR_SHIFT);
  memory_copy(nvm, src, n);
}

/**
 * Perform I/O control
 */
static int nvm_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd,
                     unsigned long arg)
{
  return -ENOTTY;
}

static int nvm_disk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
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
  printk(KERN_INFO
         "NVMSIM: [%s() %d] g_highmem_size %llu bytes g_highmemaddr %llu ; "
         "nvm_capacity_mb %llu MB \n",
         __FUNCTION__, __LINE__, g_highmem_size, g_highmem_phys_addr,
         nvm_capacity_mb, LLONG_MAX);

  // remap the highmem physical address
  if (NVM_USE_HIGHMEM())
  {
    if (nvm_highmem_map() == NULL)
    {
      return -ENOMEM;
    }
  }
  // proc
  nvm_proc_create();

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

  /* deioremap high memory space */
  if (NVM_USE_HIGHMEM())
  {
    nvm_highmem_unmap();
  }

  blk_unregister_region(MKDEV(NVM_MAJOR, 0), range);
  unregister_blkdev(NVM_MAJOR, NVM_DEVICES_NAME);

  nvm_proc_destroy();
  printk(KERN_ERR "NVMSIM: %s(%d) -Exited\n", __FUNCTION__, __LINE__);
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