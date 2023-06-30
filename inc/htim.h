
#ifndef __HTIM_H
#define __HTIM_H

#define DIV1      1
#define DIV2      2
#define DIV4      4
#define DIV8      8
#define DIV16     16
#define DIV32     32
#define DIV64     64
#define DIV128    128

void Timer3_INT_Initial(unsigned char u8TMDIV, 
                        unsigned char u8RH3, 
                        unsigned char u8RL3);
UINT32 HAL_GetTick();
void HAL_Delay(UINT16 dl);
void HAL_TIM_Pause();
void HAL_TIM_run();
#endif

