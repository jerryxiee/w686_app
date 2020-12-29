#ifndef USR_AT_H
#define USR_AT_H

#include "usr_main.h"

#ifndef _AT_TYPE
#define _AT_TYPE
typedef enum{
	AT_NULL=0,
	AT_ATE,AT_CPIN,AT_CGATT,AT_CPSI,AT_CNACT,
	AT_CSSLCFG,AT_SMCONF_URL,AT_SMCONF_CLEANSS,AT_SMCONF_KEEP,AT_SMCONF_CLIENTID,
	AT_SMCONF_PASSWORD,AT_SMCONF_QOS,AT_SMCONF_USERNAME,AT_CSSLCFG_CONVERT,AT_SMSSL,
	AT_SMCONN,AT_CFSWFILE,AT_CFSINIT,AT_CFSTERM,AT_SMPUB,
	AT_SMDISC,AT_CGREG,AT_CSQ,AT_CFUN_0,AT_CFUN_1,
	AT_CGNAPN,AT_CPOWD,AT_CCID,AT_CCLK,AT_CFGRI,
	AT_CBANDCFG,AT_CBC,AT_CNCFG,AT_CMNB_1,AT_SMSUB,
	AT_SMUNSUB,AT_DISCNACT,AT_AT,AT_GPRSEND,AT_FILE_WRITE,
	AT_ATI,AT_CNTP,AT_CNTP_SET,AT_GSN,AT_CNETLIGHT,
	AT_CFUN_4,AT_SLEDS_1,AT_SLEDS_2,AT_SLEDS_3,AT_CNETLIGHT_ON,
	AT_SLEDS,AT_CGDCONT,AT_PSOFTSIM_CK,AT_CIMI,AT_PSOFTSIM,
	AT_COPS_SET,AT_CMNB_3,AT_CSCLK,AT_CNDS,AT_CNDS_CK,
	AT_CMNB_CK,AT_CNVW,AT_PSOFTSIM_OTA,AT_PSOFTSIM_LIST,AT_HTTPTOFS,
	AT_HTTPTOFSRL,AT_CFSRFILE,AT_CMNB_2,AT_CMNB_CHECK


}AT_TYPE;
#endif

#ifndef _AT_ERROR
#define _AT_ERROR
typedef struct{
	unsigned char PsSingalEorCnt;     //ps信号相关出错计数
	unsigned char GprsConnectEorCnt;  //GPRS连接过程中相关出错计数
	unsigned char GprsSendEorCnt;     //GPRS数据发送过程中相关出错计数
	unsigned char FtpConnectEorCnt;   //FTP连接过程中相关出错计数
	unsigned char FtpGetEorCnt;       //FTP读取数据过程中出错计数
	unsigned char NoSingalCnt;        //无信号时检查注册计数
}AT_ERROR;
#endif

//模块重启原因
#define CANT_ATTACH_NET			1		//无法附着到网络
#define CONNECT_SERVICE_FAILED	2		//连接服务器过程中出错
#define NO_SIMCARD				3		//没有SIM卡或者SIM卡检测出错
#define MODUAL_INFO_ERROR		4		//开机后无法应答AT指令，开机不成功
#define MODUAL_NOACK			5		//模块超过三次无应答

#define USR_CBC_CHECK_VOL		1		//使用模块CBC指令换算成电池电压

extern AT_TYPE  AtType;
extern AT_ERROR AtError;

extern unsigned char baseTimeSec;
extern unsigned char baseTimeMin;
extern unsigned char baseTimeHor;

extern unsigned  short GprsDataLen;
extern unsigned char InitCmdTimes;
extern unsigned short GprsRecDataLen;
extern unsigned short BatVoltage;

extern char Mcc[7];
extern char Mnc[7];
extern char Cid[10];
extern char Lac[10];	
extern char GsmRev[50];		
extern char CsqValue[12];	


void AT_SendPacket(AT_TYPE temType,char * pDst);
unsigned char AT_InitReceive(AT_TYPE *temType,char * pSrc);
unsigned char AT_Receive(AT_TYPE *temType, char * pSrc);
void Flag_check(void);


#endif


