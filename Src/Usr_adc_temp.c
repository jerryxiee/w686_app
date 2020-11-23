
#include "main.h"


#define ADC_CHANNEL_CONF_RDY_TIMEOUT_MS  (   1U)
#define ADC_CALIBRATION_TIMEOUT_MS       (   1U)
#define ADC_ENABLE_TIMEOUT_MS            (   1U)
#define ADC_DISABLE_TIMEOUT_MS           (   1U)
#define ADC_STOP_CONVERSION_TIMEOUT_MS   (   1U)
#define ADC_CONVERSION_TIMEOUT_MS        (4000U)


#define ADC_DELAY_CALIB_ENABLE_CPU_CYCLES  (LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES * 32)
#define VDDA_APPLI                       (3300U)
#define ADC_UNITARY_CONVERSION_TIMEOUT_MS (   1U)
#define VAR_CONVERTED_DATA_INIT_VALUE    (__LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_12B) + 1)

__IO uint32_t ubUserButtonPressed = 0U;
__IO uint16_t uhADCxConvertedData = VAR_CONVERTED_DATA_INIT_VALUE; 
__IO uint16_t uhADCxConvertedData_Voltage_mVolt = 0U;
__IO uint8_t ubAdcGrpRegularUnitaryConvStatus = 2U; 


void Configure_ADC(void)
{
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);					//使能GPIO时钟
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_ANALOG);		//设置PB0为模拟输入
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC);					//使能ADC时钟
                                          */
	if (LL_ADC_IsEnabled(ADC1) == 0)
	{
		LL_ADC_SetClock(ADC1, LL_ADC_CLOCK_SYNC_PCLK_DIV4);				//设置ADC时钟分频
		//设置ADC采样通道及采样时间
		LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_1, LL_ADC_SAMPLINGTIME_39CYCLES_5);

	}

	if ((LL_ADC_IsEnabled(ADC1) == 0)               ||
	    (LL_ADC_REG_IsConversionOngoing(ADC1) == 0)   )
	{
		//设置ADC采样触发为软件触发
		LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);

		//设置采样为连续转换模式
		LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_CONTINUOUS);

		/* Set ADC group regular overrun behavior */
		LL_ADC_REG_SetOverrun(ADC1, LL_ADC_REG_OVR_DATA_PRESERVED);

		//清除ADC通道就绪标志位
		LL_ADC_ClearFlag_CCRDY(ADC1);

		/* Set ADC group regular sequencer configuration flexibility */
		LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);
		//等待ADC通道就绪
		while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0)
		{
		}
		//清除ADC通道就绪标志位
		LL_ADC_ClearFlag_CCRDY(ADC1);

		//禁用ADC group regular sequencer
		LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
		//等待ADC通道就绪
		while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0)
		{
		}
		//清除ADC通道就绪标志位
		LL_ADC_ClearFlag_CCRDY(ADC1);

		/* Set ADC group regular sequence: channel on the selected sequence rank. */
		LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_1);

		while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0)
		{

		}
		LL_ADC_ClearFlag_CCRDY(ADC1);
	}

	if ((LL_ADC_IsEnabled(ADC1) == 0)               ||
	    (LL_ADC_REG_IsConversionOngoing(ADC1) == 0)   )
	{
		LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_1, LL_ADC_SAMPLINGTIME_COMMON_1);

	}
}

void Activate_ADC(void)
{
	__IO uint32_t wait_loop_index = 0U;
	__IO uint32_t backup_setting_adc_dma_transfer = 0U;
                      */
	if (LL_ADC_IsEnabled(ADC1) == 0)
	{
		LL_ADC_EnableInternalRegulator(ADC1);

		wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
		while(wait_loop_index != 0)
		{
			wait_loop_index--;
		}

		backup_setting_adc_dma_transfer = LL_ADC_REG_GetDMATransfer(ADC1);
		LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);
		LL_ADC_StartCalibration(ADC1);

		while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
		{
		}

		LL_ADC_REG_SetDMATransfer(ADC1, backup_setting_adc_dma_transfer);
		wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
		while(wait_loop_index != 0)
		{
			wait_loop_index--;
		}

		LL_ADC_Enable(ADC1);

		while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
		{

		}

	}

}

void ConversionStartPoll_ADC_GrpRegular(void)
{
	if ((LL_ADC_IsEnabled(ADC1) == 1)               &&
	    (LL_ADC_IsDisableOngoing(ADC1) == 0)        &&
	    (LL_ADC_REG_IsConversionOngoing(ADC1) == 0)   )
	{
		LL_ADC_REG_StartConversion(ADC1);
	}

	while (LL_ADC_IsActiveFlag_EOC(ADC1) == 0)
	{

	}

	LL_ADC_ClearFlag_EOC(ADC1);

}


void AdcGrpRegularOverrunError_Callback(void)
{
	LL_ADC_DisableIT_OVR(ADC1);
}


