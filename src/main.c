/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2020 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include "MS51_16K.H"
#include "htim.h"
#include "usr_i2c.h"
#include "eeprom.h"
#include "vcnl4040.h"

#define CALIBRATION_ENABLE 1
#define SYS_MODE_A 0
#define SYS_MODE_B 1
#define _Sys_Mode UINT8


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
#define IS_SYS_RUN_MOD_A P30
#define TIMOUT_NON_DETECT_OBJ 1800000 // ms

#define DETECT_THRESHOLD_NOT_SET 0

#define TIME_CHECK_OBJECT 300 // ms

#define TIME_COUNT_OFFJECT 1500 // ms
#define OBJECT_INC_TIMES TIME_COUNT_OFFJECT / TIME_CHECK_OBJECT

#define NON_DETECT_COUNT 5
#define TIME_COUNT_NON_OFFJECT 1500 // ms (NON_DETECT_COUNT * TIME_CHECK_OBJECT)
#define TIMEOUT_TO_DECREASE_VALUE (17000 - TIME_COUNT_NON_OFFJECT)

#define SYSTEM_OK 0
#define ERROR_VCNL_NOT_PRESENT 1
#define ERROR_VCNL_READ_FAIL 2

code const char LCD_CODE[] = {0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00};

data UINT16 valueps;
data UINT16 obj_count = 0;
data UINT16 old_obj_count = 0xFFFF; // dif obj_count

data UINT16 btn_count = 0;
data UINT32 btn_time = 0;
data UINT32 last_btn_time = 0;

data UINT8 ss_read_fail = 0;
data UINT8 isCablibmode = 0;
data UINT32 non_dect_timout = 0xFFFFFF;
data UINT8 system_error_code = 0;
data UINT16 DETECT_THRESHOLD = 0;


data UINT32 ttime = 0;
// data UINT16 cnt_ok = 0;
data  UINT16 object_detected = 0;
data  UINT8 non_object_detected = NON_DETECT_COUNT;
data  UINT32 time_new_obj = 0xFFFFFFFF;
data _Sys_Mode Sys_Mode = SYS_MODE_B;
data UINT8 lcd_data[3];



data UINT32 Timm = 0;

void sys_set_err_code(UINT8 err)
{
	system_error_code = 0x80;
	system_error_code += (err & 0x7F);
}

void sys_clr_err_code()
{
	system_error_code = SYSTEM_OK;
}

void LCD_show(UINT16 count);

#define EEPROM_ADDR 0x3882
#define EEPROM_KEY 0x5A
typedef struct
{
	UINT16 threshold;
	UINT8 KEY;
} __struct_data;

data __struct_data struct_data;

void usr_read_eeprom_data()
{
	Read_DATAFLASH_ARRAY(EEPROM_ADDR, (unsigned char *)&struct_data, sizeof(__struct_data));
}

void usr_write_eeprom_data()
{
	Write_DATAFLASH_ARRAY(EEPROM_ADDR, (unsigned char *)&struct_data, sizeof(__struct_data));
}

void usr_reset_eeprom_data()
{
	struct_data.KEY = EEPROM_KEY + 1; // dif key to reset all to 0
	usr_write_eeprom_data();
}

int eeprom_data_init()
{
	usr_read_eeprom_data();
	if (struct_data.KEY != EEPROM_KEY)
	{ // eeprom not init yet
		struct_data.threshold = 0;
		struct_data.KEY = EEPROM_KEY;
		usr_write_eeprom_data();
		return 1;
	}
	return 0;
}

void system_reset()
{
	//	struct_data.cnt_mode_A = obj_count;
	//	struct_data.cnt_mode_B = btn_count;
	//	usr_write_eeprom_data(struct_data);
	set_SWRST;
}

void system_shutdown()
{
	data UINT8 system_shutdown_cnt = 0;
	BOD_DISABLE;
	ALL_GPIO_INPUT_MODE;
	ENABLE_BIT7_FALLINGEDGE_TRIG;
	ENABLE_PIN_INTERRUPT;
	ENABLE_GLOBAL_INTERRUPT;
	while (1)
	{
		HAL_TIM_Pause();
		LCD_PWM_OFF;
		set_PCON_PD;
		HAL_TIM_run();
		HAL_Delay(150);
		if (BUTTON_PRESSED)
		{ // debound buton
			while (BUTTON_PRESSED)
			{
				HAL_Delay(100);
				system_shutdown_cnt++;
			}
			if (system_shutdown_cnt > 15)
			{
				usr_reset_eeprom_data();
			}
			break;
		}
	}
	system_reset();
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

void LCD_send_bytes()
{
	xdata INT8 i, j;
	LCD_LAT_LOW;
	for (i = 2; i >= 0; i--)
	{
		for (j = 7; j >= 0; j--)
		{
			LCD_SCK_LOW;
			LCD_DATA = ((lcd_data[i] >> j) & 0x01);
			LCD_Delay(5);
			LCD_SCK_HIGH;
			LCD_Delay(5);
		}
	}
	LCD_LAT_HIGH;
	LCD_Delay(10);
	LCD_LAT_LOW;
}


void LCD_show(UINT16 count)
{
	data UINT8 i;

	if (system_error_code)
	{
		lcd_data[0] = 0x37; // E
		lcd_data[2] = LCD_CODE[(system_error_code & 0x7F) % 10];
		lcd_data[1] = LCD_CODE[((system_error_code & 0x7F) / 10) % 10];
	}
	else
	{
		if (Sys_Mode == SYS_MODE_B)
		{
			btn_count = (btn_count % 1000);
			lcd_data[2] = LCD_CODE[btn_count % 10];
			lcd_data[1] = LCD_CODE[(btn_count / 10) % 10];
			lcd_data[0] = LCD_CODE[btn_count / 100];
		}
		else if (isCablibmode == 1)
		{
			lcd_data[0] = LCD_CODE[8];
			lcd_data[1] = LCD_CODE[8] + LCD_DOT;
			lcd_data[2] = LCD_CODE[8];
		}
		else if (isCablibmode == 2)
		{
			count = (count % 1000);
			lcd_data[2] = LCD_CODE[count % 10];
			lcd_data[1] = LCD_CODE[(count / 10) % 10];
			lcd_data[0] = LCD_CODE[count / 100];
		}
		else
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
	}
	for (i = 0; i < 2; i++)
	{
		if (lcd_data[i] != LCD_CODE[0])
		{
			break;
		}
		lcd_data[i] = 0x00; // off;
	}

	LCD_send_bytes();
	LCD_Delay(5);
	lcd_data[0] = lcd_data[1] = lcd_data[2] = 0x00;
	LCD_send_bytes();
	LCD_Delay(5);
}

void reset_counter()
{	
	ss_read_fail = 0;
	ttime = HAL_GetTick() + TIME_CHECK_OBJECT;
	object_detected = 0;
	obj_count = 0;
	time_new_obj = 0xFFFFFFFF;
}

void Process_VCNL(void)
{

	if (HAL_GetTick() > time_new_obj)
	{
		if (obj_count)
		{
			obj_count--;
		}
		time_new_obj = 0xFFFFFFFF;
	}

	if (HAL_GetTick() > ttime)
	{
		if (VCNL_getProximity(&valueps))
		{
			// cnt_ok++;
			if (valueps > DETECT_THRESHOLD)
			{
				ss_read_fail = 0;
				object_detected++;
				non_object_detected = NON_DETECT_COUNT;
				if (object_detected == OBJECT_INC_TIMES)
				{
					obj_count++;
					time_new_obj = 0xFFFFFFFF;
				}
			}
			else if (object_detected >= OBJECT_INC_TIMES)
			{
				non_object_detected--;
				object_detected = 0;
				if (!non_object_detected)
				{
					
					non_object_detected = NON_DETECT_COUNT;
					if (obj_count % 2)
					{ // end count is 1.1 2.1 3.1 ...
						time_new_obj = HAL_GetTick() + TIMEOUT_TO_DECREASE_VALUE;
					}
					else
					{
						time_new_obj = 0xFFFFFFFF;
					}
				}
			}
		}
		else
		{
			// ttime = HAL_GetTick() + 10;
			ss_read_fail++;
			if (ss_read_fail == 10)
			{
				// VCNL4040_init();
				sys_set_err_code(ERROR_VCNL_READ_FAIL);
			}
		}
		ttime = HAL_GetTick() + TIME_CHECK_OBJECT;
	}
}

void GPIO_Init()
{
	// P0.7 input
	P0M1 |= 0x80;
	P0M2 &= 0x7F;
}
typedef enum
{
	BTN_IDLE = 0,
	BTN_DEBOUND,
	BTN_CLICKED,
	BTN_PRESSED1_5S,
	BTN_PRESSED2S,
	BTN_PRESSED2_5S,
	BTN_PRESSED3S,
	BTN_RELEASE
} _btn_state;

void btn_time_click_callback()
{
	if (Sys_Mode == SYS_MODE_A)
	{
		//		reset_counter();
	}
}
void btn_time_1_5sec_callback()
{
	if (Sys_Mode == SYS_MODE_A)
	{
		system_shutdown();
	}
}
void btn_time_2sec_callback()
{
	if (Sys_Mode == SYS_MODE_B)
	{
		//		reset_counter();
	}
}
void btn_time_2_5sec_callback()
{
	if (Sys_Mode == SYS_MODE_A)
	{
		system_shutdown();
	}
}
void btn_time_3sec_callback()
{
	if (Sys_Mode == SYS_MODE_B)
	{
		system_shutdown();
	}
}

data _btn_state btn_state = BTN_IDLE;

void BTN_process()
{
	

	
	if (BUTTON_PRESSED)
	{
		switch (btn_state)
		{
		case BTN_IDLE:
			btn_state = BTN_DEBOUND;
			btn_time = HAL_GetTick() + 25;
			break;
		case BTN_DEBOUND:
			if (HAL_GetTick() > btn_time)
			{
				btn_state = BTN_PRESSED1_5S;
				//						btn_time_click_callback();
				btn_time = HAL_GetTick() + 1500;
			}
			break;
		case BTN_PRESSED1_5S:
			if (HAL_GetTick() > btn_time)
			{
				btn_time_1_5sec_callback();
				btn_state = BTN_PRESSED2S;
				btn_time = HAL_GetTick() + 500;
			}
			break;
		case BTN_PRESSED2S:
			if (HAL_GetTick() > btn_time)
			{
				btn_time_2sec_callback();
				btn_state = BTN_PRESSED2_5S;
				btn_time = HAL_GetTick() + 500;
			}
			break;
		case BTN_PRESSED2_5S:
			if (HAL_GetTick() > btn_time)
			{
				btn_time_2_5sec_callback();
				btn_state = BTN_PRESSED3S;
				btn_time = HAL_GetTick() + 500;
			}
			break;
		case BTN_PRESSED3S:
			if (HAL_GetTick() > btn_time)
			{
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
	}
	else
	{
		switch (btn_state)
		{
		case BTN_PRESSED1_5S:
		case BTN_PRESSED2_5S:
		case BTN_PRESSED2S:
			if (Sys_Mode == SYS_MODE_B)
			{
				btn_count++;
			}
			else if (Sys_Mode == SYS_MODE_A)
			{
				//					if(HAL_GetTick() < last_btn_time)
				//					{
				//						isCablibmode = 1- isCablibmode;
				//					}
				//					last_btn_time = HAL_GetTick()+500;
				reset_counter();
			}
			break;
		}
		btn_state = BTN_IDLE;
	}
}

void PinInterrupt_ISR(void) interrupt 7
{
	_push_(SFRS);

	SFRS = 0;
	switch (PIF)
	{
	case (SET_BIT7):
	{
		PIF &= CLR_BIT7;
		break;
	}
	default:
		break;
	}

	_pop_(SFRS);
}

void check_non_obj_detect_timout(void)
{

	if (old_obj_count != (obj_count + btn_count)) // dif total 2 mode
	{
		non_dect_timout = HAL_GetTick() + TIMOUT_NON_DETECT_OBJ;
		old_obj_count = (obj_count + btn_count);
	}
	else
	{ // save data when data changed
	  //		struct_data.cnt_mode_A = obj_count;
	  //		struct_data.cnt_mode_B = btn_count;
	  //		usr_write_eeprom_data(struct_data);
	}
	if (HAL_GetTick() > non_dect_timout)
	{
		system_shutdown();
	}
}

_Sys_Mode Check_system_mode()
{
	if (IS_SYS_RUN_MOD_A)
	{
		return SYS_MODE_A;
	}
	return SYS_MODE_A;
}

// void WDT_ISR (void)   interrupt 10
// {
// _push_(SFRS);

//   /* Config Enable WDT reset and not clear couter trig reset */
//     WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
//     while(!(WDCON&=SET_BIT6));             /* Check for the WDT counter cleared */
//     P12 = ~P12;

//     CLEAR_WDT_INTERRUPT_FLAG;
// _pop_(SFRS);
// }

// void Disable_WDT_Reset_Config(void)
// {
//   UINT8 cf0,cf1,cf2,cf3,cf4;

//     set_CHPCON_IAPEN;
//     IAPAL = 0x00;
//     IAPAH = 0x00;
//     IAPCN = BYTE_READ_CONFIG;
//     set_IAPTRG_IAPGO;                                  //Storage CONFIG0 data
//     cf0 = IAPFD;
//     IAPAL = 0x01;
//     set_IAPTRG_IAPGO;                                  //Storage CONFIG1 data
//     cf1 = IAPFD;
//     IAPAL = 0x02;
//     set_IAPTRG_IAPGO;                                  //Storage CONFIG2 data
//     cf2 = IAPFD;
//     IAPAL = 0x03;
//     set_IAPTRG_IAPGO;                                  //Storage CONFIG3 data
//     cf3 = IAPFD;
//     IAPAL = 0x04;
//     set_IAPTRG_IAPGO;                                  //Storage CONFIG4 data
//     cf4 = IAPFD;
//     cf4 |= 0xF0;                                      //Moidfy Storage CONFIG4 data disable WDT reset

//     set_IAPUEN_CFUEN;
//     IAPCN = PAGE_ERASE_CONFIG;                        //Erase CONFIG all
//     IAPAH = 0x00;
//     IAPAL = 0x00;
//     IAPFD = 0xFF;
//     set_IAPTRG_IAPGO;

//     IAPCN = BYTE_PROGRAM_CONFIG;                    //Write CONFIG
//     IAPFD = cf0;
//     set_IAPTRG_IAPGO;
//     IAPAL = 0x01;
//     IAPFD = cf1;
//     set_IAPTRG_IAPGO;
//     IAPAL = 0x02;
//     IAPFD = cf2;
//     set_IAPTRG_IAPGO;
//     IAPAL = 0x03;
//     IAPFD = cf3;
//     set_IAPTRG_IAPGO;
//     IAPAL = 0x04;
//     IAPFD = cf4;
//     set_IAPTRG_IAPGO;

//     set_IAPUEN_CFUEN;
//     clr_CHPCON_IAPEN;
//     if (WDCON&SET_BIT3)
//     {
//       clr_WDCON_WDTRF;
//       set_CHPCON_SWRST;
//     }
// }

void cabib_process()
{
#if CALIBRATION_ENABLE
	data UINT16 avg;
	data UINT32 Timm_tmp = 0;
	data UINT8 count_val = 0;
	data UINT32 total = 0;
	eeprom_data_init(); // read pre data from eeprom
	if (Sys_Mode == SYS_MODE_A){  //calib mode

		total = 0;
		Timm = HAL_GetTick() + 1000;
		// readWord(VCNL_PS_ID,&valueps);
		if(struct_data.threshold == DETECT_THRESHOLD_NOT_SET){
			isCablibmode = 1;
			while(1)
			{
				if(HAL_GetTick() > Timm)
				{
					if(HAL_GetTick() > Timm_tmp) {
						if(VCNL_getProximity(&valueps))
						{
							sys_clr_err_code();
							ss_read_fail = 0;
							Timm_tmp = HAL_GetTick() + 200;
							total += valueps;
							count_val ++;
							if(valueps > DETECT_THRESHOLD)
							{
								DETECT_THRESHOLD = valueps;
							}
							if(count_val == 15)
								break;
						} else {
							Timm_tmp = HAL_GetTick() + 100;
							ss_read_fail++;
							if(ss_read_fail > 10) {
								sys_set_err_code(ERROR_VCNL_READ_FAIL);
								VCNL4040_init();
								break;
							}
						}
					}
				}
				LCD_show(valueps);
			}
			
			if(!ss_read_fail) {
				avg = (total / count_val);
				struct_data.threshold = avg +1;
				usr_write_eeprom_data();
			}
		} 
		isCablibmode=2;
		Timm_tmp = HAL_GetTick() + 500; 
		while (Timm_tmp > HAL_GetTick())
		{
			LCD_show(struct_data.threshold);
		}
		isCablibmode=0;
	}
#endif
}
void main(void)
{
	// sys_clr_err_code();

	ALL_GPIO_INPUT_MODE;
	MODIFY_HIRC(HIRC_16);
	/* Initial I2C function */
	CKDIV = 4; // 2Mhz 16/(CKDIV*2)
	GPIO_Init();

	non_dect_timout = 0xFFFFFF;
	Sys_Mode = Check_system_mode(); // check running mode by switch

	Init_I2C();

	LCD_INIT();

	Timer3_INT_Initial(DIV2, 0xFC, 0x18); // init timer for system

	if (VCNL4040_init() == 0)
	{ // triger mode ,auto sleep
	  sys_set_err_code(ERROR_VCNL_NOT_PRESENT);
	}

	cabib_process();
	DETECT_THRESHOLD = struct_data.threshold ;
	
	reset_counter();
	while (1)
	{
		//		WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
		// if(HAL_GetTick() > Timm)
		// {
		// 	Timm = HAL_GetTick() + 1000;
		// 	obj_count++;
		// }
		BTN_process();
		// I2C_reset();
		if (Sys_Mode == SYS_MODE_A)
		{
			Process_VCNL();
		}
		if (Sys_Mode != Check_system_mode())
		{
			system_reset();
		}

		LCD_show(obj_count);
		check_non_obj_detect_timout();
	}
	/* =================== */
}
