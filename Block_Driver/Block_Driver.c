#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> 
#include <linux/fs.h>     
#include <linux/errno.h>  
#include <linux/types.h>  
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

MODULE_LICENSE("GPL");
static int major_num = 0;
static int l_block_size = 512;
static int sectors = 10000;

#define KERNEL_SECTOR_SIZE 512

static struct request_queue *Queue;

// Internal representation of the device
static struct my_device {
    unsigned long size;	// size of the device
    spinlock_t lock;	// array where the disk stores its data
    u8 *data;			// for mutual exclusion
    struct gendisk *gd;	// kernel representation of our device
} Device;

int getgeo(struct block_device * block_device, struct hd_geometry * geo) {
    long size;

    size = Device.size * (l_block_size / KERNEL_SECTOR_SIZE);
    geo->cylinders = (size & ~0x3f) >> 6;
    geo->heads = 4;
    geo->sectors = 16;
    geo->start = 0;
    return 0;
}

int ioctl (struct inode *inode, struct file *filp,
        unsigned int cmd, unsigned long arg)
{
    long size;
    struct hd_geometry geo;
    printk(KERN_INFO "Inside ioctl\n");
    printk(KERN_INFO "Cmd = %d, HDIO = %d \n", cmd, HDIO_GETGEO);
    switch(cmd) {
        case HDIO_GETGEO:
            size = Device.size*(l_block_size/KERNEL_SECTOR_SIZE);
            geo.cylinders = (size & ~0x3f) >> 6;
            geo.heads = 4;
            geo.sectors = 16;
            geo.start = 0;
            printk(KERN_INFO "Inside ioctl case\n");
            if (copy_to_user((void *) arg, &geo, sizeof(geo)))
                return -EFAULT;
            return 0;
    }
    return -ENOTTY; 
}

static struct block_device_operations ops = {
    .owner = THIS_MODULE,
    .getgeo = getgeo,
    .ioctl = ioctl
};

// is really a just a memcopy with some checking
static void transfer(struct my_device *dev, sector_t sector,
        unsigned long nsect, char *buffer, int write) {
    unsigned long offset = sector * l_block_size;
    unsigned long nbytes = nsect * l_block_size;

    printk (KERN_INFO "Inside transfer\n");
    printk (KERN_INFO "offset = %ld, nbytes = %ld, dev-> size = %ld\n", offset, nbytes, dev->size);
    if ((offset + nbytes) > dev->size) {
        printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }
    printk (KERN_INFO "write = %d\n", write);
    if (write)
        memcpy(dev->data + offset, buffer, nbytes);
    else
        memcpy(buffer, dev->data + offset, nbytes);
}

// Device.lock will be held on the entry to the request function
static void request(struct request_queue *q) {
    struct request *req;
    // For getting the first request in the queue is now elv_next_request (Can use blk_fetch_request)
    // A NULL return means there are no more requests on the queue that are ready to process
    // function does not actually remove the request from the queue; it just a properly adjusted view of the top request
    req = blk_fetch_request(q);

    printk("Request %s \n", req);
    while (req != NULL) {
        if (req->cmd_type != REQ_TYPE_FS) {
            printk("Cehcking FS request \n");
            printk (KERN_NOTICE "Skip non-CMD request\n");
            // is called to finish processing of this request
            __blk_end_request_all(req, -EIO);
            continue;
        }
        transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
                req->buffer, rq_data_dir(req));
        if ( ! __blk_end_request_cur(req, 0) ) {
            req = blk_fetch_request(q);
        }
    }
    printk (KERN_INFO "Leaving request\n");
}

static __init Start(void) {
    spin_lock_init(&Device.lock);
    Device.size = sectors * l_block_size;
    Device.data = vmalloc(Device.size);
    if (Device.data == NULL)
        return -ENOMEM;

    // lock is used by the driver to serialize access to intenal resources is the best choice for controlling
    // access to the request queue as well
    Queue = blk_init_queue(request, &Device.lock);
    if (Queue == NULL) {
        vfree(Device.data);
        return -ENOMEM;
    }

    blk_queue_logical_block_size(Queue, l_block_size);
    // no device operations structure is passed to this function
    // only task performed by this function "register_blkdev" are the assignment of a dynamic major number
    // causing the block driver to show up in /proc/devices

    major_num = register_blkdev(major_num, "yatendra");
    if (major_num < 0) {
        printk( KERN_WARNING "Unable to get major number");
        vfree(Device.data);
        return -ENOMEM;
    }

    // gendisk setup
    // parameter to alloc_disk(n) is number of minor number that should be dedicated to this device
    // n-1 will suported patitions
    Device.gd = alloc_disk(2);
    if (!Device.gd) 
        unregister_blkdev(major_num, "yatendra");

    Device.gd->major = major_num;
    Device.gd->first_minor = 0;
    Device.gd->fops = &ops;
    Device.gd->private_data = &Device;
    strcpy(Device.gd->disk_name, "yatendra");

    // tell how large the device is
    set_capacity(Device.gd, sectors);
    // a pointer to the queue must be stored in the gendisk structure
    Device.gd->queue = Queue;
    // add the disk to the system
    // add_disk may well generates I/O to the device before it returns- the driver must be in state where it can handle
    // requests before adding disks. The driver also should not fail initialization after it has successfully added a disk
    add_disk(Device.gd);

    return 0;
}

static void __exit Stop(void) {
    del_gendisk(Device.gd);
    put_disk(Device.gd);
    unregister_blkdev(major_num, "yatendra");
    blk_cleanup_queue(Queue);
    vfree(Device.data);
}

module_init(Start);
module_exit(Stop);
