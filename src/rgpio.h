#ifndef __RGPIO_H__
#define __RGPIO_H__

#ifndef LINUX
	#define LINUX
#endif
#ifndef __KERNEL__
	#define __KERNEL__
#endif
#ifndef MODULE
	#define MODULE
#endif

// PRONTA DA TESTARE
// VERSIONE COLLIDERE
// MANCANO I LOCK REMOTI
// TOGLIERE IL MODE
// SPI MODE enable disable non rimuove e invece di inserire disinserisce
// CHIP mode(manca remote mode) 
// GPIO
// ROM da testare (provare a rimettere ad un kb)
// I2C da testare (problemi nel mode)
// SPI da testare (problemi nel mode)
// DRV da testare, remote mode (in corso), non viene aggiunto il boardid al device

// RMMOD da quando funziona drv non scarica il modulo, modificato il ciclo clear da testare

#include "komm.h"
#include "kaid.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/tty_driver.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/termios.h>
#include <linux/delay.h>
#include <asm/segment.h>
#include <asm/uaccess.h>


#define CLASS_NAME "rgpio"
#define DEVICE_NAME "rgpio"
#define AUTHOR "vbextreme <@>"
#define DESCRIPTION "remote gpio"
#define VERSION "0.4"

#define ARG_LEN_MAX 512
#define DEFAULT_BAUND_RATE 9600
#define DEFAULT_DEBUG_BRD 1
#define DEFAULT_DEBUG 1

#define CHIP_NAME_MAX       32
#define CHIP_PIN_MAX       256

#define CHIP_PIN_DIGITAL  0x01
#define CHIP_PIN_ADC      0x02
#define CHIP_PIN_DAC      0x04
#define CHIP_PIN_PWM      0x08
#define CHIP_PIN_I2C      0x10
#define CHIP_PIN_SPI      0x20
#define CHIP_PIN_UART     0x40
#define CHIP_PIN_ENABLE   0x80

/*
#define CHIP_MODE_ENABLE  0x10
#define CHIP_MODE_DISABLE 0x00
#define CHIP_MODE_I2C     0x01
#define CHIP_MODE_SPI     0x08
*/

#define CHIP_MODE_ROM     0x0001
#define CHIP_MODE_UART0   0x0002
#define CHIP_MODE_UART1   0x0004
#define CHIP_MODE_UART2   0x0008
#define CHIP_MODE_UART3   0x0010
#define CHIP_MODE_I2C0    0x0020
#define CHIP_MODE_I2C1    0x0040
#define CHIP_MODE_SPI0    0x0080
#define CHIP_MODE_SPI1    0x0100

#define GPIO_MODE_UNDEF    0x0
#define GPIO_MODE_DIGITALI 0x1
#define GPIO_MODE_DIGITALO 0x2
#define GPIO_MODE_PWM      0x3
#define GPIO_MODE_ANALOGI  0x4
#define GPIO_MODE_ANALOGO  0x5

#define DRV_STAT_UNEXPORT  0x00
#define DRV_STAT_EXPORT    0x01

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// *****************************  PROTOCOLLO ************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

#define DRV_MAX 16
#define DRV_ATT_MAX 8

#define OPZ_CMD_BLINK   0x01
#define OPZ_SLEEP_BLINK 0x02
#define OPZ_SLEEP_AUTO  0x04

#define KOMM_TDIS 500000

#define ERR_CMD 255
#define ERR_DRV 0xFF
#define ERR_OK    0


#define PROTO_NAME 9

#define CMD_CHP          0x00
#define CMD_CHP_VERSION  0x00 // return 3byte(MJ,MI,IR)
#define CMD_CHP_NOP      0x01 // return byte(NOP)
#define CMD_CHP_GETOPT   0x02 // return byte(OPT) 
#define CMD_CHP_SETOPT   0x03 // +byte(OPT) - return byte(OK)
#define CMD_CHP_GETAUT   0x04 // return 4byte(sleepauto)
#define CMD_CHP_SETAUT   0x05 // +4byte(sleepauto) - return byte(OK)
#define CMD_CHP_FREERAM  0x06 // return 4byte(freeram)
#define CMD_CHP_RPIO     0x07 // return byte(offset)
#define CMD_CHP_LSDRV    0x08 
#define CMD_CHP_MODE     0x09 // return 2byte(mode)
#define CMD_CHP_EEPROM   0x0A // return byte(szeeprom)
#define CMD_CHP_I2C      0x0B // return nbyte(count,sda,scl,...)
#define CMD_CHP_SPI      0x0C // return nbyte(count,mosi,miso,sck,...)
#define CMD_CHP_INIT     0x0F // return byte(pincount) Nbyte(pinmode) str(chipname)

#define CMD_MODE         0x10
#define CMD_MODE_OUTPUT  0x01 // +byte(pin) - return byte(ok)
#define CMD_MODE_INPUT   0x02 // +byte(pin) - return byte(ok)
#define CMD_MODE_INPPUL  0x03 // +byte(pin) - return byte(ok)

#define CMD_DIGITAL_WRITE 0x20 // +byte(pin) - return byte(ok)
#define CMD_DIGITAL_READ  0x30 // +byte(pin) - return byte(ok)
#define CMD_ANALOG_WRITE  0x40 // +2byte(pin,value) - return byte(ok)
#define CMD_ANALOG_READ   0x50 // +byte(pin) - return 2byte(value)

#define CMD_COM          0xA0 
#define CMD_COM_WIRE_BEGIN   0x00 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_BUS     0x01 // +2byte(id,adr) - return byte(ok)
#define CMD_COM_WIRE_START   0x02 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_END     0x03 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_WRITE   0x04 // +2byte(id,value) - return byte(ok)
#define CMD_COM_WIRE_REQUEST 0x05 // +2byte(id,nbyte read) - return byte(ok)
#define CMD_COM_WIRE_READ    0x06 // +byte(id) - return byte(read)

#define CMD_COM_SPI_ENABLE   0x07 // +byte(id) - return byte(ok)
#define CMD_COM_SPI_DISABLE  0x08 // +byte(id) - return byte(ok)
#define CMD_COM_SPI_SETTING  0x09 // +byte(id) +byte( [x SSLow/high][x LSBFIRST/MSBFIRST][xxx SPI_CLOCK_DIV2/4/8/16/32/64/128][xx SPI_MODE0/1/2/3]) - return byte(ok)
#define CMD_COM_SPI_CS       0x0A // +2byte(id,pin) - return byte(ok)
#define CMD_COM_SPI_TRANSFER 0x0B // +2byte(id,value) - return byte(read)

#define CMD_ROM          0xB0 //
#define CMD_ROM_ADDRESS  0x00 // +2byte(address) - return byte(ok)
#define CMD_ROM_WRITE    0x01 // +byte(val) - return byte(ok)
#define CMD_ROM_READ     0x02 // return byte(val)

#define CMD_DRV_EXPORT     0xC0
#define CMD_DRV_UNEXPORT   0xD0
#define CMD_DRV_READ       0xE0
#define CMD_DRV_WRITE      0xF0


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

struct drv
{
	struct board* brd;
	struct device* dev;
	char name[CHIP_NAME_MAX];
	int count;
	int id;
	int type[DRV_ATT_MAX];
	struct mutex mux;
	struct device_attribute dev_attr_remote[DRV_ATT_MAX];
	struct device_attribute dev_attr_lock;
};

struct spi
{
	struct board* brd;
	struct device* dev;
	unsigned int id;
	short int cs;
	short int mosi;
	short int miso;
	short int sck;
	int setting;
	int lastread;
	struct mutex mux;
	struct device_attribute dev_attr_cs;
	struct device_attribute dev_attr_setting;
	struct device_attribute dev_attr_value;
	struct device_attribute dev_attr_lock;
};

struct i2c
{
	struct board* brd;
	struct device* dev;
	unsigned int id;
	unsigned int bus;
	int rq;
	short int sda;
	short int scl;
	struct mutex mux;
	struct device_attribute dev_attr_bus;
	struct device_attribute dev_attr_request;
	struct device_attribute dev_attr_value;
	struct device_attribute dev_attr_lock;
};

struct eeprom
{
	struct board* brd;
	struct device* dev;
	struct bin_attribute dev_attr_value;
};

struct gpio
{
	struct board* brd;
	struct device* dev;
	short int mode;
	short int io;
	int offset;
	struct device_attribute dev_attr_value;
	struct device_attribute dev_attr_mode;
};

struct chip
{
	struct board* brd;
	int debug;
	struct device* dev;
	char name[CHIP_NAME_MAX];
	unsigned short int mode;
	unsigned short int pincount;
	unsigned int tblpin[CHIP_PIN_MAX];
	struct gpio* io;
	struct eeprom rom;
	int i2ccount;
	struct i2c* ic;
	int spicount;
	struct spi* sp;
	int drvcount;
	struct drv dr[DRV_MAX];
	struct device_attribute dev_attr_version;
	struct device_attribute dev_attr_option;
	struct device_attribute dev_attr_timesleep;
	struct device_attribute dev_attr_mode;
	struct device_attribute dev_attr_ram;
	struct device_attribute dev_attr_lock;
	struct device_attribute dev_attr_drv;
};

struct board
{	
	struct class* cls;
	struct komm k;
	struct chip ch;
	struct board* next;
};

int board_init(struct class* cls, int debug);
int board_destroy(void);

int chip_init(struct board* b, struct chip* c, int debg);
int chip_export(struct board* b);
int chip_unexport(struct board* b);

int gpio_init(struct board* b);
int gpio_clear(struct board* b);
int gpio_export(struct board* b, struct gpio* gp);
int gpio_unexport(struct board* b, struct gpio* gp);

int eeprom_init(struct board* b);
int eeprom_export(struct board* b);
int eeprom_unexport(struct board* b);

int i2c_init(struct board* b);
int i2c_clear(struct board* b);
int i2c_export(struct board* b, struct i2c* ic);
int i2c_unexport(struct board* b, struct i2c* ic);

int spi_init(struct board* b);
int spi_clear(struct board* b);
int spi_export(struct board* b, struct spi* sp);
int spi_unexport(struct board* b, struct spi* sp);

int drv_init(struct board* b);
int drv_clear(struct board* b);
int drv_export(struct board* b, struct drv* d, char* arg);
int drv_unexport(struct board* b, struct drv* d);

#endif
