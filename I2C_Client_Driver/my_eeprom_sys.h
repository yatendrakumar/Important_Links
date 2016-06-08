/*
 * eeprom.c - handle most I2C EEPROMs
 */
#ifndef __EEPROM__
#define __EEPROM__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/i2c.h>
//#include <linux/i2c/at24.h>
#include <linux/i2c/eeprom.h>
#include <linux/types.h>
#include <linux/memory.h>
#include <linux/uaccess.h>

struct eeprom_platform_data {
	u32		byte_len;		/* size (sum of all addr) */
	u16		page_size;		/* for writes */
	u8		flags;
#define EEPROM_FLAG_ADDR16	0x80	/* address pointer is 16 bit */
#define EEPROM_FLAG_READONLY	0x40	/* sysfs-entry will be read-only */
#define EEPROM_FLAG_IRUGO		0x20	/* sysfs-entry will be world-readable */
#define EEPROM_FLAG_TAKE8ADDR	0x10	/* take always 8 addresses (24c00) */
	void		(*setup)(struct memory_accessor *, void *context);
	void		*context;
};

struct eeprom_data {
	struct eeprom_platform_data chip;
	struct memory_accessor macc;
	int use_smbus;
	struct mutex lock;
	struct bin_attribute bin;
	u8 *writebuf;
	unsigned write_max;
	unsigned num_addresses;
	struct i2c_client *client[];
};

const struct i2c_device_id eeprom_ids[] = {
	{ "my_eeprom", 1 },
	{ }
};

unsigned io_limit;
unsigned write_timeout = 25;
#define EEPROM_SIZE_BYTELEN 5
#define EEPROM_SIZE_FLAGS 8
#define EEPROM_BITMASK(x) (BIT(x) - 1)

int eeprom_probe(struct i2c_client *client, const struct i2c_device_id
*id);
static int eeprom_remove(struct i2c_client *client);
struct i2c_driver eeprom_driver = {
	.driver = {
		.name = "my_eeprom",
		.owner = THIS_MODULE,
	},
    .id_table = eeprom_ids,
	.probe = eeprom_probe,
	.remove = eeprom_remove,
//	.command = eeprom_command,
};

#endif
