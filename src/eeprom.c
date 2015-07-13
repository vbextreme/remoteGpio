#include "rgpio.h"

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  EEPROM SYSFS ************************************ ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static ssize_t sysfs_eeprom_value_read(struct file* f, struct kobject* kobj, struct bin_attribute* ba, char* buf, loff_t pos, size_t size)
{
	struct device *dev;
	struct eeprom* rom;
	struct board* b;
	int i;
	
	dev = container_of(kobj, struct device, kobj);
	rom = dev_get_drvdata(dev);
	b = rom->brd;
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_ROM | CMD_ROM_ADDRESS) ) goto KER;
	if ( b->k.write16(&b->k,pos)) goto KER;
	
	for ( i = 0; i < size; ++i)
	{
		if ( pos + i >= b->ch.rom.dev_attr_value.size ) break;
		if ( b->k.write8(&b->k,CMD_ROM | CMD_ROM_READ) ) goto KER;
		buf[i] = b->k.read8(&b->k);
	}
	
	b->k.unlock(&b->k);
	return i;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_eeprom_value_write(struct file* f,struct kobject* kobj, struct bin_attribute* ba, char* buf, loff_t pos, size_t count)
{
	struct device *dev;
	struct eeprom* rom;
	struct board* b;
	int i;
	
	dev = container_of(kobj, struct device, kobj);
	rom = dev_get_drvdata(dev);
	b = rom->brd;
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_ROM | CMD_ROM_ADDRESS) ) goto KER;
	if ( b->k.write16(&b->k,pos)) goto KER;
	
	for ( i = 0; i < count; ++i )
	{
		if ( pos + i >= 512 ) break;
		if ( b->k.write8(&b->k,CMD_ROM | CMD_ROM_WRITE) ) goto KER;
		if ( b->k.write8(&b->k, buf[i] ) ) goto KER;
		if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	}
	
	b->k.unlock(&b->k);
	return i;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  EEPROM GENERAL ********************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///


int eeprom_init(struct board* b)
{
	dbg(b->ch.debug, "");
	
	b->ch.rom.brd = b;
	b->ch.rom.dev = NULL;
	b->ch.rom.dev_attr_value.attr.name = "value";
	b->ch.rom.dev_attr_value.attr.mode = 0666;
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_EEPROM) ) goto KER;
	b->ch.rom.dev_attr_value.size = b->k.read16(&b->k);
	b->k.unlock(&b->k);
	
	b->ch.rom.dev_attr_value.read = sysfs_eeprom_value_read;
	b->ch.rom.dev_attr_value.write = sysfs_eeprom_value_write;
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

int eeprom_export(struct board* b)
{
	int ret;
	char gname[CHIP_NAME_MAX];
	
	if ( b->ch.rom.dev ) return -1;
	
	sprintf(gname,"eeprom.%d",b->k.id);
	dbg(b->ch.debug,"creating device [%s]",gname);
	
	b->ch.rom.dev = device_create(b->cls, NULL, 0, &b->ch.rom, gname);
	if ( IS_ERR(b->ch.rom.dev) ) 
	{
		b->ch.rom.dev = NULL;
		dbg(b->ch.debug,"error to device create eeprom");
		return -1;
	}

	ret = sysfs_create_bin_file(&b->ch.rom.dev->kobj,&b->ch.rom.dev_attr_value);
	if (ret < 0) dbg(b->ch.debug,"error to device create file value");
	
	return 0;
}

int eeprom_unexport(struct board* b)
{
	dbg(b->ch.debug, "");
	if ( !b->ch.rom.dev ) return -1;
	
	sysfs_remove_bin_file(&b->ch.rom.dev->kobj,&b->ch.rom.dev_attr_value);
	device_destroy(b->cls, b->ch.rom.dev->devt);
	b->ch.rom.dev = NULL;
	return 0;
}
