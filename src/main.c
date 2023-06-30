/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2020 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/


#include "MS51_16K.H"
#include "htim.h"
#include "vcnl36821s.h"

typedef enum
{
	SYS_MODE_A = 0,
	SYS_MODE_B
} _Sys_Mode;

UINT8 LCD_CODE[] = {0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00};
#define LCD_DOT 0x80

#define LCD_PWM_ON P05 = 1
#define LCD_PWM_OFF P05 = 0

#define LCD_SCK_HIGH P04 = 1
#define LCD_SCK_LOW P04 = 0

#define LCD_LAT_HIGH P03 = 1
#define LCD_LAT_LOW P03 = 0

#define LCD_DATA P01
#define LCD_DATA_HIGH P01 = 1
#define LCD_DATA_LOW P01 = 0

#define BUTTON_PRESSED !P07
#define IS_SYS_RUN_MOD_A 	P15
#define TIMOUT_NON_DETECT_OBJ 120000    //ms 

UINT16 valueps;
UINT16 obj_count = 0;

void LCD_show(UINT16 count);

void system_shutdown(){
	VCNL36821_Stop();
	BOD_DISABLE;
	ALL_GPIO_INPUT_MODE;
	ENABLE_BIT7_FALLINGEDGE_TRIG;
	ENABLE_PIN_INTERRUPT;
	ENABLE_GLOBAL_INTERRUPT;
	while(1)
	{
		HAL_TIM_Pause();

		clr_PCON_IDLE;
		set_PCON_PD;
		HAL_TIM_run();
		HAL_Delay(150);
		if(BUTTON_PRESSED){   //debound buton
			while(BUTTON_PRESSED);
			break;
		}
	}
	set_SWRST;
	
}
void LCD_INIT()
{
	// P05 LCD_PW out
	// P04 LCD_SCK out
	// P03 LCD_LAT out
	// P01 LCD_DATA out
	P0M1 &= 0xC5;  // 0b11000101;
	P0M2 |= ~0xC5; // 0b00111010;
	LCD_PWM_ON;
	LCD_SCK_LOW;
	LCD_LAT_LOW;
	LCD_DATA_LOW;
}
void LCD_Delay(UINT8 dl)
{
	int i;
	for (i = 0; i < dl; i++)
	{
	}
}

void LCD_send_bytes(UINT8 *dt)
{
	INT8 i, j;
	LCD_LAT_LOW;
	for (i = 2; i >= 0; i--)
	{
		for (j = 7; j >= 0; j--)
		{
			LCD_SCK_LOW;
			LCD_DATA = ((dt[i] >> j) & 0x01);
			LCD_Delay(5);
			LCD_SCK_HIGH;
			LCD_Delay(5);
		}
	}
	LCD_LAT_HIGH;
	LCD_Delay(10);
	LCD_LAT_LOW;
}


_Sys_Mode Sys_Mode = SYS_MODE_B;
void LCD_show(UINT16 count)
{
	UINT8 lcd_data[3];
	if (Sys_Mode == SYS_MODE_A)
	{
		count = (count % 198);
		lcd_data[0] = LCD_CODE[(count + 1) / 20];
		lcd_data[1] = LCD_CODE[(((count + 1) / 2) % 10)] + LCD_DOT;
		lcd_data[2] = LCD_CODE[0];
		if (count)
		{
			lcd_data[2] = LCD_CODE[2];
			if (count % 2)
			{
				lcd_data[2] = LCD_CODE[1];
			}
		}
	}
	else
	{
		count = (count % 1000);
		lcd_data[2] = LCD_CODE[count % 10];
		lcd_data[1] = LCD_CODE[(count / 10) % 10];
		lcd_data[0] = LCD_CODE[count / 100];
	}
	LCD_send_bytes(lcd_data);
	LCD_Delay(10);
	lcd_data[0] = lcd_data[1] = lcd_data[2] = 0x00;
	LCD_send_bytes(lcd_data);
	LCD_Delay(10);
}



#define DETECT_THRESHOLD  20
#define NON_DETECT_COUNT 2

#define TIME_COUNT_OFFJECT 1500  //ms
#define TIME_CHECK_OBJECT  400		//ms
#define OBJECT_INC_TIMES  TIME_COUNT_OFFJECT/TIME_CHECK_OBJECT	
void reset_counter(){
	obj_count = 0;
}
void Process_VCNL36821S(void) {
		static UINT32 ttime = 0;
		static UINT8 error = 0;
		static UINT8 object_detected = 0;
		static UINT8 non_object_detected = NON_DETECT_COUNT;
		if (HAL_GetTick() > ttime)
		{
			ttime = HAL_GetTick() + TIME_CHECK_OBJECT;
			if(readWord(VCNL_PS_DATA,&valueps))
			{
				if(valueps > DETECT_THRESHOLD)
				{
					error = 0;
					object_detected ++;
					if(object_detected == OBJECT_INC_TIMES)
					{
						if (Sys_Mode == SYS_MODE_A)
						{	
							obj_count ++;
						}
					}
					if(object_detected > OBJECT_INC_TIMES)
					{
						object_detected = OBJECT_INC_TIMES+1;
					}
				} else if(object_detected){
						non_object_detected --;
						if(!non_object_detected){
							object_detected = 0;
							non_object_detected = NON_DETECT_COUNT;
						}
				}
			}
			else { 
				error++;
				if(error == 5){
					
				}
			}
			
		}
}

void GPIO_Init()
{
	//P0.7 input
		P0M1 |= 0x80;
		P0M2 &= 0x7F;
}
typedef enum{
	BTN_IDLE  = 0,
	BTN_DEBOUND,
	BTN_CLICKED,
	BTN_PRESSED2S,
	BTN_PRESSED2_5S,
	BTN_PRESSED3S,
	BTN_RELEASE
}_btn_state;

void btn_time_2sec_callback()
{
	if (Sys_Mode == SYS_MODE_B){
		reset_counter();
	}
}
void btn_time_2_5sec_callback()
{
	if (Sys_Mode == SYS_MODE_A){
		system_shutdown();
	}
}
void btn_time_3sec_callback()
{
	if (Sys_Mode == SYS_MODE_B){
		system_shutdown();
	}
}
void BTN_process()
{
	static UINT32 btn_time = 0;
	static _btn_state btn_state= BTN_IDLE;
	if(BUTTON_PRESSED) {
		switch(btn_state) {
			case BTN_IDLE:
					btn_state = BTN_DEBOUND;
					btn_time = HAL_GetTick() +25;
				break;
			case BTN_DEBOUND:
				if(HAL_GetTick() >btn_time){
						btn_state = BTN_PRESSED2S;
						btn_time = HAL_GetTick() +2000 ;
				}
				break;
			case BTN_PRESSED2S:
				if(HAL_GetTick() >btn_time){
						btn_time_2sec_callback();
						btn_state = BTN_PRESSED2_5S;
						btn_time = HAL_GetTick() +500 ;
				}
				break;
			case BTN_PRESSED2_5S:
				if(HAL_GetTick() >btn_time){
						btn_time_2_5sec_callback();
						btn_state = BTN_PRESSED3S;
						btn_time = HAL_GetTick() +500 ;
				}
				break;
			case BTN_PRESSED3S:
				if(HAL_GetTick() >btn_time){
					btn_time_3sec_callback();
				}
				break;
			case BTN_RELEASE:
				{
					
				}
				break;
			default:
				break;
		}
	} else {
		switch(btn_state)
		{
			case BTN_PRESSED2S:
				if (Sys_Mode == SYS_MODE_B){
					obj_count ++;
				}
				break;
		}
		btn_state = BTN_IDLE;
	}
}

void PinInterrupt_ISR (void) interrupt 7 
{
	_push_(SFRS);
	
		SFRS = 0;
		switch(PIF)
		{
		case (SET_BIT7):{
					PIF&=CLR_BIT7; 
					break;
		} 
		default: break;
		}

	_pop_(SFRS);
}

void check_non_obj_detect_timout(void)
{
	static UINT16 old_obj_count = 0xFFFF; // dif obj_count
	static xdata UINT32 timout =0;
	if(old_obj_count != obj_count)
	{
		timout = HAL_GetTick() + TIMOUT_NON_DETECT_OBJ;
		old_obj_count =obj_count;
	}
	if(HAL_GetTick() > timout)
	{
		system_shutdown();
	}
	
}

_Sys_Mode Check_system_mode()
{
	if(IS_SYS_RUN_MOD_A)
	{
		return SYS_MODE_A;
	}	
	return SYS_MODE_B;
}

void WDT_ISR (void)   interrupt 10
{
_push_(SFRS);

  /* Config Enable WDT reset and not clear couter trig reset */
    WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
    while(!(WDCON&=SET_BIT6));             /* Check for the WDT counter cleared */
    P12 = ~P12;

    CLEAR_WDT_INTERRUPT_FLAG;
_pop_(SFRS);
}


void Disable_WDT_Reset_Config(void)
{
  UINT8 cf0,cf1,cf2,cf3,cf4;
  
    set_CHPCON_IAPEN;
    IAPAL = 0x00;
    IAPAH = 0x00;
    IAPCN = BYTE_READ_CONFIG;
    set_IAPTRG_IAPGO;                                  //Storage CONFIG0 data
    cf0 = IAPFD;
    IAPAL = 0x01;
    set_IAPTRG_IAPGO;                                  //Storage CONFIG1 data
    cf1 = IAPFD;
    IAPAL = 0x02;
    set_IAPTRG_IAPGO;                                  //Storage CONFIG2 data
    cf2 = IAPFD;
    IAPAL = 0x03;
    set_IAPTRG_IAPGO;                                  //Storage CONFIG3 data
    cf3 = IAPFD;
    IAPAL = 0x04;
    set_IAPTRG_IAPGO;                                  //Storage CONFIG4 data
    cf4 = IAPFD;
    cf4 |= 0xF0;                                      //Moidfy Storage CONFIG4 data disable WDT reset
    
    set_IAPUEN_CFUEN;  
    IAPCN = PAGE_ERASE_CONFIG;                        //Erase CONFIG all
    IAPAH = 0x00;
    IAPAL = 0x00;
    IAPFD = 0xFF;
    set_IAPTRG_IAPGO;
    
    IAPCN = BYTE_PROGRAM_CONFIG;                    //Write CONFIG
    IAPFD = cf0;
    set_IAPTRG_IAPGO;
    IAPAL = 0x01;
    IAPFD = cf1;
    set_IAPTRG_IAPGO;
    IAPAL = 0x02;
    IAPFD = cf2;
    set_IAPTRG_IAPGO;
    IAPAL = 0x03;
    IAPFD = cf3;
    set_IAPTRG_IAPGO;
    IAPAL = 0x04;
    IAPFD = cf4;
    set_IAPTRG_IAPGO;

    set_IAPUEN_CFUEN;
    clr_CHPCON_IAPEN;
    if (WDCON&SET_BIT3)
    {
      clr_WDCON_WDTRF;
      set_CHPCON_SWRST;
    }
}


void main(void)
{
	UINT16 id = 0;
	UINT32 ttime = 0;
	ALL_GPIO_INPUT_MODE;
	MODIFY_HIRC(HIRC_16);
	/* Initial I2C function */
	CKDIV = 4;
	Sys_Mode = Check_system_mode();
	
	Init_I2C();
	LCD_INIT();
	Timer3_INT_Initial(DIV2, 0xFC, 0x18);
	if (Sys_Mode == SYS_MODE_A){
	VCNL_initialize();
	} else {
		VCNL36821_Stop();
	}
	GPIO_Init();
	
	
//    WDT_TIMEOUT_800MS;                     /* Setting WDT time out */
////    ENABLE_WDT_INTERRUPT;
////		WDT_RUN_IN_POWERDOWN_DISABLE;
////	  WDT_RUN_IN_POWERDOWN_ENABLE;
////    ENABLE_GLOBAL_INTERRUPT;
//    WDT_COUNTER_RUN;                       /* WDT start to run */
//    WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
	while (1)
	{
//		WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
		BTN_process();
		if (Sys_Mode == SYS_MODE_A){
			Process_VCNL36821S();
		}
		LCD_show(obj_count);
		check_non_obj_detect_timout();
	}
	/* =================== */
}
