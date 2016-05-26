#include"my_eeprom.h"

static atomic_t available = ATOMIC_INIT(1);
static dev_t first_dev_number;
static struct cdev *eeprom_cdev;
static struct class *eeprom_class;
static struct device *eeprom_device;
char *eeprom_dev_name = "my_eeprom";
struct i2c_client *eeprom_client;

#define BASE_MINOR 10
#define NR_DEV 1

int eeprom_open(struct inode *inode, struct file *filep)
{
	printk(KERN_INFO "YS: Open succesful\n");
//	if (! atomic_dec_and_test (&available)) {
//        atomic_inc(&available);
//        return -EBUSY; /* already open */
//    }
	return 0;
}

int eeprom_release(struct inode *in, struct file *fp)
{
	printk(KERN_INFO "YS: Release succesful\n");
//	atomic_inc(&available); /* release the device */
	return 0;
}

int eeprom_probe(struct i2c_client *client, 
        const struct i2c_device_id *id)
{ 
    printk(KERN_INFO "YS: Inside Probe\n");
    eeprom_client = client;
    return 0;
}

int eeprom_write(struct i2c_client *client, unsigned char reg, 
        unsigned char *buffer)
{
    char buffer_write[20];
    buffer_write[0] = 0x50;

    strcpy(&buffer_write[1], "Yatendra");
    int ret = 0;
    struct i2c_msg msg[] = {
        {
            .addr = eeprom_client->addr,
            .flags = 0,
            .len = 8,
            .buf = buffer_write,	
        },
    };

	printk(KERN_INFO "YS: client address = %x\n", eeprom_client->addr);
	printk(KERN_INFO "YS: client adapter = %s\n", eeprom_client->adapter->name);
    ret = i2c_transfer(eeprom_client->adapter, msg, 1);
    if(ret < 0) {
        printk(KERN_INFO "YS: ret = %d\n", ret);
    }
    return ret;
}

int eeprom_read(struct i2c_client *client, unsigned char reg, 
        unsigned char *buffer)
{
    char buffer_read[20];
    reg = 0x50;
    int ret = 0;
    struct i2c_msg msg[] = {
        {
            .addr = eeprom_client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,	
        },
        {
            .addr = eeprom_client->addr,
            .flags = I2C_M_RD,
            .len = 12,
            .buf = buffer_read,	
        },
    };
   	printk(KERN_INFO "YS: client address = %x\n", eeprom_client->addr);
	printk(KERN_INFO "YS: client adapter = %s\n", eeprom_client->adapter->name);
    ret = i2c_transfer(eeprom_client->adapter, msg, 2);
    if(ret < 0) {
        printk(KERN_INFO "YS; ret =  %d\n", ret);
    }
    printk(KERN_INFO "YS: message = %s\n", buffer_read); 
    return ret;
};

int eeprom_remove(struct i2c_client *client)
{
    	printk(KERN_INFO "YS: Remove eeprom\n");
    	return 0;
};

void eeprom_exit(void)
{
    printk(KERN_INFO "YS: Exiting\n");
    device_destroy(eeprom_class, first_dev_number);
    class_destroy(eeprom_class);
    cdev_del(eeprom_cdev);
    unregister_chrdev_region(first_dev_number, NR_DEV);
    i2c_del_driver(&eeprom_driver);
    return;
}

static int __init eeprom_init(void)
{
    if (alloc_chrdev_region(&first_dev_number, BASE_MINOR, NR_DEV, "my_eeprom") <
            0) {
        printk(KERN_INFO "YS: Fail in register a range of char device numbers\n");
        return -ENODEV;
    }
    
    printk(KERN_INFO "YS: Major number = %d\n", MAJOR(first_dev_number));

    if ((eeprom_cdev = cdev_alloc()) == NULL) {
        printk(KERN_INFO "YS: fail in allocate a cdev structure\n");
        unregister_chrdev_region(first_dev_number, NR_DEV);
    }
   
    cdev_init(eeprom_cdev, &eeprom_ops);
    eeprom_cdev->owner = THIS_MODULE;
    int status = cdev_add(eeprom_cdev, first_dev_number, NR_DEV);
    if (status) {
        printk(KERN_INFO "YS: cdev_add fail\n");
        kfree(eeprom_cdev);
    }
    
    eeprom_class = class_create(THIS_MODULE, eeprom_dev_name);
    if (IS_ERR(eeprom_class)) {
        printk(KERN_INFO "YS: Class create fail\n");
        cdev_del(eeprom_cdev);
    }
    eeprom_device = device_create(eeprom_class, NULL, first_dev_number, NULL,
    eeprom_dev_name);
    if (IS_ERR(eeprom_device)) {
        printk(KERN_INFO "YS: Device_create fail\n");
        class_destroy(eeprom_class);
    }

    return i2c_add_driver(&eeprom_driver);
}

MODULE_DEVICE_TABLE(i2c, eeprom_ids);
module_init(eeprom_init);
module_exit(eeprom_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C driver with /dev");
MODULE_AUTHOR("Yatendra");
