
#include "MS51_16K.H"
#include "htim.h"
#include "vcnl4040.h"
#include "usr_i2c.h"




#define LED P3
#define EEPROM_PAGE_SIZE 32
#define PAGE_NUMBER 4

#define ERROR_CODE 0x78
#define TEST_OK 0x00


// #define PS_CONF4 0x04


// /* Define constants */
#define VCNL4040_DEVICE_ID_VAL 0x0186

#define LOWER 1
#define UPPER 0


static const UINT8 VCNL4040_ALS_IT_MASK = (UINT8)~((1 << 7) | (1 << 6));
static const UINT8 VCNL4040_ALS_IT_80MS = 0;
static const UINT8 VCNL4040_ALS_IT_160MS = (1 << 7);
static const UINT8 VCNL4040_ALS_IT_320MS = (1 << 6);
static const UINT8 VCNL4040_ALS_IT_640MS = (1 << 7) | (1 << 6);

static const UINT8 VCNL4040_ALS_PERS_MASK = (UINT8)~((1 << 3) | (1 << 2));
static const UINT8 VCNL4040_ALS_PERS_1 = 0;
static const UINT8 VCNL4040_ALS_PERS_2 = (1 << 2);
static const UINT8 VCNL4040_ALS_PERS_4 = (1 << 3);
static const UINT8 VCNL4040_ALS_PERS_8 = (1 << 3) | (1 << 2);

static const UINT8 VCNL4040_ALS_INT_EN_MASK = (UINT8)~((1 << 1));
static const UINT8 VCNL4040_ALS_INT_DISABLE = 0;
static const UINT8 VCNL4040_ALS_INT_ENABLE = (1 << 1);

static const UINT8 VCNL4040_ALS_SD_MASK = (UINT8)~((1 << 0));
static const UINT8 VCNL4040_ALS_SD_POWER_ON = 0;
static const UINT8 VCNL4040_ALS_SD_POWER_OFF = (1 << 0);

static const UINT8 VCNL4040_PS_DUTY_MASK = (UINT8)~((1 << 7) | (1 << 6));
static const UINT8 VCNL4040_PS_DUTY_40 = 0;
static const UINT8 VCNL4040_PS_DUTY_80 = (1 << 6);
static const UINT8 VCNL4040_PS_DUTY_160 = (1 << 7);
static const UINT8 VCNL4040_PS_DUTY_320 = (1 << 7) | (1 << 6);

static const UINT8 VCNL4040_PS_PERS_MASK = (UINT8)~((1 << 5) | (1 << 4));
static const UINT8 VCNL4040_PS_PERS_1 = 0;
static const UINT8 VCNL4040_PS_PERS_2 = (1 << 4);
static const UINT8 VCNL4040_PS_PERS_3 = (1 << 5);
static const UINT8 VCNL4040_PS_PERS_4 = (1 << 5) | (1 << 4);

static const UINT8 VCNL4040_PS_IT_MASK = (UINT8)~((1 << 3) | (1 << 2) | (1 << 1));
static const UINT8 VCNL4040_PS_IT_1T = 0;
static const UINT8 VCNL4040_PS_IT_15T = (1 << 1);
static const UINT8 VCNL4040_PS_IT_2T = (1 << 2);
static const UINT8 VCNL4040_PS_IT_25T = (1 << 2) | (1 << 1);
static const UINT8 VCNL4040_PS_IT_3T = (1 << 3);
static const UINT8 VCNL4040_PS_IT_35T = (1 << 3) | (1 << 1);
static const UINT8 VCNL4040_PS_IT_4T = (1 << 3) | (1 << 2);
static const UINT8 VCNL4040_PS_IT_8T = (1 << 3) | (1 << 2) | (1 << 1);

static const UINT8 VCNL4040_PS_SD_MASK = (UINT8)~((1 << 0));
static const UINT8 VCNL4040_PS_SD_POWER_ON = 0;
static const UINT8 VCNL4040_PS_SD_POWER_OFF = (1 << 0);

static const UINT8 VCNL4040_PS_HD_MASK = (UINT8)~((1 << 3));
static const UINT8 VCNL4040_PS_HD_12_BIT = 0;
static const UINT8 VCNL4040_PS_HD_16_BIT = (1 << 3);

static const UINT8 VCNL4040_PS_INT_MASK = (UINT8)~((1 << 1) | (1 << 0));
static const UINT8 VCNL4040_PS_INT_DISABLE = 0;
static const UINT8 VCNL4040_PS_INT_CLOSE = (1 << 0);
static const UINT8 VCNL4040_PS_INT_AWAY = (1 << 1);
static const UINT8 VCNL4040_PS_INT_BOTH = (1 << 1) | (1 << 0);

static const UINT8 VCNL4040_PS_SMART_PERS_MASK = (UINT8)~((1 << 4));
static const UINT8 VCNL4040_PS_SMART_PERS_DISABLE = 0;
static const UINT8 VCNL4040_PS_SMART_PERS_ENABLE = (1 << 4);

static const UINT8 VCNL4040_PS_AF_MASK = (UINT8)~((1 << 3));
static const UINT8 VCNL4040_PS_AF_DISABLE = 0;
static const UINT8 VCNL4040_PS_AF_ENABLE = (1 << 3);

static const UINT8 VCNL4040_PS_TRIG_MASK = (UINT8)~((1 << 2));
static const UINT8 VCNL4040_PS_TRIG_TRIGGER = (1 << 2);

static const UINT8 VCNL4040_WHITE_EN_MASK = (UINT8)~((1 << 7));
static const UINT8 VCNL4040_WHITE_ENABLE = 0;
static const UINT8 VCNL4040_WHITE_DISABLE = (1 << 7);

static const UINT8 VCNL4040_PS_MS_MASK = (UINT8)~((1 << 6));
static const UINT8 VCNL4040_PS_MS_DISABLE = 0;
static const UINT8 VCNL4040_PS_MS_ENABLE = (1 << 6);

static const UINT8 VCNL4040_LED_I_MASK = (UINT8)~((1 << 2) | (1 << 1) | (1 << 0));
static const UINT8 VCNL4040_LED_50MA = 0;
static const UINT8 VCNL4040_LED_75MA = (1 << 0);
static const UINT8 VCNL4040_LED_100MA = (1 << 1);
static const UINT8 VCNL4040_LED_120MA = (1 << 1) | (1 << 0);
static const UINT8 VCNL4040_LED_140MA = (1 << 2);
static const UINT8 VCNL4040_LED_160MA = (1 << 2) | (1 << 0);
static const UINT8 VCNL4040_LED_180MA = (1 << 2) | (1 << 1);
static const UINT8 VCNL4040_LED_200MA = (1 << 2) | (1 << 1) | (1 << 0);

static const UINT8 VCNL4040_INT_FLAG_ALS_LOW = (1 << 5);
static const UINT8 VCNL4040_INT_FLAG_ALS_HIGH = (1 << 4);
static const UINT8 VCNL4040_INT_FLAG_CLOSE = (1 << 1);
static const UINT8 VCNL4040_INT_FLAG_AWAY = (1 << 0);




UINT16 VCNL4040_read_id( ){
    UINT16 id;
    VCNL_Read_register(VCNL4040_ID,&id);
    return id;
}

void bitMask(uint8_t commandAddress, uint8_t commandHeight, uint8_t mask, uint8_t thing)
{

    UINT16 val;
    uint8_t registerContents;
    VCNL_Read_register(commandAddress,&val);
//   // Grab current register context
//   
    if (commandHeight == LOWER) registerContents = val & 0x0F;
    else registerContents = val >>8;

//   // Zero-out the portions of the register we're interested in
    registerContents &= mask;

//   // Mask in new thing
   registerContents |= thing;

//   // Change contents
    if (commandHeight == LOWER)  {
        val = (val & 0xFF00) + registerContents;
    }
    else {
        val = (val & 0x00FF) + ((UINT16)registerContents<< 8);
    }
    VCNL_Write_register(commandAddress,val);
}


void setLEDCurrent(uint8_t currentValue)
{
	if(currentValue > 200 - 1) currentValue = VCNL4040_LED_200MA;
	else if(currentValue > 180 - 1) currentValue = VCNL4040_LED_180MA;
	else if(currentValue > 160 - 1) currentValue = VCNL4040_LED_160MA;
	else if(currentValue > 140 - 1) currentValue = VCNL4040_LED_140MA;
	else if(currentValue > 120 - 1) currentValue = VCNL4040_LED_120MA;
	else if(currentValue > 100 - 1) currentValue = VCNL4040_LED_100MA;
	else if(currentValue > 75 - 1) currentValue = VCNL4040_LED_75MA;
	else currentValue = VCNL4040_LED_50MA;

	bitMask(VCNL4040_PS_MS, UPPER, VCNL4040_LED_I_MASK, currentValue);
}


//Set the duty cycle of the IR LED. The higher the duty
//ratio, the faster the response time achieved with higher power
//consumption. For example, PS_Duty = 1/320, peak IRED current = 100 mA,
//averaged current consumption is 100 mA/320 = 0.3125 mA.
void setIRDutyCycle(uint16_t dutyValue)
{
  if(dutyValue > 320 - 1) dutyValue = VCNL4040_PS_DUTY_320;
  else if(dutyValue > 160 - 1) dutyValue = VCNL4040_PS_DUTY_160;
  else if(dutyValue > 80 - 1) dutyValue = VCNL4040_PS_DUTY_80;
  else dutyValue = VCNL4040_PS_DUTY_40;
  
  bitMask(VCNL4040_PS_CONF1, LOWER, VCNL4040_PS_DUTY_MASK, dutyValue);
}

//Sets the integration time for the proximity sensor
void setProxIntegrationTime(uint8_t timeValue)
{
  if(timeValue > 8 - 1) timeValue = VCNL4040_PS_IT_8T;
  else if(timeValue > 4 - 1) timeValue = VCNL4040_PS_IT_4T;
  else if(timeValue > 3 - 1) timeValue = VCNL4040_PS_IT_3T;
  else if(timeValue > 2 - 1) timeValue = VCNL4040_PS_IT_2T;
  else timeValue = VCNL4040_PS_IT_1T;

  bitMask(VCNL4040_PS_CONF1, LOWER, VCNL4040_PS_IT_MASK, timeValue);
}

//Sets the proximity resolution
void setProxResolution(uint8_t resolutionValue)
{
	if(resolutionValue > 16 - 1) resolutionValue = VCNL4040_PS_HD_16_BIT;
	else resolutionValue = VCNL4040_PS_HD_12_BIT;
	
  bitMask(VCNL4040_PS_CONF2, UPPER, VCNL4040_PS_HD_MASK, resolutionValue);
}


//Enable smart persistance
//To accelerate the PS response time, smart
//persistence prevents the misjudgment of proximity sensing
//but also keeps a fast response time.
void enableSmartPersistance(void)
{
  bitMask(VCNL4040_PS_CONF3, LOWER, VCNL4040_PS_SMART_PERS_MASK, VCNL4040_PS_SMART_PERS_ENABLE);
}
void disableSmartPersistance(void)
{
  bitMask(VCNL4040_PS_CONF3, LOWER, VCNL4040_PS_SMART_PERS_MASK, VCNL4040_PS_SMART_PERS_DISABLE);
}


//Power on the prox sensing portion of the device
void powerOnProximity(void)
{
  bitMask(VCNL4040_PS_CONF1, LOWER, VCNL4040_PS_SD_MASK, VCNL4040_PS_SD_POWER_ON);
}


UINT8 VCNL_getProximity(uint16_t *res )
{
  UINT8 ret;
  ret =  VCNL_Read_register(VCNL4040_PS_DATA,res);
  bitMask(VCNL4040_PS_CONF3, LOWER, VCNL4040_PS_TRIG_MASK, VCNL4040_PS_TRIG_TRIGGER);  // enable for next convert
  return ret;
  
}

int VCNL4040_init() {
//    Adafruit_BusIO_Register chip_id = Adafruit_BusIO_Register(sensor->i2c_dev, VCNL4040_DEVICE_ID, 2);

   if (VCNL4040_read_id() != VCNL4040_DEVICE_ID_VAL) {
       return 0;
   }
  setLEDCurrent(140);
  setIRDutyCycle(160); //Set to highest duty cycle

  setProxIntegrationTime(8); //Set to max integration

  setProxResolution(16); //Set to 16-bit output
  
  enableSmartPersistance(); //Turn on smart presistance
  bitMask(VCNL4040_ALS_CONF, LOWER, VCNL4040_ALS_IT_MASK, VCNL4040_ALS_IT_160MS);

  powerOnProximity(); //Turn on prox sensing

  bitMask(VCNL4040_PS_CONF3, LOWER, VCNL4040_PS_AF_MASK, VCNL4040_PS_AF_ENABLE);
   
  return 1;
}
