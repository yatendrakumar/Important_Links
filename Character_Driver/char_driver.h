#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>

static int device_open(struct inode *, struct file *);
static int device_close(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
/* In Linux kernel 2.6.x the declaration for _ioctl calls changed from
 * static long wait_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
 * To: 
 * static long wait_ioctl(struct file *, unsigned int, unsigned long);
 */
//static int device_ioctl(struct inode *inode, struct file *filep, unsigned int cmd, unsigned long arg);
static int device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);

static struct file_operations pugs_fops =
{
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_close,
    .read = device_read,
    .unlocked_ioctl = device_ioctl,
    .write = device_write
};

/*_IO, _IOW, _IOR, _IORW are helper macros to create a unique ioctl identifier and add the required R/W needed features.
 * these can take the following params: magic number, the command id, and the data type that will be passed (if any)
 * the magic number is a unique number that will tell the kernel which driver is supposed to be called. 
 * the command id, is a way for your dirver to understand what command is needed to be called.
 * last parameter (the type) will allow kernel to understand the size to be copied.
*/
#define MAGIC_NUMBER 'G'
#define READ_IOCTL _IOR(MAGIC_NUMBER, 0, int)
#define WRITE_IOCTL _IOW(MAGIC_NUMBER, 1, int)

#define SUCCESS 1
#define FAILURE 0
#define BUFFER_SIZE 100
#define IOC_MAXNR 1

