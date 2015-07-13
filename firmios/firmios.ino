#include "firmios.h"
#include <Wire.h>
#include <SPI.h>
#include <EEPROM.h>
//#include <pgmspace.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////  CHIP DESCRIPTOR      /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__AVR_ATmega328P__) 
    //pwm 3,5,6,9,10,11
    //spi mosi 11 miso 12 sck 13
    //i2c sda a4 scl a5
    char CHIP_NAME[] = "avr328p";
    byte  CHIP_PIN_COUNT = 20; 
    //////////////////////////   D0   D1   D2   D3   D4   D5   D6   D7 
    byte  CHIP_PIN_MODE[20] = {0x00,0x00,0x81,0x89,0x81,0x89,0x89,0x81,
    //////////////////////////   D8   D9  D10  D11  D12  D13   A0   A1 
                               0x81,0x89,0x89,0xA9,0xA1,0xA1,0x83,0x83,
    //////////////////////////   A2   A3   A4   A5
                               0x83,0x83,0x93,0x93};
                               
    int CHIP_MODE = 0x01;
    #define EEPROM_SIZE 512
    #define I2C_COUNT 1
    #define I2C_0_SDA 18
    #define I2C_0_SCL 19    
    #define SPI_COUNT   1
    #define SPI_0_MOSI 11
    #define SPI_0_MISO 12
    #define SPI_0_SCK  13
    
#else
  #error "Firmios not support this board"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////  EXPAND SERIAL        /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte Firmios::read8()
{
    while ( !Serial.available() );
    return Serial.read();
}

int Firmios::read16()
{
    int ret = (int)(read8()) << 8;
    ret |= read8(); 
    return ret;
}

unsigned long Firmios::read32()
{
    unsigned long ret = (unsigned long)(read16()) << 16;
    ret |= read16();
    return ret;
}

char* Firmios::readstr()
{
    _bufc = 0;
    while(1)
    {
        _bufrd[_bufc] = (char) read8(); 
        if (_bufrd[_bufc] == '\0') break;
        ++_bufc;
    }
    
    return _bufrd;
}

void Firmios::send8(byte val)
{
    Serial.write(val);
}

void Firmios::send16(int val)
{
    send8(val >> 8);
    send8(val & 0xFF);
}

void Firmios::send32(unsigned long val)
{
    send16(val >> 16);
    send16(val & 0xFFFF);
}

void Firmios::sendstr(const char* val)
{
    while(*val)
      send8(*val++);
    send8(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////  FIRMIOS SERIAL        ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Firmios::setting(int baund, byte option, unsigned long autosleepms)
{
    Serial.begin(baund);
    _option = option;
    _timesleep = autosleepms;
    _lastsleep = millis();
    _wirebus = 0;
    _mux = 0;
    _cs = 13;
    _csmode = 1;
    _romadr = 0;
    _drvcount = 0;
    if ( _option & OPZ_CMD_BLINK || _option & OPZ_SLEEP_BLINK ) pinMode(13,OUTPUT);
}

void Firmios::pinSetting(byte pin, byte enable)
{
    if ( enable ) 
        CHIP_PIN_MODE[pin] |= 0x80;
    else 
        CHIP_PIN_MODE[pin] &= ~0x80;
}



void Firmios::blink13(int rp, int tonms, int toffms)
{
    while( rp-- )
    {
        digitalWrite(13,1);
        delay(tonms);
        digitalWrite(13,0);
        delay(toffms);
    }
}

void Firmios::sleep()
{ 
    if ( _option & OPZ_SLEEP_BLINK ) blink13(DBG_ONS_REP,DBG_ONS_TON,DBG_ONS_TOF);
    
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer1_disable();
    power_timer2_disable();
    power_twi_disable();
    sleep_mode();
    sleep_disable();
    power_all_enable();
    
    if ( _option & OPZ_SLEEP_BLINK ) blink13(DBG_ONW_REP,DBG_ONW_TON,DBG_ONW_TOF);
    if ( _option & OPZ_SLEEP_AUTO ) _lastsleep = millis();
}

int Firmios::freeram()
{
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void Firmios::lock(byte mux)
{
    while ( _mux & mux )  run();
    _mux |= mux;
}

void Firmios::unlock(byte mux)
{
    _mux &= ~mux;  
}

void Firmios::SPIrestoreSetting()
{
    byte r0 = _spisetting;
    SPI.setDataMode(r0 & 0x03);
    r0 = r0 >> 2;
    switch ( r0 & 0x07)
    {
        case 0: SPI.setClockDivider(SPI_CLOCK_DIV2);break;
        default: case 1: SPI.setClockDivider(SPI_CLOCK_DIV4);break;
        case 2: SPI.setClockDivider(SPI_CLOCK_DIV8);break;
        case 3: SPI.setClockDivider(SPI_CLOCK_DIV16);break;
        case 4: SPI.setClockDivider(SPI_CLOCK_DIV32);break;
        case 5: SPI.setClockDivider(SPI_CLOCK_DIV64);break;
        case 6: SPI.setClockDivider(SPI_CLOCK_DIV128);break;
    }
    r0 = r0 >> 3;
    SPI.setBitOrder(r0 & 0x01);
    r0 = r0 >> 1;
    _csmode = r0 & 0x01;
    digitalWrite(_cs,_csmode);
}

HDRV Firmios::registerDrv(ONEVENT ev, byte attcount)
{
    if ( _drvcount == DRV_MAX ) return DRV_ERR;
    _drv[_drvcount].ev = ev;
    _drv[_drvcount].ev = ev;
    _drv[_drvcount].count = attcount;
    return _drvcount++;
}

char Firmios::run()
{
    char ret = 0;
    
    if ( (_option & OPZ_SLEEP_AUTO) && (millis() > _lastsleep + _timesleep) ) sleep();
    
    if ( !Serial.available() ) return 0;
    
    if ( _option & OPZ_CMD_BLINK ) digitalWrite(13,1);
    
    byte rd = Serial.read();
    
    byte cmdh = (rd & 0xF0);
    byte cmdl = (rd & 0x0F);
    byte r0,r1;
    
    switch( cmdh )
    {
        case CMD_CHP:
            switch( cmdl )
            {              
                case CMD_CHP_VERSION: 
                    send8(VERSION_MJ);
                    send8(VERSION_MI);
                    send8(VERSION_IR);
                break;
                
                case CMD_CHP_NOP: 
                    send8(CMD_CHP_NOP); 
                break;
                
                case CMD_CHP_GETOPT:
                  send8(_option);
                break;
                
                case CMD_CHP_SETOPT :
                    _option = read8();
                    send8(ERR_OK);
                break;
                
                case CMD_CHP_GETAUT: 
                    send32(_timesleep); 
                break;
                
                case CMD_CHP_SETAUT: 
                    _timesleep = read32(); 
                    send8(ERR_OK); 
                break;
                
                case CMD_CHP_FREERAM: 
                    send32(freeram()); 
                break;
                
                case CMD_CHP_RPIO:
                    r0 = read8();
                    
                    #if defined(__AVR_ATmega328P__) 
                        send8( (CHIP_PIN_MODE[r0] & 0x02) ? 14 : 0 );
                    #else
                        #error "need define real pin";
                    #endif
                    
                break;
                
                case CMD_CHP_LSDRV  :
                    send8(_drvcount);
                    for ( r0 = 0; r0 < _drvcount; ++r0)
                    {
                        char drvname[DRV_NAME_MAX];
                        
                        if ( _drv[r0].ev(r0,0,EVENT_DRV_GETNAME,drvname,DRV_NAME_MAX,NULL,NULL) )
                            send8(0);
                        else
                        {
                            sendstr(drvname);
                        }
                        
                        send8(_drv[r0].count);
                        byte a;
                        for ( a = 0; a < _drv[r0].count; ++a)
                        {
                            if ( _drv[r0].ev(r0,a,EVENT_ATT_GETNAME,drvname,DRV_NAME_MAX,NULL,NULL) )
                                send8(0);
                            else
                            {
                                sendstr(drvname);
                            } 
                        }
                    }
                break;             
                
                case CMD_CHP_MODE:
                    send16(CHIP_MODE);
                break;
                
                case CMD_CHP_EEPROM:
                    send16(EEPROM_SIZE);
                break;    
                
                case CMD_CHP_I2C:
                    send8(I2C_COUNT);
                    #if defined(__AVR_ATmega328P__) 
                        send8(I2C_0_SDA);
                        send8(I2C_0_SCL);
                    #else
                        #error "need define i2c proto";
                    #endif
                break;

                case CMD_CHP_SPI:
                    send8(SPI_COUNT);
                    #if defined(__AVR_ATmega328P__) 
                        send8(SPI_0_MOSI);
                        send8(SPI_0_MISO);
                        send8(SPI_0_SCK);
                    #else
                        #error "need define i2c proto";
                    #endif
                break;
                
                case CMD_CHP_INIT:
                    send8(CHIP_PIN_COUNT);
                    for ( r0 = 0; r0 < CHIP_PIN_COUNT; ++r0) send8(CHIP_PIN_MODE[r0]);
                    sendstr(CHIP_NAME);
                break;
                
                default: send8(ERR_CMD); break;
            }
        break;
        
        case CMD_MODE:
            r0 = read8();
            switch( cmdl )
            {
                case CMD_MODE_OUTPUT: pinMode(r0,OUTPUT); send8(ERR_OK); break;
                case CMD_MODE_INPUT: pinMode(r0,INPUT); send8(ERR_OK); break;
                case CMD_MODE_INPPUL: pinMode(r0,INPUT); digitalWrite(r0,1); send8(ERR_OK); break;
                default: send8(ERR_CMD); break;
            }
        break;
        
        case CMD_DIGITAL_WRITE: 
            r0 = read8(); 
            digitalWrite(r0,cmdl); 
            send8(ERR_OK); 
        break;
        
        case CMD_DIGITAL_READ: 
            r0 = read8(); 
            send8(digitalRead(r0)); 
        break;
        
        case CMD_ANALOG_WRITE: 
            r0 = read8();
            analogWrite(r0,read8());
            send8(ERR_OK);
        break;
        
        case CMD_ANALOG_READ: 
            r0 = read8(); 
            send16(analogRead(r0)); 
        break;
        
        
        case CMD_COM:
            switch ( cmdl )
            {
                case CMD_COM_WIRE_BEGIN: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__) 
                        Wire.begin(); 
                        send8(ERR_OK); 
                    #else
                        #error "need define wire begin";
                    #endif
                break;
                
                case CMD_COM_WIRE_BUS: 
                    r0 = read8(); 
                    #if defined(__AVR_ATmega328P__)                   
                        _wirebus = read8(); 
                    #else
                        #error "need define wire bus";
                    #endif                    
                break;
                  
                case CMD_COM_WIRE_START: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)
                        _mux |= MUX_WIRE; 
                        Wire.beginTransmission(_wirebus);
                        send8(ERR_OK);
                    #else
                        #error "need define wire start";
                    #endif                    
                break;
                
                case CMD_COM_WIRE_END:                    
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)
                        _mux &= ~MUX_WIRE; 
                        Wire.endTransmission(); 
                        send8(ERR_OK); 
                    #else
                        #error "need define wire end";
                    #endif                        
                break;
                
                case CMD_COM_WIRE_WRITE: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)                    
                        r0 = read8(); 
                        Wire.write(r0); 
                        send8(ERR_OK); 
                    #else
                        #error "need define wire write";
                    #endif                                            
                break;
                
                case CMD_COM_WIRE_REQUEST: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)                    
                        _mux |= MUX_WIRE; 
                        _wirerq = read8(); 
                        Wire.requestFrom(_wirebus,_wirerq); 
                        send8(ERR_OK); 
                    #else
                        #error "need define wire write";
                    #endif                    
                break;
                
                case CMD_COM_WIRE_READ:
                    r0 = read8(); 
                    #if defined(__AVR_ATmega328P__)                            
                        if ( !_wirerq-- ) { send8(ERR_CMD); _mux &= ~MUX_WIRE; break;}
                        while ( !Wire.available() ); 
                        r0 = Wire.read(); send8(r0); 
                        if ( !_wirerq ) _mux &= ~MUX_WIRE;
                    #else
                        #error "need define wire read";
                    #endif                                        
                break;
                
                case CMD_COM_SPI_ENABLE: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)               
                        SPI.begin();
                        send8(ERR_OK); 
                    #else
                        #error "need define spi enable";
                    #endif
                break;
                
                case CMD_COM_SPI_DISABLE: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)
                        SPI.end(); 
                        send8(ERR_OK); 
                    #else
                        #error "need define spi disable";
                    #endif
                break;
                
                case CMD_COM_SPI_CS: 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)
                      _cs=read8(); 
                      pinMode(_cs,OUTPUT); 
                      send8(ERR_OK); 
                    #else
                        #error "need define spi cs";
                    #endif
                break;
                
                case CMD_COM_SPI_SETTING : 
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)                
                        _spisetting = read8();
                        SPIrestoreSetting();
                        send8(ERR_OK);
                    #else
                        #error "need define spi setting";
                    #endif
                break;
                
                case CMD_COM_SPI_TRANSFER:
                    r0 = read8();
                    #if defined(__AVR_ATmega328P__)                                
                        _csmode = !_csmode;
                        digitalWrite(_cs,_csmode);
                        r0 = read8();
                        r0 = SPI.transfer(r0);
                        _csmode = !_csmode;
                        digitalWrite(_cs,_csmode);
                        send8(r0);
                    #else
                        #error "need define spi stransfert";
                    #endif
                break;                    
                
                default: send8(ERR_CMD); break;
            }
        break;
        
        case CMD_ROM:
            switch ( cmdl )
            {
                case CMD_ROM_ADDRESS: 
                    _romadr = read16(); 
                    send8(ERR_OK); 
                break;
                
                case CMD_ROM_WRITE: 
                    r0 = read8(); 
                    EEPROM.write(_romadr++,r0); 
                    send8(ERR_OK); 
                break;
                
                case CMD_ROM_READ: 
                    send8(EEPROM.read(_romadr++));
                break;
                
                default: send8(ERR_CMD); break;
            }
        break;
        
        case CMD_DRV_EXPORT:
            if ( cmdl >= _drvcount ) { send8(ERR_DRV); break;}
            send8(ERR_OK);
            readstr();
            if ( _drv[cmdl].ev(cmdl,0,EVENT_DRV_EXPORT,_bufrd,_bufc,NULL,NULL) ) { send8(ERR_DRV); break; }
            send8(ERR_OK);
            send8(_drv[cmdl].count);
            for ( r0 = 0; r0 < _drv[cmdl].count; ++r0)
            {
                byte type;
                if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_EXPORT,_bufrd,_bufc,&type,NULL) ) { send8(ERR_DRV); break; }
                send8(type);
            }
        break;
        
        case CMD_DRV_UNEXPORT:
            if ( cmdl >= _drvcount ) { send8(ERR_DRV); break;}
            send8(ERR_OK);
            if ( _drv[cmdl].ev(cmdl,0,EVENT_DRV_UNEXPORT,NULL,0,NULL,NULL) ) { send8(ERR_DRV); break; }
            send8(ERR_OK);
        break;
        
        case CMD_DRV_READ:
            r0 = read8();
            if ( cmdl >= _drvcount ) { send8(ERR_DRV); break;}
            if ( r0 >= _drv[cmdl].count ) { send8(ERR_DRV); break;}
            send8(ERR_OK);
            {
                byte ty = read8();
                byte buf[32];
                if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_READ,NULL,_bufc,&ty,buf) ) {send8(ERR_DRV); break;}
                switch( ty )
                {
                    case 0: sendstr((char*)buf); break;
                    case 1: send8(*buf); break;
                    case 2: send8(buf[0]); send8(buf[1]); break;
                    case 4: send8(buf[0]); send8(buf[1]); send8(buf[2]); send8(buf[3]); break;
                    default: send8(ERR_DRV); break;
                }
            }
        break;
        
        case CMD_DRV_WRITE:
            r0 = read8();
            r1 = read8();
            if ( cmdl >= _drvcount ) { send8(ERR_DRV); break;}
            if ( r0 >= _drv[cmdl].count ) { send8(ERR_DRV); break;}
            send8(ERR_OK);
            
            switch (r1)
            {
                case 0:
                    readstr();
                    if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_WRITE, _bufrd,_bufc,&r1,NULL) ) {send8(ERR_DRV); break;}
                    send8(ERR_OK);
                break;
                
                case 1:
                {
                    byte r2;
                    r2 = read8();
                    if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_WRITE, NULL, 0,&r1,&r2) ) {send8(ERR_DRV); break;}
                    send8(ERR_OK);
                }
                break;
                
                case 2:
                {
                    int r2;
                    r2 = read16();
                    if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_WRITE, NULL, 0,&r1,&r2) ) {send8(ERR_DRV); break;}
                    send8(ERR_OK);
                }
                break;
                
                case 4:
                {
                    long r2;
                    r2 = read32();
                    if ( _drv[cmdl].ev(cmdl,r0,EVENT_ATT_WRITE, NULL, 0,&r1,&r2) ) {send8(ERR_DRV); break;}
                    send8(ERR_OK);
                }
                break;                
            }
        break;        
        
        default: send8(ERR_CMD); break;
    }

    if ( _option & OPZ_SLEEP_AUTO ) _lastsleep = millis();    
    if ( _option & OPZ_CMD_BLINK ) digitalWrite(13,0);

    return ret;
}




















