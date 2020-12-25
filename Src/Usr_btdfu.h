#ifndef USR_BTDFU_H
#define USR_BTDFU_H

#ifndef _BTDFU_INFO
#define _BTDFU_INFO
typedef struct{
	unsigned char 	OnePackRecvOver:1;			//一包数据接收数据结束
	unsigned char 	UpgrateFail:1;				//远程升级动作失败
	unsigned char 	NeedCheckUploadState:1;		//下载文件过程中需要周期性查询下载状态
	unsigned char 	NeedUpdata:1;				//开始远程升级
	unsigned char 	NeedDiscontHttp:1;			//需要断开HTTP连接
	unsigned char 	NeedWaitUpgrade:1;			//需要等到指定时间后开始升级
	unsigned char 	HaveGetRankData:1;			//已经获取到了随机升级倒计时数

	unsigned char 	WaitRspDataOverTime;		//等待应答超时,单位是0.1s
	unsigned short 	UpgPacketNums;
	unsigned int  	UpgExFlashAddr;		        //APP在Flash存储地址
	unsigned char 	Verify_ok;			        //MD5校验一致
	unsigned char 	RetryCnt;			        //因网络原因请求失败重试测试
	unsigned char 	RetryWaitCnt;		        //重试等待时间间隔
	unsigned char 	Md5decrypt[17];
}BTDFU_INFO;

#endif

extern BTDFU_INFO BtDfuInfo;

void serial_dfu_start(void);

#endif
