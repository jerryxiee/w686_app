#include "usr_main.h"
#include "stm32g0xx_ll_flash.h"
//使用stm32的flash模拟eeprom
FS Fs;

//faddr:读地址(此地址必须为2的倍数!!)
//返回值:对应数据.
u32 STMFLASH_ReadWord(u32 faddr)
{
	return *(vu32 *)faddr;
}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针

void STMFLASH_Read(u32 ReadAddr, u32 *pBuffer, u16 NumToRead)
{
	#if 1
	u16 i;
	for (i = 0; i < NumToRead; i++)
	{
		pBuffer[i] = STMFLASH_ReadWord(ReadAddr); 		//读取2个字节.
		ReadAddr += 4;								  	//偏移2个字节.
	}
	#endif
}

//不检查的写入
//WriteAddr:起始地址
//pBuffer:数据指针
//NumToWrite:字(32位)数
void STMFLASH_Write_NoCheck(u32 WriteAddr, u64 *pBuffer, u16 NumToWrite)
{
	#if 1
	u16 i;
	LL_FLASH_EnableProgram();						//使能写入（允许写入）
	while (LL_FLASH_IsActiveFlag_BSY()) {}
	for (i = 0; i < NumToWrite; i++)
	{
		LL_FLASH_Program(ProgaraType_DATA64, WriteAddr, pBuffer[i]);
		WriteAddr += 8; 							//地址增加4.
	}
	while (LL_FLASH_IsActiveFlag_BSY()) {}
	LL_FLASH_DisenableProgram();					//禁止写入
	#endif
}


#define STM_SECTOR_SIZE 2048						//STM32G070一个扇区2K大小

//此函数专用于写Fs数据(地址正是flash的最后一个扇区)
void STMFLASH_WriteFs(u32 WriteAddr, u64 *pBuffer, u16 NumToWrite)
{
	#if 1
	u32 secpos;  							//扇区地址
	u32 offaddr; 							//去掉0X08000000后的地址
	if (WriteAddr < STM32_FLASH_BASE || (WriteAddr >= (STM32_FLASH_BASE + 1024 * STM32_FLASH_SIZE)))
	{
		return; 							//非法地址
	}
	LL_Flash_Unlock();						//解锁
	offaddr = WriteAddr - STM32_FLASH_BASE; //实际偏移地址.
	secpos = offaddr / STM_SECTOR_SIZE;		//所在扇区地址  0~63 for STM32G070RB

	LL_Flash_PageErase(secpos); 			//擦除这个扇区
	STMFLASH_Write_NoCheck(WriteAddr, pBuffer, NumToWrite);		  //写已经擦除了的,直接写入扇区剩余区间.

	LL_Flash_Lock(); 							//上锁
	#endif
}

//只是初始化变量
static void FS_FactroyValue(void)
{
	strcpy(Fs.Ok, "OK");

	memset(Fs.IpAdress, '\0', 50);
	memset(Fs.IpPort, '\0', 8);
	memset(Fs.MccMnc, '\0', 7);
	memset(Fs.ApnName, '\0', 32);
	memset(Fs.GprsUserName, '\0', 32);
	memset(Fs.GprsPassWord, '\0', 16);

	strcpy(Fs.IpPort, "8883"); 
	strcpy(Fs.IpAdress, "device2.iotpf.mb.softbank.jp");
#if 1
	strcpy(Fs.ApnName, "mtc.gen");  
	strcpy(Fs.GprsUserName, "mtc"); 
	strcpy(Fs.GprsPassWord, "mtc"); 
#else
	strcpy(Fs.ApnName, "sb.mvno");  
//	strcpy(Fs.ApnName, "dm.jplat.net"); 
	strcpy(Fs.GprsUserName, "his@his"); 
	strcpy(Fs.GprsPassWord, "his"); 
#endif

	Fs.BKSavedCnt = 0;
	Fs.BkSendCnt = 0;
	Fs.BkSendLen = 0;
	Fs.Interval = 300;
	Fs.SensorCkInterval = 1;
	Fs.HaveSetApn = 0;
	Fs.Sensor = 4;
	Fs.FotaSwitch = 0x01;
	Flag.HaveGetMccMnc = 0;
	Fs.HaveSetMode = 1;						//初始化为1，默认不设置网络模式
	Fs.Co2WarnThreshold = 1000;
	Fs.Co2AlarmThreshold = 1500;
}

void FSUPG_FactroyValue(void)
{

	strcpy(FsUpg.Ok, "OK");
	memset(FsUpg.AppFilePath, '\0', 50);
	memset(FsUpg.AppFileName, '\0', 50);
	memset(FsUpg.AppIpAdress, '\0', 50);
	memset(FsUpg.HttpError, '\0', 32);

	FsUpg.UpgEnJamp = 0x00;
	FsUpg.UpgNeedSendGprs = 0x00;
	FsUpg.AppLenBuf = 0;

//	EXFLASH_Write((u8 *)&FsUpg, FLASH_UPG_ADDR, sizeof(FsUpg));
}

//将参数文件初始化
void FS_FactroyValueFile(void)
{
	FS_FactroyValue();
	Flag.NeedUpdateFs = 1;
	FS_UpdateValue();
}

void FS_InitValue(void)
{
	char *p1 = NULL;
	u8   i = 0;

	memset(&Fs, 0, sizeof(Fs));
	STMFLASH_Read(FLASH_SAVE_ADDR, (u32 *)&Fs, (u16)(sizeof(Fs))/4);

	//flash空白 要初始化
//	if ('O' != Fs.Ok[0] || 'K' != Fs.Ok[1])

	if(strcmp(Fs.IpAdress,"device2.iotpf.mb.softbank.jp") != 0)
	{
		strcpy(Fs.UserID, "999999000004"); 				//这个变量在w686中暂时不使用
		printf("\r\nFormat the eeprom\r\n");
		FS_FactroyValue();
		Flag.NeedUpdateFs = 1;
//		return;
	}

	if (FsUpg.UpgEnJamp == 0xaa)
	{
//		Flag.UpgrateAppSuccess = 1;
		FsUpg.UpgNeedSendGprs = 0;
		Flag.NeedUpgradeResultResponse = 1;
	}
	else if (FsUpg.UpgNeedSendGprs)
	{
		FsUpg.UpgNeedSendGprs = 0;
		Flag.NeedUpgradeResultResponse = 1;
	}

	//读取外部flash中的IMEI_ID号，并判断合法性
	memset(IMEI_ID,0,sizeof(IMEI_ID));
	EXFLASH_ReadBuffer((u8 *)IMEI_ID,IMEIADDR,sizeof(IMEI_ID));   
	if(strstr(IMEI_ID, "#"))							//如果读取到结束符
	{
		p1 = strstr(IMEI_ID, "#");

		for(i = 0;i < 15;i++)
		{
			if(('0' <= IMEI_ID[i]) && ('9' >= IMEI_ID[i])) {}
			else break;
		}

		if((p1 - IMEI_ID == 15) && (i == 15))			//判断IMEI是否合法
		{
			*p1 = 0;  									//清除末尾结束符
		}
		else
		{
			memset(IMEI_ID,0,sizeof(IMEI_ID));			//读取的IMEI不合法，清除，使用模组自身的IMEI
		}
	}
	
	if(strlen(IMEI_MANUAL) == 15)		//如果手动设定的IMEI，优先使用手动设置的IMEI
	{
		strcpy(IMEI,IMEI_MANUAL);
	}
	else if(strlen(IMEI_ID) == 15)		//如果外部写入了IMEI，使用外部写入的IMEI
	{
		strcpy(IMEI,IMEI_ID);
	}
	if(strcmp(Fs.DeviceImei,IMEI) != 0)		//如果IMEI和自身保存的有变化，更新
	{
		strncpy(Fs.DeviceImei,IMEI,sizeof(Fs.DeviceImei));
		Flag.NeedUpdateFs = 1;
	}

	printf("\r\n------Device parameters as follows:------\r\n\r\n");
	#if USR_FOR_JP
	printf("Device suitable for Japan\r\n\r\n");
	#else
	printf("Device suitable for China\r\n\r\n");
	#endif
	printf("Device IMEI:             %s\r\n",Fs.DeviceImei);
	printf("Fs.IpAdress:             %s\r\n",Fs.IpAdress);
	printf("Fs.IpPort:               %s\r\n",Fs.IpPort);
	printf("Fs.ApnName: 	           %s\r\n",Fs.ApnName);
	printf("Fs.GprsUserName:         %s\r\n",Fs.GprsUserName);
	printf("Fs.GprsPassWord:         %s\r\n",Fs.GprsPassWord);
	printf("Fs.Co2WarnThreshold:     %d\r\n",Fs.Co2WarnThreshold);
	printf("Fs.Co2AlarmThreshold: 	 %d\r\n",Fs.Co2AlarmThreshold);
	printf("Fs.SensorCkInterval:     %d\r\n",Fs.SensorCkInterval);
	printf("Fs.AutoCalibrateState:   0x%x\r\n",Fs.AutoCalibrateState);
	printf("\r\n-----------------------------------------\r\n");
}

void FS_UpdateValue(void)
{
	u16 Fs_len = 0;

	if (!Flag.NeedUpdateFs)
	{
		return;
	}
	Flag.NeedUpdateFs = 0;

	//由于STM32G070的内部flash只能以8byte写入，这个就导致如果Fs大小不是8byte整数倍时会写入不完全，需要+1把剩下的也加进去
	Fs_len = sizeof(Fs);
	if((Fs_len % 8) != 0)
	{
		STMFLASH_WriteFs(FLASH_SAVE_ADDR, (u64 *)&Fs, Fs_len/8 + 1);
	}
	else
	{
		STMFLASH_WriteFs(FLASH_SAVE_ADDR, (u64 *)&Fs, Fs_len/8);
	}
	printf("Updata the Fs value\r\n");
}
