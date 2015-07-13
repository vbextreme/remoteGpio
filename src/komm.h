#ifndef __KOMM_H__
#define __KOMM_H__

#ifndef LINUX
	#define LINUX
#endif
#ifndef __KERNEL__
	#define __KERNEL__
#endif
#ifndef MODULE
	#define MODULE
#endif

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

#define KOMM_MAX 1024
#define KOMM_ADR 5
#define KOMM_SIZE 32
#define KOMM_DISCONNECT 0
#define KOMM_CONNECT    1
#define KOMM_SERIAL_TIMEOUT 90000
#define KOMM_SERIAL 1

struct komm
{
	int debug;
	int id;
	int status;
	unsigned int timeout;
    struct termios old;
	struct file* fs;
	struct mutex mux;
	int (*open)(struct komm* k, const char* adr, unsigned int ex);
	int (*close)(struct komm* k);
	int (*available)(struct komm* k);
	int (*write)(struct komm* k, const unsigned char *buf, int count);
	int (*write8)(struct komm* k, const unsigned char buf);
	int (*write16)(struct komm* k, const unsigned short int buf);
	int (*write32)(struct komm* k, const unsigned int buf);
	int (*writestr)(struct komm* k, const char* buf);
	int (*read)(struct komm* k, unsigned char* buf, int szbuf);
	int (*read8)(struct komm* k);
	int (*read16)(struct komm* k);
	int (*read32)(struct komm* k);
	int (*readstr)(struct komm* k, char* d, unsigned int maxsz);
	void (*lock)(struct komm* k);
	void (*unlock)(struct komm* k);
	int (*islock)(struct komm* k);
	int (*discharge)(struct komm* k, unsigned int t);
};

int komm_init(struct komm* k, int debug, int model, unsigned int ex);
void komm_releaseid(unsigned int id);

#endif
