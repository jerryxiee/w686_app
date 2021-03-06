#ifndef __USR_IIC_H
#define __USR_IIC_H

#include "usr_main.h"

#define 	DELAYTIME			20
#define 	SDA_HIGH 			LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_10)
#define 	SDA_LOW 			LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_10)

#define 	SCL_HIGH 			LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_9)
#define 	SCL_LOW 			LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_9)

#define 	SET_SDA_IN			LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_INPUT)
#define 	SET_SDA_OUT			LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT)

#define 	SDA_ST				LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_10)

void IIC_Init(void);
void CCS811_Test(void);
void G_ensor_Test(void);
void SHT31_Test(void);

uint8_t IIC_Read(uint8_t IICADDRESS, uint8_t *buff, uint8_t len);
uint8_t IIC_Write(uint8_t IICADDRESS,uint8_t *buff, uint8_t len);
uint8_t I2C_Master_Read(u8 deviceId,u8 *register_buf,u8 register_len,u8 *read_buf,u8 read_len);
uint8_t I2C_Master_Write(u8 deviceId,u8 *register_buf,u8 register_len,u8 *write_buf,u8 write_len);

#endif


