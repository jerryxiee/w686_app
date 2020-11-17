
#ifndef __USR_GPIO_H
#define __USR_GPIO_H

#include "usr_main.h"

#define GSM_KEY_ON		        LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_6)
#define GSM_KEY_OFF		        LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_6)

#define POWER_ON		        LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_7)
#define POWER_OFF		        LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_7)
	
#define RED_ON    		        LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_12)					
#define RED_OFF   		        LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_12)			
#define RED_NEG   		        LL_GPIO_IsInputPinSet(GPIOA,LL_GPIO_PIN_12)? LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_12):LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_12)

#define GREEN_ON    	        LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_11)					
#define GREEN_OFF   	        LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_11)		
#define GREEN_NEG   	        LL_GPIO_IsInputPinSet(GPIOA,LL_GPIO_PIN_11)? LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_11):LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_11)

#define CM1106LN_READY          LL_GPIO_IsInputPinSet(GPIOA,LL_GPIO_PIN_4)
#define SHT3X_ALERT             LL_GPIO_IsInputPinSet(GPIOA,LL_GPIO_PIN_5)
#define CHRG_STAT               LL_GPIO_IsInputPinSet(GPIOB,LL_GPIO_PIN_14)
#define DC_DET                  LL_GPIO_IsInputPinSet(GPIOB,LL_GPIO_PIN_15)
#define IR                      LL_GPIO_IsInputPinSet(GPIOD,LL_GPIO_PIN_1)

#define CCS811_INT_SET          LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_6)
#define CCS811_INT_RESET        LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_6)	  

#define CCS811_WAKE_SET         LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_7)
#define CCS811_WAKE_RESET       LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_7)

#define CCS811_RESET_SET        LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_4)
#define CCS811_RESET_RESET      LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_4)

#define SHT3X_RESET_SET         LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_5)
#define SHT3X_RESET_RESET       LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_5)

#define CO2_POWER_EN_SET        LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_4)
#define CO2_POWER_EN_RESET      LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_4)

#define SHT31_POWER_EN_SET      LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_1)
#define SHT31_POWER_EN_RESET    LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_1)

#define CM1106LN_ENABLE         LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_5)
#define CM1106LN_DISABLE        LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_5)

#define BUZZER_SET              LL_GPIO_SetOutputPin(GPIOB,LL_GPIO_PIN_1)
#define BUZZER_RESET            LL_GPIO_ResetOutputPin(GPIOB,LL_GPIO_PIN_1)

#define BT_POWER_EN_SET         LL_GPIO_SetOutputPin(GPIOB,LL_GPIO_PIN_13)
#define BT_POWER_EN_RESET       LL_GPIO_ResetOutputPin(GPIOB,LL_GPIO_PIN_13)

#define GPS_ANTE_POWER_SET      LL_GPIO_SetOutputPin(GPIOA,LL_GPIO_PIN_8)
#define GPS_ANTE_POWER_RESET    LL_GPIO_ResetOutputPin(GPIOA,LL_GPIO_PIN_8)

#define NRF52_WAKEUP_SET        LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_10)
#define NRF52_WAKEUP_RESET      LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_10)

#define MODULE_WAKEUP_SET        LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_2)
#define MODULE_WAKEUP_RESET      LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_2)


void GPIO_init(void);
void GPIO_ledHandler(void);
void GPIO_Hand(void);
void GPIO_ExintSIMInit(void);
void GPIO_StopExintInit(void);
void MCU_WakeUp_From_StopMode(void);
void MCU_Goto_StopMode(void);
void GPIO_ExintPwrInit(void);
void Shock_DeInit(void);
void Sys_Setting_Before_StopMode(void);
void Exit_GPIO_Interrupt_Init(void);
void StopMode_TurnOn_Some_GPIOs(void); 
void GPIO_Init_Before_Shutdown(void);
extern unsigned char KeyDefaultCnt;
extern unsigned char KeyPwrActCnt;
extern unsigned char ledCnt;
#endif
