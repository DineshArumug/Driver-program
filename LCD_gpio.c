#include<linux/device.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>
#include<linux/delay.h>


static dev_t dev;
struct class *myclass;
struct cdev mycdev;
static int gpios[] = {
	2,
	3,
	4,
	17,
	27,
	22,
	10,
	9,
	11,
	5,
	6
};
static int myopen(struct inode *node,struct file *flip)
{
	pr_info("MYOOPEN: function is called\n");
	return 0;
}
static int myclose(struct inode *node,struct file *flip)
{
	pr_info("MYCLOSE: function is called\n");
	return 0;
}
static ssize_t mywrite(struct file*flip,const char *buf,size_t count,loff_t *pos)
{
	pr_info("MYWRITE: function is called\n");
	return count;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open  = myopen,
	.release = myclose,
	.write = mywrite,
};
static void LCD_data(unsigned char data)
{
	int i;
	for(i = 0;i<sizeof(data);i++)
	{
		gpio_set_value(gpios[i+3],((data)&(1<<i))>>i);
	}
	gpio_set_value(gpios[0],1);
	gpio_set_value(gpios[1],0);
	gpio_set_value(gpios[2],1);
	msleep(1000);
	gpio_set_value(gpios[2],0);
	pr_info("LCD_data: function");
}

static void LCD_cmd(unsigned char cmd)
{
	int i;
	for(i = 0;i<sizeof(cmd);i++)
	{
		gpio_set_value(gpios[i+3],((cmd)&(1<<i))>>i);
	}
	gpio_set_value(gpios[0],0);
	gpio_set_value(gpios[1],0);
	gpio_set_value(gpios[2],1);
	msleep(5);
	gpio_set_value(gpios[2],0);
	pr_info("LCD_cmd: function\n");
}

static void LCD_init(void)
{
	LCD_cmd(0x01);
	pr_info("LCD Init: function");
	LCD_cmd(0x02);
	LCD_cmd(0x0C);
	LCD_cmd(0x38);
	LCD_cmd(0x80);
}

static int __init LCD_init_function(void)
{
	int i = 0;
	char text[] = "Hello";
	char *names[] = {"RESGISTER_SELECT","RESGISTER_WRITE","ENABLE","DATA_PIN0","DATA_PIN1","DATA_PIN2","DATA_PIN3","DATA_PIN4","DATA_PIN5","DATA_PIN6","DATA_PIN7"};
	pr_info("Hello Dinesh driver\n");
	if((alloc_chrdev_region(&dev,1,0,"Dinesh_Device_Number")) < 0)
	{
		pr_info("Device Number cannot be allocate\n");
		return -1;
	}
	pr_info("Major = %d and Minor = %d\n",MAJOR(dev),MINOR(dev));
	if((myclass=class_create(THIS_MODULE,"Myclass"))== NULL)
	{
		pr_err("Class cannot be create\n");
		goto unreg;
	}
	if(device_create(myclass,NULL,dev,NULL,"Dinesh_driver") == NULL)
	{
		pr_err("Device file cannot be created\n");
		goto unclass;
	}
	cdev_init(&mycdev,&fops);
	if(cdev_add(&mycdev,dev,1) == -1)
	{
		pr_err("Cdev is not register with kernel\n");
		goto cdev;
	}

	for(i=0;i<11;i++)
	{
		gpio_request(gpios[i],names[i]);
	}
	for(i=0;i<11;i++)
	{
		if(gpio_direction_output(gpios[i],0)) {
			pr_err("LCD_direction: error");
			goto gpio;
		}
	}
	LCD_init();
	LCD_cmd(0x80);
	for(i=0;i<sizeof(text)-1;i++)
		LCD_data(text[i]);

	pr_info("Driver is inserted successfully\n");
	return 0;
gpio:
	for(i = 9;i>=0;i--)
	{
		gpio_free(gpios[i]);
	}
cdev:
	cdev_del(&mycdev);
	device_destroy(myclass,dev);
unclass:
	class_destroy(myclass);
unreg:
	unregister_chrdev_region(dev,1);
	return -1;
}

static void LCD_remove_function(void)
{
	pr_info("Exit function\n");
	cdev_del(&mycdev);
	device_destroy(myclass,dev);
	class_destroy(myclass);
	unregister_chrdev_region(dev,1);
	pr_err("Kernel module is removed successfully\n");
}
module_init(LCD_init_function);
module_exit(LCD_remove_function);

MODULE_AUTHOR("DINESH ARUMUGAM");
MODULE_DESCRIPTION("SAMPLE LCD DISPLAY PROGRAM");
MODULE_LICENSE("GPL");


