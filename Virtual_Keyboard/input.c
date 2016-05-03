#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/io.h>

// Try with the vitual machine

/* This function services keyboard interrupts */
irq_handler_t irq_handler (int irq, void *dev_id, struct pt_regs *regs) {
    static unsigned char scancode;
    // Read keyboard status
    // This family of functions is used to do low-level port input and output. 
    // The out* functions do port output, the in* functions do port input; 
    // the b-suffix functions are byte-width and the w-suffix functions word-width; 
    // the _p-suffix functions pause until the I/O completes.
    // They are primarily designed for internal kernel use, but can be used from user space. 
    scancode = inb (0x60);
    printk (KERN_INFO "Scancode value = %x !\n", scancode);

    if ((scancode == 0x01) || (scancode == 0x81))
    {
        printk (KERN_INFO "You pressed Esc !\n");
    }
    else {
        printk (KERN_INFO "You pressed something else !\n");
    }

    return (irq_handler_t) IRQ_HANDLED;
}

/* Initialize the module and Register the IRQ handler */
static int __init keybrd_int_register(void)
{
    int result;
    /* Request IRQ 1, the keyboard IRQ */    
    printk (KERN_INFO "inside register !\n");
    //  allocate an interrupt line 
    // int request_irq (unsigned int irq, irq_handler_t handler, unsigned long irqflags, const char *devname, void *dev_id);
    // irq: Interrupt line to allocate 
    // handler: Function to be called when the IRQ occurs
    // irqflag: Interrupt type flags 
    // devname: An ascii name for the claiming device 
    // dev_id: A cookie passed back to the handler function    
    result = request_irq (1, (irq_handler_t) irq_handler, IRQF_SHARED, "keyboard_stats_irq", (void *)(irq_handler));

    if (result)
        printk(KERN_INFO "can't get shared interrupt for keyboard\n");

    return result;
}

/* Remove the interrupt handler */
static void __exit keybrd_int_unregister(void) {
    free_irq(1, (void *)(irq_handler)); /* i can't pass NULL, this is a shared interrupt handler! */
}

MODULE_LICENSE ("GPL");
module_init(keybrd_int_register);
module_exit(keybrd_int_unregister);
