#include "komm.h"
#include "kaid.h"

static unsigned int baid[KOMM_SIZE] = {~0};

static int serial_open(struct komm* k, const char* serialport, unsigned int baund)
{
	mm_segment_t seg_old;
	struct termios termios_new;
	
	dbg(k->debug,"");
	
	if ( k->fs ) return -1;
	if ( !serialport ) return -1;
	
	switch ( baund )
	{
		case 4800: baund = B4800; break;
		default: case 9600: baund = B9600; break;
		case 19200: baund =  B19200; break;
		case 38400: baund = B38400; break;
		case 57600: baund = B57600; break;
		case 115200: baund = B115200; break;
		case 230400: baund = B230400; break;
		case 460800: baund = B460800; break;
		case 500000: baund = B500000; break;
		case 576000: baund = B576000; break;
		case 921600: baund = B921600; break;
		case 1000000: baund = B1000000; break;
		case 1152000: baund = B1152000; break;
		case 1500000: baund = B1500000; break;
		case 2000000: baund = B2000000; break;
		case 2500000: baund = B2500000; break;
		case 3000000: baund = B3000000; break;
		case 3500000: baund = B3500000; break;
		case 4000000: baund = B4000000; break;
	}
	
	seg_old = get_fs();
	set_fs(KERNEL_DS);
	k->fs = filp_open(serialport,O_RDWR | O_NOCTTY | O_NONBLOCK,0);
	
	if ( IS_ERR(k->fs) )
	{
		set_fs(seg_old);
		dbg(k->debug,"error open serial file");
		k->fs = NULL;
		return -1;
	}	
	
	k->fs->f_op->unlocked_ioctl(k->fs,TCGETS,(unsigned long)&k->old);
	
	memset(&termios_new,0,sizeof(struct termios));
	termios_new.c_cflag = CRTSCTS | CS8 | baund | CLOCAL | CREAD;
	termios_new.c_iflag = IGNPAR;
	termios_new.c_lflag = 0;
	termios_new.c_cc[VTIME] = 5;
	termios_new.c_cc[VMIN] = 1;
	termios_new.c_oflag = 0;
	
	k->fs->f_op->unlocked_ioctl(k->fs,TCFLSH,TCIFLUSH);
	k->fs->f_op->unlocked_ioctl(k->fs,TCSETS,(unsigned long)&termios_new);
	
	set_fs(seg_old);
	
	dbg(k->debug,"connected to serial port");
	k->status = KOMM_CONNECT;
	return 0;
}

static int serial_close(struct komm* k)
{
	mm_segment_t seg_old;
	
	dbg(k->debug,"");
	
	if ( !k->fs ) {dbg(k->debug,"no komm"); return -1;}
	
	seg_old = get_fs();
	set_fs(KERNEL_DS);
	
	k->fs->f_op->unlocked_ioctl(k->fs,TCSETS,(unsigned long)&k->old);
	
	filp_close(k->fs,NULL);
	k->fs = NULL;
	
	set_fs(seg_old);
	k->status = KOMM_DISCONNECT;
	return 0;
}

static int serial_available(struct komm* k)
{
	long ret = 0;
	mm_segment_t seg_old;
	
	if ( !k->fs ) return -1;
	
	seg_old = get_fs();
	set_fs(KERNEL_DS);
	
	k->fs->f_op->unlocked_ioctl(k->fs,FIONREAD,(unsigned long)&ret);
	
	set_fs(seg_old);
	return ret;
}

static int serial_write(struct komm* k, const unsigned char *buf, int count)
{
	int ret;
	mm_segment_t seg_old;

	dbg(k->debug,"");
	
	if ( !k->fs ) {dbg(k->debug,"no komm"); return -1;}
	
	seg_old = get_fs();
	set_fs(KERNEL_DS);
	
	k->fs->f_pos = 0;
	ret = k->fs->f_op->write(k->fs, buf, count, &k->fs->f_pos);
	
	set_fs(seg_old);
	
	return ret;
}

static int serial_write8(struct komm* k, const unsigned char val)
{
	return (serial_write(k,(unsigned char*)&val,1) == 1) ? 0 : -1;
}

static int serial_write16(struct komm* k, const unsigned short int val)
{
	if ( serial_write8(k,val >> 8) ) return -1;
	return serial_write8(k,val & 0xFF);
}

static int serial_write32(struct komm* k, const unsigned int val)
{
	if ( serial_write8(k,(val >> 24) & 0xFF) ) return -1;
	if ( serial_write8(k,(val >> 16) & 0xFF) ) return -1;
	if ( serial_write8(k,(val >> 8) & 0xFF) ) return -1;
	return serial_write8(k,val & 0xFF);
}
	
static int serial_writestr(struct komm* k, const char* val)
{
	unsigned int l;
	int i;
	l = strlen(val) + 1;
	for ( i = 0; i < l; ++i, ++val)
		if ( serial_write8(k,*val) ) return -1;
	return 0;
}

static int serial_read(struct komm* k, unsigned char* buf, int szbuf)
{
	int ret;
	int retries;
	mm_segment_t seg_old;
	
	dbg(k->debug,"");
	
	if ( !k->fs ) {dbg(k->debug,"no komm"); return -1;}
	
	seg_old = get_fs();
	set_fs(KERNEL_DS);
	
	ret = 0;
	retries = 0;
	
	while (1) 
	{
		unsigned char ch;
		++retries;
		if (retries >= k->timeout)	break;

		k->fs->f_pos = 0;
		if (k->fs->f_op->read(k->fs, &ch, 1, &k->fs->f_pos) == 1)
		{
			*buf++ = ch;
			++ret;
			if ( ret >= szbuf ) break;
			retries = 0;
		}
		udelay(100);
	}
		
	set_fs(seg_old);
	
	return ret;
}

static int serial_read8(struct komm* k)
{
	unsigned char r;
	return (serial_read(k,&r,1) == 1) ? r : -1;
}

static int serial_read16(struct komm* k)
{
	unsigned short int r;
	r =  (serial_read8(k) & 0xFF) << 8;
	r |= (serial_read8(k) & 0xFF);
	return r;
}

static int serial_read32(struct komm* k)
{
	unsigned int r;
	r  = (serial_read8(k) & 0xFF) << 24;
	r |= (serial_read8(k) & 0xFF) << 16;
	r |= (serial_read8(k) & 0xFF) << 8;
	r |= serial_read8(k);
	return r;
}

static int serial_readstr(struct komm* k, char* buf,unsigned int max)
{
	int c,r;
	
	c = 0;
	r = 1;
	while( r )
	{
		if ( (r = serial_read8(k)) == -1 ) return -1;
		if ( c < max ) *buf++ = r;
		++c;
	}
	if ( c >= max ) *(buf-1) = '\0';
	return c;
}

static int serial_discharge(struct komm* k, unsigned int t)
{
	unsigned char d;
	unsigned int rp;
				 rp = 0;
	
	dbg(k->debug,"");
	
	while( rp < t )
	{
		while ( k->available(k) ) d = k->read8(k);
		udelay(100);
	}
	
	return 0;
}

static int serial_init(struct komm* k, int debug, unsigned int timeout)
{	
	dbg(debug, "");
	
	k->debug = debug;
	if ( !timeout ) timeout = KOMM_SERIAL_TIMEOUT;
	k->timeout = timeout;
	k->fs = NULL;
	
	k->open = serial_open;
	k->close = serial_close;
	k->available = serial_available;
	k->write = serial_write;
	k->write8 = serial_write8;
	k->write16 = serial_write16;
	k->write32 = serial_write32;
	k->writestr = serial_writestr;
	k->read = serial_read;
	k->read8 = serial_read8;
	k->read16 = serial_read16;
	k->read32 = serial_read32;
	k->readstr = serial_readstr;
	k->discharge = serial_discharge;
	return 0;
}

static void komm_lock(struct komm* k)
{
	dbg(k->debug,"lock");
	mutex_lock(&k->mux);
}

static void komm_unlock(struct komm* k)
{
	dbg(k->debug,"unlock");
	mutex_unlock(&k->mux);
}

static int komm_islock(struct komm* k)
{
	dbg(k->debug,"islock");
	return mutex_is_locked(&k->mux);
}


int komm_init(struct komm* k, int debug, int model, unsigned int ex)
{
	int id;
	
	id = kbita_freedom(baid,KOMM_SIZE,KOMM_ADR);
		if ( id < 0 || id > KOMM_MAX ) return -1;
	kbita_res(baid,KOMM_ADR,id);
	
	if ( model == KOMM_SERIAL )
	{
		serial_init(k,debug,ex);
	}
	else
	{
		return -1;
	}
	
	mutex_init(&k->mux);
	k->lock = komm_lock;
	k->unlock = komm_unlock;
	k->islock = komm_islock;
	k->status = KOMM_DISCONNECT;
	k->id = id;
	
	return 0;
}

void komm_releaseid(unsigned int id)
{
	kbita_set(baid,KOMM_ADR,id);
}
