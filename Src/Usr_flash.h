#ifndef USR_FLASH_H
#define USR_FLASH_H

#include "usr_main.h"

#define STM32_FLASH_SIZE 128 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址

//设置FLASH保存数据地址(必须为偶数)
//FLASH的起始地址为0X8000000，STM32103FRC的flash大小为256k(0x40000),预留保存数据的空间大小为2k(0x800)
//所以地址为0X8000000+0x40000-0x800=0X0803F800
#define FLASH_SAVE_ADDR  0X08020000 - 0x800		//STMG070RB flash为128K，使用末尾2K作为数据存储区


#ifndef _FS
#define _FS
typedef struct{
	char   Ok[3];			   			//为"OK"表示从flash读出的数据有效，不是"OK"表示此flash没有初始化
	char   UserID[16];
	char   DeviceImei[16];
	char   IpPort[8];
	char   AppIpPort[6];       			//远程升级用FTP端口号
	char   IpAdress[50];				//服务器IP或者域名
	char   AppIpAdress[50];   		 	//远程升级用http的IP或地址
	char   ApnName[32];
	char   GprsUserName[32];    		//gprs用户名
	char   GprsPassWord[16];    		//gprs密码
	char   MccMnc[7];          			//最后的MCCMNC

	unsigned char   Sensor;				//灵敏度值
	unsigned char   ModeSet;            //设备模式设置
	unsigned char 	HaveCertificate;	//模块是否有烧录证书，0xAA为已经烧录，其他为未烧录
	unsigned char   HaveSetApn;			//非零表示客户有短信设置apn
	unsigned short  BKSavedCnt;         //已存断点计数，最大为500
	unsigned int    BkSavedLen;         //已存断点字长计数，单位byte
	unsigned short  BkSendCnt;          //已发断点计数
	unsigned int  	BkSendLen;          //已发断点字长计数，单位byte
	unsigned short  Interval;           //定位包上传间隔	
	unsigned char 	FotaSwitch;			//远程升级开关，0xAA为关闭远程升级，其他为开启远程升级
	unsigned char 	HaveSetMode;		//是否设置模块网络模式
	unsigned short 	Co2WarnThreshold; 	//二氧化碳传感器告警值
	unsigned short  Co2AlarmThreshold;	//二氧化碳传感器报警值
	unsigned short  SensorCkInterval;   //传感器检测时间间隔
	unsigned char 	AutoCalibrateState;	//二氧化碳传感器自动校准状态；1表示关闭，其他表示开启
}FS;
#endif


extern FS Fs;


void FS_InitValue(void);
void FS_UpdateValue(void);
void FS_FactroyValueFile(void);
void STMFLASH_WriteFs(u32 WriteAddr,u64 *pBuffer,u16 NumToWrite);
void STMFLASH_Read(u32 ReadAddr, u32 *pBuffer, u16 NumToRead);
void FSUPG_FactroyValue(void);

#endif


