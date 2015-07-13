#include "rgpio.h"

static int drv_att_id(struct drv* d,const char* name);

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  DRV SYSFS *************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static ssize_t sysfs_drv_remote_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	struct drv* d;
	struct board* b;
	int aid;
	char rbu[ARG_LEN_MAX];
	
	d = dev_get_drvdata(dev);
	b = d->brd;
	dbg(b->ch.debug,"");
	
	aid = drv_att_id(d,attr->attr.name);
		if ( aid == -1 ) return -EIO;
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_DRV_READ | (unsigned char)(d->id) )) goto KER;
	if ( b->k.write8(&b->k, aid )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	if ( b->k.write8(&b->k, d->type[aid] )) goto KER;
	aid = b->k.read8(&b->k);
	
	switch (aid)
	{
		case 0:
			b->k.readstr(&b->k,rbu,ARG_LEN_MAX);
			b->k.unlock(&b->k);
		return sprintf(buf,"%s",rbu);
		
		case 1:
			aid = b->k.read8(&b->k);
		break;
		
		case 2:
			aid = b->k.read16(&b->k);
		break;
		
		case 4:
			aid = b->k.read32(&b->k);
		break;
		
		default: goto KER;
	}
	
	b->k.unlock(&b->k);
	return sprintf(buf,"%d",aid);
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

static ssize_t sysfs_drv_remote_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct drv* d;
	struct board* b;
	int aid;
	int val;
	char rbu[ARG_LEN_MAX];
	
	d = dev_get_drvdata(dev);
	b = d->brd;
	dbg(b->ch.debug,"");
	
	aid = drv_att_id(d,attr->attr.name);
		if ( aid == -1 ) return -EIO;
	if ( karg_normalize(rbu,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_DRV_WRITE | (unsigned char)(d->id) )) goto KER;
	if ( b->k.write8(&b->k, aid )) goto KER;
	if ( b->k.write8(&b->k, d->type[aid] )) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	
	if ( d->type[aid] == 0 )
	{
		b->k.writestr(&b->k,rbu);
		if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
		b->k.unlock(&b->k);
		return count;
	}
	
	val = katoi(rbu);
	
	switch ( d->type[aid] )
	{
		case 1:
			if ( b->k.write8(&b->k,val) ) goto KER;
		break;
		
		case 2:
			if ( b->k.write16(&b->k,val) ) goto KER;
		break;
		
		case 4:
			if ( b->k.write32(&b->k,val) ) goto KER;
		break;
		
		default: goto KER;
	}
	
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}


static ssize_t sysfs_drv_lock_read(struct device* dev, struct device_attribute* attr, char* buf)
{	
	struct drv* d;
	
	d = dev_get_drvdata(dev);
	dbg(d->brd->ch.debug,"");
	
	return sprintf(buf,"%d\n",d->brd->k.islock(&d->brd->k));
}

static ssize_t sysfs_drv_lock_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct drv* d;
	
	d = dev_get_drvdata(dev);
	dbg(d->brd->ch.debug,"");
	
	if ( *buf == '1' )
		d->brd->k.lock(&d->brd->k);
	else if ( *buf == '0' ) 
		d->brd->k.unlock(&d->brd->k);
	else
		return -EINVAL;
	
	return count;
}
/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  DRV GENERAL ************************************* ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

int drv_init(struct board* b)
{
	int i,k;
	
	
	dbg(b->ch.debug,"");
	
	b->ch.drvcount = 0;
	
	for ( i = 0; i < DRV_MAX; ++i )
	{
		b->ch.dr[i].brd = b;
		b->ch.dr[i].dev = NULL;
		b->ch.dr[i].count = 0;
		b->ch.dr[i].id = i;
		mutex_init(&b->ch.dr[i].mux);
		
		b->ch.dr[i].dev_attr_lock.attr.name = "lock";
		b->ch.dr[i].dev_attr_lock.attr.mode = 0666;
		b->ch.dr[i].dev_attr_lock.show = sysfs_drv_lock_read;
		b->ch.dr[i].dev_attr_lock.store = sysfs_drv_lock_write;
		
		for (k = 0; k < DRV_ATT_MAX; ++k)
		{
			b->ch.dr[i].dev_attr_remote[k].attr.name = NULL;
			b->ch.dr[i].dev_attr_remote[k].attr.mode = 0666;
			b->ch.dr[i].dev_attr_remote[k].show = sysfs_drv_remote_read;
			b->ch.dr[i].dev_attr_remote[k].store = sysfs_drv_remote_write;
		}
	}
	
	return 0;
}

int drv_clear(struct board* b)
{
	int i;
	
	dbg(b->ch.debug,"");
	for ( i = 0; i < b->ch.drvcount; ++i)
	{
		if ( b->ch.dr[i].dev )
			drv_unexport(b,&b->ch.dr[i]);
	}
	
	return 0;
}

int drv_export(struct board* b, struct drv* d, char* arg)
{
	int i;
	int ret;
	
	if ( d->dev ) return -1;
	
	dbg(b->ch.debug,"creating remote device [%s]",d->name);
	
	d->dev = device_create(b->cls, NULL, 0, d, d->name);
	if ( IS_ERR(d->dev) ) 
	{
		d->dev = NULL;
		dbg(b->ch.debug,"error to device create remote");
		return -1;
	}
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_DRV_EXPORT | (unsigned char)(d->id) )) {dbg(b->ch.debug,"error on send export");goto KER;}
	if ( b->k.read8(&b->k) != ERR_OK ) {dbg(b->ch.debug,"error remote not ok responce");goto KER;}
	if ( arg )
	{
		if ( b->k.writestr(&b->k,arg) ) {dbg(b->ch.debug,"error on send arg");goto KER;}
	}
	else
	{
		if ( b->k.write8(&b->k,0) ) {dbg(b->ch.debug,"error on send 0 arg");goto KER;}
	}
	if ( b->k.read8(&b->k) != ERR_OK ) {dbg(b->ch.debug,"error remote not ok2 responce");goto KER;}
	d->count = b->k.read8(&b->k);
		if ( d->count < 0 ) {dbg(b->ch.debug,"error remote bad count attribute");goto KER;}
	
	for ( i = 0; i < d->count; ++i)
	{
		dbg(b->ch.debug,"create attribute");
		d->type[i] = b->k.read8(&b->k);
		ret = device_create_file(d->dev, &d->dev_attr_remote[i]);
			if (ret < 0) dbg(b->ch.debug,"error to device create remote %d",i);
	}
	
	b->k.unlock(&b->k);
	
	ret = device_create_file(d->dev, &d->dev_attr_lock);
		if (ret < 0) dbg(b->ch.debug,"error to device create lock remote");
	
	return 0;
	
	KER:
	device_destroy(b->cls, d->dev->devt);
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

int drv_unexport(struct board* b, struct drv* d)
{
	int i;
	
	if ( !d->dev ) return -1;
	
	dbg(b->ch.debug,"");
	for ( i = 0; i < d->count; ++i)
	{
		device_remove_file(d->dev, &d->dev_attr_remote[i]);
		kdelete(d->dev_attr_remote[i].attr.name);
	}
		
	device_destroy(b->cls, d->dev->devt);
	d->dev = NULL;
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k, CMD_DRV_UNEXPORT | (unsigned char)(d->id)) ) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	if ( b->k.read8(&b->k) != ERR_OK ) goto KER;
	b->k.unlock(&b->k);
	
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -1;
}

static int drv_att_id(struct drv* d, const char* name)
{
	int i;
	for ( i = 0; i < d->count; ++i)
		if ( !strcmp(name,d->dev_attr_remote[i].attr.name) ) return i;
	return -1;
}
