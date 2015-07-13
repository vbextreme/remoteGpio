#include "rgpio.h"

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  GPIO SYSFS ************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static ssize_t sysfs_gpio_value_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct gpio* gp;
	struct board* b;
	int ret; 
	
	gp = dev_get_drvdata(dev);
	b = gp->brd;
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	
	if ( gp->mode == GPIO_MODE_DIGITALI )
	{
		if ( b->k.write8(&b->k, CMD_DIGITAL_READ) ) goto KER;
		if ( b->k.write8(&b->k, gp->io - gp->offset) ) goto KER;
		ret = b->k.read8(&b->k);
		b->k.unlock(&b->k);
		return sprintf(buf,"%d\n",ret);
	}
	else if ( gp->mode == GPIO_MODE_ANALOGI )
	{
		if ( b->k.write8(&b->k, CMD_ANALOG_READ) ) goto KER;
		if ( b->k.write8(&b->k, gp->io - gp->offset) ) goto KER;
		ret = b->k.read16(&b->k);
		b->k.unlock(&b->k);;
		return sprintf(buf,"%d\n",ret);
	}
	
	b->k.unlock(&b->k);
	return -EBADFD;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

static ssize_t sysfs_gpio_value_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{	
	struct gpio* gp;
	struct board* b;
	char a[ARG_LEN_MAX];
	int ret; 
	
	gp = dev_get_drvdata(dev);
	b = gp->brd;
	dbg(b->ch.debug,"");
	
	b->k.lock(&b->k);
	
	if ( gp->mode == GPIO_MODE_DIGITALO ) 
	{
		if ( b->k.write8(&b->k, CMD_DIGITAL_WRITE | (*buf - '0') ) ) goto KER;
		if ( b->k.write8(&b->k, gp->io - gp->offset) ) goto KER;
		ret = b->k.read8(&b->k);
	}
	else if ( gp->mode == GPIO_MODE_ANALOGO || gp->mode == GPIO_MODE_PWM ) 
	{
		if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) goto KER;
		ret = katoi(a);
		
		if ( b->k.write8(&b->k, CMD_ANALOG_WRITE ) ) goto KER;
		if ( b->k.write8(&b->k, gp->io - gp->offset) ) goto KER;
		if ( b->k.write8(&b->k, ret) ) goto KER;
		ret = b->k.read8(&b->k);
	}
	else
	{
		b->k.unlock(&b->k);
		return -EBADFD; 
	}
	
	b->k.unlock(&b->k);
	return (ret == ERR_OK) ? count : -ECOMM;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}
													 
static ssize_t sysfs_gpio_mode_read(struct device* dev, struct device_attribute* attr, char* buf)
{
	struct gpio* gp;
	
	gp = dev_get_drvdata(dev);
	dbg(gp->brd->ch.debug,"");
	
	switch ( gp->mode )
	{
		case GPIO_MODE_UNDEF   : return sprintf(buf,"undefinited\n");
		case GPIO_MODE_DIGITALO: return sprintf(buf,"digital output\n");
		case GPIO_MODE_ANALOGO : return sprintf(buf,"analog output\n");
		case GPIO_MODE_DIGITALI: return sprintf(buf,"digital input\n");
		case GPIO_MODE_ANALOGI : return sprintf(buf,"analog input\n");
		case GPIO_MODE_PWM     : return sprintf(buf,"pwm\n");
	}
	return -ENODATA;
}

static ssize_t sysfs_gpio_mode_write(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	struct gpio* gp;
	struct board* b;
	char a[ARG_LEN_MAX];
	int ret; 
	int io;
	
	gp = dev_get_drvdata(dev);
	b = gp->brd;
	dbg(b->ch.debug,"");
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	
	io = gp->io;
	ret = CMD_MODE;
	if ( !strcmp(a,"output") ) 
	{
		if ( !(b->ch.tblpin[gp->io] & CHIP_PIN_DIGITAL) ) return -EINVAL;
		gp->mode = GPIO_MODE_DIGITALO;
		ret |= CMD_MODE_OUTPUT; 
		
	}
	else if ( !strcmp(a,"analogoutput") ) 
	{
		if ( !(b->ch.tblpin[io] & CHIP_PIN_ADC) ) return -EINVAL;
		gp->mode = GPIO_MODE_ANALOGO;
		ret |= CMD_MODE_OUTPUT;
	}
	else if ( !strcmp(a,"pwm") ) 
	{
		if ( !(b->ch.tblpin[io] & CHIP_PIN_PWM) ) return -EINVAL;
		gp->mode = GPIO_MODE_PWM;
		ret |= CMD_MODE_OUTPUT;
	}
	else if ( !strcmp(a,"input") ) 
	{
		if ( !(b->ch.tblpin[io] & CHIP_PIN_DIGITAL) ) return -EINVAL;
		gp->mode = GPIO_MODE_DIGITALI;
		ret |= CMD_MODE_INPUT;
	}
	else if ( !strcmp(a,"pullup") ) 
	{
		if ( !(b->ch.tblpin[io] & CHIP_PIN_DIGITAL) ) return -EINVAL;
		gp->mode = GPIO_MODE_DIGITALI;
		ret |= CMD_MODE_INPPUL;
	}
	else if ( !strcmp(a,"analoginput") ) 
	{
		if ( !(b->ch.tblpin[io] & CHIP_PIN_ADC) ) return -EINVAL;
		io -= gp->offset;
		gp->mode = GPIO_MODE_ANALOGI;
		ret |= CMD_MODE_INPUT;
	}
	else 
	{
		return -EINVAL;
	} 
	
	b->k.lock(&b->k);
	if ( b->k.write8(&b->k,ret) ) goto KER;
	if ( b->k.write8(&b->k,io) ) goto KER;
	ret = b->k.read8(&b->k);
		if ( ret != ERR_OK ) goto KER;
	
	b->k.unlock(&b->k);
	return count;
	
	KER:
	dbg(b->ch.debug,"KER");
	b->k.unlock(&b->k);
	return -ECOMM;
}

/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  GPIO GENERAL ************************************ ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

int gpio_init(struct board* b)
{
	int i;
	
	dbg(b->ch.debug,"");
	
	b->ch.io = kvnew(struct gpio, b->ch.pincount);
		if ( b->ch.io == NULL) return -1;
	
	
	b->k.lock(&b->k);
	
	for ( i = 0; i < b->ch.pincount; ++i )
	{
		b->ch.io[i].brd = b;
		b->ch.io[i].dev = NULL;
		b->ch.io[i].mode = 0;
		b->ch.io[i].io = i;
		
		if ( b->k.write8(&b->k,CMD_CHP | CMD_CHP_RPIO) ) goto KER;
		if ( b->k.write8(&b->k, i) ) goto KER;
		b->ch.io[i].offset = b->k.read8(&b->k);
		
		b->ch.io[i].dev_attr_value.attr.name = "value";
		b->ch.io[i].dev_attr_value.attr.mode = 0666;
		b->ch.io[i].dev_attr_value.show = sysfs_gpio_value_read;
		b->ch.io[i].dev_attr_value.store = sysfs_gpio_value_write;
		b->ch.io[i].dev_attr_mode.attr.name = "mode";
		b->ch.io[i].dev_attr_mode.attr.mode = 0666;
		b->ch.io[i].dev_attr_mode.show = sysfs_gpio_mode_read;
		b->ch.io[i].dev_attr_mode.store = sysfs_gpio_mode_write;
		
		gpio_export(b,&b->ch.io[i]);
	}
	
	b->k.unlock(&b->k);
	
	return 0;
	
	KER:
	dbg(b->ch.debug,"KER");
	kfree(b->ch.io);
	b->ch.io = NULL;
	b->k.unlock(&b->k);
	return -1;
}

int gpio_clear(struct board* b)
{
	int i;
	
	dbg(b->ch.debug,"");
	if ( !b->ch.io ) return -1;
	
	for ( i = 0; i < b->ch.pincount; ++i )
		gpio_unexport(b,&b->ch.io[i]);
	kfree(b->ch.io);
	return 0;
}
	
int gpio_export(struct board* b, struct gpio* gp)
{
	int ret;
	char gpioname[127];
	
	dbg(b->ch.debug,"io[%d] off[%d]",gp->io,gp->offset);
	
	if ( !gp ) { dbg(b->ch.debug,"gp(null)"); return -1;}
	if ( gp->io < 0 && gp->io >= b->ch.pincount ) { dbg(b->ch.debug,"io out of range"); return -1;}
	if ( gp->dev ) { dbg(b->ch.debug,"alredy dev"); return -1;}
	if ( !(b->ch.tblpin[gp->io] & CHIP_PIN_ENABLE) ) { dbg(b->ch.debug,"no chip enable"); return -1;}
	
	if ( b->ch.tblpin[gp->io] & CHIP_PIN_ADC )
		sprintf(gpioname,"a%d.%d",gp->io - gp->offset,b->k.id);
	else if ( b->ch.tblpin[gp->io] & CHIP_PIN_DIGITAL )
		sprintf(gpioname,"d%d.%d",gp->io,b->k.id);
	else
		return -1;
	
	dbg(b->ch.debug,"creating device [%s]",gpioname);
	
	gp->dev = device_create(b->cls, NULL, 0, gp, gpioname);
	if ( IS_ERR(gp->dev) ) 
	{
		gp->dev = NULL;
		dbg(b->ch.debug,"error to device create on gpio [%d]",gp->io);
		return -1;
	}

	ret = device_create_file(gp->dev, &gp->dev_attr_value);
		if (ret < 0) dbg(b->ch.debug,"error to device create file value on gpio [%d]",gp->io);
	ret = device_create_file(gp->dev, &gp->dev_attr_mode);
		if (ret < 0) dbg(b->ch.debug,"error to device create file mode on gpio [%d]",gp->io);
	
	return 0;
}

int gpio_unexport(struct board* b, struct gpio* gp)
{
	dbg(b->ch.debug,"");
	
	if ( !gp ) return -1;
	if ( gp->io < 0 && gp->io >= b->ch.pincount ) return -1;
	if ( !gp->dev ) return -1;
	
	device_remove_file(gp->dev, &gp->dev_attr_value);
	device_remove_file(gp->dev, &gp->dev_attr_mode);
	device_destroy(b->cls, gp->dev->devt);
	gp->dev = NULL;
	
	return 0;
}
