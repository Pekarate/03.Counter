#include "usr_i2c.h"
#include "MS51_16K.H"

void Init_I2C(void)
{

        clr_I2CON_I2CEN;
        _delay_();
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

bit I2C_Reset_Flag;

//========================================================================================================
UINT8 I2C_SI_WAIT(void)
{
        clr_I2CON_SI;
        clr_I2TOC_I2TOF;
        while (!SI)
        {
							if (I2TOC & SET_BIT0)
							{
											clr_I2TOC_I2TOF;
											return 1;
							}
        }
        return 0;
}
//========================================================================================================
void I2C_SI_Check(void)
{
        if ((I2STAT == 0x00) || (I2STAT == 0x10) || (I2STAT == 0x20) || (I2TOC & SET_BIT0))
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
UINT8 VCNL_Write_register(UINT8 reg, UINT16 val)
{
        xdata UINT8 u8low;
        xdata UINT8 u8high;
        u8low = val;
        u8high = (val >> 8);
        /* Step1 */
        set_I2CON_STA; /* Send Start bit to I2C EEPROM */
                       //        clr_I2CON_SI;
                       //        while (!SI)
                       //                ;
        if (I2C_SI_WAIT())
        {
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
        I2DAT = VCNL_ADDR | I2C_WR_BIT; /* Send (SLA+W) to EEPROM */
                                        //        clr_I2CON_SI;
                                        //        while (!SI)
                                        //                ;
        if (I2C_SI_WAIT())
        {
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
        if (I2C_SI_WAIT())
        {
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
        if (I2C_SI_WAIT())
        {
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
        if (I2C_SI_WAIT())
        {
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
UINT8 cnt = 0;
UINT8 VCNL_Read_register(UINT8 reg, UINT16 *val)
{
        UINT8 u8DAT[2];
        UINT8 u8Count;
        
        /* Step1 */
        set_I2CON_STA; /* Send Start bit to I2C EEPROM */
                       //        clr_I2CON_SI;
                       //        while (!SI)
                       //                ;
        if (I2C_SI_WAIT())
        {
                I2C_Reset_Flag = 1;
                cnt = __LINE__;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x08) /* 0x08:  A START condition has been transmitted*/
        {
                if (I2STAT == 0x48)
                {
                        STO = 1;
                        AA = 1;
                }
                I2C_Reset_Flag = 1;
                cnt = __LINE__;
                
                goto Read_Error_Stop;
        }

        /* Step2 */
        I2DAT = (VCNL_ADDR | I2C_WR_BIT); /* Send (SLA+W) to EEPROM */
        clr_I2CON_STA;                    /* Clear STA and Keep SI value in I2CON */
                                          //        clr_I2CON_SI;
                                          //        while (!SI)
                                          //                ;
        if (I2C_SI_WAIT())
        {
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x18) /* 0x18: SLA+W has been transmitted; ACK has been received */
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step3 */
        I2DAT = reg; /* Send I2C EEPROM's High Byte Address */
                     //        clr_I2CON_SI;
                     //        while (!SI)
                     //                ;
        if (I2C_SI_WAIT())
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x28) /* 0x28:  Data byte in S1DAT has been transmitted; ACK has been received */
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        /* Step5 */
        set_I2CON_STA; /* Repeated START */
                       //        clr_I2CON_SI;
                       //        while (!SI)
                       //                ;
        if (I2C_SI_WAIT())
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x10) /* 0x10: A repeated START condition has been transmitted */
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }

        /* Step6 */
        clr_I2CON_STA;                    /* Clear STA and Keep SI value in I2CON */
        I2DAT = (VCNL_ADDR | I2C_RD_BIT); /* Send (SLA+R) to EEPROM */
                                          //        clr_I2CON_SI;
                                          //        while (!SI)
                                          //                ;
        if (I2C_SI_WAIT())
        {
                cnt = __LINE__;
                I2C_Reset_Flag = 1;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x40) /* 0x40:  SLA+R has been transmitted; ACK has been received */
        {
                cnt = __LINE__;
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
                if (I2C_SI_WAIT())
                {
                        cnt = __LINE__;
                        I2C_Reset_Flag = 1;
                        goto Read_Error_Stop;
                }
                if (I2STAT != 0x50) /* 0x50:Data byte has been received; NOT ACK has been returned */
                {
                        cnt = __LINE__;
                        I2C_Reset_Flag = 1;
                        goto Read_Error_Stop;
                }
                u8DAT[u8Count] = I2DAT;
        }
        *val = u8DAT[1];
        *val = (*val * 256) + u8DAT[0];
        /* Step8 */
        clr_I2CON_AA; /* Send a NACK to disconnect 24xx64 */
                      //        clr_I2CON_SI;
                      //        while (!SI)
                      //                ;
        if (I2C_SI_WAIT())
        {
                I2C_Reset_Flag = 1;
                cnt = __LINE__;
                goto Read_Error_Stop;
        }
        if (I2STAT != 0x58) /* 0x58:Data byte has been received; ACK has been returned */
        {
                I2C_Reset_Flag = 1;
                cnt = __LINE__;
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