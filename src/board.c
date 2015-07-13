#include "rgpio.h"

static int board_debug;
static struct class* rgpio_class = NULL;
static struct board* _board = NULL;


/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// *****************************  BOARD DEVICE ************************************ ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static void brd_free(struct board* b)
{
	dbg(board_debug,"");
	
	spi_clear(b);
	i2c_clear(b);
	eeprom_unexport(b);
	gpio_clear(b);
	chip_unexport(b);
	b->k.close(&b->k);
	komm_releaseid(b->k.id);
	kfree(b);
}

static struct board* brd_new(int kommdebug, int model, unsigned int ex, char* adr, unsigned int openex)
{
	struct board* b;
	
	dbg(board_debug, "");
	
	b = knew(struct board);
		if ( !b ) {dbg(board_debug,"end of memory"); return NULL;}
	b->cls = rgpio_class;
	
	b->next = _board;
	_board = b;
	
	
	if ( komm_init(&b->k,kommdebug,model,ex) ) { dbg(board_debug,"board free no comm"); kfree(b); return NULL;}
	if ( b->k.open(&b->k,adr,openex) ) { dbg(board_debug,"board free no open"); kfree(b); return NULL;}
		
	if ( chip_init(b,&b->ch,board_debug) ) { dbg(board_debug,"board free no chip init"); kfree(b); return NULL;}
	if ( chip_export(b) ) { dbg(board_debug,"board free no export chip"); kfree(b); return NULL;}
	
	return b;
}

static int brd_destroy(struct board* b)
{
	struct board* f;
	struct board* r;
				  r = NULL;
	
	dbg(board_debug,"");
	
	if ( !b ) {dbg(board_debug,"no board select"); return -1;}
	if ( !_board ) {dbg(board_debug,"board !init"); return -1;}
	
	if ( _board == b )
	{
		dbg(board_debug, "is board");
		r = _board;
		_board = _board->next;
		brd_free(r);
		return 0;
	}
	
	dbg(board_debug,"find board");
	for ( f = _board; f->next ; f = f->next)
	{
		if ( f->next == b )
		{
			r = f->next;
			f->next = f->next->next;
			break;
		}	
	}
	
	if ( !r ) {dbg(board_debug,"not find"); return -1;}
	brd_free(r);
	return 0;
}

static void brd_clear(void)
{
	struct board* n;
	
	dbg(board_debug,"");
	
	for ( ; _board; _board = n )
	{
		n = _board->next;
		brd_free(_board);
	}
}

static struct board* brd_idcom(unsigned int id)
{
	struct board* f;
	
	dbg(board_debug,"");
	for ( f = _board; f; f = f->next )
		if ( f->k.id == id ) return f;
	return NULL;
}




/// //////////////////////////////////////////////////////////////////////////////// ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// **********************************  BOARD  ************************************* ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// ******************************************************************************** ///
/// //////////////////////////////////////////////////////////////////////////////// ///

static int board_argument(char* d, unsigned int sz,char** model, char** vdbg, char** vmode, char** vadr, char** vex,  const char* arg, size_t count)
{
	char* n;
	char* v;
	
	dbg(board_debug,"");
	
	*model = karg_split(&d);
		if ( !*model ) return -1;
	
	while ( (n=karg_split(&d)) )
	{
		dbg(board_debug,"get value");
		v = karg_value(n);
		dbg(board_debug,"variable[%s]", n);
		dbg(board_debug,"value[%s]", v);
		
		if ( !strcmp("debug",n) )
			*vdbg = v;
		else if ( !strcmp("mode",n) )
			*vmode = v;
		else if ( !strcmp("to",n) )
			*vadr = v;
		else if ( !strcmp("ex",n) )
			*vex = v;
		else
			return -1;
	}
	
	dbg(board_debug,"end");
	
	return 0;
}


static ssize_t sysfs_board_read(struct class* cls, struct class_attribute* attr, char* buf)
{	
	struct board* b;
	char chid[100];
	
	dbg(board_debug,"");	
	if ( !_board ) return sprintf(buf,"no board\n");
	
	strcpy(buf,"idcomm boardname:\n");
	
	for (b = _board; b; b = b->next)
	{
		sprintf(chid,"%d\n",b->k.id);
		strcat(buf,chid);
	}
	
	return strlen(buf);
}

static ssize_t sysfs_board_write(struct class* cls, struct class_attribute* attr, const char* buf, size_t count)
{	
	struct board* b;
	char a[ARG_LEN_MAX];
	char* model;
	char* dbg;
	char* mode;
	char* adr;
	char* cex;
	unsigned int ex;
	int debu;
	int kmo;
	
	dbg(board_debug,"");
	
	cex = NULL;
	adr = NULL;
	mode = NULL;
	model = NULL;
	dbg = NULL;
	
	if ( karg_normalize(a,ARG_LEN_MAX,buf,count) ) return -EINVAL;
	board_argument(a,ARG_LEN_MAX,&model,&dbg,&mode,&adr,&cex,buf,count);
	
	if ( !strcmp(model,"connect") )
	{
		dbg(board_debug,"connect");
		if ( mode && strcmp(mode,"serial") ) {dbg(board_debug,"available only serial"); return -EINVAL;}
		if ( !adr ) return -EINVAL;
		
		kmo = KOMM_SERIAL;
		debu = (dbg) ? katoi(dbg) : DEFAULT_DEBUG_BRD;
		ex = (cex) ? katoi(cex) : DEFAULT_BAUND_RATE;
		
		b = brd_new(debu,kmo,0,adr,ex);
			if ( !b ) {dbg(board_debug,"error on create board"); return -EINVAL;}
		
		return count;
	}
	else if ( !strcmp(model,"disconnect") )
	{
		dbg(board_debug,"disconnect");
		if ( !adr ) {dbg(board_debug,"error adr");return -EINVAL;}
		dbg(board_debug,"find idboard");
		b = brd_idcom(katoi(adr));
			if ( !b ) {dbg(board_debug,"error board");return -EINVAL;}
		brd_destroy(b);
		
		return count;
	}
	
	return -EINVAL;
}

static struct class_attribute dev_attr_board = {.attr ={.name = "board", .mode = 0666},
												.show = sysfs_board_read,
												.store = sysfs_board_write
												};

int board_init(struct class* cls, int debug)
{
	int ret;
	
	board_debug = debug;
	dbg(board_debug,"");
	
	rgpio_class = cls;
	
	dbg(board_debug,"create file");
	ret = class_create_file(cls, &dev_attr_board);
		if (ret < 0) { dbg(board_debug,"error to device create file board"); return -1;}
	return 0;
}


int board_destroy(void)
{
	dbg(board_debug,"");
	class_remove_file(rgpio_class, &dev_attr_board);
	brd_clear();
	
	return 0;
}





