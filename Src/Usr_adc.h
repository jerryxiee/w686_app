#ifndef __USR_ADC_H
#define __USR_ADC_H

void Adc_init(void);
u16 Adc_Value_Get(void);

void Flooding_sensor_init(void);
extern u32	BatVoltage_Adc;
extern u32  Flooding_Adc;	

#endif
