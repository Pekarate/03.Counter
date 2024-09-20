#ifndef _USR_I2C_H
#define _USR_I2C_H
#include "MS51_16K.H"

#define SYS_DIV 1
#define I2C_CLOCK 4 /* Setting I2C clock as 100K */
#define VCNL_ADDR 0xC0
#define I2C_WR_BIT 0
#define I2C_RD_BIT 1

void Init_I2C(void);

UINT8 VCNL_Write_register(UINT8 reg, UINT16 val);
UINT8 VCNL_Read_register(UINT8 reg,  UINT16 *val);

#endif