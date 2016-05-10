/*
 * Driver for memory based ft5406 touchscreen
 *
 * Copyright (C) 2015 Raspberry Pi
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/input/mt.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <soc/bcm2835/raspberrypi-firmware.h>

#define MAXIMUM_SUPPORTED_POINTS 10
struct ft5406_regs {
	uint8_t device_mode;
	uint8_t gesture_id;
	uint8_t num_points;
	struct ft5406_touch {
		uint8_t xh;
		uint8_t xl;
		uint8_t yh;
		uint8_t yl;
		uint8_t res1;
		uint8_t res2;
	} point[MAXIMUM_SUPPORTED_POINTS];
};

// Screen size 
// Default screen orientation changed
// Now the rotation is 270 degree (Please change in config file of raspberry pi)
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 800

struct ft5406 {
	struct platform_device * pdev;
	struct input_dev       * input_dev;
	void __iomem           * ts_base;
	struct ft5406_regs     * regs;
	struct task_struct     * thread;
};

/* Thread to poll for touchscreen events
 * 
 * This thread polls the memory based register copy of the ft5406 registers
 * using the number of points register to know whether the copy has been
 * updated (we write 99 to the memory copy, the GPU will write between 
 * 0 - 10 points)
 */
static int ft5406_thread(void *arg)
{
	struct ft5406 *ts = (struct ft5406 *) arg;
	struct ft5406_regs regs;
	int known_ids = 0;
	
    /**
     * kthread_should_stop - should this kthread return now?
     *
     * When someone calls kthread_stop() on your kthread, it will be woken
     * and this will return true.  You should then return, and your return
     * value will be passed through to kthread_stop().
     */
	while(!kthread_should_stop())
	{
		// 60fps polling
		msleep(17);
        /*
         * Copy data from IO memory space to "real" memory space.
         */
		memcpy_fromio(&regs, ts->regs, sizeof(*ts->regs));
		writel(99, &ts->regs->num_points);
		// Do not output if theres no new information (num_points is 99)
		// or we have no touch points and don't need to release any
		if(!(regs.num_points == 99 || (regs.num_points == 0 && known_ids == 0)))
		{
			int i;
			int modified_ids = 0, released_ids;
            printk(KERN_INFO "ft5406-debug: regs points %d\n", regs.num_points);
			for(i = 0; i < regs.num_points; i++)
			{
                printk(KERN_INFO "ft5406-debug: xh = %d and xl = %d\n", regs.point[i].xh, regs.point[i].xl);
                printk(KERN_INFO "ft5406-debug: yh = %d and yl = %d\n", regs.point[i].yh, regs.point[i].yl);
				int x = (((int) regs.point[i].xh & 0xf) << 8) + regs.point[i].xl;
                int y = 0;
                if (((int)regs.point[i].yh) == 1)
                    y = (SCREEN_WIDTH/2) - (int)regs.point[i].yl;
                else 
    				y = (SCREEN_WIDTH/2) + ((1 & 0xf) << 8) - regs.point[i].yl;
				int touchid = (regs.point[i].xh >> 4) & 0xf;
				
				modified_ids |= 1 << touchid;

				if(!((1 << touchid) & known_ids))
					dev_dbg(&ts->pdev->dev, "x = %d, y = %d, touchid = %d\n", x, y, touchid);
					printk(KERN_INFO "ft5406-debug: x = %d, y = %d, touchid = %d\n", x, y, touchid);
				
                /**
                 * internally called by input_mt_slot
	             * input_event(dev, EV_ABS, ABS_MT_SLOT, slot); // slot is touchid
                 * input_event() - report new input event
                 * @dev: device that generated the event
                 * @type: type of the event
                 * @code: event code
                 * @value: value of the event
                 *
                 * This function should be used by drivers implementing various input
                 * devices to report input events. See also input_inject_event().
                 *
                 * NOTE: input_event() may be safely used right after input device was
                 * allocated with input_allocate_device(), even before it is registered
                 * with input_register_device(), but the event will not reach any of the
                 * input handlers. Such early invocation of input_event() may be used
                 * to 'seed' initial state of a switch or initial position of absolute
                 * axis, etc.
                 *  void input_event(struct input_dev *dev,
                 *      unsigned int type, unsigned int code, int value)
                 */
				input_mt_slot(ts->input_dev, touchid);
                /**
                 * input_mt_report_slot_state() - report contact state
                 * @dev: input device with allocated MT slots
                 * @tool_type: the tool type to use in this slot
                 * @active: true if contact is active, false otherwise
                 *
                 * Reports a contact via ABS_MT_TRACKING_ID, and optionally
                 * ABS_MT_TOOL_TYPE. If active is true and the slot is currently
                 * inactive, or if the tool type is changed, a new tracking id is
                 * assigned to the slot. The tool type is only reported if the
                 * corresponding absbit field is set.
                 */
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);

	            // internally called by input_report_abs with different flag as input_mt_slot called it
                // input_event(dev, EV_ABS, code, value);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, y);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, x);

			}

			released_ids = known_ids & ~modified_ids;
		    printk(KERN_INFO "ft5406-debug: known ids = %x modified = %x\n", known_ids, modified_ids);

			for(i = 0; released_ids && i < MAXIMUM_SUPPORTED_POINTS; i++)
			{
				if(released_ids & (1<<i))
				{
					dev_dbg(&ts->pdev->dev, "Released %d, known = %x modified = %x\n", i, known_ids, modified_ids);
					printk(KERN_INFO "ft5406-debug: mt Released %d, known = %x modified = %x\n", i, known_ids, modified_ids);
					input_mt_slot(ts->input_dev, i);
					input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
					modified_ids &= ~(1 << i);
				}
			}
			known_ids = modified_ids;
    		printk(KERN_INFO "ft5406-debug: outside Released %d, known = %x modified = %x\n", i, known_ids, modified_ids);
			
            /**
             * input_mt_report_pointer_emulation() - common pointer emulation
             * @dev: input device with allocated MT slots
             * @use_count: report number of active contacts as finger count
             *
             * Performs legacy pointer emulation via BTN_TOUCH, ABS_X, ABS_Y and
             * ABS_PRESSURE. Touchpad finger count is emulated if use_count is true.
             *
             * The input core ensures only the KEY and ABS axes already setup for
             * this device will produce output.
             */
			input_mt_report_pointer_emulation(ts->input_dev, true);
			input_sync(ts->input_dev);
		}
	}
	
	return 0;
}

static int ft5406_probe(struct platform_device *pdev)
{
	int ret;
    /**
     * input_allocate_device - allocate memory for new input device
     *
     * Returns prepared struct input_dev or %NULL.
     *
     * NOTE: Use input_free_device() to free devices that have not been
     * registered; input_unregister_device() should be used for already
     * registered devices.
     */
	struct input_dev * input_dev = input_allocate_device();
	struct ft5406 * ts;
	struct device_node *fw_node;
	struct rpi_firmware *fw;
	u32 touchbuf;
	
	dev_info(&pdev->dev, "Probing device\n");
    printk(KERN_INFO "ft-5406-debug: Probing device\n");
	
    /**
     * of_parse_phandle - Resolve a phandle property to a device_node pointer
     * @np: Pointer to device node holding phandle property
     * @phandle_name: Name of property holding a phandle value
     * @index: For properties holding a table of phandles, this is the index into
     *         the table
     *
     * Returns the device_node pointer with refcount incremented.  Use
     * of_node_put() on it when done.
     * struct device_node *of_parse_phandle(const struct device_node *np,
                         const char *phandle_name, int index)
     * @of_node: Associated device tree node                   
     * @fw_node: Associated device node supplied by platform firmware
     */
	fw_node = of_parse_phandle(pdev->dev.of_node, "firmware", 0);
	if (!fw_node) {
		dev_err(&pdev->dev, "Missing firmware node\n");
        printk(KERN_INFO "ft-5406-debug: Missing firmware node\n");
		return -ENOENT;
	}

    /**
     * rpi_firmware_get - Get pointer to rpi_firmware structure.
     * @firmware_node:    Pointer to the firmware Device Tree node.
     *
     * Returns NULL is the firmware device is not ready.
     * struct rpi_firmware *rpi_firmware_get(struct device_node *firmware_node)
     */
	fw = rpi_firmware_get(fw_node);
	if (!fw)
		return -EPROBE_DEFER;

    /**
     * rpi_firmware_property - Submit single firmware property
     * @fw:		Pointer to firmware structure from rpi_firmware_get().
     * @tag:	One of enum_mbox_property_tag.
     * @tag_data:	Tag data buffer.
     * @buf_size:	Buffer size.
     *
     * Submits a single tag to the VPU firmware through the mailbox
     * property interface.
     *
     * This is a convenience wrapper around
     * rpi_firmware_property_list() to avoid some of the
     * boilerplate in property calls.
     */
    printk(KERN_INFO "ft-5406-debug: Size of touch buffer = %d\n", sizeof(touchbuf));
    printk(KERN_INFO "ft-5406-debug: TS Buffer at 0x%x\n", touchbuf);
	ret = rpi_firmware_property(fw, RPI_FIRMWARE_FRAMEBUFFER_GET_TOUCHBUF,
				    &touchbuf, sizeof(touchbuf));
	if (ret) {
		dev_err(&pdev->dev, "Failed to get touch buffer\n");
        printk(KERN_INFO "ft-5406-debug: failed to get touch buffer\n");
		return ret;
	}

	if (!touchbuf) {
		dev_err(&pdev->dev, "Touchscreen not detected\n");
        printk(KERN_INFO "ft-5406-debug: Touch Screen not detected\n");
		return -ENODEV;
	}

	dev_dbg(&pdev->dev, "Got TS buffer 0x%x\n", touchbuf);
    printk(KERN_INFO "ft-5406-debug: Got TS Buffer 0x%x\n", touchbuf);

    /**
     * kzalloc - allocate memory. The memory is set to zero.
     * @size: how many bytes of memory are required.
     * @flags: the type of memory to allocate (see kmalloc).
     */
    // When GFP_KERNEL is specified, it is an indicationthat memory request is from a kernel
    // running in a process context.So, Kernel may block the process if requested memory is not availbale immediately.
    // If GFP_ATOMIC is used, Kernel will never block the process.
    // __GFP_DMA is added if memory is required for DMA purpose.
	ts = kzalloc(sizeof(struct ft5406), GFP_KERNEL);

	if (!ts || !input_dev) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Failed to allocate memory\n");
		return ret;
	}
	ts->input_dev = input_dev;
	platform_set_drvdata(pdev, ts);
	ts->pdev = pdev;
	
	input_dev->name = "FT5406 memory based driver";
	
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);

    /**
     * Called by input_set_abs_params
     * input_alloc_absinfo - allocates array of input_absinfo structs
     * @dev: the input device emitting absolute events
     *
     * If the absinfo struct the caller asked for is already allocated, this
     * functions will not do anything.
     */
     /**
      * absinfo changed inside the input_set_abs_params
      * absinfo - array of struct input_absinfo elements holding information about absolute axes (current value, min, max, flat, fuzz, resolution) 
      */

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
			     SCREEN_WIDTH, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
			     SCREEN_HEIGHT, 0, 0);

    /**
     * input_mt_init_slots() - initialize MT input slots
     * @dev: input device supporting MT events and finger tracking
     * @num_slots: number of slots used by the device
     * @flags: mt tasks to handle in core
     *
     * This function allocates all necessary memory for MT slot handling
     * in the input device, prepares the ABS_MT_SLOT and
     * ABS_MT_TRACKING_ID events for use and sets up appropriate buffers.
     * Depending on the flags set, it also performs pointer emulation and
     * frame synchronization.
     *
     * May be called repeatedly. Returns -EINVAL if attempting to
     * reinitialize with a different number of slots.
     */
	input_mt_init_slots(input_dev, MAXIMUM_SUPPORTED_POINTS, INPUT_MT_DIRECT);

	input_set_drvdata(input_dev, ts);
	
	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&pdev->dev, "could not register input device, %d\n",
			ret);
		return ret;
	}
	
	// mmap the physical memory
	touchbuf &= ~0xc0000000;
    //touchbuf++;
    printk(KERN_INFO "ft-5406-debug: Got TS Buffer after Physical mapping at 0x%x\n", touchbuf);
    printk(KERN_INFO "ft-5406-debug: Size of *ts-> Regs = %d\n", sizeof(*ts->regs));
    printk(KERN_INFO "ft-5406-debug: Size of ts-> Regs = %d\n", sizeof(ts->regs));
	ts->ts_base = ioremap(touchbuf, sizeof(*ts->regs));
    printk(KERN_INFO "ft-5406-debug: 1 Address of ts-> base after ioremap = 0x%x\n", ts->ts_base);
    printk(KERN_INFO "ft-5406-debug: 2 Address of ts-> base after ioremap = 0d%x\n", ts->ts_base);
	if(ts->ts_base == NULL)
	{
		dev_err(&pdev->dev, "Failed to map physical address\n");
		input_unregister_device(input_dev);
		kzfree(ts);
		return -ENOMEM;
	}
	
	ts->regs = (struct ft5406_regs *) ts->ts_base;

	// create thread to poll the touch events
	ts->thread = kthread_run(ft5406_thread, ts, "ft5406");
	if(ts->thread == NULL)
	{
		dev_err(&pdev->dev, "Failed to create kernel thread");
		iounmap(ts->ts_base);
		input_unregister_device(input_dev);
		kzfree(ts);
	}

	return 0;
}

static int ft5406_remove(struct platform_device *pdev)
{
	struct ft5406 *ts = (struct ft5406 *) platform_get_drvdata(pdev);
	
	dev_info(&pdev->dev, "Removing rpi-ft5406\n");
    	
	kthread_stop(ts->thread);
	iounmap(ts->ts_base);
	input_unregister_device(ts->input_dev);
	kzfree(ts);
	
	return 0;
}

/*
 * Struct used for matching a device
 */
static const struct of_device_id ft5406_match[] = {
	{ .compatible = "rpi,rpi-ft5406", },
	{},
};
MODULE_DEVICE_TABLE(of, ft5406_match);

static struct platform_driver ft5406_driver = {
	.driver = {
		.name   = "ys_rpi-ft5406",
		.owner  = THIS_MODULE,
		.of_match_table = ft5406_match,
	},
	.probe          = ft5406_probe,
	.remove         = ft5406_remove,
};

module_platform_driver(ft5406_driver);

MODULE_AUTHOR("Yatendra");
MODULE_DESCRIPTION("Touchscreen driver for memory based FT5406");
MODULE_LICENSE("GPL");
