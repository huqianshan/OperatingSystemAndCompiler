#include <linux/module.h>
#include <linux/init.h>

#define SIZE 0x1000
#define TDLCD_MAJOR 250

struct tdlcd_dev{
    struct cdev cdev;
    unsigned char mem[SIZE];
}

static int tdlcd_major=TDLCD_MAJOR
module_param(tdlcd_major,int,S_IRUGO)

MODULE_AUTHOR("hjl,UESTC")
MODULE_LICENSE("Dual BSD/GPL")

struct tdlcd_dev *tdlcd_devices;

static void tdlcd_setup_cdev(struct tdlcd_dev *dev,int index);

/* file_operations */
static const struct file_operations tdlcd_fops={
    .owner=THIS_MODULE,
    .llseek = tdlcd_llseek,
	.read = tdlcd_read,
	.write = tdlcd_write,
	.unlocked_ioctl = tdlcd_ioctl,
	.open = tdlcd_open,
	.release = tdlcd_release,
}

/*drivers modules*/

static int __init tdlcd_init(void);
static void __exit tdlcd_exit(void);

/* functions*/

static tdlcd_open(struct inode *inode,struct file *filp);
static tdlcd_release(struct inode *inode,struct file *filp);

static long globalmem_ioctl(strcut file *filp,unsigned int cmd,unsigned long arg);
static ssize_t globalmem_read(struct file *filp, char __user * buf, size_t size,
			      loff_t * ppos);
static ssize_t globalmem_write(struct file *filp, const char __user * buf,
			       size_t size, loff_t * ppos);
static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig);

static int __init tdlcd_init(void){
    //1.0 init cdev
    cdev_init();
    
    // 2.0 get char device number
    
    // 3.0 register devices

};


static void __exit tdlcd_exit(void){
    // 1.0 release devices
    // 2.0 release device number
};

module_init(tdlcd_init);
module_exit(tdlcd_exit);


