#include "kaid.h"

int karg_normalize(char* d, int max, const char* buf, size_t count)
{
	if ( count >= max - 1 ) return -1; 
	strncpy(d,buf,count);
	if ( d[count - 1] == '\n' )
		d[count - 1] = '\0';
	else
		d[count] = '\0';
		
	return 0;
}

char* karg_split(char** args)
{
	char* a;
	char* f;
	
	a = *args;
	f = NULL;
	
	if ( !*a ) return NULL;
	
	while ( *a && (*a == ' ' || *a == '\t')) ++a;
		if ( !*a ) return NULL;
		
	f = a;
	while ( *a && *a != ' ' && *a != '\t') ++a;
	
	if ( *a ) *a++ = '\0';
	*args = a;
	return f; 
}

char* karg_value(char* a)
{
	while ( *a && *a != '=' ) ++a;
		if ( !*a ) return NULL;
	*a++ = '\0';
	return a;
} 

int karg_intlen(unsigned int v)
{
	if ( v < 10 ) return 1;
	if ( v < 100 ) return 2;
	if ( v < 1000 ) return 3;
	if ( v < 10000 ) return 4;
	return 12;
}

char* kstrhep(char* str)
{
	char* s = kvnew(char,strlen(str) + 1);
	strcpy(s,str);
	return s;
}

int katoi(char* v)
{
	char* s;
	int base = 10;
	
	if ( *v == '0' && *(v+1) != '\0' && *(v+1) >= '0' && *(v+1) <= '9' )
		return simple_strtol(v,NULL,8);
		
	for ( s = v; *s; ++s)
		if ( *s == 'x' || *s == 'X' ) {base = 16; break;}
	
	return simple_strtol(v,NULL,base);
}

int kbit_rightfirstcount(unsigned int val)
{
	int i;
	
	for ( i = 0; i < 32; ++i)
		if ( (val >> i) & 1 ) break;
	
	return ( i >= 32 ) ? -1 : i;
}

void kbita_set(unsigned int* ba, unsigned int adrsz, unsigned int id)
{
	ba[id >> adrsz] |= 1 << ( id & 0x1F);
}

void kbita_res(unsigned int* ba, unsigned int adrsz, unsigned int id)
{
	ba[id >> adrsz] &= ~(1 << ( id & 0x1F));
}

int kbita_freedom(unsigned int* ba, unsigned int sz, unsigned int adrsz)
{
	unsigned int i;
	int id;
	id = -1;
	for (i = 0; i < sz; ++i)
	{
		id = kbit_rightfirstcount(ba[i] & 0x1F);
		if ( id >= 0 )
		{
			id = id | ( i << adrsz);
			break;
		}
	}
	return id;
}
