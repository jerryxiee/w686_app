#ifndef __USR_GSM_H
#define __USR_GSM_H

#include "usr_main.h"

#ifndef _GSMON_
#define _GSMON_
typedef struct{
    unsigned char   PowerOffWaitCnt;            //GSM模块关闭供电维持时间
	unsigned char 	PowerOnWaitCnt;		        //GSM模块开启供电维持时间
    unsigned char 	PowerKeyOffWaitCnt;	        //GSM模块powerkey拉高维持时间
	unsigned char 	PowerKeyOnWaitCnt;	        //GSM模块powerkey拉低维持时间
    unsigned char 	WaitSendAtCnt;	            //GSM模块完成开机后，等待一段时间后发送AT指令
    unsigned char 	PowerOnStep;	            //GSM模块完成开机阶段
}GSMON;
#endif

void Usr_DeviceContral(void);
void GsmOn_Timer_Init(void);
extern unsigned char NoAckRstCnt;
extern unsigned char NoSimCardCnt;	
extern GSMON GsmOn;
#endif 
