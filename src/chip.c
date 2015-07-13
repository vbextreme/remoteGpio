#include "rgpio.h"

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  CHIP PROTO ************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static int proto_chip_lsdrv(struct board* b, char* dest)
{
	int i,k;
	char re[127];
	
	dbg(b->ch.debug,"");
	
	if ( dest ) strcpy(dest,"id name nattr\n");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_LSDRV) ) goto KER;
	b->ch.drvcount = b->k.read8(&b->k);
		if (b->ch.drvcount < 0 ) {b->ch.drvcount = 0; goto KER;}
	
	for ( i = 0; i < b->ch.drvcount; ++i)
	{
		b->k.readstr(&b->k,b->ch.dr[i].name,CHIP_NAME_MAX);
		b->ch.dr[i].count = b->k.read8(&b->k);
			if (b->ch.dr[i].count < 0 ) {b->ch.dr[i].count = 0; goto KER;}
		if ( dest )
		{
			sprintf(re,"%d %s %d\n",i,b->ch.dr[i].name,b->ch.dr[i].count);
			strcat(dest,re);
		}
		
		for ( k = 0; k < b->ch.dr[i].count; ++k)
		{
			b->k.readstr(&b->k,re,127);
			if ( b->ch.dr[i].dev_attr_remote[k].attr.name ) continue;
			b->ch.dr[i].dev_attr_remote[k].attr.name = kstrhep(re);
		}
	}
	
	
	b->k.unlock(&b->k);
	
	return 0;
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -1;
}


static int proto_chip_init(struct board* b)
{
	int ret;
	int i;
	
	dbg(b->ch.debug,"");
	
	dbg(b->ch.debug,"CMD_CHP_INIT");
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_CHP | CMD_CHP_INIT) ) goto KER;
	
	dbg(b->ch.debug,"read pincount");
	b->ch.pincount = b->k.read8(&b->k);
		if ( b->ch.pincount == ERR_CMD ) goto KER;
	
	dbg(b->ch.debug,"read pin %d",b->ch.pincount);	
	for (i = 0; i < b->ch.pincount; ++i)
	{
		ret = b->k.read8(&b->k);
			dbg(b->ch.debug,"pin %d mode %d",i,ret);	
			if ( b->ch.tblpin[i] == -1 ) goto KER;
		b->ch.tblpin[i] = ret;
	}
	
	dbg(b->ch.debug,"read chip name");	
	ret = b->k.readstr(&b->k,b->ch.name,CHIP_NAME_MAX);
		if ( ret <= 0 ) goto KER;
	dbg(b->ch.debug,"name[%s] size[%d]",b->ch.name,ret);	
	
	dbg(b->ch.debug,"CMD_CHP_MODE");
	if ( b->k.write8(&b->k,CMD_CHP | CMD_CHP_MODE) ) goto KER;
	b->ch.mode = b->k.read16(&b->k);
	
	b->k.unlock(&b->k);
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -1;
}

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  CHIP SYSFS ************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static ssize_t sysfs_chip_version_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	int mj,mi,ir;
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_VERSION) ) goto KER;
	mj = b->k.read8(&b->k);
	mi = b->k.read8(&b->k);
	ir = b->k.read8(&b->k);
	b->k.unlock(&b->k);
	
	return sprintf(buf,"%d.%d.%d\n",mj,mi,ir);
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_chip_version_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	return -EPERM;
}


static ssize_t sysfs_chip_option_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	int ret;
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_GETOPT) ) goto KER;
	ret = b->k.read8(&b->k);
	b->k.unlock(&b->k);
	
	return sprintf(buf,"cmdblink = %s\nsleepblink = %s\nautosleep = %s\n", ( ret & OPZ_CMD_BLINK) ? "true":"false",
																		   ( ret & OPZ_SLEEP_BLINK) ? "true":"false",
																		   ( ret & OPZ_SLEEP_AUTO) ? "true":"false");
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_chip_option_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	int ret;
	char a[ARG_LEN_MAX];
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	ret = katoi(a);
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_SETOPT) ) goto KER;
	if ( b->k.write8(&b->k, ret) ) goto KER;
	ret = b->k.read8(&b->k);
		if ( ret != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_chip_timesleep_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	int ret;
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_GETAUT) ) goto KER;
	ret = b->k.read32(&b->k);
	b->k.unlock(&b->k);
	
	return sprintf(buf,"%u\n", (unsigned int) ret);
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_chip_timesleep_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	int ret;
	char a[ARG_LEN_MAX];
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	ret = katoi(a);
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_SETAUT) ) goto KER;
	if ( b->k.write32(&b->k, ret) ) goto KER;
	ret = b->k.read8(&b->k);
		if ( ret != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

ssize_t sysfs_chip_mode_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	return -EPERM;
}

ssize_t sysfs_chip_mode_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	char a[ARG_LEN_MAX];
	struct board* b;
	char* arg;
	char* type;
	char* n;
	char* v;
	int id;
	int enable;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	arg = a;
	
	type = karg_split(&arg);
		if ( !type ) return -EINVAL;
	
	enable = 1;
	n = karg_split(&arg);
	if ( n )
	{
		if ( !strcmp("enable",n) ) 
			enable = 1;
		else if ( !strcmp("disable",n) )
			enable = 0;
		else
			return -EINVAL;		
	}
	
	id = 0;
	n = karg_split(&arg);
	if ( n )
	{
		if ( strncmp("id",n,2) ) return -EINVAL;
		v = karg_value(n);
		id = katoi(v);
	}
	
	if ( !strcmp("i2c",type) )
	{
		if ( id >= b->ch.i2ccount ) return -EINVAL;
		if ( enable )
			i2c_export(b,&b->ch.ic[id]);
		else
			i2c_unexport(b,&b->ch.ic[id]);
	}
	else if ( !strcmp("spi",type) )
	{
		if ( id >= b->ch.spicount ) return -EINVAL;
		if ( enable )
			spi_export(b,&b->ch.sp[id]);
		else
			spi_unexport(b,&b->ch.sp[id]);
	}
	
	return count;
}

static ssize_t sysfs_chip_ram_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	int ret;
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_CHP | CMD_CHP_FREERAM) ) goto KER;
	ret = b->k.read32(&b->k);
	b->k.unlock(&b->k);
	
	return sprintf(buf,"%u\n", (unsigned int) ret);
																		   
	KER:
	dbg(b->ch.debug,"KER");	
	//b->k.discharge(&b->k,KOMM_TDIS);
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_chip_ram_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	return -EPERM;
}

static ssize_t sysfs_chip_lock_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	return sprintf(buf,"%d\n",b->k.islock(&b->k));
}

static ssize_t sysfs_chip_lock_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	if ( *buf == '1' )
		b->k.lock(&b->k);
	else if ( *buf == '0' ) 
		b->k.unlock(&b->k);
	else
		return -EINVAL;
	
	return count;
}

static ssize_t sysfs_chip_drv_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	char alld[ARG_LEN_MAX];
	struct board* b;
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	
	proto_chip_lsdrv(b,alld);
	
	return sprintf(buf,"%s\n", alld);
}

static ssize_t sysfs_chip_drv_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	char args[ARG_LEN_MAX];
	char* a;
	char* mode;
	char* iddrv;
	int   idval;
	char* argdrv;
	struct board* b;
	
	
	b = dev_get_drvdata(dev);
	dbg(b->ch.debug,"");
	if ( karg_normalize(args,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	a = args;
	
	proto_chip_lsdrv(b,NULL);
	
	if ( !(mode = karg_split(&a)) ) {dbg(b->ch.debug,"error no mode");return -EINVAL;}
	if ( !(iddrv = karg_split(&a)) ) {dbg(b->ch.debug,"error no id");return -EINVAL;}
	idval = katoi(iddrv);
	if ( idval < 0 || idval >= b->ch.drvcount ) {dbg(b->ch.debug,"error id out of range");return -EINVAL;}
	
	if ( !strcmp(mode,"export") )
	{
		dbg(b->ch.debug,"export");
		argdrv = karg_split(&a);
		if ( drv_export(b,&b->ch.dr[idval],argdrv) ) {dbg(b->ch.debug,"error drv can't export");return -EINVAL;}
		return count;
	}
	else if ( !strcmp(mode,"unexport") )
	{
		dbg(b->ch.debug,"unexport");
		if ( drv_unexport(b,&b->ch.dr[idval]) ) {dbg(b->ch.debug,"error drv can't unexport");return -EINVAL;}
		return count;
	}
	
	dbg(b->ch.debug,"error val mode");
	return -EINVAL;
}


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  CHIP GENERAL ************************************ ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

int chip_init(struct board* b, struct chip* c, int debg)
{
	dbg(debg,"");
	
	c->debug = debg;
	c->brd = b;
	c->dev = NULL;
	c->name[0] = '\0';
	c->mode = 0;
	c->pincount = 0;
	c->io = NULL;
	c->ic = NULL;
	c->sp = NULL;
	
	if ( proto_chip_init(b) ){ dbg(b->ch.debug,"fail proto init"); return -1; }
	
	c->dev_attr_version.attr.name = "version";
	c->dev_attr_version.attr.mode = 0666;
	c->dev_attr_version.show = sysfs_chip_version_read;
	c->dev_attr_version.store = sysfs_chip_version_write;
	c->dev_attr_option.attr.name = "option";
	c->dev_attr_option.attr.mode = 0666;
	c->dev_attr_option.show = sysfs_chip_option_read;
	c->dev_attr_option.store = sysfs_chip_option_write;
	c->dev_attr_timesleep.attr.name = "timesleep";
	c->dev_attr_timesleep.attr.mode = 0666;
	c->dev_attr_timesleep.show = sysfs_chip_timesleep_read;
	c->dev_attr_timesleep.store = sysfs_chip_timesleep_write;
	c->dev_attr_mode.attr.name = "mode";
	c->dev_attr_mode.attr.mode = 0666;
	c->dev_attr_mode.show = sysfs_chip_mode_read;
	c->dev_attr_mode.store = sysfs_chip_mode_write;
	c->dev_attr_ram.attr.name = "ram";
	c->dev_attr_ram.attr.mode = 0666;
	c->dev_attr_ram.show = sysfs_chip_ram_read;
	c->dev_attr_ram.store = sysfs_chip_ram_write;
	c->dev_attr_lock.attr.name = "lock";
	c->dev_attr_lock.attr.mode = 0666;
	c->dev_attr_lock.show = sysfs_chip_lock_read;
	c->dev_attr_lock.store = sysfs_chip_lock_write;
	c->dev_attr_drv.attr.name = "drv";
	c->dev_attr_drv.attr.mode = 0666;
	c->dev_attr_drv.show = sysfs_chip_drv_read;
	c->dev_attr_drv.store = sysfs_chip_drv_write;
	
	gpio_init(b);
	if ( !eeprom_init(b) )
	{
		if ( c->mode & CHIP_MODE_ROM ) eeprom_export(b);
	}
	
	i2c_init(b);
	spi_init(b);
	drv_init(b);
	
	return 0;
}

int chip_export(struct board* b)
{
	int ret;
	char gname[CHIP_NAME_MAX];
	
	dbg(b->ch.debug,"");
	if ( b->ch.dev ) { dbg(b->ch.debug,"error alredy export"); return -1;}
	
	sprintf(gname,"%s.%d",b->ch.name,b->k.id);
	
	dbg(b->ch.debug,"creating chip device");
	b->ch.dev = device_create(b->cls, NULL, 0, b, gname);
	if ( IS_ERR(b->ch.dev) ) 
	{
		b->ch.dev = NULL;
		dbg(b->ch.debug,"error to create device");
		return -1;
	}
	
	
	dbg(b->ch.debug,"create file");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_version);
		if (ret < 0) dbg(b->ch.debug,"error to device create file version");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_option);
		if (ret < 0) dbg(b->ch.debug,"error to device create file option");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_timesleep);
		if (ret < 0) dbg(b->ch.debug,"error to device create file timesleep");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_mode);
		if (ret < 0) dbg(b->ch.debug,"error to device create file mode");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_ram);
		if (ret < 0) dbg(b->ch.debug,"error to device create file ram");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_lock);
		if (ret < 0) dbg(b->ch.debug,"error to device create file lock");
	ret = device_create_file(b->ch.dev, &b->ch.dev_attr_drv);
		if (ret < 0) dbg(b->ch.debug,"error to device create file drv");
	
	return 0;
}

int chip_unexport(struct board* b)
{
	dbg(b->ch.debug,"");
	if ( !b ) { dbg(b->ch.debug,"board not exist"); return -1;}
	if ( !b->ch.dev ) { dbg(b->ch.debug,"chip not exported"); return -1;}
	
	device_remove_file(b->ch.dev, &b->ch.dev_attr_version);
	device_remove_file(b->ch.dev, &b->ch.dev_attr_option);
	device_remove_file(b->ch.dev, &b->ch.dev_attr_timesleep);
	device_remove_file(b->ch.dev, &b->ch.dev_attr_mode);
	device_remove_file(b->ch.dev, &b->ch.dev_attr_lock);
	drv_clear(b);
	device_remove_file(b->ch.dev, &b->ch.dev_attr_drv);
	
	device_destroy(b->cls, b->ch.dev->devt);
	b->ch.dev = NULL;
	
	return 0;
}
