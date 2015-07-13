#ifndef __KAID_H__
#define __KAID_H__

#ifndef LINUX
	#define LINUX
#endif
#ifndef __KERNEL__
	#define __KERNEL__
#endif
#ifndef MODULE
	#define MODULE
#endif

#ifndef CLASS_NAME
	#define CLASS_NAME "rgpio"
#endif

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>

#define ARG_LEN_MAX	512

#define UDTS  10000
#define UDTMS 10

#define dbg(DBG,FORMAT, arg...) do { if (DBG) printk(CLASS_NAME ": %s: " FORMAT "\n", __FUNCTION__, ## arg); } while (0)
#define knew(OBJ) (OBJ*) kmalloc(sizeof(OBJ),GFP_KERNEL)
#define kvnew(OBJ,N) (OBJ*) kmalloc(sizeof(OBJ) * (N),GFP_KERNEL)
#define kdelete(OBJ) do{kfree(OBJ); OBJ = NULL;}while(0);

int karg_normalize(char* d, int max, const char* buf, size_t count);
char* karg_split(char** args);
char* karg_value(char* a);
int karg_intlen(unsigned int v);
char* kstrhep(char* str);
int katoi(char* v);
void kbita_set(unsigned int* ba, unsigned int adrsz, unsigned int id);
void kbita_res(unsigned int* ba, unsigned int adrsz, unsigned int id);
int kbit_rightfirstcount(unsigned int val);
int kbita_freedom(unsigned int* ba, unsigned int sz, unsigned int adrsz);

#endif
