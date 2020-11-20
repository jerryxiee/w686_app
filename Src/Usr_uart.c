#include "usr_main.h"

unsigned short Uart1Index;			//串口1串口数据buf接收到的位置
unsigned char Uart1RecCnt;			//串口1数据接收到计时，用来判断串口数据已经接收完全
char Uart1Buf[UART1_BUF_LEN]; 

unsigned short Uart2Index;			//串口2串口数据buf接收到的位置
unsigned char Uart2RecCnt;			//串口2数据接收到计时，用来判断串口数据已经接收完全
char Uart2Buf[UART2_BUF_LEN];  

unsigned short Uart3Index;			//串口3串口数据buf接收到的位置
unsigned char Uart3RecCnt;			//串口3数据接收到计时，用来判断串口数据已经接收完全
char Uart3Buf[UART3_BUF_LEN]; 

unsigned short Uart4Index;			//串口4串口数据buf接收到的位置
unsigned char Uart4RecCnt;			//串口4数据接收到计时，用来判断串口数据已经接收完全
char Uart4Buf[UART4_BUF_LEN]; 

u8 At_test_buf[6] = {"AT\r\n"};

//----------------------printf功能实现------------------------------//

#if 0
#pragma import(__use_no_semihosting)                           
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
void _sys_exit(int x) 
{ 
	x = x; 
} 
int fputc(int ch, FILE *f)
{      
	while((USART3->ISR&0X40)==0);
	{
		USART3->TDR = (unsigned char) ch;
	}
			
	return ch;
}
#endif

//---------------------------------------------------------------//

void UART_Send(USART_TypeDef *USARTx, uint8_t *data,uint16_t dataleng)
{
    uint16_t leng_temp = 0;

    while(leng_temp < dataleng)
    {
        LL_USART_TransmitData8(USARTx,data[leng_temp]);
		leng_temp ++;
        while(LL_USART_IsActiveFlag_TC(USARTx) == RESET)
        {
        }
    }
}


void UART_AtInit(void)
{
	Uart1Index = 0;
	Flag.Uart1HaveData = 0;
	memset(Uart1Buf,0,sizeof(Uart1Buf));
}

void UART_CO2Init(void)
{
	Uart2Index = 0;
	Flag.Uart2HaveData = 0;
	memset(Uart2Buf,0,sizeof(Uart2Buf));
}

void Debug_Receive(void)
{
	Test_Receive();
}

void UART_DebugInit(void)
{
	Uart3Index = 0;
	Flag.Uart3HaveData = 0;
	memset(Uart3Buf,0,sizeof(Uart3Buf));	
}
 
void BLE_Receive(void)
{
	
}

void UART_BleInit(void)
{
	Uart4Index = 0;
	Flag.Uart4HaveData = 0;
	memset(Uart4Buf,0,sizeof(Uart4Buf));	
}


void At_Receive(void)
{
	char *ptem = NULL;
	char *p1 = NULL;

	//有时在AT无应答时，发送指令后，模块会返回0x0D这一个无意义的数，需要过滤掉
	if(strlen(Uart1Buf) >= 2)
	{
		AtTimeOutCnt = 0;
	}
	
	printf("\r\n[%d-%d-%d %d:%d:%d]", Rtc.year, Rtc.mon, Rtc.day, Rtc.hour, Rtc.min, Rtc.sec);
	printf("\r\nmsg from AT port (len:%d):\r\n", strlen(Uart1Buf));
	printf("%s", Uart1Buf);



	if (Flag.AtInitCmd)
	{
		if (AT_InitReceive(&AtType, Uart1Buf))
		{
			Flag.WaitAtAck = FALSE; //20150121_2
		}
	}
	else
	{
		if (AT_Receive(&AtType, Uart1Buf))
		{
			Flag.WaitAtAck = FALSE; //20150121_2
		}
	}

	if(strstr(Uart1Buf,"+CPIN: READY")||Flag.SendAtWithoutRDY)
	{
	    Flag.SendAtWithoutRDY=0;
		Flag.WaitAtAck = 0;
		Flag.PwrOnModule = 0;
		Flag.PsSignalOk=0; 
		Flag.ModuleOn=1;		

		if(Flag.HaveSmsReady == 0)
		{
			AtType=AT_ATE;                  //开始初始化模块，直至置位Gsm.SignalOk
			Flag.AtInitCmd=1;
			Flag.HaveSmsReady = 1;
		}
		
	}

	if (strstr(Uart1Buf, "OK") )
	{
		Flag.RcvAtAckOK = 1;
	}

	if ((strstr(Uart1Buf, "UNDER-VOLTAGE WARNNING") )&&(DC_DET == 0))
	{
		Flag.NeedShutDown = 1;
	}

	if (strstr(Uart1Buf, "+SMSUB:")) //有来自平台信息
	{
		WIRELESS_GprsReceive(Uart1Buf, Uart1Index);	
	}

	//有上报NTP时间同步完成时发送查询指令查询网络时间
	if (strstr(Uart1Buf, "+CNTP: 1,"))
	{
		Flag.NtpGetCCLK = 1;
	}

	if ((strstr(Uart1Buf, "\"closed\",1")) != NULL || (strstr(Uart1Buf, "\"pdpdeact\",1")) != NULL)
	{
		if (Flag.GprsConnectOk)
			Flag.NeedCloseGprs = 1;
		Flag.GprsConnectOk = 0;
	}

	//http下载升级文件结果上报
	if (strstr(Uart1Buf, "+HTTPTOFS:"))
	{
		if(strstr(Uart1Buf, "+HTTPTOFS: 200,") != NULL)
		{
			UpgInfo.AppDownloadOk = 1;			//数据接收成功，准备提取数据
		}
		else
		{
			FsUpg.UpgNeedSendGprs = 1;			//数据接收错误，需要返回错误内容
			UpgInfo.UpgrateFail = 1;

			if(strstr(Uart1Buf, "+HTTPTOFS: 400") != NULL)
			{
				strcpy(FsUpg.HttpError,"Bad Request");
			}
			else if(strstr(Uart1Buf, "+HTTPTOFS: 404") != NULL)
			{
				strcpy(FsUpg.HttpError,"Not Found");
			}
			else if(strstr(Uart1Buf, "+HTTPTOFS: 408") != NULL)
			{
				strcpy(FsUpg.HttpError,"Request Time-out");
			}
			else if(strstr(Uart1Buf, "+HTTPTOFS: 500") != NULL)
			{
				strcpy(FsUpg.HttpError,"Internal Server Error");
			}		
			else if(strstr(Uart1Buf, "+HTTPTOFS: 600") != NULL)
			{
				strcpy(FsUpg.HttpError,"Not HTTP PDU");
			}	
			else if(strstr(Uart1Buf, "+HTTPTOFS: 601") != NULL)
			{
				strcpy(FsUpg.HttpError,"Network Error");
				UpgInfo.RetryWaitCnt = 30;
				if(UpgInfo.RetryCnt > 0)
				{
					printf("Network Error,retry after 30s...\r\n");
				}
				
			}		
			else
			{
				strcpy(FsUpg.HttpError,"Other Errors");
			}
		}
	}

	if ((p1 = strstr(Uart1Buf, "APRev:")) != NULL)
	{
		p1 += 6;
		memset(GsmRev,0,sizeof(GsmRev));
		ptem = strstr(p1, "\r\n");

		if(ptem - p1 < sizeof(GsmRev))
		{
			strncpy(GsmRev,p1,ptem - p1);
			Test.GetModuleAti = 1;
		}
	}
	//提起模块的IMEI
	if ((p1 = strstr(Uart1Buf, "IMEI:")) != NULL)
	{
		p1 += 5;
		memset(IMEI,0,sizeof(IMEI));
		ptem = strstr(p1, "\r\n");

		if(ptem - p1 < sizeof(IMEI))
		{
			strncpy(IMEI,p1,ptem - p1);
			Test.GetIMEI = 1;
		}
	}

}


void UART_Handle(void)
{
	static u8 i = 0;

	if (Flag.Uart1HaveData && !Uart1RecCnt)
	{
		At_Receive();
		UART_AtInit();
	}

	if (Flag.Uart2HaveData && !Uart2RecCnt)
	{
		CO2_Data_Receive();
		UART_CO2Init();
	}

	if (Flag.Uart3HaveData && !Uart3RecCnt)
	{
		Debug_Receive();
		UART_DebugInit();
	}

	if (Flag.Uart4HaveData && !Uart4RecCnt)
	{
		BLE_Receive();
		UART_BleInit();
	}

	if(Flag.NeedWakeMdByAt)
	{
		if(Flag.RcvAtAckOK == 0)
		{	
			i++;
			UART_Send(AT_PORT, At_test_buf, strlen((char *)At_test_buf));	
			TIMER_DelayMs(500);
			if(i > 50)
			{
				NeedModuleReset = MODUAL_INFO_ERROR;
				Flag.NeedWakeMdByAt = 0;
			}
		}
		else
		{
			Flag.NeedWakeMdByAt = 0;
			Flag.RcvAtAckOK = 0;
			Flag.ModuleOn = 1;
		}
	}
}



void Usr_USART1_UART_Init(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_DisableFIFO(USART1);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_EnableIT_RXNE_RXFNE(USART1);

    LL_USART_Enable(USART1);

    while((!(LL_USART_IsActiveFlag_TEACK(USART1))) || (!(LL_USART_IsActiveFlag_REACK(USART1))))
    {
    }

}


void Usr_USART2_UART_Init(void)
{

	LL_USART_InitTypeDef USART_InitStruct = {0};

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	/**USART2 GPIO Configuration  
	 PA2   ------> USART2_TX
	PA3   ------> USART2_RX 
	*/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	/* USART2 interrupt Init */
	NVIC_SetPriority(USART2_IRQn, 1);
	NVIC_EnableIRQ(USART2_IRQn);


	/* USER CODE END USART2_Init 1 */
	USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
	USART_InitStruct.BaudRate = 9600;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART2, &USART_InitStruct);
	LL_USART_SetTXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_1_8);
	LL_USART_SetRXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_1_8);
	LL_USART_DisableFIFO(USART2);
	LL_USART_ConfigAsyncMode(USART2);
	LL_USART_EnableIT_RXNE_RXFNE(USART2);

	LL_USART_Enable(USART2);

	/* Polling USART2 initialisation */
	while((!(LL_USART_IsActiveFlag_TEACK(USART2))) || (!(LL_USART_IsActiveFlag_REACK(USART2))))
	{
	}

}


void Usr_USART3_UART_Init(void)
{

	LL_USART_InitTypeDef USART_InitStruct = {0};

	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};


	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOD);
	

	/*  USART3 GPIO Configuration  
	PB8   ------> USART3_TX
	PB9   ------> USART3_RX 
	*/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_8|LL_GPIO_PIN_9; 
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE; 
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
	LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* USART3 interrupt Init */
	NVIC_SetPriority(USART3_4_IRQn, 1);
	NVIC_EnableIRQ(USART3_4_IRQn);

	USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
	USART_InitStruct.BaudRate = 115200;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART3, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USART3);
	LL_USART_EnableIT_RXNE_RXFNE(USART3);


	LL_USART_Enable(USART3);

	/* Polling USART3 initialisation */
	while((!(LL_USART_IsActiveFlag_TEACK(USART3))) || (!(LL_USART_IsActiveFlag_REACK(USART3))))
	{
	}
}


void Usr_USART4_UART_Init(void)
{

	LL_USART_InitTypeDef USART_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART4);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	/**USART4 GPIO Configuration  
	 PA0   ------> USART4_TX
	PA1   ------> USART4_RX 
	*/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_0|LL_GPIO_PIN_1;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART4 interrupt Init */
	NVIC_SetPriority(USART3_4_IRQn, 1);
	NVIC_EnableIRQ(USART3_4_IRQn);

	USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
	USART_InitStruct.BaudRate = 115200;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART4, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USART4);
	LL_USART_EnableIT_RXNE_RXFNE(USART4);

	LL_USART_Enable(USART4);

	/* Polling USART4 initialisation */
	while((!(LL_USART_IsActiveFlag_TEACK(USART4))) || (!(LL_USART_IsActiveFlag_REACK(USART4))))
	{
	}
}


void UART_Init(void)
{
	Usr_USART1_UART_Init();
    Usr_USART2_UART_Init();
    Usr_USART3_UART_Init();
    Usr_USART4_UART_Init();
	
	UART_AtInit();
	UART_CO2Init();
	UART_DebugInit();
	UART_BleInit();
}


int ChangeNum(char *str, int length)
{
	char revstr[16] = {0}; //根据十六进制字符串的长度，这里注意数组不要越界
	int i;
	int num[16] = {0};
	int count = 1;
	int result = 0;
	strcpy(revstr, str);
	for (i = length - 1; i >= 0; i--)
	{
		if ((revstr[i] >= '0') && (revstr[i] <= '9'))
			num[i] = revstr[i] - 48; //字符0的ASCII值为48
		else if ((revstr[i] >= 'a') && (revstr[i] <= 'f'))
			num[i] = revstr[i] - 'a' + 10;
		else if ((revstr[i] >= 'A') && (revstr[i] <= 'F'))
			num[i] = revstr[i] - 'A' + 10;
		else
			num[i] = 0;
		result = result + num[i] * count;
		count = count * 16; //十六进制(如果是八进制就在这里乘以8)
	}
	return result;
}





void USART1_IRQHandler(void)
{
    uint8_t tmp;

    if(LL_USART_IsActiveFlag_IDLE(USART1))
    {
		LL_USART_ClearFlag_IDLE(USART1);
    }

    if(LL_USART_IsActiveFlag_ORE(USART1)) 
    {
      	LL_USART_ClearFlag_ORE(USART1);
    }

    if(LL_USART_IsActiveFlag_RXNE(USART1)) 
    {
		Flag.Uart1HaveData = 1;
		Uart1RecCnt = 2;
		
		tmp = LL_USART_ReceiveData8(USART1); 
		if(Uart1Index < sizeof(Uart1Buf)) 
		{
			Uart1Buf[Uart1Index++]=tmp;   
		}
	}
}


void USART2_IRQHandler(void)
{
    uint8_t tmp;

    if(LL_USART_IsActiveFlag_IDLE(USART2))
    {
      LL_USART_ClearFlag_IDLE(USART2);
    }

    if(LL_USART_IsActiveFlag_ORE(USART2)) 
    {
      LL_USART_ClearFlag_ORE(USART2);
    }

    if(LL_USART_IsActiveFlag_RXNE(USART2)) 
    {
		Uart2RecCnt = 2;
		Flag.Uart2HaveData = 1;

		tmp = LL_USART_ReceiveData8(USART2); 
		if(Uart2Index < sizeof(Uart2Buf)) 
		{
			Uart2Buf[Uart2Index++]=tmp;   
		}
	}
}


void USART3_4_IRQHandler(void)
{
    uint8_t tmp;

    if(LL_USART_IsActiveFlag_IDLE(USART3))
    {
      LL_USART_ClearFlag_IDLE(USART3);
    }

    if(LL_USART_IsActiveFlag_ORE(USART3)) 
    {
      LL_USART_ClearFlag_ORE(USART3);
    }

    if(LL_USART_IsActiveFlag_RXNE(USART3)) 
    {
		Uart3RecCnt = 2;
		Flag.Uart3HaveData = 1;

		tmp = LL_USART_ReceiveData8(USART3); 
		if(Uart3Index < sizeof(Uart3Buf)) 
		{
			Uart3Buf[Uart3Index++]=tmp;   
		}
	}


    if(LL_USART_IsActiveFlag_IDLE(USART4))
    {
      LL_USART_ClearFlag_IDLE(USART4);
    }

    if(LL_USART_IsActiveFlag_ORE(USART4)) 
    {
      LL_USART_ClearFlag_ORE(USART4);
    }	
    
	if(LL_USART_IsActiveFlag_RXNE(USART4)) //检测是否接收中断
    {
		Uart4RecCnt = 2;
		Flag.Uart4HaveData = 1;

		tmp = LL_USART_ReceiveData8(USART4); 
		if(Uart4Index < sizeof(Uart4Buf)) 
		{
			Uart4Buf[Uart4Index++]=tmp;   
		}
	}
}











