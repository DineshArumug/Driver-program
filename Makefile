#obj-m += Led_gpio.o
#obj-m += Led_two.o
#obj-m += Led_thread.o
#obj-m += LCD_gpio.o
obj-m += LCD_time.o
#obj-m += Led_Interrupt.o
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
