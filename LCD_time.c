#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include<linux/kthread.h>
#include<linux/sched.h>
/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dinesh");
MODULE_DESCRIPTION("A driver to write to a LCD time and date display");

/* Variables for device and device class */
dev_t dev;
static struct class *my_class;
static struct cdev my_device;
static struct task_struct *mythread;
struct timespec64 ts;

#define DRIVER_NAME "lcd_time"
#define DRIVER_CLASS "Myclass"

/* LCD char buffer */
//atic char lcd_buffer[17];

/* Pinout for LCD Display */
unsigned int gpios[] = {
	3, /* Enable Pin */
	2, /* Register Select Pin */
	4, /* Data Pin 0*/
	17, /* Data Pin 1*/
	27, /* Data Pin 2*/
	22, /* Data Pin 3*/
	10, /* Data Pin 4*/
	9, /* Data Pin 5*/
	11, /* Data Pin 6*/
	5, /* Data Pin 7*/
	15, /*LED*/
};

#define REGISTER_SELECT gpios[1]

void LCD_command(uint8_t data)
{
	int i;
	gpio_set_value(REGISTER_SELECT, 0);
	for(i=0;i<8;i++)
	{
		gpio_set_value(gpios[i+2],((data >> i) & 1));	
	}
	gpio_set_value(gpios[0],1);
	msleep(5);
	gpio_set_value(gpios[0],0);
	pr_info("LCD_Command: is recieved\n");
}

void LCD_data(uint8_t data)
{
	int i;
	gpio_set_value(REGISTER_SELECT, 1);
	for(i=0;i<8;i++)
	{
		gpio_set_value(gpios[i+2],((data >> i) & 1));	
	}
	gpio_set_value(gpios[0],1);
	msleep(5);
	gpio_set_value(gpios[0],0);
	pr_info("LCD_Data: is recieved\n");
}

void LCD_init(void)
{
	LCD_command(0x01);
	LCD_command(0x02);
	LCD_command(0x0c);
	LCD_command(0x38);
	LCD_command(0x80);
}

int LCD_timeFun(void *data)
{
	int i,hh,mm,ss,count=0,count_mm=1,check = 99;
	struct tm tm;
	char pass,first_let,second_let;
	char text[] = "TIME-00:00:00";
	char text1[] = "DATE-00/00/00";
	for(i=0;i<sizeof(text)-1;i++)
	{
		LCD_data(text[i]);
	}
	LCD_command(0xc0);
	for(i=0;i<sizeof(text1)-1;i++)
	{
		LCD_data(text1[i]);
	}
	msleep(5000);

	while(!kthread_should_stop()) {
		ktime_get_real_ts64(&ts);
		time64_to_tm(ts.tv_sec,0,&tm);
		if(tm.tm_sec != check)
		{
			//seconds
			LCD_command(0x8c);
			pass = tm.tm_sec;
			first_let = pass % 10;
			LCD_data(48 + first_let);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0x8b);
			LCD_data(48 + second_let);

			//mins
			LCD_command(0x89);
			pass = tm.tm_min;
			first_let = pass % 10;
			LCD_data(48 + first_let);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0x88);
			LCD_data(48 + second_let);

			//hour
			LCD_command(0x86);
			pass = tm.tm_hour;
			first_let = pass % 10;
			LCD_data(48 + first_let);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0x85);
			LCD_data(48 + second_let);

			//years
			LCD_command(0xcc);
			pass = tm.tm_year+1900;
			first_let = pass % 10;
			//	LCD_data(48 + first_let);
			LCD_data(48+4);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0xcb);
			//	LCD_data(48 + second_let);
			LCD_data(48+2);

			//months
			LCD_command(0xc9);
			pass = tm.tm_mon+1;
			first_let = pass % 10;
			LCD_data(48 + first_let);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0xc8);
			LCD_data(48 + second_let);

			//days
			LCD_command(0xc6);
			pass = tm.tm_mday;
			first_let = pass % 10;
			LCD_data(48 + first_let);
			pass /= 10;
			second_let = pass % 10;
			LCD_command(0xc5);
			LCD_data(48 + second_let);
			
			check = tm.tm_sec;
			pr_info("TIME : %d:%d:%d\n",tm.tm_hour,tm.tm_min,tm.tm_sec);
		}
	}
	return 0;
}

static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	return count;
}

static int driver_open(struct inode *device_file, struct file *instance) {
	printk("dev_nr - open was called!\n");
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance) {
	printk("dev_nr - close was called!\n");
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.write = driver_write
};

static int __init ModuleInit(void) {
	int i;
	char *names[] = {"ENABLE_PIN", "REGISTER_SELECT", "DATA_PIN0", "DATA_PIN1", "DATA_PIN2", "DATA_PIN3", "DATA_PIN4", "DATA_PIN5", "DATA_PIN6", "DATA_PIN7","LED"};
	printk("Hello, Kernel!\n");

	if( alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME) < 0) {
		printk("Device Nr. could not be allocated!\n");
		return -1;
	}
	printk("read_write - Device Nr. Major: %d, Minor: %d was registered!\n", MAJOR(dev),MINOR(dev));

	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("Device class can not be created!\n");
		goto ClassError;
	}

	if(device_create(my_class, NULL, dev, NULL, DRIVER_NAME) == NULL) {
		printk("Can not create device file!\n");
		goto FileError;
	}

	cdev_init(&my_device, &fops);

	if(cdev_add(&my_device, dev, 1) == -1) {
		printk("lcd-driver - Registering of device to kernel failed!\n");
		goto AddError;
	}

	printk("lcd-driver - GPIO Init\n");
	for(i=0; i<11; i++) {
		if(gpio_request(gpios[i], names[i])) {
			printk("lcd-driver - Error Init GPIO %d\n", gpios[i]);
			goto GpioInitError;
		}
	}

	printk("lcd-driver - Set GPIOs to output\n");
	for(i=0; i<11; i++) {
		if(gpio_direction_output(gpios[i], 0)) {
			printk("lcd-driver - Error setting GPIO %d to output\n", i);
			goto GpioDirectionError;
		}
	}
	//	pr_info("Data and time = %04ld-%02ld-%02ld = %02ld:%02ld:%02ld\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

	LCD_init();
	mythread = kthread_create(LCD_timeFun,NULL,"mythread");
	//	LCD_data('A');
	//	LCD_data('B');
	if(mythread)
	{
		pr_info("Kthread is created\n");
		wake_up_process(mythread);
	}
	else
	{
		pr_err("Error : kthread\n");
	}

	gpio_set_value(gpios[10],1);

	return 0;
GpioDirectionError:
	i=10;
GpioInitError:
	for(;i>=0; i--)
		gpio_free(gpios[i]);
AddError:
	device_destroy(my_class, dev);
FileError:
	class_destroy(my_class);
ClassError:
	unregister_chrdev_region(dev, 1);
	return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit ModuleExit(void) {
	int i;
	LCD_init();
	for(i=0; i<11; i++){
		gpio_set_value(gpios[i], 0);
		gpio_free(gpios[i]);
	}
	cdev_del(&my_device);
	device_destroy(my_class, dev);
	class_destroy(my_class);
	unregister_chrdev_region(dev, 1);
	printk("Goodbye, Kernel\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);
