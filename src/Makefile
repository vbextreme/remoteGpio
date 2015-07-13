obj-m := remotegpio.o
remotegpio-objs := kaid.o komm.o drv.o spi.o i2c.o eeprom.o gpio.o chip.o board.o module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
