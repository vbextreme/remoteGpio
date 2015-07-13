#include "rgpio.h"
	
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");

///OPTION
int rgpio_debug = DEFAULT_DEBUG;
module_param(rgpio_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rgpio_debug, "enable debug");

///GLOBAL
static int rgpio_major;
static struct class* rgpio_class = NULL;

/// /////////////// ///
/// DEVICE FUNCTION ///
/// /////////////// ///

static int rgpio_open(struct inode* inode, struct file* filp)
{
	dbg(rgpio_debug,"");
	return -EACCES;
}

static int rgpio_close(struct inode* inode, struct file* filp)
{
	dbg(rgpio_debug,"");
	return -EACCES;
}

static struct file_operations rgpio_fnc = { .open = rgpio_open,
											 .release = rgpio_close
										   };

/// ////// ///
/// MODULE ///
/// ////// ///


static int __init rgpio_module_init(void)
{	
	dbg(rgpio_debug,"");
	
	rgpio_major = register_chrdev(0, DEVICE_NAME, &rgpio_fnc);
	if (rgpio_major < 0) 
	{
		dbg(rgpio_debug,"failed to register device: error %d\n", rgpio_major);
		return -1;
	}
	
	rgpio_class = class_create(THIS_MODULE, CLASS_NAME);
	if ( IS_ERR(rgpio_class) ) 
	{
		dbg(rgpio_debug,"failed to register device class '%s'\n", CLASS_NAME);
		unregister_chrdev(rgpio_major, DEVICE_NAME);
		return -1;
	}
	
	board_init(rgpio_class,rgpio_debug);
	
	return 0;
}

static void __exit rgpio_module_exit(void)
{	
	dbg(rgpio_debug,"");
	
	board_destroy();
	
	dbg(rgpio_debug,"destroy class");
	class_unregister(rgpio_class);
	class_destroy(rgpio_class);
	unregister_chrdev(rgpio_major, DEVICE_NAME);
}

module_init(rgpio_module_init);
module_exit(rgpio_module_exit);
