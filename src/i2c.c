#include "rgpio.h"

static void i2c_lock(struct i2c* ic);
static void i2c_unlock(struct i2c* ic);
static int i2c_islock(struct i2c* ic);

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  I2C SYSFS *************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///


static ssize_t sysfs_i2c_bus_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct i2c* ic;
	
	ic = dev_get_drvdata(dev);
	dbg(ic->brd->ch.debug,"");
	 
	return sprintf(buf,"0x%X\n",ic->bus);
}

static ssize_t sysfs_i2c_bus_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct i2c* ic;
	struct board* b;
	char a[ARG_LEN_MAX];
	
	ic = dev_get_drvdata(dev);
	b = ic->brd;
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	ic->bus = katoi(a);
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_BUS) ) goto KER;
	if ( b->k.write8(&b->k,ic->id ) )goto KER;
	if ( b->k.write8(&b->k,ic->bus ) )goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_i2c_request_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct i2c* ic;
	
	ic = dev_get_drvdata(dev);
	dbg(ic->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",ic->rq);
}

static ssize_t sysfs_i2c_request_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct i2c* ic;
	struct board* b;
	char a[ARG_LEN_MAX];
	
	ic = dev_get_drvdata(dev);
	b = ic->brd;
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	ic->rq = katoi(a);
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_REQUEST) ) goto KER;
	if ( b->k.write8(&b->k,ic->id ) )goto KER;
	if ( b->k.write8(&b->k,ic->rq ) ) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_i2c_value_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct i2c* ic;
	struct board* b;
	char a[ARG_LEN_MAX];
	int k;
	
	ic = dev_get_drvdata(dev);
	b = ic->brd;
	dbg(b->ch.debug,"");
	
	if ( ic->rq <= 0 ) return -ENOLCK;
	k = 0;
	
	b->k.lock(&b->k);
	while( ic->rq-- )
	{
		if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_READ) ) goto KER;
		if ( b->k.write8(&b->k,ic->id ) )goto KER;
		a[k++] = b->k.read8(&b->k);
	}
	b->k.unlock(&b->k);
	
	return sprintf(buf,"%*s\n",k,a);
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_i2c_value_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct i2c* ic;
	struct board* b;
	int k;
	
	ic = dev_get_drvdata(dev);
	b = ic->brd;
	dbg(b->ch.debug,"");
	
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_START) ) goto KER;
	if ( b->k.write8(&b->k,ic->id ) )goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	
	for ( k = 0; k < count; ++k )
	{
		if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_WRITE) ) goto KER;
		if ( b->k.write8(&b->k,ic->id ) )goto KER;
		if ( b->k.write8(&b->k,buf[k]) ) goto KER;
		if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	}
	
	if ( b->k.write8(&b->k,CMD_COM | CMD_COM_WIRE_END) ) goto KER;
	if ( b->k.write8(&b->k,ic->id ) )goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	
	b->k.unlock(&b->k);
	return k;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_i2c_lock_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	struct i2c* ic;
	
	ic = dev_get_drvdata(dev);
	dbg(ic->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",i2c_islock(ic));
}

static ssize_t sysfs_i2c_lock_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct i2c* ic;
	
	ic = dev_get_drvdata(dev);
	dbg(ic->brd->ch.debug,"");
	
	if ( *buf == '1' )
		i2c_lock(ic);
	else if ( *buf == '0' ) 
		i2c_unlock(ic);
	else
		return -EINVAL;
	
	return count;
}


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  I2C GENERAL ************************************* ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

int i2c_init(struct board* b)
{
	int i;
	struct i2c* ic;
	
	dbg(b->ch.debug,"");
	ic = NULL;
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_CHP | CMD_CHP_I2C) ) goto KER;
	b->ch.i2ccount = b->k.read8(&b->k);
		if ( b->ch.i2ccount < 0 ) goto KER;	
	if ( b->ch.i2ccount == 0 ) return -1; 
	
	
	ic = kvnew(struct i2c, b->ch.i2ccount);
	
	for (i = 0; i < b->ch.i2ccount; ++i);
	{
		ic[i].sda = b->k.read8(&b->k);
		ic[i].scl = b->k.read8(&b->k);
		if ( ic[i].sda < 0 || ic[i].scl < 0 ) goto KER;
		ic[i].id = i;
		mutex_init(&ic[i].mux);
		ic[i].dev = NULL;
		ic[i].rq = 0;
		ic[i].bus = 0;
		ic[i].dev_attr_bus.attr.name = "bus";
		ic[i].dev_attr_bus.attr.mode = 0666;
		ic[i].dev_attr_bus.show = sysfs_i2c_bus_read;
		ic[i].dev_attr_bus.store = sysfs_i2c_bus_write;
		ic[i].dev_attr_request.attr.name = "request";
		ic[i].dev_attr_request.attr.mode = 0666;
		ic[i].dev_attr_request.show = sysfs_i2c_request_read;
		ic[i].dev_attr_request.store = sysfs_i2c_request_write;
		ic[i].dev_attr_value.attr.name = "value";
		ic[i].dev_attr_value.attr.mode = 0666;
		ic[i].dev_attr_value.show = sysfs_i2c_value_read;
		ic[i].dev_attr_value.store = sysfs_i2c_value_write;
		ic[i].dev_attr_lock.attr.name = "lock";
		ic[i].dev_attr_lock.attr.mode = 0666;
		ic[i].dev_attr_lock.show = sysfs_i2c_lock_read;
		ic[i].dev_attr_lock.store = sysfs_i2c_lock_write;
	}
	
	b->k.unlock(&b->k);
	b->ch.ic = ic;
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	if ( ic ) kfree(ic);
	b->k.unlock(&b->k);
	return -1;
}

int i2c_clear(struct board* b)
{
	int i;
	
	dbg(b->ch.debug,"");
	
	if ( b->ch.i2ccount <= 0 ) return -1;
	if ( !b->ch.ic ) return -1;
	
	for (i = 0; i < b->ch.i2ccount; ++i)
		i2c_unexport(b,&b->ch.ic[i]);
	kfree(b->ch.ic);
	b->ch.ic = NULL;
	
	return 0;
}

int i2c_export(struct board* b, struct i2c* ic)
{
	int ret;
	char i2cname[127];
	
	if ( ic->dev ) {dbg(b->ch.debug,"alredy i2c"); return -1;}
	
	sprintf(i2cname,"i2c%d.%d",ic->id,b->k.id);
	
	dbg(b->ch.debug,"CMD_COM_WIRE_BEGIN");
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_COM | CMD_COM_WIRE_BEGIN) ) goto KER;
	if ( b->k.write8(&b->k, ic->id )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	dbg(b->ch.debug,"creating device [%s]",i2cname);
	
	ic->dev = device_create(b->cls, NULL, 0, ic, i2cname);
	if ( IS_ERR(ic->dev) ) 
	{
		ic->dev = NULL;
		dbg(b->ch.debug,"error to create device i2c");
		return -1;
	}

	ret = device_create_file(ic->dev, &ic->dev_attr_bus);
		if (ret < 0) dbg(b->ch.debug,"error to device create file bus");
	ret = device_create_file(ic->dev, &ic->dev_attr_request);
		if (ret < 0) dbg(b->ch.debug,"error to device create file request");
	ret = device_create_file(ic->dev, &ic->dev_attr_value);
		if (ret < 0) dbg(b->ch.debug,"error to device create file value");
	ret = device_create_file(ic->dev, &ic->dev_attr_lock);
		if (ret < 0) dbg(b->ch.debug,"error to device create file lock");
	
	gpio_unexport(b,&b->ch.io[ic->sda]);
	gpio_unexport(b,&b->ch.io[ic->scl]);
	
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

int i2c_unexport(struct board* b, struct i2c* ic)
{
	dbg(b->ch.debug, "");
	if ( !ic->dev ) return -1;
	
	device_remove_file(ic->dev, &ic->dev_attr_bus);
	device_remove_file(ic->dev, &ic->dev_attr_request);
	device_remove_file(ic->dev, &ic->dev_attr_value);
	device_remove_file(ic->dev, &ic->dev_attr_lock);
	device_destroy(b->cls, ic->dev->devt);
	ic->dev = NULL;
	
	gpio_export(b,&b->ch.io[ic->sda]);
	gpio_export(b,&b->ch.io[ic->scl]);
	
	return 0;
}

static void i2c_lock(struct i2c* ic)
{
	mutex_lock(&ic->mux);
}

static void i2c_unlock(struct i2c* ic)
{
	mutex_unlock(&ic->mux);
}

static int i2c_islock(struct i2c* ic)
{
	return mutex_is_locked(&ic->mux);
}

