#ifndef __USR_SENSOR_H
#define __USR_SENSOR_H

#include "usr_main.h"

#define 	CCS811_ADDRESS			(0x5A<<1)
#define 	SHT31_ADDRESS			(0x44<<1)
#define 	GSENSOR_ADDRESS			(0x26<<1)
#define 	CCS811_ID_ADDRESS		0x20

#define     CCS811_INIT_FAILED      0x0001

#define     CO2_ALARM_THRESHOLD     1500

extern uint16_t     co2_module_value;           //二氧化碳模块传感器读取的二氧化碳值
extern float        humidity_value;             //SHT31湿度值，单位%
extern float        temperature_value;          //SHT31温度值，单位摄氏度
extern uint16_t     ccs811_co2_value;           //CCS811读取的二氧化碳值
extern uint16_t     ccs811_tvoc_value;          //CCS811读取的TVOC值
extern uint16_t     ir_value;                   //红外检测到有人经过次数
extern uint8_t      try_get_co2_cnt;            //成功读取二氧化碳数据尝试次数
extern uint8_t      sensor_type;
extern uint8_t      sensor_check_step;
void Read_CO2_Value(void);
void Sensor_Handle(void);
void CO2_Data_Receive(void);
void Sensor_Init(void);
void Get_CO2_Sensor_Type(void);

uint8_t CCS811_Init(void);

#endif
