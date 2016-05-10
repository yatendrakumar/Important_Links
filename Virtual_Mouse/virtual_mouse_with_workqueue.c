#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/random.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

static struct input_dev *vms_input_dev;
static struct platform_device *vms_dev;
static struct timer_list vms_timer;
static struct work_struct vms_work;

static int x = 100, y = 100;

static void vms_work_fn(struct work_struct *data)
{
	input_report_rel(vms_input_dev, REL_X, x);
	input_report_rel(vms_input_dev, REL_Y, y);
	input_sync(vms_input_dev);

	x++; y++;

	if (x > 300) {
		x = 50;
		y=50;
	}
}

void vms_timer_expired(unsigned long arg)
{
	vms_timer.expires = jiffies + HZ;
	//add_timer(&vms_timer);
	schedule_work(&vms_work);
}

int __init vms_init(void)
{
	unsigned long current_time = jiffies;
	unsigned long expiry_time = current_time + HZ;

	vms_dev = platform_device_register_simple("vms", -1, NULL, 0);
	if (IS_ERR(vms_dev)) {
		PTR_ERR(vms_dev);
		printk(KERN_ERR "platform_device_register_simple() failed");
		return -1;
	}

	vms_input_dev = input_allocate_device();
	if (!vms_input_dev) {
		printk(KERN_ERR "input_alloc_device() failed");
		return -1;
	}

	vms_input_dev->name = "Virtual Mouse";
	vms_input_dev->phys = "vms/input0";
	vms_input_dev->id.bustype = BUS_VIRTUAL;
	vms_input_dev->id.vendor  = 0x0000;
	vms_input_dev->id.product = 0x0000;
	vms_input_dev->id.version = 0x0000;
	vms_input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	vms_input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	vms_input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	vms_input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
	vms_input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);
	input_register_device(vms_input_dev);

	INIT_WORK(&vms_work, vms_work_fn);

	init_timer(&vms_timer);
	vms_timer.function = vms_timer_expired;
	vms_timer.expires = expiry_time;
	vms_timer.data = 0;
	add_timer(&vms_timer);

	return 0;
}

void vms_cleanup(void)
{
	input_unregister_device(vms_input_dev);
	platform_device_unregister(vms_dev);
	del_timer(&vms_timer);
	cancel_work_sync(&vms_work);

	return;
}

module_init(vms_init);
module_exit(vms_cleanup);

MODULE_AUTHOR("Yatendra ");
MODULE_DESCRIPTION("Virtual Mouse device Driver");
MODULE_LICENSE("GPL");

