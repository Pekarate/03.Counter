/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* SPDX-License-Identifier: Apache-2.0                                                                     */
/* Copyright(c) 2020 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include "MS51_16K.H"
#include "htim.h"
#include "usr_i2c.h"
// #include "eeprom.h"
#include "vcnl4040.h"

//#define CONTROL_COM_ENABLE


#define CALIBRATION_ENABLE 1

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

#define LCD_COM_HIGH P12 = 1
#define LCD_COM_LOW P12 = 0

#define BUTTON_PRESSED !P07
#define IS_SYS_RUN_MOD_A P30
#define NO_OBJECT_SLEEP_TIMEOUT_MS (30UL * 60UL * 1000UL)

#define DETECT_THRESHOLD_NOT_SET 0

#define TIME_CHECK_OBJECT 300 // ms

#define TIME_COUNT_OFFJECT 1500 // ms
#define OBJECT_INC_TIMES TIME_COUNT_OFFJECT / TIME_CHECK_OBJECT

#define NON_DETECT_COUNT 2
#define TIME_COUNT_NON_OFFJECT 1500 // ms (NON_DETECT_COUNT * TIME_CHECK_OBJECT)
#define TIMEOUT_TO_DECREASE_VALUE (20000)

#define SYSTEM_OK 0
#define ERROR_VCNL_NOT_PRESENT 1
#define ERROR_VCNL_READ_FAIL 2

code const char LCD_CODE[] = {0x7E, 0x48, 0x3D, 0x6D, 0x4B, 0x67, 0x77, 0x4C, 0x7F, 0x6F, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00};

xdata UINT16 valueps;
xdata UINT16 obj_count = 0;
// xdata UINT16 old_obj_count = 0xFFFF; // dif obj_count

xdata UINT16 btn_count = 0;
xdata UINT32 btn_time = 0;
xdata UINT32 last_btn_time = 0;

xdata UINT8 ss_read_fail = 0;
xdata UINT8 isCablibmode = 0;
xdata UINT32 non_dect_timout = 0;
xdata UINT8 system_error_code = 0;
xdata UINT16 DETECT_THRESHOLD = 0;

xdata UINT32 ttime = 0;
// data UINT16 cnt_ok = 0;
xdata UINT8 object_detected = 0;
xdata UINT8 non_object_detected = NON_DETECT_COUNT;
xdata UINT32 time_new_obj = 0xFFFFFFFF;
xdata UINT8 lcd_data[3];
bit show_sensor_raw = 0;
uint16_t sensor_raw_data = 0;
bit lcd_off = 0;
xdata UINT32 Timm = 0;

void reset_non_obj_detect_timout();

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

xdata __struct_data struct_data;

void usr_read_eeprom_data()
{
	// Read_DATAFLASH_ARRAY(EEPROM_ADDR, (unsigned char *)&struct_data, sizeof(__struct_data));
	return;
}

void usr_write_eeprom_data()
{
	// Write_DATAFLASH_ARRAY(EEPROM_ADDR, (unsigned char *)&struct_data, sizeof(__struct_data));
	return;
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
	set_SWRST;
}

void system_shutdown()
{
	data UINT8 system_shutdown_cnt = 0;
	lcd_off = 1;
	LCD_show(0);
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
	P1M1 &= ~(1 << 2); // Clear bit 2 to set P1.2 as output mode (push-pull)
	P1M2 |= (1 << 2);  // Set bit 2 to enable push-pull output for P1.2
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

bit isobjectvisible = 0;
bit is_object_visible()
{
	return isobjectvisible;
}
void set_object_visible()
{
	reset_non_obj_detect_timout();
	isobjectvisible = 1;
}
void reset_object_visible()
{
	isobjectvisible = 0;
}

#ifdef CONTROL_COM_ENABLE
void LCD_show(UINT16 count)
{
	data UINT8 i;
	static UINT8 lcd_count = 0;
	lcd_count = 1 - lcd_count;
	if( lcd_off == 0) {
		if (lcd_count) {
			if (system_error_code)
			{
				lcd_data[0] = 0x37; // E
				lcd_data[2] = LCD_CODE[(system_error_code & 0x7F) % 10];
				lcd_data[1] = LCD_CODE[((system_error_code & 0x7F) / 10) % 10];
			}
			else
			{
				if (isCablibmode == 1)
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
					if (show_sensor_raw)
					{
						sensor_raw_data = (sensor_raw_data % 1000);
						lcd_data[0] = LCD_CODE[(sensor_raw_data / 100) % 10];
						lcd_data[1] = LCD_CODE[(sensor_raw_data / 10) % 10];
						lcd_data[2] = LCD_CODE[sensor_raw_data % 10];
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
			}
			for (i = 0; i < 2; i++)
			{
				if (lcd_data[i] != LCD_CODE[0])
				{
					break;
				}
				lcd_data[i] = 0x00; // off;
			}
		}
		else {
					lcd_data[0] ^= 0xFF;
					lcd_data[1] ^= 0xFF;
					lcd_data[2] ^= 0xFF;
			}
	} else {
		lcd_data[0] = lcd_data[1] = lcd_data[2] = 0xFF;
		lcd_count = 0;
	}
	LCD_send_bytes();
	if (lcd_count) {
        LCD_COM_LOW;
    } else {
        LCD_COM_HIGH;
    }
}

#else
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
		if (isCablibmode == 1)
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
			if (show_sensor_raw)
			{
				sensor_raw_data = (sensor_raw_data % 1000);
				lcd_data[0] = LCD_CODE[(sensor_raw_data / 100) % 10];
				lcd_data[1] = LCD_CODE[(sensor_raw_data / 10) % 10];
				lcd_data[2] = LCD_CODE[sensor_raw_data % 10];
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
	}
	for (i = 0; i < 2; i++)
	{
		if (lcd_data[i] != LCD_CODE[0])
		{
			break;
		}
		lcd_data[i] = 0x00; // off;
	}
//	if(sensor_raw_data > DETECT_THRESHOLD)
//	{
//		lcd_data[0] = lcd_data[0] + LCD_DOT;
//	}
	LCD_send_bytes();
	LCD_Delay(5);
	lcd_data[0] = lcd_data[1] = lcd_data[2] = 0x00;
	LCD_send_bytes();
	LCD_Delay(5);
}

#endif

void reset_counter()
{
	ss_read_fail = 0;
	ttime = HAL_GetTick() + TIME_CHECK_OBJECT;
	object_detected = 0;
	obj_count = 0;
	time_new_obj = 0xFFFFFFFF;
}

xdata uint32_t ss_read_count = 0;
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
		ss_read_count++;
		if (VCNL_getProximity(&valueps))
		{
			// cnt_ok++;
			sensor_raw_data = valueps;
			ss_read_fail = 0;
			sys_clr_err_code();
			if (valueps > DETECT_THRESHOLD)
			{
				time_new_obj = 0xFFFFFFFF;
				printf("rcount: %ld sensor_raw_data: %d \r\n",ss_read_count,sensor_raw_data);
				non_object_detected = NON_DETECT_COUNT;
				if (is_object_visible() == 0) // non
				{
					object_detected++;
					if (object_detected == OBJECT_INC_TIMES)
					{
						set_object_visible();
						obj_count++;
					}
					if (object_detected <= OBJECT_INC_TIMES)
					{
						object_detected++;
					}
				}
			}
			else if (is_object_visible() == 1 && (non_object_detected > 0))
			{
				object_detected = 0;
				non_object_detected--;
				if (non_object_detected == 0)
				{
					reset_object_visible();
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
			else
			{
				object_detected = 0;
			}
		}
		else
		{
			printf("ss_read_fail : %ld\r\n",ss_read_count);
			sensor_raw_data = 0;
			ttime = HAL_GetTick() + 100;
			ss_read_fail++;
			if (ss_read_fail == 10)
			{
				VCNL4040_init();
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

void btn_time_1_5sec_callback()
{
	system_shutdown();
}

void btn_time_2_5sec_callback()
{
	system_shutdown();
}
void btn_time_3sec_callback()
{
		system_shutdown();
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

			reset_counter();
			//show_sensor_raw = !show_sensor_raw;
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

void reset_non_obj_detect_timout()
{
	non_dect_timout = HAL_GetTick() + NO_OBJECT_SLEEP_TIMEOUT_MS;
}
void check_non_obj_detect_timout(void)
{

	// if (old_obj_count != (obj_count + btn_count)) // dif total 2 mode
	// {
	// 	non_dect_timout = HAL_GetTick() + NO_OBJECT_SLEEP_TIMEOUT_MS;
	// 	old_obj_count = (obj_count + btn_count);
	// }
	// else
	// { // save data when data changed
	//   //		struct_data.cnt_mode_A = obj_count;
	//   //		struct_data.cnt_mode_B = btn_count;
	//   //		usr_write_eeprom_data(struct_data);
	// }
	if (HAL_GetTick() > non_dect_timout)
	{
		system_shutdown();
	}
}


void cabib_process()
{
#if CALIBRATION_ENABLE
	data UINT16 avg;
	data UINT32 Timm_tmp = 0;
	data UINT8 count_val = 0;
	data UINT32 total = 0;
	eeprom_data_init(); // read pre data from eeprom
	total = 0;
	Timm = HAL_GetTick() + 1000;
	// readWord(VCNL_PS_ID,&valueps);
	if (struct_data.threshold == DETECT_THRESHOLD_NOT_SET)
	{
		isCablibmode = 1;
		while (1)
		{
			if (HAL_GetTick() > Timm)
			{
				if (HAL_GetTick() > Timm_tmp)
				{
					if (VCNL_getProximity(&valueps))
					{
						sensor_raw_data = valueps;
						sys_clr_err_code();
						ss_read_fail = 0;
						Timm_tmp = HAL_GetTick() + 200;
						total += valueps;
						count_val++;
						if (valueps > DETECT_THRESHOLD)
						{
							DETECT_THRESHOLD = valueps;
						}
						if (count_val == 15)
							break;
					}
					else
					{
						Timm_tmp = HAL_GetTick() + 100;
						ss_read_fail++;
						if (ss_read_fail > 10)
						{
							sys_set_err_code(ERROR_VCNL_READ_FAIL);
							VCNL4040_init();
							break;
						}
					}
				}
			}
			LCD_show(valueps);
		}

		if (!ss_read_fail)
		{
			avg = (total / count_val);
			struct_data.threshold = avg + 5;
			// if (struct_data.threshold < 2)
			// {
			// 	struct_data.threshold = 2;
			// }
			usr_write_eeprom_data();
		}
	}
	isCablibmode = 2;
//	Timm_tmp = HAL_GetTick() + 500;
//	while (Timm_tmp > HAL_GetTick())
//	{
//		LCD_show(struct_data.threshold);
//	}
	isCablibmode = 0;
#endif
}
char putchar (char c)  {
  while (!TI);
  TI = 0;
  return (SBUF = c);
}
xdata uint32_t task_count = 0;
void main(void)
{
	// sys_clr_err_code();

	ALL_GPIO_INPUT_MODE;
	MODIFY_HIRC(HIRC_16);
	/* Initial I2C function */
	CKDIV = 1; // 8Mhz 16/(CKDIV*2)
	
	
	GPIO_Init();
	P06_QUASI_MODE;
	UART_Open(8000000,UART0_Timer1,19200);
	ENABLE_UART0_PRINTF;
	Init_I2C();

	LCD_INIT();

	Timer3_INT_Initial(DIV8, 0xB1, 0xE0);  //20ms

	if (VCNL4040_init() == 0)
	{ // triger mode ,auto sleep
		sys_set_err_code(ERROR_VCNL_NOT_PRESENT);
	}

	cabib_process();
	DETECT_THRESHOLD = struct_data.threshold;

	reset_counter();
	reset_non_obj_detect_timout();
	Timm = 0;
	while (1)
	{
		
//		WDT_COUNTER_CLEAR;                     /* Clear WDT counter */
		// if(HAL_GetTick() > task_count)
		// {
		//  	task_count = HAL_GetTick() + 1000;
		// 	task_count+=1;
		// 	printf("task_count: %ld\r\n",task_count);
		// }
		check_non_obj_detect_timout();
		BTN_process();
		Process_VCNL();
		LCD_show(obj_count);
		// printf(">>>>\r\n");
#ifdef CONTROL_COM_ENABLE
		set_PCON_IDLE;
#endif
		// printf("<<<<\r\n");
	}
	/* =================== */
}
