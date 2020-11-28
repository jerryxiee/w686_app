
#include "usr_main.h"

/********************************************************************
 * Extern Variables (Extern /Global)
 ********************************************************************/
FLAG Flag;

unsigned short ResetCnt = 0;
unsigned short WaitAtTime;		//等待AT超时时间，默认是75，可以根据不同AT指令修改
unsigned char WatchDogCnt = 0; 
unsigned char AtTimeOutCnt; 	//AT超时次数，超时三次重启模块
unsigned char NeedModuleReset;
unsigned short NoShockCnt;		//没有振动计时，用于处理地下停车场长期无网络时不循环重启模块问题
unsigned char ModePwrDownCnt;	//执行关机操作后等待模块回应关机消息倒计时
unsigned char CheckModeCnt;		//模块开机后，等待主动上报内容，超过10秒，跳过等待，直接开始发送AT指令
const unsigned char SoftwareBuilt[50] = {0};
char Edition[50] = {0};


char Edition_STD[50] = {"w868_SIM7080G_V0.0.5"};				//程序的稳定版本，手动设置版本型号
char HardWare_Edition[50] = {"TY197_MAIN_V1.0"};		//硬件版本，手动设置版本型号

u8 Built_year[5] = {'\0'};
u8 Built_mon[3] = {'\0'};
u8 Built_day[3] = {'\0'};
u8 Built_hour[3] = {'\0'};
u8 Built_min[3] = {'\0'};

void Usr_InitHardware(void);
void Usr_InitValue(void);
void Flag_Check(void);

void time_convert(void)
{
	Built_year[0] = BUILD_YEAR_CH2;
	Built_year[1] = BUILD_YEAR_CH3;

	Built_mon[0] = BUILD_MONTH_CH0;
	Built_mon[1] = BUILD_MONTH_CH1;

	Built_day[0] = BUILD_DAY_CH0;
	Built_day[1] = BUILD_DAY_CH1;

	Built_hour[0] = BUILD_HOUR_CH0;
	Built_hour[1] = BUILD_HOUR_CH1;

	Built_min[0] = BUILD_MIN_CH0;
	Built_min[1] = BUILD_MIN_CH1;
}

int main(void)
{
	time_convert();
	printf("\r\n============================================");
	printf("\r\n===========%s=============",Edition_STD);
	sprintf(Edition, "w868_SIM7080G_%s%s%s%s%s", Built_year, Built_mon, Built_day, Built_hour, Built_min);
	printf("\r\n==software built time:%s %s==", __DATE__, __TIME__);
	printf("\r\n============================================\r\n");
	Usr_InitHardware();
	Usr_InitValue();
	Test_Init();
	while (1)
	{
		Usr_DeviceContral();
		UART_Handle();
		WIRELESS_Handle();
		GPIO_Hand();
		GPRS_SaveBreakPoint();
		FS_UpdateValue();
		Sensor_Handle();
		Flag_Check();
		Test_Handle();
		WatchDogCnt = 0;
	}
}

void Usr_InitHardware(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    SystemClock_Config();
	//开启停止模式下的debug时钟会使功耗增加28mA左右，同时可以在停止模式下烧录程序。该设置掉电保存
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);		//开启停止模式下debug时钟
	LL_DBGMCU_EnableDBGStopMode();								//使能停止模式时debug功能
	LL_DBGMCU_DisableDBGStopMode();

	GPIO_init();
	UART_Init();
	TIMER_Init();
	IIC_Init();
	EXFLASH_SpiInit();
	Sensor_Init();	
	Adc_init();
}


void Usr_InitValue(void)
{
	GprsSend.posCnt = 1;
	GprsSend.posFlag = 1;

	Flag.NeedLogIn = 1;
	Flag.NeedCheckCO2Sensor = 1;
	Flag.NeedSendToSleep = 1;
	Flag.NeedModuleOn = 1;
	Flag.Bma250NeedInit = 1;
	Flag.NeedcheckLBS = 1;
	Flag.PorOldOn = 1; 		//电池供电没有接外部供电时，不产生断电报警，只有在接过供电再断电时才产生断电报警
	Flag.WakeUpMode = 1;
	Flag.NeedcheckCCID = 1;
	Flag.NtpGetCCLK = 1;
	Flag.NeedSetNtp = 1;
	Flag.NeedGetIMEI = 1;
	#if USE_SOFTSIM
	Flag.NeedChangeSoftSim = 1;
	#endif
	AT_CBC_IntervalTemp = 20;
	ActiveTimer = ACTIVE_TIME;
	
	memset(MccMnc, '\0', 7);

	AtType = AT_NULL;
	Flag.WaitAtAck = 0;
	WatchDogCnt = 0;

	FS_InitValue();						

	memset(&FsUpg,0,sizeof(FsUpg));
  	EXFLASH_ReadBuffer((u8 *)&FsUpg,FLASH_UPG_ADDR,sizeof(FsUpg));
	//远程升级成功,需要发送升级成功数据
	if(FsUpg.UpgEnJamp==0xAA)		
	{
		Flag.NeedSendUpgResult = 1;
		//清除升级结果
		memset(&FsUpg,0,sizeof(FsUpg));
		EXFLASH_EraseSector(FLASH_UPG_ADDR);
		EXFLASH_WriteBuffer((u8 *)&FsUpg,FLASH_UPG_ADDR,sizeof(FsUpg));
		printf("\r\nUpgrade App success!\r\n");
	}

	if(Fs.HaveCertificate == 0xAA)
	{
		Flag.ModuleHasCA = 1;
	}

	if (Fs.Interval == 0 || Fs.Interval == 0xffff)
	{
		Fs.Interval = 120;
		Flag.NeedUpdateFs = 1;
	}
	
	#if NO_SLEEP
	IntervalTemp = 300;
	#else
	IntervalTemp = Fs.Interval;
	#endif
	//读取到的关键参数合法性判断
	if(strlen(Fs.IpAdress) < 5)			
	{
		memset(Fs.IpAdress,0,sizeof(Fs.IpAdress));
		memset(Fs.IpPort,0,sizeof(Fs.IpPort));

		strcpy(Fs.IpAdress,"device2.iotpf.mb.softbank.jp");
		strcpy(Fs.IpPort,"8883");

		Flag.NeedUpdateFs = 1;
	}


	memset(UserIDBuf,0, sizeof(UserIDBuf));
	strncpy(UserIDBuf,Fs.UserID, sizeof(UserIDBuf)); 

	RTC_Wake_Init(300);		//五分钟产生一次闹钟事件
}

void Flag_Check(void)
{
	if(UpgInfo.UpgrateFail)
	{
		Flag.IsUpgrate = 0;
		UpgInfo_InitValue();
		if(UpgInfo.RetryCnt == 0)
		{
			Flag.UpgrateFailed = 1;
			Flag.NeedSendUpgResult = 1;
		}
	}

	if(Flag.NeedGetBatVoltage)
	{
		Flag.NeedGetBatVoltage = 0;
		BatVoltage_Adc = Adc_Value_Get();
		BatVoltage_Adc = BatVoltage_Adc * 34/10;		//转换成电池电压,x3.2
//		printf("The battery voltage is %d mv\r\n",BatVoltage_Adc);
	}

	if((UpgInfo.NeedWaitUpgrade)&&(Rtc.hour == 1))
	{
		UpgInfo.NeedWaitUpgrade = 0;

		UpgInfo.NeedUpdata = 1;				//需要开始升级
		Flag.NeedResponseFrist = 1;			//需要首先应答平台消息后在开始升级
		Flag.NeedSendResponse = 1;
		UpgInfo.RetryCnt = 2;				//升级失败重复次数

		printf("Need upgrade the device,upgrade file name is: %s\r\n",FsUpg.AppFilePath);
		sprintf(RespServiceBuf,"Fota file name is :%s,ready upgrade...",FsUpg.AppFilePath);
	}
	
}



