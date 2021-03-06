/*
 * ramDevice.h
 * NVM Simulator: RAM based block device driver
 */
#ifndef __RAMDEVICE_H
#define __RAMDEVICE_H

/*
 * support definitions
 */
#define TRUE (1)
#define FALSE (0)
//typedef unsigned int word_t;

#define MEMCOPY_DEFAULT 1 // use memcopy
#define MEMCOPY_BARRIER 0 // use barrier

#define NVM_CONFIG_VMALLOC 0 /* use vmalloc() to allocate memory*/
#define NVM_CONFIG_HIGHMEM 1 /* use ioremap to map highmemory-based memory*/
/**
 * NVM operation codes
 */
#define NVM_READ 0
#define NVM_WRITE 1

/* high memory
 ** high memory configs */
unsigned g_nvm_type = NVM_CONFIG_HIGHMEM;
#define NVM_USE_HIGHMEM() (g_nvm_type == NVM_CONFIG_HIGHMEM)
/* size of the reserved physical mem space (bytes) */
uint64_t g_highmem_size = 0;
void *g_highmem_virt_addr = NULL; /* beginning of the reserve HIGH_MEM space */
/* beginning of the available HIGH_MEM space for alloc*/
void *g_highmem_curr_addr = NULL;
#define NVM_HIGHMEM_AVAILABLE_SPACE \
  (g_highmem_virt_addr + g_highmem_size - g_highmem_curr_addr)

/**
 *  The SIZE TRANSFER
 */
#define SECTOR_SHIFT 9
#define KB_SHIFT 10
#define MB_SHIFT 20
#define GB_SHIFT 30
#define MB_TO_BYTES(N) ((N) << MB_SHIFT)
#define GB_TO_BYTES(N) ((N) << GB_SHIFT)
#define BYTES_TO_MB(N) ((N) >> MB_SHIFT)
#define BYTES_TO_GB(N) ((N) >> GB_SHIFT)
#define MB_TO_SECTORS(N) ((N) << (MB_SHIFT - SECTOR_SHIFT))
#define GB_TO_SECTORS(N) ((N) << (GB_SHIFT - SECTOR_SHIFT))
#define SECTORS_TO_MB(N) ((N) >> (MB_SHIFT - SECTOR_SHIFT))
#define SECTORS_TO_GB(N) ((N) >> (GB_SHIFT - SECTOR_SHIFT))
#define SECTOR_TO_PAGE(N) ((N) >> (PAGE_SHIFT - SECTOR_SHIFT))
#define SECTOR_TO_BYTE(N) ((N) << SECTOR_SHIFT)
#define BYTE_TO_SECTOR(N) ((N) >> SECTOR_SHIFT)

#define PAGE_SECTORS_SHIFT (PAGE_SHIFT - SECTOR_SHIFT)
#define PAGE_SECTORS (1 << PAGE_SECTORS_SHIFT)

#define HARDSECT_SIZE (512)
#define KERNEL_SECT_SIZE (512)

#define MB_PER_BYTES_SHIFT (20)
#define MB_PER_SECTOR_SHIFT (11)

#define PARTION_PER_DISK (1)

#define NVM_MAJOR 231
#define NVMDEV_MEM_MAX_SECTORS (8)

/* idle period timer */
#define NVM_FLUSH_IDLE_TIMEOUT (12)                           /*  12 millisecond */
#define NVM_AFTER_FLUSH_SLEEPTIME (10 * HZ) /*  10 seconds */ // HZ = 250
#define NVM_DEV_UPDATE_ACCESS_TIME(NVM)             \
  {                                                 \
    spin_lock(&(NVM)->nvm_stat->stat_lock);         \
    (NVM)->nvm_stat->last_access_jiffies = jiffies; \
    spin_unlock(&(NVM)->nvm_stat->stat_lock);       \
  }

#define NVM_DEV_GET_ACCESS_TIME(NVM, T)         \
  {                                             \
    spin_lock(&(NVM)->nvm_stat->stat_lock);     \
    (T) = (NVM)->nvm_stat->last_access_jiffies; \
    spin_unlock(&(NVM)->nvm_stat->stat_lock);   \
  }

#define NVM_DEV_IS_IDLE(IDLE) ((IDLE) > NVM_FLUSH_IDLE_TIMEOUT)


/*
 * NVM    spin_lock(&(NVM)->nvm_stat->stat_lock);         \
    (NVM)->nvm_stat->last_access_jiffies = jiffies; \
    spin_unlock(&(NVM)->nvm_stat->stat_lock); \ physical block information
 * (each corresponding to a PM block)
 */

typedef struct nvm_stat
{
  /* stat_lock does not protect cycles_*[] counters */
  spinlock_t stat_lock; /* protection lock */

  unsigned last_access_jiffies; /* the timestamp of the most recent access */
  uint64_t num_sectors_read;    /* total num of sectors being read */
  uint64_t num_sectors_write;   /* total num of sectors being written */
  uint64_t num_requests_read;   /* total num of requests for read */
  uint64_t num_requests_write;  /* total num of request for write */
  uint64_t num_write_barrier;   /* total num of write barriers received */
  uint64_t num_write_fua;       /* total num of write barriers received */
} NVM_STAT_T;

/**
 * The simulated NVM device
 */
typedef struct nvm_device
{
  char nvm_name[32]; /* device name */

  int nvmdev_number; // The device number
  unsigned long
      nvmdev_capacity;                // The capacity in sectors BUG should in bytes?
  u8 *nvmdev_data;                    // The backing data store
  spinlock_t nvmdev_lock;             // The lock protecting the data store
  struct request_queue *nvmdev_queue; /// Request queue
  struct gendisk *nvmdev_disk;        /// Disk

  struct list_head
      nvmdev_list; // The collection of lists the device belongs to

  NVM_STAT_T *nvm_stat;                /* statistics data */
  struct proc_dir_entry *proc_devstat; /* the proc output */

  /* Wear leveling*/
  word_t avg_times;
  struct task_struct *syncer;
  spinlock_t syncer_lock;
  spinlock_t flush_lock; // maynot use FIXME
  word_t *head;
  word_t *tail;
  word_t head_tail_size;
  word_t flag;

  /*MapTable*/
  spinlock_t map_lock;
  word_t *MapTable;
  word_t *ExtractedPbnMapTable;
  word_t *ExtractedIndexMapTable;
  word_t ExtractedSize;

  /* Bitmap*/
  word_t *BitMap;
  spinlock_t bit_lock;

  /* Inforation Map*/
  word_t *InfoTable;
  spinlock_t info_lock;
} NVM_DEVICE_T;

/***
 * nvm_buffer:
 *      wear leveling
typedef struct nvm_buffer{
  struct nvm_device *nvm;

  // wear leveling for maptable
  word_t *head;
  word_t *tail;
  word_t size;

  spinlock_t buffer_lock;

}
*/

/**
 * Allocate and free the NVM device
 */
static NVM_DEVICE_T *nvm_alloc(int index, unsigned capacity_mb);
static int nvm_create(NVM_DEVICE_T *nvm);
static int nvm_stat_alloc(NVM_DEVICE_T *device);
static int nvm_mem_space_alloc(NVM_DEVICE_T *device);
static int nvm_buffer_space_alloc(NVM_DEVICE_T *device);
static int nvm_pbi_space_alloc(NVM_DEVICE_T *device);

static void nvm_free(struct nvm_device *device);
static int nvm_destroy(NVM_DEVICE_T *device);
static int nvm_stat_free(NVM_DEVICE_T *device);
static int nvm_mem_space_free(NVM_DEVICE_T *device);
static int nvm_buffer_space_free(NVM_DEVICE_T *device);
static int nvm_pbi_space_free(NVM_DEVICE_T *device);

static uint64_t nvm_device_is_idle(NVM_DEVICE_T *device);

/*
 **************************************************************************
 * /proc file system entries
 **************************************************************************
 */
#define PROC_CHAR_SIZZE (1024) /* should not bigger than A PAGE SIZE*/
static int nvm_proc_create(void);
static int nvm_proc_destroy(void);

static int nvm_proc_devstat_create(NVM_DEVICE_T *device);
static int nvm_proc_devstat_destroy(NVM_DEVICE_T *device);

ssize_t nvm_proc_devstat_read(struct file *filp, char *buf, size_t count,
                              loff_t *offp);
ssize_t nvm_proc_nvmcfg_read(struct file *filp, char *buf, size_t count,
                             loff_t *offp);
ssize_t nvm_proc_nvmstat_read(struct file *filp, char *buf, size_t count,
                              loff_t *offp);

/*
 **************************************************************************
 * Wear leveling system entries
 **************************************************************************
 */
static int nvm_syncer_init(NVM_DEVICE_T *device);
static int nvm_syncer_stop(NVM_DEVICE_T *device);

static int nvm_syncer_worker(void *device);

int nvm_block_above_level(NVM_DEVICE_T *device);
int nvm_check_head_tail(NVM_DEVICE_T *device);

int nvm_block_check_flush(NVM_DEVICE_T *device);
/**
 *  NOTE: we can also use ioremap_* functions to directly set memory
 *  page attributes when do remapping,
 */
void *nvm_highmem_map(void);
void nvm_highmem_unmap(void);
static void *hmalloc(uint64_t bytes);
static int hfree(void *addr)
{
  /* FIXME: no support for dynamic alloc/dealloc in HIGH_MEM space */
  return 0;
}

/**
 * Binder request to queue
 */
static void nvm_make_request(struct request_queue *q, struct bio *bio);
/**  nvmdev_do_bvec		Process a single request*/
static int nvm_do_bvec(struct nvm_device *device, struct page *page,
                       unsigned int len, unsigned int off, int rw,
                       sector_t sector);

/**
 * Copy n bytes to from the NVM to dest starting at the given sector
 */
void __always_inline copy_from_nvm(void *dest, struct nvm_device *device,
                                   sector_t sector, size_t n);
void __always_inline copy_to_nvm(struct nvm_device *device, const void *src,
                                 sector_t sector, size_t n);

/**
 * Perform I/O control
 */
static int nvm_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd,
                     unsigned long arg);
/* Set Disk Arguments*/
static int nvm_disk_getgeo(struct block_device *bdev, struct hd_geometry *geo);

/**
 * Maptabl function
 */
// get the flag(access information)
#define MAP_PER_SECTORS_SHIFT (5)
#define MAP_PER_SECOTRS (1 << MAP_PER_SECTORS_SHIFT)

/**
 * key helper function
 * 18+14
 * physical-page-number + access information
 */
// get the physical-page-number by key bug mapping not page
#define PHY_SEC_NUM(key) (key >> 14)

// get the flag(access information)
#define ACCESS_TIME(key) (key & 0x3fff)

// make newkey by pbn and access time
#define MAKE_KEY(pbn, num) ((pbn << 14) + num)

word_t extract_maptable(word_t *map_table, word_t table_size, word_t **arr,
                        word_t **index);
int nvm_get_extracted_maptable(NVM_DEVICE_T *device);
void nvm_free_extracted_maptable(NVM_DEVICE_T *device);

/**
 * total helper function
 */
int write_helper(struct nvm_device *device, sector_t sector, word_t len);

#endif