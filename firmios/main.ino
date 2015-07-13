#include "firmios.h"

Firmios firmios;

void setup()
{
    //firmios.setting(9600, OPZ_CMD_BLINK | OPZ_SLEEP_BLINK | OPZ_SLEEP_AUTO, 60000);
    firmios.setting(9600, OPZ_SLEEP_BLINK | OPZ_SLEEP_AUTO, 60000);
    firmios.registerDrv(servo_onevent,3);
}

void loop()
{
    firmios.run();
    /*your code here*/
}

#include <Servo.h>
#include <string.h>

Servo servo;

#define ATTRIBUTE_ATTACH 0
#define ATTRIBUTE_DETACH 1
#define ATTRIBUTE_ANGLE  2

const char *att_name[] = {"attach","detach","angle"};

byte servo_onevent(const HDRV hd, const HATT ha, byte event, char* buf, byte count, byte* bex, void* vex)
{
    byte* val = (byte*) vex;
  
    switch (event)
    {
        case EVENT_DRV_GETNAME:
            strcpy(buf,"servo");
        break;
        
        case EVENT_DRV_EXPORT:
        break;

        case EVENT_DRV_UNEXPORT:
            servo.detach();
        break;
        
        case EVENT_ATT_GETNAME:
            strcpy(buf,att_name[ha]);
        break;

        case EVENT_ATT_EXPORT:
            *bex = ( ATTRIBUTE_DETACH == ha ) ? 0 : 1;
        break;
        
        case EVENT_ATT_READ:
        return 1;
        
        case EVENT_ATT_WRITE:
            switch ( ha )
            {
                case ATTRIBUTE_ATTACH:
                    servo.attach(*val);
                break;
                
                case ATTRIBUTE_DETACH:
                    servo.detach();
                break;
                
                case ATTRIBUTE_ANGLE:
                    servo.write(*val);
                break;
            }
        break;
    }
    
    return 0; 
}

/*
byte servo5_init(const HDRV hd, const char* buf, int count)
{
    servo.attach(5);
    return 0;
}

byte servo5_read(const HDRV hd, const HATT ha, char* buf)
{
    return ERR_DRV;
}

byte servo5_write(const HDRV hd, const HATT ha, const char* buf, int count)
{
    byte v = atoi(buf);
    if ( v > 180 ) return ERR_DRV;
    servo.write(v);
    return ERR_OK;
}
*/
