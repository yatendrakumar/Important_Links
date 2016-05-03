#include "char_driver.h"

static dev_t first;     // Global variable for the first device number 
static struct cdev c_dev;   // Global variable for the character device structure
static struct class *cl;    // Global variable for the device class

static char msg_buffer[BUFFER_SIZE];

static int count;       /* initialized to 0 by default */
static uid_t owner;     /* initialized to 0 by default */
static spinlock_t lock; /* = SPIN_LOCK_UNLOCKED; */
static DECLARE_WAIT_QUEUE_HEAD(wait);

static inline int available(void) {
   return count == 0 ||
      owner == current->cred->uid ||
      owner == current->cred->euid ||
      capable(CAP_DAC_OVERRIDE);
}

static int device_open(struct inode *i, struct file *f)
{
    spin_lock(&lock);
    while (!available(  )) {
        spin_unlock(&lock);
        if (f->f_flags & O_NONBLOCK) return -EAGAIN;
        if (wait_event_interruptible (wait, available(  )))
            return -ERESTARTSYS; /* tell the fs layer to handle it */
        spin_lock(&lock);
    }
    if (count == 0)
        owner = current->cred->uid; /* grab it */
    count++;
    spin_unlock(&lock);
    printk(KERN_INFO "Driver: open()\n");
    return 0;
}

static int device_close(struct inode *i, struct file *f)
{
    int temp;

    spin_lock(&lock);
    count--;
    temp = count;
    spin_unlock(&lock);

    if (temp == 0)
        wake_up_interruptible_sync(&wait); /* awake other uid's */
    printk(KERN_INFO "Driver: close()\n");
    return 0;
}

char buf[200];
//static int device_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg) {
static int device_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    int err = 0, tmp;
    int retval = 0;

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != MAGIC_NUMBER) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

	int len = 200;
	switch(cmd) {
		case READ_IOCTL:	
			copy_to_user((char *)arg, buf, 200);
			break;
	
		case WRITE_IOCTL:
			copy_from_user(buf, (char *)arg, len);
			break;

		default:
			return -ENOTTY;
	}
	return len;
}

static ssize_t device_read(struct file *f, char __user *buf, size_t
        len, loff_t *off)
{
    while (len) {
        if (copy_to_user(buf, &msg_buffer, sizeof(msg_buffer)) != 0)
            return -EFAULT;
        else 
            (*off)++;
        len--;
    }
    return 0;

	// Another method to read the single character 
/*    if (*off == 0)
    {
        if (copy_to_user(buf, &msg_buffer, 1) != 0)
            return -EFAULT;
        else
        {
            printk(KERN_INFO "Driver: Buffsadfasdfsda %s %s \n", buf, msg_buffer);
            (*off)++;
        }
    }
    else
        return 0;
*/
}

static ssize_t device_write(struct file *f, const char __user *buf,
        size_t len, loff_t *off)
{
    sprintf(msg_buffer,buf, sizeof(buf));
//    msg_buffer[0] = buf[0];
//    if (copy_from_user(&msg_buffer, buf + len - sizeof(buf), 1) != 0)		// taking care of only last character
    if (copy_from_user(&msg_buffer, buf , 1) != 0)
        return -EFAULT;
    else
        return len;
}

static int __init ofcd_init(void) /* Constructor */
{
    if (alloc_chrdev_region(&first, 0, 1, "Yatendra") < 0)
    {
        return -1;
    }
    printk(KERN_INFO "Registered");
    if ((cl = class_create(THIS_MODULE, "char_drv")) == NULL)
    {
        unregister_chrdev_region(first, 1);
        return -1;
    }
    if (device_create(cl, NULL, first, NULL, "mychar") == NULL)
    {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }
    cdev_init(&c_dev, &pugs_fops);
    if (cdev_add(&c_dev, first, 1) == -1)
    {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return -1;
    }
    return 0;
}

static void __exit ofcd_exit(void) /* Destructor */
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "unregistered");
}

module_init(ofcd_init);
module_exit(ofcd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yatendra");
MODULE_DESCRIPTION("Character Driver");
