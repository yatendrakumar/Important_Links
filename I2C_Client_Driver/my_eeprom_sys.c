#include"my_eeprom.h"

module_param(io_limit, uint, 0);
MODULE_PARM_DESC(io_limit, "Maximum bytes per I/O (default 128)");

module_param(write_timeout, uint, 0);
MODULE_PARM_DESC(write_timeout, "Time (in ms) to try writes (default 25)");

MODULE_DEVICE_TABLE(i2c, eeprom_ids);

static ssize_t eeprom_write(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buffer, loff_t offset, size_t count)
{
    struct eeprom_data *eeprom;
    struct i2c_client *client;
    char buffer_write[count];
    buffer_write[0] = 0x50;
    int addr = 0, j=1;
    printk(KERN_INFO "YS: %s %d\n", __func__, __LINE__);
    eeprom = dev_get_drvdata(container_of(kobj, struct device, kobj));
    printk(KERN_INFO "Offset = %d\n", offset);
    client = eeprom->client[0];
    printk(KERN_INFO "address = %x\n", buffer_write[0]);
    printk(KERN_INFO "buffer = %s\n", buffer);
    printk(KERN_INFO "count = %d\n", count);
    while(*buffer != '\0') {
        buffer_write[j] = *buffer;
        buffer++;
        j++;
    }
    printk(KERN_INFO "Buffer write  = %s\n", buffer_write);
    int ret = 0;
    struct i2c_msg msg[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 8,
            .buf = buffer_write,	
        },
    };

	printk(KERN_INFO "YS: client address = %x\n", client->addr);
	printk(KERN_INFO "YS: client adapter = %s\n", client->adapter->name);
    ret = i2c_transfer(client->adapter, msg, 1);
    if(ret < 0) {
        printk(KERN_INFO "YS: ret = %d\n", ret);
    }
    return ret;

}
static ssize_t eeprom_read(struct file *fd, struct kobject *kobj, struct
        bin_attribute *attr, char *buf, loff_t offset, size_t count) {
    struct eeprom_data *eeprom;
    struct i2c_client *client;
    unsigned i;
    char buffer_read[20];
    char reg = 0x50;
    int ret = 0;

    printk(KERN_INFO "YS: %s %d\n", __func__, __LINE__);
    eeprom = dev_get_drvdata(container_of(kobj, struct device, kobj));
    if(!count){
        printk(KERN_INFO "YS: count is %d returning\n", count); 
        return count;
    }
    if (eeprom->chip.flags & EEPROM_FLAG_ADDR16) {
        i = offset >> 16;
        offset &= 0xffff;
    } else {
        i = offset >> 8;
        offset &= 0xff;
    }
    client = eeprom->client[i];
            
    struct i2c_msg msg[] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,	
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = 12,
            .buf = buffer_read,	
        },
    };
   	printk(KERN_INFO "YS: client address = %x\n", client->addr);
	printk(KERN_INFO "YS: client adapter = %s\n", client->adapter->name);
    ret = i2c_transfer(client->adapter, msg, 2);
    if(ret < 0) {
        printk(KERN_INFO "YS; ret =  %d\n", ret);
    }
    printk(KERN_INFO "YS: message = %s\n", buffer_read); 
    return ret;
}

int eeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct eeprom_platform_data chip;
	bool writable;
	int use_smbus = 0;
	struct eeprom_data *eeprom;
	int err;
	unsigned i, num_addresses;
	kernel_ulong_t driver_data; // driver data from the id table

    printk(KERN_INFO "YS: %s:%d - %s() ", __FILE__, __LINE__, __FUNCTION__ ); 
	if (client->dev.platform_data) {
		chip = *(struct eeprom_platform_data *)client->dev.platform_data;
	} else {
		if (!id->driver_data) {
			err = -ENODEV;
			goto err_out;
		}
		driver_data = id->driver_data;
//		chip.byte_len = BIT(driver_data & EEPROM_BITMASK(EEPROM_SIZE_BYTELEN));
		chip.byte_len = 2048;
        driver_data >>= EEPROM_SIZE_BYTELEN;
		chip.flags = driver_data & EEPROM_BITMASK(EEPROM_SIZE_FLAGS);
		chip.page_size = 1;

        // Not checking for page size from dts
        if(client->dev.of_node){
            if (of_get_property(client->dev.of_node, "read-only", NULL))
                chip.flags |= EEPROM_FLAG_READONLY;
            int val = of_get_property(client->dev.of_node, "pagesize", NULL);
            if (val)
                chip.page_size = be32_to_cpup(val);
        }
//		eeprom_get_ofdata(client, &chip);

		chip.setup = NULL;
		chip.context = NULL;
	}

	if (!is_power_of_2(chip.byte_len))
		printk(KERN_INFO "byte_len looks suspicious (no power of 2)!\n");
	if (!chip.page_size) {
		dev_err(&client->dev, "page_size must not be 0!\n");
		err = -EINVAL;
		goto err_out;
	}
	if (!is_power_of_2(chip.page_size))
	    printk(KERN_INFO "page_size looks suspicious (no power of 2)!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "I2c Functionality is not supported\n");
        err = -EPFNOSUPPORT;
        goto err_out;
	}

    printk(KERN_INFO "Chip flag = %d\n", chip.flags);
    printk(KERN_INFO "Chip byte_len = %d\n", chip.byte_len);
	if (chip.flags & EEPROM_FLAG_TAKE8ADDR)
		num_addresses = 8;
	else
		num_addresses =	DIV_ROUND_UP(chip.byte_len,
			(chip.flags & EEPROM_FLAG_ADDR16) ? 65536 : 256);

    // Setting memory for the eeprom_data
	eeprom = kzalloc(sizeof(struct eeprom_data) +
		num_addresses * sizeof(struct i2c_client *), GFP_KERNEL);
	if (!eeprom) {
		err = -ENOMEM;
		goto err_out;
	}

//	mutex_init(&eeprom->lock);
	eeprom->chip = chip;
	eeprom->num_addresses = num_addresses;

	sysfs_bin_attr_init(&eeprom->bin);
	eeprom->bin.attr.name = "my_eeprom";
	eeprom->bin.attr.mode = chip.flags & EEPROM_FLAG_IRUGO ? S_IRUGO : S_IRUSR;
	eeprom->bin.read = eeprom_read;
	eeprom->bin.write = eeprom_write;
	eeprom->bin.size = chip.byte_len;

	writable = !(chip.flags & EEPROM_FLAG_READONLY);
	if (writable)
        printk(KERN_INFO "We can write into the I2C client\n");
    else 
        printk(KERN_INFO "We cann't write into the I2C client\n");

	eeprom->client[0] = client;

    // i2c_new_dummy 
    // without this unregistering call halt the system
    // unregister client is not happening
	for (i = 6; i < 7; i++) {
		eeprom->client[i-5] = i2c_new_dummy(client->adapter,
					client->addr + i);
		if (!eeprom->client[i-5]) {
			dev_err(&client->dev, "address 0x%02x unavailable\n",
					client->addr + i);
			err = -EADDRINUSE;
			goto err_clients;
		}
	}

	err = sysfs_create_bin_file(&client->dev.kobj, &eeprom->bin);
	if (err)
		goto err_clients;

	i2c_set_clientdata(client, eeprom);
    printk(KERN_INFO "Client Address = %x\n", eeprom->client[0]->addr);
    printk(KERN_INFO "Client Address = %x\n", client->addr);
	dev_info(&client->dev, "%zu byte %s EEPROM, %s, %u bytes/write\n",
		eeprom->bin.size, client->name,
		writable ? "writable" : "read-only", eeprom->write_max);
	printk(KERN_INFO "%zu byte %s EEPROM, %s, %u bytes/write\n",
		eeprom->bin.size, client->name,
		writable ? "writable" : "read-only", eeprom->write_max);

	return 0;

err_clients:
	printk(KERN_INFO "Sysfs client err\n");
	for (i = 6; i < 7; i++)
		if (eeprom->client[i-5])
			i2c_unregister_device(eeprom->client[i-5]);

    return err;
err_out:
	dev_dbg(&client->dev, "probe error %d\n", err);
	printk(KERN_INFO "probe error %d\n", err);
	return err;
}

static int eeprom_remove(struct i2c_client *client)
{
	struct eeprom_data *eeprom;
	int i;

    printk(KERN_INFO "Removing \n");
	eeprom = i2c_get_clientdata(client);
    if (!eeprom) {
        printk(KERN_INFO "Not able to find the client\n");
    }
    printk(KERN_INFO "eeprom bin attribute name = %s\n", eeprom->bin.attr.name);
	sysfs_remove_bin_file(&client->dev.kobj, &eeprom->bin);

    printk(KERN_INFO "Client Address = %x\n", eeprom->client[0]->addr);
    printk(KERN_INFO "Client Address = %x\n", client->addr);
//	i2c_unregister_device(client);
	for (i = 6; i < 7; i++)
		i2c_unregister_device(eeprom->client[i-5]);
    printk(KERN_INFO "Unregistered device\n");

//	kfree(eeprom->writebuf);
//	kfree(eeprom);
	return 0;
}

static int __init eeprom_init(void)
{
    printk(KERN_INFO "Inside init\n");
	io_limit = rounddown_pow_of_two(128);
    printk(KERN_INFO "Driver name = %s\n", eeprom_driver.driver.name);
    return i2c_add_driver(&eeprom_driver);
}

module_init(eeprom_init);

static void __exit eeprom_exit(void)
{
	i2c_del_driver(&eeprom_driver);
}
module_exit(eeprom_exit);

MODULE_DESCRIPTION("Driver for most I2C EEPROMs");
MODULE_AUTHOR("Yatendra");
MODULE_LICENSE("GPL");
