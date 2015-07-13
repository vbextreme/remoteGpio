#include "rgpio.h"

static void spi_lock(struct spi* sp);
static void spi_unlock(struct spi* sp);
static int spi_islock(struct spi* sp);

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  SPI SYSFS *************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

ssize_t sysfs_spi_cs_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct spi* sp;
	 
	sp = dev_get_drvdata(dev);
	dbg(sp->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",sp->cs);
}

ssize_t sysfs_spi_cs_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct spi* sp;
	struct board* b;
	char a[ARG_LEN_MAX];
	 
	sp = dev_get_drvdata(dev);
	b = sp->brd;
	dbg(b->ch.debug,"");
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	if ( sp->cs > -1 ) gpio_export(b,&b->ch.io[sp->cs]);
	
	sp->cs = katoi(a);
	
	if ( gpio_export(b,&b->ch.io[sp->cs]) ) 
	{
		sp->cs = -1;
		return -EINVAL;
	}
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_COM | CMD_COM_SPI_CS) ) goto KER;
	if ( b->k.write8(&b->k, sp->id )) goto KER;
	if ( b->k.write8(&b->k, sp->cs )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

ssize_t sysfs_spi_setting_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct spi* sp;
	int div;
	
	sp = dev_get_drvdata(dev);
	dbg(sp->brd->ch.debug,"");
	
	if ( sp->setting < 0 )
		return sprintf(buf,"undefined\n");
	
	switch ( (sp->setting >> 2) & 0x07)
    {
		case 0: div = 2; break;
        default: case 1: div = 4; break;
        case 2: div = 8; break;
        case 3: div = 16; break;
        case 4: div = 32; break;
        case 5: div = 64; break;
        case 6: div = 128; break;
    }
	
	return sprintf(buf,"SSdisable = %s\nData = %s\nClockDiv = %d\nMode = %d\n", ( sp->setting & 0x80) ? "High" : "Low",
																			    ( sp->setting & 0x40) ? "MSBfirst" : "LSBfirst",
																			    div,
																			    sp->setting & 0x03);
}

ssize_t sysfs_spi_setting_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct spi* sp;
	struct board* b;
	char a[ARG_LEN_MAX];
	 
	sp = dev_get_drvdata(dev);
	b = sp->brd;
	dbg(b->ch.debug,"");
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	sp->setting = katoi(a);
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_COM | CMD_COM_SPI_SETTING) ) goto KER;
	if ( b->k.write8(&b->k, sp->id )) goto KER;
	if ( b->k.write8(&b->k, sp->setting )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

ssize_t sysfs_spi_value_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct spi* sp;
	
	sp = dev_get_drvdata(dev);
	dbg(sp->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",sp->lastread);
}

ssize_t sysfs_spi_value_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct spi* sp;
	struct board* b;
	int k;
	 
	sp = dev_get_drvdata(dev);
	b = sp->brd;
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	for ( k = 0; k < count; ++k )
	{
		if ( b->k.write8(&b->k, CMD_COM | CMD_COM_SPI_TRANSFER) ) goto KER;
		if ( b->k.write8(&b->k, sp->id )) goto KER;
		if ( b->k.write8(&b->k, buf[k] )) goto KER;
		sp->lastread = b->k.read8(&b->k);
	}
	b->k.unlock(&b->k);
	
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

ssize_t sysfs_spi_lock_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	struct spi* sp;
	
	sp = dev_get_drvdata(dev);
	dbg(sp->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",spi_islock(sp));
}

ssize_t sysfs_spi_lock_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct spi* sp;
	
	sp = dev_get_drvdata(dev);
	dbg(sp->brd->ch.debug,"");
	
	if ( *buf == '1' )
		spi_lock(sp);
	else if ( *buf == '0' ) 
		spi_unlock(sp);
	else
		return -EINVAL;
	
	return count;
}


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  SPI GENERAL ************************************* ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

int spi_init(struct board* b)
{	
	int i;
	struct spi* sp;
	
	dbg(b->ch.debug,"");
	sp = NULL;
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,CMD_CHP | CMD_CHP_SPI) ) goto KER;
	b->ch.spicount = b->k.read8(&b->k);
		if ( b->ch.spicount < 0 ) goto KER;	
	if ( b->ch.spicount == 0) return -1;
	
	sp = kvnew(struct spi, b->ch.spicount);
	
	for ( i = 0; i < b->ch.spicount; ++i)
	{
		sp[i].mosi = b->k.read8(&b->k);
		sp[i].miso = b->k.read8(&b->k);
		sp[i].sck  = b->k.read8(&b->k);
		if ( sp[i].mosi < 0 || sp[i].miso < 0 || sp[i].sck < 0 ) goto KER;
		sp[i].id = i;
		mutex_init(&sp[i].mux);
		sp[i].brd = b;
		sp[i].dev = NULL;
		sp[i].cs = -1;
		sp[i].setting = -1;
		sp[i].dev_attr_cs.attr.name = "cs";
		sp[i].dev_attr_cs.attr.mode = 0666;
		sp[i].dev_attr_cs.show = sysfs_spi_cs_read;
		sp[i].dev_attr_cs.store = sysfs_spi_cs_write;
		sp[i].dev_attr_setting.attr.name = "setting";
		sp[i].dev_attr_setting.attr.mode = 0666;
		sp[i].dev_attr_setting.show = sysfs_spi_setting_read;
		sp[i].dev_attr_setting.store = sysfs_spi_setting_write;
		sp[i].dev_attr_value.attr.name = "value";
		sp[i].dev_attr_value.attr.mode = 0666;
		sp[i].dev_attr_value.show = sysfs_spi_value_read;
		sp[i].dev_attr_value.store = sysfs_spi_value_write;
		sp[i].dev_attr_lock.attr.name = "lock";
		sp[i].dev_attr_lock.attr.mode = 0666;
		sp[i].dev_attr_lock.show = sysfs_spi_lock_read;
		sp[i].dev_attr_lock.store = sysfs_spi_lock_write;
	}
	
	b->ch.sp = sp;
	b->k.unlock(&b->k);
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	if ( sp ) kfree(sp);
	b->k.unlock(&b->k);
	return -1;
}

int spi_clear(struct board* b)
{
	int i;
	
	dbg(b->ch.debug,"");
	
	if ( b->ch.spicount <= 0 ) return -1;
	if ( !b->ch.sp ) return -1;
	
	for (i = 0; i < b->ch.spicount; ++i)
		spi_unexport(b,&b->ch.sp[i]);
	kfree(b->ch.sp);
	b->ch.sp = NULL;
	
	return 0;
}


int spi_export(struct board* b, struct spi* sp)
{
	int ret;
	char spiname[127];
	
	if ( sp->dev ) {dbg(b->ch.debug,"alredy spi"); return -1;}
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_COM | CMD_COM_SPI_ENABLE) ) goto KER;
	if ( b->k.write8(&b->k, sp->id )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	sprintf(spiname,"spi%d.%d",sp->id,b->k.id);
	
	dbg(b->ch.debug,"creating device [%s]", spiname);
	
	sp->dev = device_create(b->cls, NULL, 0, sp, spiname);
	if ( IS_ERR(sp->dev) ) 
	{
		sp->dev = NULL;
		dbg(b->ch.debug,"error to device create spi");
		return -1;
	}

	ret = device_create_file(sp->dev, &sp->dev_attr_cs);
		if (ret < 0) dbg(b->ch.debug,"error to device create file ss");
	ret = device_create_file(sp->dev, &sp->dev_attr_setting);
		if (ret < 0) dbg(b->ch.debug,"error to device create file setting");
	ret = device_create_file(sp->dev, &sp->dev_attr_value);
		if (ret < 0) dbg(b->ch.debug,"error to device create file value");
	ret = device_create_file(sp->dev, &sp->dev_attr_lock);
		if (ret < 0) dbg(b->ch.debug,"error to device create file lock");
	
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

int spi_unexport(struct board* b, struct spi* sp)
{
	dbg(b->ch.debug,"");
	if ( !sp->dev ) return -1;
	device_remove_file(sp->dev, &sp->dev_attr_cs);
	device_remove_file(sp->dev, &sp->dev_attr_setting);
	device_remove_file(sp->dev, &sp->dev_attr_value);
	device_remove_file(sp->dev, &sp->dev_attr_lock);
	device_destroy(b->cls, sp->dev->devt);
	sp->dev = NULL;
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_COM | CMD_COM_SPI_DISABLE) ) goto KER;
	if ( b->k.write8(&b->k, sp->id )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

static void spi_lock(struct spi* sp)
{
	mutex_lock(&sp->mux);
}

static void spi_unlock(struct spi* sp)
{
	mutex_unlock(&sp->mux);
}

static int spi_islock(struct spi* sp)
{
	return mutex_is_locked(&sp->mux);
}
