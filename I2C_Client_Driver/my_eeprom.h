#ifndef _EEPROM_I2C_
#define _EEPROM_I2C_

#include<linux/i2c.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/of.h>
#include<linux/slab.h>
#include<linux/spinlock.h>
#include<linux/printk.h>
#include<linux/kobject.h>
#include<linux/sysfs.h>
#include<linux/fs.h>
#include<linux/string.h>
#include<linux/errno.h>
#include<asm/uaccess.h> 
#include<linux/device.h> 
#include<linux/sched.h>
#include<linux/cdev.h> 
#include<linux/types.h>
#include<linux/err.h>

const struct i2c_device_id eeprom_ids[] = {
    {"my_eeprom", 0},
    {"my_eeprom", 2},
    {}
};

int eeprom_probe(struct i2c_client *client, const struct i2c_device_id *id); 
int eeprom_remove(struct i2c_client *client);
int eeprom_open(struct inode *inode, struct file *filep);
int eeprom_release(struct inode *in, struct file *fp);
int eeprom_read(struct i2c_client *client, unsigned char reg, 
        unsigned char *buffer);
int eeprom_write(struct i2c_client *client, unsigned char reg, 
        unsigned char *buffer);

//void eeprom_exit(void);

struct i2c_driver eeprom_driver = {
   .driver = {
        .name = "my_eeprom",
        .owner = THIS_MODULE,
   },
   .id_table = eeprom_ids,
   .probe = eeprom_probe,
   .remove = eeprom_remove,
};

struct file_operations eeprom_ops = {
    .owner = THIS_MODULE,
    .open = eeprom_open,
    .release = eeprom_release,
    .read = eeprom_read,
    .write = eeprom_write,
};

#endif

