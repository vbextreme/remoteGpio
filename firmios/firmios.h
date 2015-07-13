#ifndef _FIRMIOS_H_
#define _FIRMIOS_H_

#define VERSION_MJ 0
#define VERSION_MI 3
#define VERSION_IR 0


//MANCA DRV_EXPORT al posto di INIT
//MANCA DRV_UNEXPORT
//RIDURRE USO MEMORIA
//SAFE STRING

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>

#define DBG_ONS_REP 3
#define DBG_ONS_TON 350
#define DBG_ONS_TOF 350
#define DBG_ONW_REP 3 
#define DBG_ONW_TON 130
#define DBG_ONW_TOF 130

#define OPZ_CMD_BLINK   0x01
#define OPZ_SLEEP_BLINK 0x02
#define OPZ_SLEEP_AUTO  0x04

#define MUX_WIRE  0x01
#define READ_MAX  64

#define DRV_NAME_MAX 32
#define DRV_MAX 0x0F
#define DRV_ATT_MAX 0x08
#define DRV_ERR 0xFF
#define DRV_PIN_D2   0x00000001
#define DRV_PIN_D3   0x00000002
#define DRV_PIN_D4   0x00000004
#define DRV_PIN_D5   0x00000008
#define DRV_PIN_D6   0x00000010
#define DRV_PIN_D7   0x00000020
#define DRV_PIN_D8   0x00000040
#define DRV_PIN_D9   0x00000080
#define DRV_PIN_D10  0x00000100
#define DRV_PIN_D11  0x00000200
#define DRV_PIN_D12  0x00000400
#define DRV_PIN_D13  0x00000800
#define DRV_PIN_A0   0x00001000
#define DRV_PIN_A1   0x00002000
#define DRV_PIN_A2   0x00004000
#define DRV_PIN_A3   0x00008000
#define DRV_PIN_A4   0x00010000
#define DRV_PIN_A5   0x00020000
#define DRV_WIRE     0x00040000
#define DRV_SPI      0x00080000
#define DRV_ROM      0x00100000

#define DRV_EXPORT(F,V) ((F) | (V))
#define DRV_UNEXPORT(F,V) ((F) & ~(V))

//0000 0000
//PIN:

#define ERR_CMD 255
#define ERR_DRV 254
#define ERR_OK    0

#define PROTO_NAME 9

#define CMD_CHP          0x00
#define CMD_CHP_VERSION  0x00 // return 3byte(MJ,MI,IR)
#define CMD_CHP_NOP      0x01 // return byte(NOP)
#define CMD_CHP_GETOPT   0x02 // return byte(OPT) 
#define CMD_CHP_SETOPT   0x03 // +byte(OPT) - return byte(OK)
#define CMD_CHP_GETAUT   0x04 // return 4byte(sleepauto)
#define CMD_CHP_SETAUT   0x05 // +4byte(sleepauto) - return byte(OK)
#define CMD_CHP_FREERAM  0x06 // return 4byte(freeram)
#define CMD_CHP_RPIO     0x07 // return byte(offset)
#define CMD_CHP_LSDRV    0x08 
#define CMD_CHP_MODE     0x09 // return 2byte(mode)
#define CMD_CHP_EEPROM   0x0A // return byte(szeeprom)
#define CMD_CHP_I2C      0x0B // return nbyte(count,sda,scl,...)
#define CMD_CHP_SPI      0x0C // return nbyte(count,mosi,miso,sck,...)
#define CMD_CHP_INIT     0x0F // return byte(pincount) Nbyte(pinmode) str(chipname)

#define CMD_MODE         0x10
#define CMD_MODE_OUTPUT  0x01 // +byte(pin) - return byte(ok)
#define CMD_MODE_INPUT   0x02 // +byte(pin) - return byte(ok)
#define CMD_MODE_INPPUL  0x03 // +byte(pin) - return byte(ok)

#define CMD_DIGITAL_WRITE 0x20 // +byte(pin) - return byte(ok)
#define CMD_DIGITAL_READ  0x30 // +byte(pin) - return byte(ok)
#define CMD_ANALOG_WRITE  0x40 // +2byte(pin,value) - return byte(ok)
#define CMD_ANALOG_READ   0x50 // +byte(pin) - return 2byte(value)

#define CMD_COM          0xA0 
#define CMD_COM_WIRE_BEGIN   0x00 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_BUS     0x01 // +2byte(id,adr) - return byte(ok)
#define CMD_COM_WIRE_START   0x02 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_END     0x03 // +byte(id) - return byte(ok)
#define CMD_COM_WIRE_WRITE   0x04 // +2byte(id,value) - return byte(ok)
#define CMD_COM_WIRE_REQUEST 0x05 // +2byte(id,nbyte read) - return byte(ok)
#define CMD_COM_WIRE_READ    0x06 // +byte(id) - return byte(read)

#define CMD_COM_SPI_ENABLE   0x07 // +byte(id) - return byte(ok)
#define CMD_COM_SPI_DISABLE  0x08 // +byte(id) - return byte(ok)
#define CMD_COM_SPI_SETTING  0x09 // +byte(id) +byte( [x SSLow/high][x LSBFIRST/MSBFIRST][xxx SPI_CLOCK_DIV2/4/8/16/32/64/128][xx SPI_MODE0/1/2/3]) - return byte(ok)
#define CMD_COM_SPI_CS       0x0A // +2byte(id,pin) - return byte(ok)
#define CMD_COM_SPI_TRANSFER 0x0B // +2byte(id,value) - return byte(read)

#define CMD_ROM          0xB0 //
#define CMD_ROM_ADDRESS  0x00 // +2byte(address) - return byte(ok)
#define CMD_ROM_WRITE    0x01 // +byte(val) - return byte(ok)
#define CMD_ROM_READ     0x02 // return byte(val)

#define CMD_DRV_EXPORT     0xC0
#define CMD_DRV_UNEXPORT   0xD0
#define CMD_DRV_READ       0xE0
#define CMD_DRV_WRITE      0xF0


#define EVENT_DRV_GETNAME  0x00
#define EVENT_DRV_EXPORT   0x01
#define EVENT_ATT_EXPORT   0x02
#define EVENT_DRV_UNEXPORT 0x03
#define EVENT_ATT_GETNAME  0x04
#define EVENT_ATT_READ     0x05
#define EVENT_ATT_WRITE    0x06

typedef byte HDRV;
typedef byte HATT;
typedef byte(*ONEVENT)(const HDRV hd, const HATT ha, byte event, char* buf, byte count, byte* bex, void* vex);

typedef struct _DRV
{
    HDRV hd;
    ONEVENT ev;
    byte count;
}DRV;

class Firmios
{
    public:
        void setting(int baund, byte option, unsigned long autosleepms);
        void pinSetting(byte pin, byte enable);
        void blink13(int rp, int tonms, int toffms);
        void sleep();
        int freeram();
        void lock(byte mux);
        void unlock(byte mux);
        void SPIrestoreSetting();
        HDRV registerDrv(ONEVENT ev, byte attcount);
        char run();
        
        
    private:
        byte read8();
        int read16();
        unsigned long read32();
        char* readstr();
        void send8(byte val);
        void send16(int val);
        void send32(unsigned long val);
        void sendstr(const char* val);
    
        byte _option;
        unsigned long _timesleep;
        unsigned long _lastsleep;
        byte _mux;
        byte _wirebus;
        byte _wirerq;
        byte _cs;
        byte _csmode;
        byte _spisetting;
        int _romadr;
        byte _drvcount;
        DRV _drv[DRV_MAX];
        char _bufrd[READ_MAX];
        byte _bufc;
};



#endif
