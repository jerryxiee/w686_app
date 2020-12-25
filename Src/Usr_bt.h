#ifndef USR_BT_H
#define USR_BT_H

#include "usr_main.h"

#ifndef _BT_TYPE
#define _BT_TYPE
//数据包类型
typedef enum
{
    BT_NULL,
	BT_PIN_APP,              //命令nrf52跳转到APP?
	BT_PIN_BL,               //命令nrf52跳转到bootloader
	BT_PING_0,               //ping nrf52的bootloader
	BT_PING_1,               //ping nrf52的bootloader
	BT_PRN_SET,              //设置PRN
	BT_MTU_GET,              //获取nrf52的单包数据允发送的最大长度
	BT_SELECT_OBJECT,        //选择发送对象
	BT_READY_SEND_DATA,      //查询nrf52是否已经准备好接受指定长度的初始化数据
    BT_DATA_SEND,            //发送数据
    BT_SEND_OVER,            //数据发送完成
    BT_CRC_COMFIRM,          //核对应答的CRC数据校验
	BT_CREATE_OBJECT,        //创建一个对象
    BT_READY_SEND_DATA_FW    //查询nrf52是否已经准备好接受指定长度的固件数据
} BT_TYPE;

#endif


#ifndef _BTDFU_INFO
#define _BTDFU_INFO

typedef struct{
	unsigned char 	HaveSendInitPack:1;			//需要发送初始化数据包
	unsigned char 	InDfu:1;					//升级中
	unsigned char 	WaitBtAck:1;				//等待nrf52应答
	unsigned char 	LastPieceData:1;			//当发送的数据是一块数据里面的最后一片时置位
	unsigned char 	SendFileOver:1;				//完成DFU文件发送
	unsigned char 	DfuFailed:1;				//DFU升级失败

	unsigned short 	EachPackLen;				//每次要发送的数据块长度
	unsigned short 	HaveSendLen;				//已经发送的单块数据包长度
	unsigned int 	AllHaveSendLen;				//已经发送的所有固件数据包长度
	unsigned short  Dfu_Mtu;					//nrf52的单包数据允发送的最大长度
	unsigned char 	SendBuf[128];				//需要发送的数据
	unsigned int  	UpgExFlashAddr;		        //读取到的当前外部flash地址
	unsigned char 	Retry_Cnt;					//升级出错，重试次数
	unsigned char 	Retry_Wait_Cnt;				//升级出错，重试延时

	unsigned int  	IpAddr;		        		//初始化文件存放在外部flash地址
	unsigned int  	IpSzie;		        		//初始化文件大小
	unsigned int  	FwAddr;		        		//固件存放在外部flash的地址
	unsigned int  	FwSzie;		        		//固件大小
	unsigned char 	WaitRspDataOverTime;		//等待应答超时,单位是0.1s

}BTDFU_INFO;

#endif

extern BT_TYPE 		BtType; 
extern BTDFU_INFO 	BtDfu_Info;
extern char 		Bt_Mac[20];			
extern char 		Scan_Mac[400];

u16 BT_SendPacket(BT_TYPE temType, char *pDst);
void BT_Dfu_Receive(BT_TYPE *temType, char *pSrc);
void BT_Data_Receive(char *pSrc);
void Bt_Handle(void);

#endif

