#include "Usr_iic.h"

void delay(uint8_t time)
{
	uint8_t i;
	for(i = 0; i < time; i++)
	{
		__NOP();
	}
}

void i2cGpioInit(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOD);
	
	GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_1);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_4);


	GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_5);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
}

void IIC_Init(void)
{
	i2cGpioInit();

	SDA_HIGH;
	delay(DELAYTIME);
	SCL_HIGH;
	delay(DELAYTIME);
}


void IIC_Start(void)
{
	SDA_HIGH;
	delay(DELAYTIME);
	SCL_HIGH;
	delay(DELAYTIME);
	SDA_LOW;
	delay(DELAYTIME);
}

void IIC_Stop(void)
{
	SDA_LOW;
	delay(DELAYTIME);
	SCL_HIGH;
	delay(DELAYTIME);
	SDA_HIGH;
	delay(DELAYTIME);
}

void IIC_SendByte(uint8_t data)
{
	uint8_t i, dataTemp;
	dataTemp = data;
	for(i = 0; i < 8; i++)
	{
		SCL_LOW;
		delay(DELAYTIME);
		if(dataTemp & 0x80)
		{
			SDA_HIGH;
		}
		else
		{
			SDA_LOW;
		}
		dataTemp <<= 1;
		SCL_HIGH;
		delay(DELAYTIME);
	}
	SCL_LOW;
}

uint8_t IIC_ReadByte(void)
{
	uint8_t i, dataTemp;

	SCL_LOW;
	delay(DELAYTIME);
	SDA_HIGH;
	delay(DELAYTIME);
	SET_SDA_IN;
	for(i = 0; i < 8; i++)
	{
		SCL_HIGH;
		delay(DELAYTIME);
		dataTemp <<= 1;
		if(SDA_ST)
		{
			dataTemp |= 1;
		}
		SCL_LOW;
		delay(DELAYTIME);
	}
	SET_SDA_OUT;
	return dataTemp;
}

uint8_t IIC_Ack(void)
{
	uint8_t i = 0;

	SET_SDA_IN;
	SCL_HIGH;
	while(SDA_ST && i < 50)
	{
		i++;
	}
	SCL_LOW;
	SET_SDA_OUT;
	if(i < 50)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void IIC_TxAck(void)
{
	SDA_LOW;
	delay(DELAYTIME);
	SCL_HIGH;
	delay(DELAYTIME);
	SCL_LOW;
	delay(DELAYTIME);
}

void IIC_TxNoAck(void)
{
	SDA_HIGH;
	delay(DELAYTIME);
	SCL_HIGH;
	delay(DELAYTIME);
	SCL_LOW;
	delay(DELAYTIME);
}

uint8_t IIC_Read(uint8_t IICADDRESS, uint8_t *buff, uint8_t len)
{
	uint8_t i;

	IIC_Start();
	IIC_SendByte(IICADDRESS | 0x01);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return 0;
	}
	for(i = 0; i < len - 1; i++)
	{
		*buff++ = IIC_ReadByte();
		IIC_TxAck();
	}
	*buff++ = IIC_ReadByte();
	IIC_TxNoAck();
	IIC_Stop();
	return 1;
}

uint8_t IIC_Write(uint8_t IICADDRESS,uint8_t *buff, uint8_t len)
{
	uint8_t i;
	IIC_Start();
	IIC_SendByte(IICADDRESS);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return 0;
	}
	for(i = 0; i < len; i++)
	{
		IIC_SendByte(*buff++);
		if(IIC_Ack() == 0)
		{
			IIC_Stop();
			return 0;
		}
	}
	IIC_Stop();
	return 1;
}

uint8_t I2C_Master_Write(u8 deviceId,u8 *register_buf,u8 register_len,u8 *write_buf,u8 write_len)
{
	u8 i = 0;

	IIC_Start();									//起始位

	IIC_SendByte(deviceId);							//发送器件地址，末位为0，写操作
	if(IIC_Ack() == 0)								//等待应答
	{
		IIC_Stop();
		return 0;
	}

	for(i = 0; i < register_len; i++)
	{
		IIC_SendByte(*register_buf++);				//写入要读取的寄存器地址
		if(IIC_Ack() == 0)							//等待应答
		{
			IIC_Stop();
			return 0;
		}
	}

	for(i = 0; i < write_len; i++)
	{
		IIC_SendByte(*write_buf++);
		if(IIC_Ack() == 0)
		{
			IIC_Stop();
			return 0;
		}
	}
	IIC_Stop();
	return 1;
}

//向IIC设备中读取某个寄存器开始指定长度的数据，成功返回1
uint8_t I2C_Master_Read(u8 deviceId,u8 *register_buf,u8 register_len,u8 *read_buf,u8 read_len)
{
	u8 i = 0;

	IIC_Start();									//起始位

	IIC_SendByte(deviceId);							//发送器件地址，末位为0，写操作
	if(IIC_Ack() == 0)								//等待应答
	{
		IIC_Stop();
		return 0;
	}

	for(i = 0; i < register_len; i++)
	{
		IIC_SendByte(*register_buf++);				//写入要读取的寄存器地址
		if(IIC_Ack() == 0)							//等待应答
		{
			IIC_Stop();
			return 0;
		}
	}

	IIC_Start();									//重新发送起始位，转换方向，准备接收数据

	IIC_SendByte(deviceId | 0x01);					//发送器件地址，末位置1，读操作
	if(IIC_Ack() == 0)								//等待应答
	{
		IIC_Stop();
		return 0;
	}

	for(i = 0; i < read_len - 1; i++)
	{
		*read_buf++ = IIC_ReadByte();				//将数据依次读取到read_buf中
		IIC_TxAck();								//发送应答
	}
	*read_buf++ = IIC_ReadByte();					//最后一包数据不需要发送应答，这里单独处理
	IIC_TxNoAck();									//不发送应答
	IIC_Stop();										//停止

	return 1;
}


void SHT31_Test(void)
{		
	uint8_t temp[6] = {0};
    uint16_t t_sample = 0;
    uint16_t rh_sample = 0;

    IIC_Start();

	IIC_SendByte(SHT31_ADDRESS);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
		
	IIC_SendByte(0x2C);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_SendByte(0x06);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_Stop();
	
	IIC_Start();
	
	IIC_SendByte(SHT31_ADDRESS|1);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	temp[0] = IIC_ReadByte();
	IIC_TxAck();
	temp[1] = IIC_ReadByte();
	IIC_TxAck();
	temp[2] = IIC_ReadByte();
	IIC_TxAck();
	temp[3] = IIC_ReadByte();
	IIC_TxAck();
	temp[4] = IIC_ReadByte();
	IIC_TxAck();
	temp[5] = IIC_ReadByte();
	IIC_TxNoAck();
	IIC_Stop();


	t_sample = (temp[0] << 8) | temp[1];
    rh_sample = (temp[3] << 8) | temp[4];

	temperature_value= (175.0*(float)t_sample/65535.0-45.0) ;                         // T = -45 + 175 * tem / (2^16-1)
	humidity_value= (100.0*(float)rh_sample/65535.0);                                 // RH = hum*100 / (2^16-1)
	
	if((temperature_value>=-20)&&(temperature_value<=125)&&(humidity_value>=0)&&(humidity_value<=100))  //过滤错误数据
	{
        printf("Temperature: %6.2f*C, Humidity: %6.2f%%\r\n",temperature_value,humidity_value);
    }

}






void CCS811_Test(void)
{	
	IIC_Start();

	IIC_SendByte(CCS811_ADDRESS);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
		
	IIC_SendByte(0x20);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_Stop();
	
	IIC_Start();
	
	IIC_SendByte(CCS811_ADDRESS|0x01);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_ReadByte();
	
	IIC_TxNoAck();
	IIC_Stop();

}


void G_ensor_Test(void)
{	
	IIC_Start();

	IIC_SendByte(GSENSOR_ADDRESS);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
		
	IIC_SendByte(0x01);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_Stop();
	
	IIC_Start();
	
	IIC_SendByte(GSENSOR_ADDRESS|0x01);
	if(IIC_Ack() == 0)
	{
		IIC_Stop();
		return ;
	}
	IIC_ReadByte();
	
	IIC_TxNoAck();
	IIC_Stop();
}
