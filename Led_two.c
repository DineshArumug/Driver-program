#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/delay.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>
#include<linux/err.h>
#include<linux/stat.h>
#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/types.h>
#include<linux/kthread.h>
#include<linux/sched.h>
#define GPIO_21 (21)




dev_t dev;
static struct class *dev_create;
static struct cdev my_cdev;
static struct task_struct *my_thread;

static int my_open(struct inode *node,struct file *file)
{
	pr_info("Device File is Opened\n");
	return 0;
}

static int my_close(struct inode *node,struct file *file)
{
	pr_info("Device file is closed\n");
	return 0;
}

static ssize_t my_read(struct file *file,char __user *buf,size_t len,loff_t *off)
{
	uint8_t gpio_state = 0;
	pr_info("Device file is ready to read the data/n");
	gpio_state = gpio_get_value(GPIO_21);
	len = 1;
	if(copy_to_user(buf,&gpio_state,len) > 0)
	{
		pr_err("Error: Not all the bytes not cpoied to user\n");
	}
	pr_info("Read function: GPIO_21 = %d\n",GPIO_21);
	return 0;
}

static ssize_t my_write(struct file *file,const char *buf,size_t len,loff_t *off)
{
	uint8_t rec_buf[10] = {0};
	pr_info("Device file is ready to write the data/n");
	if(copy_from_user(rec_buf,buf,len) > 0)
	{
		pr_err("Error: Not all the bytes is copied from user\n");
	}
	pr_info("Write function: GPIO_21 Set = %d\n",rec_buf[0]);
	if(rec_buf[0] == '1') {
		gpio_set_value(GPIO_21,1); 
		pr_info("Write Function: GPIO set is 1\n");
	}
	else if(rec_buf[0] == '0') {
		gpio_set_value(GPIO_21,0);
		pr_info("Write Function: GPIO set is 0\n");
	}
	else {
		pr_err("Unknown Command: Please give correct \n");
	}
	return len;
}

static struct file_operations fops = 
{
	.owner = THIS_MODULE,
	.read  = my_read,
	.write = my_write,
	.open  = my_open,
	.release=my_close,
};

int led_blink(void *data)
{
	int count=20,i;
	for(i=0;i<count;i++)
	{
		pr_info("Set_Led: delay function %d\n",i);
		gpio_set_value(GPIO_21,1);
		mdelay(2000);
		gpio_set_value(GPIO_21,0);
		mdelay(2000);
	}
	return 0;
}
static int __init main_function(void)
{
	pr_info("Welcome to Dinesh driver device driver programming\n");
	if(gpio_is_valid(GPIO_21) == false)
	{
		pr_err("GPIO %d is not valid\n",GPIO_21);
		goto gpio;
	}
	if(gpio_request(GPIO_21,"GPIO_21") < 0)
	{
		pr_err("Error: GPIO %d request \n",GPIO_21);
		goto gpio;
	}
	gpio_direction_output(GPIO_21,0);
//	gpio_export(GPIO_21,false);
/*
	while(!kthread_should_stop())
	{
		gpio_set_value(GPIO_21,1);
		msleep(1000);
		gpio_set_value(GPIO_21,0);
		msleep(1000);
	}	
	*/
	led_blink("Create thread");

	pr_info("Module is successfully inserted\n");
	return 0;
gpio:
	gpio_free(GPIO_21);
	return -1;
}
module_init(main_function);


static void exit_function(void)
{
	gpio_free(GPIO_21);
	pr_info("Module is removed successfully\n");
}
module_exit(exit_function);
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("sAMPLE led program");
MODULE_LICENSE("GPL");
