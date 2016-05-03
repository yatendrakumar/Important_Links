#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/module.h>

// Input device
struct input_dev* virtual_mouse_input_dev;
// Platform device
static struct platform_device* virtual_mouse_dev;

static ssize_t write_mouse_virtual(struct device* dev,
                         struct device_attribute* attr,
                         const char* buffer, size_t count)
{
   int x,y;
   /* count is not used !? */
   sscanf(buffer, "%d%d", &x, &y);
   
   // report relative event
   input_report_rel(virtual_mouse_input_dev, REL_X, x);
   input_report_rel(virtual_mouse_input_dev, REL_Y, y);

   // call to tell those who receive the events that we've sent a complete report
   // where you don't want the X and Y values to be interpreted separately, because that'd result in a different movement.
   input_sync(virtual_mouse_input_dev);
   printk("x = %d, y = %d\n", x, y);
   
   return count;
}

DEVICE_ATTR(coordinates, 0664, NULL, write_mouse_virtual);

static struct attribute* virtual_mouse_attr[] = 
{
   &dev_attr_coordinates.attr,
   NULL
};

static struct attribute_group virtual_mouse_attr_group = 
{
   .attrs = virtual_mouse_attr,
};

int __init virtual_mouse_init(void)
{
   // Add a platform-level device and its resources 
   /* You can use platform_device_alloc() to dynamically allocate a device, which
    * you will then initialize with resources and platform_device_register().
    * A better solution is usually:
        struct platform_device *platform_device_register_simple(
                const char *name, int id,
                struct resource *res, unsigned int nres);
    * You can use platform_device_register_simple() as a one-step call to allocate
    * and register a device.
    */
   virtual_mouse_dev = platform_device_register_simple("virtual_mouse", -1, NULL, 0);
   // test for an error condition from a pointer  
   // Returns non-0 value if the ptr is an error. Otherwise 0 if it's not an error.
   if(IS_ERR(virtual_mouse_dev))
   {
      PTR_ERR(virtual_mouse_dev);
      printk("virtual_mouse_init: error\n");
   }
   // create a group of files in sysfs file system using  sysfs_create_group
   // The first argument to this function is a pointer to kobject (or directory ) 
   // under which files should be created. Second argument is the group of files specified using attribute_group.
   sysfs_create_group(&virtual_mouse_dev->dev.kobj, &virtual_mouse_attr_group);
   
   // allocate memory for new input device 
   // struct input_dev * input_allocate_device (void);
   // Returns prepared struct input_dev or NULL
   virtual_mouse_input_dev = input_allocate_device();

   if(!virtual_mouse_input_dev)
   {
      printk("Bad input_alloc_device()\n");
   }

   virtual_mouse_input_dev->name = "Test";

   //   virtual_mouse_input_dev->id.bustype = BUS_HOST;
   // Atomically set a bit in memory 
   // first argument: the bit to set
   // second argument: the address to start counting from
   // This function is atomic and may not be reordered
   set_bit(EV_REL, virtual_mouse_input_dev->evbit);   
   set_bit(REL_X, virtual_mouse_input_dev->relbit);
   set_bit(REL_Y, virtual_mouse_input_dev->relbit);

   // mark device as capable of a certain event
   // void input_set_capability (struct input_dev * dev,	unsigned int type, unsigned int code);
   // dev: device that is capable of emitting or accepting event 
   // type: type of the event (EV_KEY, EV_REL, etc...) 
   // code: event code
   input_set_capability(virtual_mouse_input_dev, EV_KEY, BTN_LEFT);
   input_set_capability(virtual_mouse_input_dev, EV_KEY, BTN_MIDDLE);
   input_set_capability(virtual_mouse_input_dev, EV_KEY, BTN_RIGHT);

   // register device with input core 
   // dev: device to be registered
   // This function registers device with input core. 
   // The device must be allocated with input_allocate_device and all it's capabilities set up before registering. 
   // If function fails the device must be freed with input_free_device. 
   // Once device has been successfully registered it can be unregistered with input_unregister_device; 
   // input_free_device should not be called in this case. 
   input_register_device(virtual_mouse_input_dev);
   printk("virtual Mouse Driver Initialized.\n");
   
   return 0;
}

// Exit/Cleanup function
void __exit virtual_mouse_cleanup(void)
{
   // Use input_free_device to free devices that have not been registered; 
   // input_unregister_device should be used for already registered devices
   input_unregister_device(virtual_mouse_input_dev);
   
   sysfs_remove_group(&virtual_mouse_dev->dev.kobj, &virtual_mouse_attr_group);
   
   platform_device_unregister(virtual_mouse_dev);
   
   return;
}

module_init(virtual_mouse_init);
module_exit(virtual_mouse_cleanup);

MODULE_AUTHOR("Yatendra");
MODULE_LICENSE("GPL");
