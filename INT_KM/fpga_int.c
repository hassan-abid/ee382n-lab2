/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  fpga_int.c
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  AUTHOR: 	Sarnab Bhattacharya
 *  CREATED: 	March 15, 2018
 *
 *  DESCRIPTION: This kernel module registers interrupts from the FPGA 
 *
 *  DEPENDENCIES: none
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */




/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/module.h>
#include <asm/gpio.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/gpio/driver.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define MODULE_VER "1.0"

#define INTERRUPT 164	// 

#define FPGA_MAJOR 200
#define MODULE_NM "fpga_int"

#undef DEBUG

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int interruptcount = 0, temp = 0, len = 0;
char *msg;
unsigned int gic_interrupt;

static struct proc_dir_entry *interrupt_arm_file;

static struct fasync_struct *fasync_fpga_queue ;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: interrupt_handler
 *
 */

irqreturn_t interrupt_handler(int irq, void *dev_id)
{
    interruptcount++;

#ifdef DEBUG
    printk(KERN_INFO "fpga_int: Interrupt detected in kernel \n");  // DEBUG
#endif

    /* Signal the user application that an interupt occured */  
    kill_fasync(&fasync_fpga_queue, SIGIO, POLL_IN);

    return IRQ_HANDLED;

}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: proc_read_interrupt_arm
 *
 * The kernel executes this function when a read operation occurs on
 * /proc/interrupt_arm. 
 */

static int read_proc(struct file *filp,char *buf,size_t count,loff_t *offp) 
{
    int len;
    char outString[64];

    len = sprintf(outString, "Total number of interrupts %19i\n", interruptcount);

    copy_to_user(buf, outString, len);
    return len;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: fpga_open
 *
 * This function is called when the fpga_int device is opened
 *
 */

static int fpga_open (struct inode *inode, struct file *file) {
#ifdef DEBUG
    printk(KERN_INFO "fpga_int: Inside fpga_open \n");  // DEBUG
#endif
    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: fpga_release
 *
 * This function is called when the fpga_int device is
 * released
 *
 */

static int fpga_release (struct inode *inode, struct file *file) {

#ifdef DEBUG
    printk(KERN_INFO "\nfpga_int: Inside fpga_release \n");  // DEBUG
#endif
    return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: fpga_fasync
 *
 * This is invoked by the kernel when the user program opens this
 * input device and issues fcntl(F_SETFL) on the associated file
 * descriptor. fasync_helper() ensures that if the driver issues a
 * kill_fasync(), a SIGIO is dispatched to the owning application.
 */



static int fpga_fasync (int fd, struct file *filp, int on)
{
#ifdef DEBUG
    printk(KERN_INFO "\nfpga_int: Inside fpga_fasync \n");  // DEBUG
#endif
    return fasync_helper(fd, filp, on, &fasync_fpga_queue);
} 



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Define which file operations are supported
 *
 */

struct file_operations fpga_fops = {
    .owner	=	THIS_MODULE,
    .llseek	=	NULL,
    .read	=	NULL,
    .write	=	NULL,
    .poll	=	NULL,
    .unlocked_ioctl	=	NULL,
    .mmap	=	NULL,
    .open	=	fpga_open,
    .flush	=	NULL,
    .release=	fpga_release,
    .fsync	=	NULL,
    .fasync	=	fpga_fasync,
    .lock	=	NULL,
    .read	=	NULL,
    .write	=	NULL,
};

static const struct file_operations proc_fops = {
    .owner  = THIS_MODULE,
    .open       = NULL,
    .read       = read_proc,
};

static const struct of_device_id fpga_gpio_of_match[] = {
    { .compatible = "xlnx,gpio-interrupt-1.0" },
    { /* end of table */ }
};
MODULE_DEVICE_TABLE(of, fpga_gpio_of_match);

static int fpga_gpio_probe(struct platform_device *pdev)
{
    struct resource *res;
    // This code gets the IRQ number by probing the system.
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!res) {
        printk("No IRQ found\n");
        return 0;
    }
    // Get interrupt number
    gic_interrupt = res->start;
    return 0;
}
static int fpga_gpio_remove(struct platform_device *pdev)
{
    //struct fpga_gpio *gpio = platform_get_drvdata(pdev)
    return 0;
} 
static struct platform_driver fpga_gpio_driver = {
    .driver = {
        .name = MODULE_NM,
        .of_match_table = fpga_gpio_of_match,
    },
    .probe = fpga_gpio_probe,
    .remove = fpga_gpio_remove,
};   

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: init_interrupt_arm
 *
 * This function creates the /proc directory entry interrupt_arm. It
 * 
 */

static int __init init_interrupt_arm(void)
{

    int rv = 0, err = 0;

    platform_driver_unregister(&fpga_gpio_driver);  

    printk("FPGA Interrupt Module\n");
    printk("FPGA Driver Loading.\n");
    printk("Using Major Number %d on %s\n", FPGA_MAJOR, MODULE_NM);

    err = platform_driver_register(&fpga_gpio_driver);

    if(err != 0) printk("Driver register error with number %d\n",err);
    else printk("Driver registered with no error\n");

    if (register_chrdev(FPGA_MAJOR, MODULE_NM, &fpga_fops)) {
        printk("fpga_int: unable to get major %d. ABORTING!\n", FPGA_MAJOR);
        goto no_gpio_interrupt;
    }

    interrupt_arm_file = proc_create("interrupt_arm", 0444, NULL, &proc_fops );

    if(interrupt_arm_file == NULL) {
        printk("fpga_int: create /proc entry returned NULL. ABORTING!\n");
        goto no_gpio_interrupt;
    }


    // request interrupt from linux

    rv = request_irq(gic_interrupt, interrupt_handler, IRQF_TRIGGER_RISING,
            "interrupt_arm", NULL);

    if ( rv ) {
        printk("Can't get interrupt %d\n", INTERRUPT);
        goto no_gpio_interrupt;
    }


    /* everything initialized */
    printk(KERN_INFO "%s %s Initialized\n",MODULE_NM, MODULE_VER);
    return 0;

    /* remove the proc entry on error */
no_gpio_interrupt:
    unregister_chrdev(FPGA_MAJOR, MODULE_NM);
    platform_driver_unregister(&fpga_gpio_driver);
    remove_proc_entry("interrupt_arm", NULL);
    return -EBUSY;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * function: cleanup_interrupt_arm
 *
 * This function frees interrupt then removes the /proc directory entry 
 * interrupt_arm. 
 */

static void __exit cleanup_interrupt_arm(void)
{

    /* free the interrupt */
    free_irq(gic_interrupt,NULL);

    unregister_chrdev(FPGA_MAJOR, MODULE_NM);
    platform_driver_unregister(&fpga_gpio_driver);
    remove_proc_entry("interrupt_arm", NULL);
    printk(KERN_INFO "%s %s removed\n", MODULE_NM, MODULE_VER);
}

module_init(init_interrupt_arm);
module_exit(cleanup_interrupt_arm);

MODULE_AUTHOR("Sarnab Bhattacharya");
MODULE_DESCRIPTION("fpga_int proc module");
MODULE_LICENSE("GPL");

