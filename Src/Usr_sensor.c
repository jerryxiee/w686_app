#include "usr_sensor.h"

static uint8_t  check_co2_sf[4] = {0x11,0x01,0x01,0xED};	                            //四方光电查询指令
static uint8_t  check_co2_ws[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};	//纬胜查询指令
static uint8_t  check_co2_rd[8] = {0xFE,0x04,0x00,0x03,0x00,0x01,0xD5,0xC5};		    //瑞典s8查询指令
static uint8_t  check_sf_version[4] = {0x11,0x01,0x1E,0xD0};	                        //四方光电查询固件版本

uint8_t  sensor_type;	                    //传感器类型，程序会依次发送三款传感器查询指令，通过判断回复来确认传感器类型：1，四方光电；2，纬胜；3，瑞典S8
uint8_t  sensor_check_step;		            //轮询传感器步数
uint16_t        co2_module_value;           //二氧化碳模块传感器读取的二氧化碳值
float           humidity_value;             //SHT31湿度值，单位%
float           temperature_value;          //SHT31温度值，单位摄氏度
uint16_t        ccs811_co2_value;           //CCS811读取的二氧化碳值
uint16_t        ccs811_tvoc_value;          //CCS811读取的TVOC值
uint16_t        ir_value;                   //红外检测到有人经过次数
uint16_t        sensor_error;               //传感器出错标志
uint8_t         try_get_co2_cnt;            //发送查询传感器指令后等待接收数据计时

uint8_t CCS811_Init(void);

void Sensor_Init(void)
{
    sensor_check_step = 4;
	sensor_type = 0xFE;		//开始查询模块标志
        
    CO2_POWER_EN_SET;           //开启二氧化碳传感器供电
    SHT31_POWER_EN_RESET;       //开启SHT31和CCS811传感器供电
    CCS811_WAKE_RESET;

    LL_mDelay(100);
    if(CCS811_Init())
    {
        sensor_error |= CCS811_INIT_FAILED;
 //       LED_NET_RED_ON;
    }

}





/*----------------------------------------------二氧化碳串口传感器部分代码---------------------------------------------*/

void get_co2_uart_sensor(uint8_t *check_data,uint16_t len)
{
    UART_Send(USART2,check_data,len);
}


//识别当前接入的二氧化碳传感器类型
void Get_CO2_Sensor_Type(void)
{
    if(sensor_check_step > 0)
    {
        sensor_check_step --;
    }
    
	if(sensor_type == 0xFE)
	{
		if(sensor_check_step == 3)
		{
            CM1106LN_ENABLE;
            Uart2Index = 0;    //CM1106SL-N在使能时，串口接收会在使能瞬间接收到0x00的一个干扰，这里需要手动清除一下
            LL_mDelay(100);

			get_co2_uart_sensor(check_co2_sf,sizeof(check_co2_sf));
		}
		else if(sensor_check_step == 2)
		{
			get_co2_uart_sensor(check_co2_ws,sizeof(check_co2_ws));
		}
		else if(sensor_check_step == 1)
		{
			get_co2_uart_sensor(check_co2_rd,sizeof(check_co2_rd));
		}
		else
		{
			sensor_type = 0;
            Flag.Co2SensorError = 1;
			printf("Can't find CO2 sensor!\n");
		}
	}

	//如果是四方光电的sensor，继续验证是不是CM1106SL-N
	if(sensor_type == 1)
	{
		get_co2_uart_sensor(check_sf_version,sizeof(check_sf_version));
	}

    LL_mDelay(300);
}


//解析接收到的CO2数据
void CO2_Data_Receive(void)
{
//    uint16_t i = 0;

    //过滤CM1106SL-N在使能串口干扰
    if(Uart2Buf[0] == 0)
    {
        return;
    }
    Flag.HaveSendCheckCO2Value = 0;
    try_get_co2_cnt = 0;

    //如果是在查询传感器模块型号时收到数据，那么则认定当前传感器类型
    if(sensor_type == 0xFE) 
    {
        if((sensor_check_step == 3) && (Uart2Buf[2] == 0x01))
        {
            sensor_type = 1;      //在查询到是四方观光电传感器后，还需要继续查询是不是低功耗版本，这里不将Flag.NeedCheckCO2Sensor = 0;
        }
        else if(sensor_check_step == 2)
        {
            sensor_type = 2;
            Flag.NeedCheckCO2Sensor = 0;
            printf("\nThe CO2 sensor is ws MH-Z19B\n");
        }
        else if(sensor_check_step == 1)
        {
            sensor_type = 3;
            Flag.NeedCheckCO2Sensor = 0;
            printf("\nThe CO2 sensor is RD s8\n");
        }
        else
        {
            Flag.NeedCheckCO2Sensor = 0;
            printf("\nThe CO2 sensor can not read\n");
        }
    }
#if 0
    printf("\nuart receive data:\n");
    for(i = 0; i < Uart2Index; i ++)
    {
        printf("%02x ",Uart2Buf[i]);
    }
#endif
    if((Uart2Buf[2] == 0x01)&&(sensor_type == 4))
    {
        co2_module_value = (uint16_t)Uart2Buf[3]*256 + (uint16_t)Uart2Buf[4];
    }
    else if((Uart2Buf[2] == 0x01)&&(sensor_type == 1))
    {
        co2_module_value = (uint16_t)Uart2Buf[3]*256 + (uint16_t)Uart2Buf[4];
    }
    else if(sensor_type == 1)
    {
        if(NULL != strstr(Uart2Buf,"SL-N"))
        {
            sensor_type = 4;
            Flag.NeedCheckCO2Sensor = 0;
            printf("\nThe CO2 sensor is sfgd CM1106SL-N\n");
        }
        else
        {
            Flag.NeedCheckCO2Sensor = 0;
            printf("\nThe CO2 sensor is sfgd CM1106S\n");
        }
        
    }
    else if((Uart2Buf[1] == 0x86)&&(sensor_type == 2))
    {
        co2_module_value = (uint16_t)Uart2Buf[2]*256 + (uint16_t)Uart2Buf[3];
    }
    else if((Uart2Buf[2] == 0x02)&&(sensor_type == 3))
    {
        co2_module_value = (uint16_t)Uart2Buf[3]*256 + (uint16_t)Uart2Buf[4];
    }
#if 0
    if(co2_module_value > 500)
    {
        co2_module_value -= 100;        //应客户要求，这里需要减掉100
    } 
    else
    {
        co2_module_value = 400;        
    }
#endif

    if(co2_module_value > 5000)
    {
        co2_module_value = 5000;       
    } 
    else if(co2_module_value < 400)
    {
        co2_module_value = 400;        
    }
    else if(co2_module_value == 0)      //读取不到传感器数据
    {
        Flag.Co2SensorError = 1;
    }
    else                                //传感器数据正常
    {
        Flag.Co2SensorError = 0;
    }
    Test.GetCo2Date = 1;
    printf("\nThe CO2 value is %d ppm\n",co2_module_value);

}

//读取二氧化碳浓度
void Read_CO2_Value(void)
{
    uint8_t wait_ready = 0;

    Flag.HaveSendCheckCO2Value = 1;
    try_get_co2_cnt ++;

    if(sensor_type == 1)
    {
        get_co2_uart_sensor(check_co2_sf, sizeof(check_co2_sf));
    }
    else if(sensor_type == 2)
    {
        get_co2_uart_sensor(check_co2_ws, sizeof(check_co2_ws));
    }
    else if(sensor_type == 3)
    {
        get_co2_uart_sensor(check_co2_rd, sizeof(check_co2_rd));
    }
    else if(sensor_type == 4)
    {
        CM1106LN_ENABLE;
        Uart2Index = 0;    //CM1106SL-N在使能时，串口接收会在使能瞬间接收到0x00的一个干扰，这里需要手动清除一下
        LL_mDelay(300);
        while(CM1106LN_READY)
        {
            wait_ready ++;
            if(wait_ready > 150)
            {
                wait_ready = 0;
                break;
            }
            LL_mDelay(2);
        }
        get_co2_uart_sensor(check_co2_sf, sizeof(check_co2_sf));
        LL_mDelay(100);
        
        CM1106LN_DISABLE;
    }
    else
    {
        
    } 
}


/*-------------------------------------------------------------------------------------------------------------------*/




/*--------------------------------------------CCS811二氧化碳传感部分代码-----------------------------------------------*/
uint8_t CCS811_Init(void)
{
    uint8_t ReadCCS811Id[1] = {0};
    uint8_t CCS811dAddr[1] = {CCS811_ID_ADDRESS};
    uint8_t CCS811dAddr_Mode[1] = {0x01};
    uint8_t CCS81write_Mode[1] = {0x10};

    if(I2C_Master_Write(CCS811_ADDRESS,CCS811dAddr_Mode,1,CCS81write_Mode, 1) == 0)
    {
        printf("CCS811 mode set failed\r\n");
    }

    if(I2C_Master_Read(CCS811_ADDRESS,CCS811dAddr,(u8)sizeof(CCS811dAddr),ReadCCS811Id,(u8)sizeof(ReadCCS811Id)))
    {
        if(ReadCCS811Id[0] == 0x81)
        {
            printf("CCS811 init ok\r\n");
            return 1;
        }
        else
        {
            printf("CCS811 init failed\r\n");
            return 0;          
        }
        
    }
    else
    {
        printf("CCS811 init failed\r\n");
        return 0;      
    }
}

void Read_CCS811_Data(void)
{
    uint8_t write_buf[1] = {0x02};
    uint8_t read_buf[8] = {0};


    if(I2C_Master_Read(CCS811_ADDRESS, write_buf,(u8)sizeof(write_buf),read_buf,(u8)sizeof(read_buf)))
    {
        ccs811_co2_value = (u16)read_buf[0]*256+read_buf[1];
        ccs811_tvoc_value= (u16)read_buf[2]*256+read_buf[3];

        if(ccs811_tvoc_value > 5000)
        {
            ccs811_tvoc_value = 0;
            ccs811_co2_value = 0;
            printf("CCS811 data read failed\r\n");
        }

        printf("eco2=%d  tvoc=%d\r\n",ccs811_co2_value,ccs811_tvoc_value);
    }
    else
    {
        printf("CCS811 data read failed\r\n");
    }

}

/*-------------------------------------------------------------------------------------------------------------------*/



/*----------------------------------------------SHT3X温湿度传感部分代码-----------------------------------------------*/
u8 SHT31_Init(void)
{
    return 0;
}

void Read_SHT31_Data(void)
{
    uint8_t write_buf[2] = {0x2C,0x06};
    uint8_t read_buf[8] = {0};
    uint16_t t_sample = 0;
    uint16_t rh_sample = 0;
    float temperature_value_temp = 0;          
    float humidity_value_temp = 0;             
    static uint8_t data_error_cnt = 0;  //检测数据出连续错次数
    char debug[50] = {0};           //在使用RTT打印浮点数输出时会有bug，需要转换一下在打印

    I2C_Master_Read(SHT31_ADDRESS, write_buf,sizeof(write_buf),read_buf,sizeof(read_buf));

    t_sample = (read_buf[0] << 8) | read_buf[1];
    rh_sample = (read_buf[3] << 8) | read_buf[4];

#if SENSOR_3
    // T = -45 + 175 * tem / (2^16-1)
    if(sensor_type == 2)
    {
        temperature_value_temp= (175.0*(float)t_sample/65535.0-45.0) - 5 ;  //如果是伟盛的传感器，这里修正5度
    }	                  
	else
    {
        temperature_value_temp= (175.0*(float)t_sample/65535.0-45.0) - 2 ;  //结果偏大，这里做一下修正 
    }
#else
    temperature_value_temp= (175.0*(float)t_sample/65535.0-45.0) - 2 ;  //结果偏大，这里做一下修正
#endif  
    humidity_value_temp= (100.0*(float)rh_sample/65535.0);                             // RH = hum*100 / (2^16-1)
	
	if((temperature_value_temp>=-20)&&(temperature_value_temp<=125)&&(humidity_value_temp>0)&&(humidity_value_temp<100))  //过滤错误数据
	{
        Flag.SHT3xSensorError = 0;

        if((temperature_value_temp >= 0) && (temperature_value_temp <= 50))
        {
            temperature_value = temperature_value_temp;
        }
        else if(temperature_value_temp < 0)
        {
            temperature_value = 0;
        }
        else if(temperature_value_temp > 50)
        {
            temperature_value = 50;
        }

        if((humidity_value_temp >= 0) && (humidity_value_temp <= 95))
        {
            humidity_value = humidity_value_temp;
        }
        else if(humidity_value_temp < 0)
        {
            humidity_value = 0;
        }
        else if(humidity_value_temp > 95)
        {
            humidity_value = 95;
        }

        data_error_cnt = 0;
        Test.GetSht31Data = 1;
        
        sprintf(debug,"Temperature: %.1f*C, Humidity: %.1f%%\r\n",temperature_value,humidity_value);
        printf(debug);
    }
    else
    {
        
        data_error_cnt ++;
        printf("SHT31 sensor data error, no use in this time!\r\n");
        if(data_error_cnt >= 10)
        {
            Flag.SHT3xSensorError = 1;
            printf("SHT31 sensor data error over 10 times, need reset sensor!\r\n");
            SHT31_POWER_EN_SET;         //关闭温湿度传感器电源
            LL_mDelay(1000);
            SHT31_POWER_EN_RESET;       //开启温湿度传感器电源
            data_error_cnt = 0;
        }
    }
    
}

/*-------------------------------------------------------------------------------------------------------------------*/





//传感器主体函数
void Sensor_Handle(void)
{
    if(Flag.IsUpgrate)
    {
        return;
    }

    if(Flag.NeedCheckCCS811Value)
    {
        Flag.NeedCheckCCS811Value = 0;
        Read_CCS811_Data();
        return;
    }

    if(Flag.NeedCheckSHT3XSensor)
    {
        Flag.NeedCheckSHT3XSensor = 0;
        Read_SHT31_Data();
        return;
    }

    if(Flag.NeedCheckCO2Sensor)
    {
        Get_CO2_Sensor_Type();
        return;
    }

    if(Flag.NeedCheckCO2Value)
    {
        Flag.NeedCheckCO2Value = 0;
        Read_CO2_Value();
        return;
    }
}

