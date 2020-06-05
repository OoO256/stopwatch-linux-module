#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/ioctl.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <mach/gpio.h>

#define MAJOR_NUMBER 242
#define DEVICE "/dev/stopwatch"

#define IOM_FND_MAJOR 261
#define IOM_FND_NAME "fpga_fnd"
#define IOM_FND_ADDRESS 0x08000004

// driver variables
static int inter_major=242, inter_minor=0;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;

// timer variables
static struct timer_list timer;
static int timer_cnt = 3600, timer_clock;
static int kernel_timer_usage = 0;
static unsigned long prev_voldown_jiffies = 0;
static unsigned long prev_start_jiffies = 0;
static unsigned long prev_pause_jiffies = 0;
static int timer_deleted = 1;

// fnd variables
static int fpga_fnd_port_usage = 0;
static unsigned char *iom_fpga_fnd_addr;

// timer functions
void set_timer(unsigned long prev_jiffies);
void timer_handler();
void fnd_write();

// module functions
int iom_open(struct inode *minode, struct file *mfile);
int iom_release(struct inode *minode, struct file *mfile);
int iom_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

// interrupt handlers
irqreturn_t inter_handler_home(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler_back(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler_volup(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler_voldown(int irq, void* dev_id, struct pt_regs* reg);

// wait queue
wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

// define file_operations structure 
struct file_operations fops = {
	.owner		=	THIS_MODULE,
	.open		=	iom_open,
	.release	=	iom_release,
	.write		=	iom_write
};

irqreturn_t inter_handler_home(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "start timer!\n");
	
	printk("prev_pause_jiffies : %lu\n", prev_pause_jiffies);
	printk("prev_start_jiffies : %lu\n", prev_start_jiffies);
	printk("ms : %lu\n", ( (prev_pause_jiffies - prev_start_jiffies) % HZ ));
	
	// start timer if timer was deleted
	if (timer_deleted)
		set_timer(prev_pause_jiffies - prev_start_jiffies);

	// set prev_start_jiffies
	prev_start_jiffies = jiffies;
	timer_deleted = 0;
	return IRQ_HANDLED;
}

irqreturn_t inter_handler_back(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "pause timer!\n");
	// set last prev_pause_jiffies
	prev_pause_jiffies = jiffies;

	// if timer is not deleted, delete timer
	if (timer_deleted == 0)
		del_timer(&timer);
	timer_deleted = 1;
	return IRQ_HANDLED;
}

irqreturn_t inter_handler_volup(int irq, void* dev_id,struct pt_regs* reg) {
	printk(KERN_ALERT "reset timer!\n");

	// if timer is not deleted, delete timer
	if (timer_deleted == 0)
		del_timer(&timer);

	// reset timer variables
	timer_clock = 0;
	prev_start_jiffies = 0;
	prev_pause_jiffies = 0;
	timer_deleted = 1;
	fnd_write();
	return IRQ_HANDLED;
}

irqreturn_t inter_handler_voldown(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "voldown!\n");

	unsigned int voldown_state = gpio_get_value(IMX_GPIO_NR(5, 14));

	if (voldown_state) {
		// voldown botton rise

		// if more than 3 secs passed, wake up application
		unsigned long curr_jiffies = jiffies;
		if (curr_jiffies - prev_voldown_jiffies >= 3*HZ)
		{
			wake_up_interruptible(&wq_write);
		}
		prev_voldown_jiffies = curr_jiffies;
	}
	else {
		// voldown botton fall
		prev_voldown_jiffies = jiffies;
	}

	return IRQ_HANDLED;
}

int iom_open(struct inode *minode, struct file *mfile)
{
	if(fpga_fnd_port_usage)
        return -EBUSY;
   	
	int irq, ret; 
    // request home interrupt
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler_home, IRQF_TRIGGER_FALLING, "home", 0);

    // request back interrupt
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler_back, IRQF_TRIGGER_FALLING, "back", 0);

    // request volup interrupt
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler_volup, IRQF_TRIGGER_FALLING, "volup", 0);

    // request voldown interrupt
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler_voldown, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	// set timer variables
    timer_clock = 0;
    fpga_fnd_port_usage = 1;
    kernel_timer_usage = 1;
	fnd_write();
    return 0;
}

int iom_release(struct inode *minode, struct file *mfile)
{
    timer_clock = 0;
    // if timer is still working, delete timer
	if (timer_deleted == 0)
    	del_timer(&timer);

	// show 0000
	fnd_write();

    // release devices
    fpga_fnd_port_usage = 0;
    kernel_timer_usage = 0;

	// free irqs
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	return 0;
}

int iom_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
  	printk("write start\n");
	interruptible_sleep_on(&wq_write);
  	printk("write end\n");
	return 0;
}

void set_timer(unsigned long prev_jiffies)
{
    // set and add next timer
	// calculate expires if last prev_jiffies is not 0
    timer.expires = jiffies + HZ - ( prev_jiffies % HZ );
    timer.function = timer_handler;
    add_timer(&timer);
}

void timer_handler()
{
    // check timeout 
    if (timer_clock < timer_cnt){
		fnd_write();
        set_timer(0);
    }
    else{
        printk("timeout\n");
    }

    // increase clock
    timer_clock++;
}

void fnd_write(){
	// write timer_clock on fnd display in mm:ss format

    unsigned int value[4] = {
		( timer_clock / 600 ) % 6, 
		( timer_clock / 60 ) % 10, 
		( timer_clock / 10 ) % 6, 
		timer_clock % 10
	};

    unsigned short int value_short = 0;

    value_short = value[0] << 12 | value[1] << 8 |value[2] << 4 |value[3];
    outw(value_short,(unsigned int)iom_fpga_fnd_addr);
}

static int inter_register_cdev(void)
{
	// register chrdev region
	int error;
	if(inter_major) {
		inter_dev = MKDEV(inter_major, inter_minor);
		error = register_chrdev_region(inter_dev,1, DEVICE);
	}else{
		error = alloc_chrdev_region(&inter_dev,inter_minor,1, DEVICE);
		inter_major = MAJOR(inter_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", inter_major);
	cdev_init(&inter_cdev, &fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}

int __init iom_init(void)
{	
	// init module
	printk("init module\n");

    // register device driver
	int result = inter_register_cdev();
    if(result < 0) {
        printk("Cant register driver\n");
        return result;
    }
	printk("Register driver. name : %s, major number : %d\n", DEVICE, MAJOR_NUMBER);

    // init timer
    init_timer(&timer);

    //map devices
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
	return 0;
}

void __exit iom_exit(void) 
{
	// unregister device driver
	printk("exit module\n");
	cdev_del(&inter_cdev);
	unregister_chrdev_region(inter_dev, 1);

	// unmap devices
    iounmap(iom_fpga_fnd_addr);
}

module_init(iom_init);
module_exit(iom_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yonguk");
