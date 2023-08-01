#include "vcnl36821s.h"
#include "MS51_16K.H"
#include "htim.h"

#define SYS_DIV 1
#define I2C_CLOCK 4 /* Setting I2C clock as 100K */

#define EEPROM_SLA 0xC0
#define EEPROM_WR 0
#define EEPROM_RD 1

#define LED P3
#define EEPROM_PAGE_SIZE 32
#define PAGE_NUMBER 4

#define ERROR_CODE 0x78
#define TEST_OK 0x00

bit I2C_Reset_Flag;
//========================================================================================================
void Init_I2C(void)
{
        P13_OPENDRAIN_MODE; // Modify SCL pin to Open drain mode. don't forget the pull high resister in circuit
        P14_OPENDRAIN_MODE; // Modify SDA pin to Open drain mode. don't forget the pull high resister in circuit

        /* Set I2C clock rate */
        I2CLK = I2C_CLOCK;
			 /* Enable I2C time out divier as clock base is Fsys/4, the time out is about 4ms when Fsys = 16MHz */
				set_I2TOC_I2TOCEN;
				set_I2TOC_DIV;
				clr_I2TOC_I2TOF;
        /* Enable I2C */
        set_I2CON_I2CEN;
}
//========================================================================================================
UINT8 I2C_SI_WAIT(void)
{
        clr_I2CON_SI;
				clr_I2TOC_I2TOF;
        while (!SI){ 
					 if ( I2TOC&SET_BIT0 ) {
						 clr_I2TOC_I2TOF;
						 return 1;
					 }
				}
				return 0;
}
//========================================================================================================
void I2C_SI_Check(void)
{
        if ((I2STAT == 0x00) || (I2STAT == 0x10)|| (I2TOC&SET_BIT0))
        {
								clr_I2TOC_I2TOF;
                I2C_Reset_Flag = 1;
                set_I2CON_STO;
                SI = 0;
                if (SI)
                {
										clr_I2CON_I2CEN;
										set_I2CON_I2CEN;
                }
        }
}
//========================================================================================================
UINT8 VCNL36821_Write_register(UINT8 reg, UINT8 u8low, UINT8 u8high)
{
        /* Step1 */
        set_I2CON_STA; /* Send Start bit to I2C EEPROM */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
				}
        if (I2STAT != 0x08) /* 0x08:  A START condition has been transmitted*/
        {
                I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
        }

        /* Step2 */
        clr_I2CON_STA;                  /* Clear STA and Keep SI value in I2CON */
        I2DAT = EEPROM_SLA | EEPROM_WR; /* Send (SLA+W) to EEPROM */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
				}
        if (I2STAT != 0x18) /* 0x18: SLA+W has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
        }

        /* Step3 */
        I2DAT = reg; /* Send EEPROM's High Byte Address */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
				}
        if (I2STAT != 0x28) /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
        }

        /* Step3 */
        I2DAT = u8low; /* Send EEPROM's High Byte Address */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
				}
        if (I2STAT != 0x28) /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
        }

        /* Step4 */
        I2DAT = u8high; /* Send EEPROM's Low Byte Address */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
				}
        if (I2STAT != 0x28) /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Write_Error_Stop;
        }
        /* Step7 */
        set_I2CON_STO; /* Set STOP Bit to I2C EEPROM */
        clr_I2CON_SI;
				clr_I2TOC_I2TOF;
        while (STO) /* Check STOP signal */
        {
                I2C_SI_Check();
                if (I2C_Reset_Flag)
                        goto Write_Error_Stop;
        }

Write_Error_Stop:
				clr_I2TOC_I2TOF;
        if (I2C_Reset_Flag)
        {
                I2C_SI_Check();
                I2C_Reset_Flag = 0;
                return 0;
        }
        return 1;
}
//========================================================================================================

UINT8 VCNL36821_Read_register(UINT8 command, UINT8 *u8DAT)
{
        UINT8 u8Count;
        /* Step1 */
        set_I2CON_STA; /* Send Start bit to I2C EEPROM */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x08) /* 0x08:  A START condition has been transmitted*/
        {
								if(I2STAT == 0x48)
								{
									STO = 1;
									AA = 1;

								}
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step2 */
        I2DAT = (EEPROM_SLA | EEPROM_WR); /* Send (SLA+W) to EEPROM */
        clr_I2CON_STA;                    /* Clear STA and Keep SI value in I2CON */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x18) /* 0x18: SLA+W has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step3 */
        I2DAT = command; /* Send I2C EEPROM's High Byte Address */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x28) /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        /* Step5 */
        set_I2CON_STA; /* Repeated START */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x10) /* 0x10: A repeated START condition has been transmitted */
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step6 */
        clr_I2CON_STA;                    /* Clear STA and Keep SI value in I2CON */
        I2DAT = (EEPROM_SLA | EEPROM_RD); /* Send (SLA+R) to EEPROM */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x40) /* 0x40:  SLA+R has been transmitted; ACK has been received */
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step7 */ /* Verify I2C EEPROM data */
        for (u8Count = 0; u8Count < 2; u8Count++)
        {
                set_I2CON_AA; /* Set Assert Acknowledge Control Bit */
			//        clr_I2CON_SI;
			//        while (!SI)
			//                ;
							if(I2C_SI_WAIT()) {
											I2C_Reset_Flag = 1;
											goto Read_Error_Stop;
							}
                if (I2STAT != 0x50) /* 0x50:Data byte has been received; NOT ACK has been returned */
                {
                        I2C_Reset_Flag = 1;
                        goto Read_Error_Stop;
                }
                u8DAT[u8Count] = I2DAT;
        }
        /* Step8 */
        clr_I2CON_AA; /* Send a NACK to disconnect 24xx64 */
//        clr_I2CON_SI;
//        while (!SI)
//                ;
				if(I2C_SI_WAIT()) {
								I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
				}
        if (I2STAT != 0x58) /* 0x58:Data byte has been received; ACK has been returned */
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step9 */
        clr_I2CON_SI;
        set_I2CON_STO;
				clr_I2TOC_I2TOF;
        while (STO) /* Check STOP signal */
        {
                I2C_SI_Check();
                if (I2C_Reset_Flag)
                        goto Read_Error_Stop;
        }

Read_Error_Stop:
				clr_I2TOC_I2TOF;
        if (I2C_Reset_Flag)
        {
                I2C_SI_Check();
                I2C_Reset_Flag = 0;
                return 0;
        }
        return 1;
}
#define PS_CONF4 0x04

UINT8 readWord(UINT8 tmpreg, volatile UINT16 *rdata)
{

        UINT8 u8data[2];
        UINT8 res = VCNL36821_Read_register(tmpreg, u8data);
        *rdata = u8data[1];
        *rdata = (*rdata * 256) + u8data[0];
        return res;
}

UINT8 writeWord(UINT8 reg, UINT16 rdata)
{
        return VCNL36821_Write_register(reg, (UINT8)rdata, (UINT8)(rdata >> 8));
}

UINT8 bitsUpdate(UINT8 reg, UINT16 mask, UINT16 update)
{
        UINT16 value;

        if (!readWord(reg, &value))
        {
                return 0;
        }
        value &= mask;
        value |= update;
        return writeWord(reg, value);
}

UINT8 set_PS_I_VCSEL(UINT8 i_vcsel)
{
        return bitsUpdate(VCNL36821S_REG_PS_CONF3, ~VCNL36821S_PS_I_VCSEL_MASK, i_vcsel << VCNL36821S_PS_I_VCSEL_SHIFT);
}

#define PS_IT_3 (3 << 6)
#define PS_ITB_1 (1 << 11)
#define PS_AF_1 (1 << 6)
#define LED_I_15 (15 << 8)
#define PS_AC_PERIOD (0)
#define PS_AC_NUM (3 << 4)

#define LEDI_144mA 0x0E
#define LEDI_156mA 0x0F
void VCNL_initialize(void)
{
  // clean config bytes
  VCNL36821_Write_register(VCNL_PS_CONF1,0x01,0x00);
  VCNL36821_Write_register(VCNL_PS_CONF2,0xF0,0xE0);
  VCNL36821_Write_register(VCNL_PS_CONF3,0x00,LEDI_156mA);//config 3,4
  VCNL36821_Write_register(VCNL_PS_THDL,0x00,0x00);//
  VCNL36821_Write_register(VCNL_PS_THDH,0xFF,0x0F);//
  VCNL36821_Write_register(VCNL_PS_CANC,0x00,0x00);//
  VCNL36821_Write_register(VCNL_PS_AC_L,0x00,0x03);//

  
  VCNL36821_Write_register(VCNL_PS_CONF1,0x02,0x00);// PS_ON = 1
  VCNL36821_Write_register(VCNL_PS_CONF1,0x82,0x00);// PS_INIT=1
  VCNL36821_Write_register(VCNL_PS_CONF1,0x82,0x02);// set bit 1 of PS_CONF1, PS_ST = 0

}
void VCNL36821_Stop(void)
{
	writeWord(VCNL_PS_CONF1,0x0001);
  writeWord(VCNL_PS_CONF2,0x0001);
	writeWord(VCNL_PS_CONF3,0x0000);

}