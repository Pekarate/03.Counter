#ifndef _ADAFRUIT_VCNL4040_H
#define _ADAFRUIT_VCNL4040_H
#include "MS51_16K.H"

#define uint8_t UINT8
#define uint16_t UINT16

#define VCNL4040_I2CADDR_DEFAULT 0x60 ///< VCNL4040 default i2c address


#define VCNL4040_ALS_CONF 0x00
#define VCNL4040_ALS_THDH 0x01
#define VCNL4040_ALS_THDL 0x02
#define VCNL4040_PS_CONF1 0x03 //Lower
#define VCNL4040_PS_CONF2 0x03 //Upper
#define VCNL4040_PS_CONF3 0x04 //Lower
#define VCNL4040_PS_MS 0x04 //Upper
#define VCNL4040_PS_CANC 0x05
#define VCNL4040_PS_THDL 0x06
#define VCNL4040_PS_THDH 0x07
#define VCNL4040_PS_DATA 0x08
#define VCNL4040_ALS_DATA 0x09
#define VCNL4040_WHITE_DATA 0x0A
#define VCNL4040_INT_FLAG 0x0B //Upper
#define VCNL4040_ID 0x0C

UINT8 VCNL_getProximity(uint16_t *res );

int VCNL4040_init();


#endif

