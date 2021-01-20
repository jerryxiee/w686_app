#include "Usr_main.h"

#define	SLIP_END				    0xC0
#define	SLIP_ESC				    0xDB
#define	SLIP_ESC_END			    0xDC
#define	SLIP_ESC_ESC			    0xDD

#define EVERY_PACK_LEN				64			//每一个分包固件大小为64个字节
#define EVERY_FWPACK_LEN			4096		//每一个固件分包大小为4k个字节

char Bt_Info[50];			//nrf52的mac地址
char Scan_Mac[400];			//nrf52的扫描到的mac地址及信号强度

const u8 To_Pin_App[6]      = {0x59,0x01,0x00,0x32,0x21,0x21};
const u8 To_Pin_Bl[6]       = {0x59,0x01,0x00,0x31,0x42,0x11};
const u8 Ping_0[3]          = {0x09,0x00,0xC0};
const u8 Ping_1[3]          = {0x09,0x02,0xC0};
const u8 Prn_Get[4]         = {0x02,0x00,0x00,0xC0};
const u8 Mtu_Get[2]         = {0x07,0xC0};
const u8 Select_Object[3]   = {0x06,0x01,0xC0};
u8 Ready_Send_Data[7] 		= {0x01,0x01,0x8D,0x00,0x00,0x00,0xC0};
const u8 Send_Over[2]       = {0x03,0xC0};
const u8 Crc_Over[2]        = {0x04,0xC0};
const u8 Create_Object[3]   = {0x06,0x02,0xC0};
u8 Ready_Send_Data_Fw[7] 	= {0x01,0x02,0x00,0x10,0x00,0x00,0xC0};

BT_TYPE 	BtType; 
BTDFU_INFO 	BtDfu_Info;
u8 			Bt_SendBuf[150];               //蓝牙发送数据buf

//小端模式的数组的两个成员变量整合成16进制数据
static u16 get_u16_le(u8* p_data)
{
	u16 data;

	data  = ((u16)*(p_data + 0) << 0);
	data += ((u16)*(p_data + 1) << 8);

	return data;
}

//小端模式的16进制数拆分成数组中的两个成员变量
static void put_u16_le(u8* p_data, u16 data)
{
	*(p_data + 0) = (u8)(data >> 0);
	*(p_data + 1) = (u8)(data >> 8);
}

static u32 get_u32_le(const u8* p_data)
{
	u32 data;

	data  = ((u32)*(p_data + 0) <<  0);
	data += ((u32)*(p_data + 1) <<  8);
	data += ((u32)*(p_data + 2) << 16);
	data += ((u32)*(p_data + 3) << 24);

	return data;
}

//pSrcData原buf数据；nSrcSize，原数据长度；pDestData，转译后的buf数据；pDestSize转译后的数据长度
void encode_slip(u8 *pDestData, u32 *pDestSize, const u8 *pSrcData, u32 nSrcSize)
{
	u32 n, nDestSize;

	nDestSize = 0;

	for (n = 0; n < nSrcSize; n++)
	{
		u8 nSrcByte = *(pSrcData + n);

		if (nSrcByte == SLIP_END)
		{
			*pDestData++ = SLIP_ESC;
			*pDestData++ = SLIP_ESC_END;
			nDestSize += 2;
		}
		else if (nSrcByte == SLIP_ESC)
		{
			*pDestData++ = SLIP_ESC;
			*pDestData++ = SLIP_ESC_ESC;
			nDestSize += 2;
		}
		else
		{
			*pDestData++ = nSrcByte;
			nDestSize++;
		}
	}

	*pDestData = SLIP_END;
	nDestSize++;

	*pDestSize = nDestSize;
}

int decode_slip(u8 *pDestData, u32 *pDestSize, const u8 *pSrcData, u32 nSrcSize)
{
	int err_code = 1;
	u32 n, nDestSize = 0;
	bool is_escaped = false;

	for (n = 0; n < nSrcSize; n++)
	{
		u8 nSrcByte = *(pSrcData + n);

		if (nSrcByte == SLIP_END)
		{
			if (!is_escaped)
				err_code = 0;  // Done. OK

			break;
		}
		else if (nSrcByte == SLIP_ESC)
		{
			if (is_escaped)
			{
				// should not get SLIP_ESC twice...
				err_code = 1;
				break;
			}
			else
				is_escaped = true;
		}
		else if (nSrcByte == SLIP_ESC_END)
		{
			if (is_escaped)
			{
				is_escaped = false;

				*pDestData++ = SLIP_END;
			}
			else
				*pDestData++ = nSrcByte;

			nDestSize++;
		}
		else if (nSrcByte == SLIP_ESC_ESC)
		{
			if (is_escaped)
			{
				is_escaped = false;

				*pDestData++ = SLIP_ESC;
			}
			else
				*pDestData++ = nSrcByte;

			nDestSize++;
		}
		else
		{
			*pDestData++ = nSrcByte;
			nDestSize++;
		}
	}

	*pDestSize = nDestSize;

	return err_code;
}


u16 BT_SendPacket(BT_TYPE temType, char *pDst)
{
	u16 data_len = 0;

    switch (temType)
	{
		case BT_PIN_APP:		
			data_len = sizeof(To_Pin_App);							
			memcpy(pDst, To_Pin_App, data_len);	
			break;

		case BT_PIN_BL:	
			data_len = sizeof(To_Pin_Bl);								
			memcpy(pDst, To_Pin_Bl, data_len);
			break;

		case BT_PING_0:		
			data_len = sizeof(Ping_0);							
			memcpy(pDst, Ping_0, data_len);
			break;

		case BT_PING_1:	
			data_len = sizeof(Ping_1);								
			memcpy(pDst, Ping_1, data_len);
			break;

		case BT_PRN_SET:
			data_len = sizeof(Prn_Get);									
			memcpy(pDst, Prn_Get, data_len);
			break;

		case BT_MTU_GET:
			data_len = sizeof(Mtu_Get);										
			memcpy(pDst, Mtu_Get, data_len);
			break;

		case BT_SELECT_OBJECT:	
			data_len = sizeof(Select_Object);									
			memcpy(pDst, Select_Object, data_len);
			break;

		case BT_CREATE_OBJECT:	
			data_len = sizeof(Create_Object);									
			memcpy(pDst, Create_Object, data_len);
			break;

		case BT_READY_SEND_DATA:	
			put_u16_le(Ready_Send_Data + 2, BtDfu_Info.EachPackLen);		//将本次需要发送的数据包长度写入到Ready_Send_Data

			data_len = sizeof(Ready_Send_Data);	
			memcpy(pDst, Ready_Send_Data, data_len);
			break;

		case BT_READY_SEND_DATA_FW:	
			if(BtDfu_Info.FwSzie - BtDfu_Info.AllHaveSendLen < EVERY_FWPACK_LEN)
			{
				BtDfu_Info.EachPackLen = BtDfu_Info.FwSzie - BtDfu_Info.AllHaveSendLen;
				put_u16_le(Ready_Send_Data_Fw + 2, (u16)(BtDfu_Info.FwSzie - BtDfu_Info.AllHaveSendLen));
				BtDfu_Info.SendFileOver = 1;
			}
			else
			{
				put_u16_le(Ready_Send_Data_Fw + 2, EVERY_FWPACK_LEN);
			}			
			
			data_len = sizeof(Ready_Send_Data_Fw);	
			memcpy(pDst, Ready_Send_Data_Fw, data_len);
			break;

		case BT_DATA_SEND:	
		{
			u8 send_buf_temp[128] = {0x08};
			u16 len_temp = 0;

			memset(BtDfu_Info.SendBuf,0,sizeof(BtDfu_Info.SendBuf));

			if(BtDfu_Info.EachPackLen - BtDfu_Info.HaveSendLen > EVERY_PACK_LEN)		//如果剩下需要发送的数据长度大于单包的长度
			{
				BtDfu_Info.LastPieceData = 0;
				EXFLASH_ReadBuffer(send_buf_temp + 1,BtDfu_Info.UpgExFlashAddr,EVERY_PACK_LEN);		//读取数据
				encode_slip(BtDfu_Info.SendBuf, (u32 *)&data_len, send_buf_temp, EVERY_PACK_LEN + 1);		//转译数据
				BtDfu_Info.UpgExFlashAddr += EVERY_PACK_LEN;
				BtDfu_Info.HaveSendLen += EVERY_PACK_LEN;
				BtDfu_Info.AllHaveSendLen += EVERY_PACK_LEN;
			}		
			else
			{
				BtDfu_Info.LastPieceData = 1;
				len_temp = BtDfu_Info.EachPackLen - BtDfu_Info.HaveSendLen;
				EXFLASH_ReadBuffer(send_buf_temp + 1,BtDfu_Info.UpgExFlashAddr,len_temp);		//读取数据
				encode_slip(BtDfu_Info.SendBuf, (u32 *)&data_len, send_buf_temp, len_temp + 1);		//转译数据
				BtDfu_Info.UpgExFlashAddr += len_temp;
				BtDfu_Info.HaveSendLen += len_temp;		
				BtDfu_Info.AllHaveSendLen += len_temp;
			}
						
			memcpy(pDst, BtDfu_Info.SendBuf, data_len);
			break;
		}

		case BT_SEND_OVER:				
			data_len = sizeof(Send_Over);			
			memcpy(pDst, Send_Over, data_len);
			break;

		case BT_CRC_COMFIRM:	
			data_len = sizeof(Crc_Over);			
			memcpy(pDst, Crc_Over, data_len);
			break;

		default:
			break;
	}
	return data_len;
}



void BT_Dfu_Receive(BT_TYPE *temType, char *pSrc)
{
	switch (*temType)
	{
		case BT_PIN_APP:
			if ((*pSrc == 0x51) && (*(pSrc+4) == 0x6F) && (*(pSrc+5) == 0x6B))
			{
				*temType = BT_PIN_BL;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break;    

		case BT_PIN_BL:
			if ((*pSrc == 0x51) && (*(pSrc+4) == 0x6F) && (*(pSrc+5) == 0x6B))
			{
				*temType = BT_PING_0;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_PING_0:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x09) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_PING_1;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_PING_1:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x09) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_PRN_SET;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_PRN_SET:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x02) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_MTU_GET;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_MTU_GET:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x07) && (*(pSrc+2) == 0x01))
			{
				BtDfu_Info.Dfu_Mtu = get_u16_le((u8 *)pSrc + 3);

				*temType = BT_SELECT_OBJECT;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_SELECT_OBJECT:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x06) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_READY_SEND_DATA;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_READY_SEND_DATA:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x01) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_DATA_SEND;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_READY_SEND_DATA_FW:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x01) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_DATA_SEND;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break;

		case BT_DATA_SEND:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x01) && (*(pSrc+2) == 0x01))
			{
				//判断是否为最后一片数据包
				if(((BtDfu_Info.EachPackLen - BtDfu_Info.HaveSendLen > EVERY_PACK_LEN) && !BtDfu_Info.HaveSendInitPack)|| \
				((BtDfu_Info.EachPackLen - BtDfu_Info.HaveSendLen > EVERY_FWPACK_LEN) && BtDfu_Info.HaveSendInitPack))
				{
					*temType = BT_DATA_SEND;
				}
				else
				{
					*temType = BT_SEND_OVER;
				}
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_SEND_OVER:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x03) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_CRC_COMFIRM;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_CRC_COMFIRM:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x04) && (*(pSrc+2) == 0x01))
			{
				//首次开始发送Fw固件
				if(BtDfu_Info.HaveSendInitPack == 0)	
				{
					BtDfu_Info.HaveSendInitPack = 1;
					BtDfu_Info.UpgExFlashAddr = BtDfu_Info.FwAddr;
					BtDfu_Info.HaveSendLen = 0;
					BtDfu_Info.AllHaveSendLen = 0;

					//如果固件大小不足4k
					if(BtDfu_Info.FwSzie <= EVERY_FWPACK_LEN)
					{
						BtDfu_Info.EachPackLen = BtDfu_Info.FwSzie;
					}
					else
					{
						BtDfu_Info.EachPackLen = EVERY_FWPACK_LEN;
					}
					
					*temType = BT_CREATE_OBJECT;
				}
				else	//如果是在固件发送期间
				{
					*temType = BT_READY_SEND_DATA_FW;
					BtDfu_Info.HaveSendLen = 0;
				}

				if(BtDfu_Info.SendFileOver)
				{
					*temType = BT_NULL;
					printf("\r\n--------------------> DFU file send Fished <--------------------\r\n");
				}
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		case BT_CREATE_OBJECT:
			if ((*pSrc == 0x60) && (*(pSrc+1) == 0x06) && (*(pSrc+2) == 0x01))
			{
				*temType = BT_READY_SEND_DATA_FW;
			}
			else 
			{
				BtDfu_Info.DfuFailed = 1;
				*temType = BT_NULL;
			}
			break; 

		default:
			break;
	}
}


//读取bin文件中开头的info部分内容，该内容存放了文件IP地址，IP大小，固件地址和固件大小信息
void dfu_file_info(u32* ip_addr, u32* ip_size, u32* fw_addr, u32* fw_size)
{
    u8 p_file_header[100] = {0};

    EXFLASH_ReadBuffer(p_file_header,EXFLASH_BTAPP_ADDR,100);

    *ip_addr = EXFLASH_BTAPP_ADDR + 0x80;
    *fw_addr = EXFLASH_BTAPP_ADDR + 0x280;

    *ip_size = get_u32_le(&p_file_header[24]);
    *fw_size = get_u32_le(&p_file_header[32]);

    printf("ip addr: %08x\r\n", *ip_addr);
    printf("ip size: %08x\r\n", *ip_size);
    printf("fw addr: %08x\r\n", *fw_addr);
    printf("fw size: %08x\r\n", *fw_size);
}


void Bt_Dfu_Init(void)
{
	dfu_file_info(&BtDfu_Info.IpAddr, &BtDfu_Info.IpSzie, &BtDfu_Info.FwAddr, &BtDfu_Info.FwSzie);

	BtDfu_Info.EachPackLen = BtDfu_Info.IpSzie;
	BtDfu_Info.UpgExFlashAddr = BtDfu_Info.IpAddr;

	BtDfu_Info.WaitBtAck = 0;
	BtDfu_Info.HaveSendInitPack = 0;
	BtDfu_Info.LastPieceData = 0;
	BtDfu_Info.HaveSendLen = 0;
	BtDfu_Info.AllHaveSendLen = 0;
	BtDfu_Info.Retry_Wait_Cnt = 0;
	BtDfu_Info.SendFileOver = 0;

	memset(BtDfu_Info.SendBuf,0,sizeof(BtDfu_Info.SendBuf));
}


void Start_Bt_Dfu(void)
{
	BtDfu_Info.InDfu = 1;
	BtType = BT_PIN_APP;
	BtDfu_Info.WaitRspDataOverTime = 20;
	Bt_Dfu_Init();
}

void Bt_Handle(void)
{
	u16 data_len = 0;

	if(Flag.NeedBtPowerOn)
	{
		Flag.NeedBtPowerOn = 0;
		Flag.BtPowerOn = 1;
		BT_POWER_EN_SET;
		Usr_USART4_UART_Init();
		return;
	}

	if(Flag.NeedBtPowerOff)
	{
		Flag.NeedBtPowerOff = 0;
		Flag.BtPowerOn = 0;
		BT_POWER_EN_RESET;
		Usr_USART4_UART_DeInit();
		return;
	}

	//需要升级NRF52
	if(UpgInfo.NeedDfuNrf52)
	{
		if(!Flag.BtPowerOn)
		{
			Flag.NeedBtPowerOn = 1;
			return;
		}

		UpgInfo.NeedDfuNrf52 = 0;
		Start_Bt_Dfu();
		return;
	}

	//升级过程中超时，退出升级
	if((BtDfu_Info.WaitRspDataOverTime == 0) && (BtDfu_Info.InDfu) && !BtDfu_Info.SendFileOver)
	{
		BtType = BT_NULL;
		BtDfu_Info.InDfu = 0;
		BtDfu_Info.DfuFailed = 1;
		return;
	}

	//有数据要发送并收到了前面的应答
	if((BtType != BT_NULL) && (!BtDfu_Info.WaitBtAck))
	{
		data_len = BT_SendPacket(BtType,(char *)Bt_SendBuf);
		UART_Send(USART4,Bt_SendBuf,data_len);

		//由于蓝牙芯片切换到BootLoader需要点时间，所以这条指令需要等一下
		if(BtType == BT_PIN_BL)
		{
			delay_ms(500);
		}

		BtDfu_Info.WaitRspDataOverTime = 20;	//超时时间设置为2秒	

		//由于BT_DATA_SEND在发送数据片时，对方无应答，所以这里需要特殊判断一下
		if(BtType != BT_DATA_SEND)
		{
			BtDfu_Info.WaitBtAck = 1;
		}
		else if(BtDfu_Info.LastPieceData == 1)	//如果是最后一片数据，发送完成之后需要发送BT_SEND_OVER
		{
			BtType = BT_SEND_OVER;
		}
		return;
	}

	if(BtDfu_Info.DfuFailed && (BtDfu_Info.Retry_Wait_Cnt == 0) && (BtDfu_Info.Retry_Cnt > 0))
	{
		BtDfu_Info.DfuFailed = 0;
		BtDfu_Info.Retry_Wait_Cnt = 10;
		BtDfu_Info.Retry_Cnt --;
		BtDfu_Info.InDfu = 0;

		printf("Nrf52 Dfu failed,retry after 10s... \r\n");
		
		//如果升级失败，重启蓝牙，重新开始
		BT_POWER_EN_RESET;
		delay_ms(500);
		BT_POWER_EN_SET;
		return;
	}

	if(BtDfu_Info.DfuFailed && (BtDfu_Info.Retry_Cnt == 0))
	{
		BtDfu_Info.DfuFailed = 0;
		printf("Nrf52 Dfu failed\r\n");
	}

}

//将指定长度的十六进制转换为字符串
void Bt_Mac_Transform(char *pSrc, char *pDst, u16 len)
{
	u8 i;
	char *pSrc_temp = NULL;
	char *pDst_temp = NULL;
	char temp_buf[2] = {0};

	pSrc_temp = (char *)pSrc;
	pDst_temp = (char *)pDst;

	for(i = 0; i < len; i ++)
	{
		sprintf(pDst_temp, "%02X", pSrc_temp[i]);

		pDst_temp += 2;
		memset(temp_buf,0,sizeof(pSrc_temp));
	}
}

char debug_buf[500] = {0};

void BT_Data_Receive(char *pSrc)
{
	char   *p0 = NULL;
	u8   data_type = 0;
	u16	 data_len = 0;

	p0 = pSrc;
	memset(debug_buf,0,sizeof(debug_buf));

	//蓝牙串口起始字节合法性判断
	if((*p0 != 0x51) && (*p0 != 0x59))
	{
		return;
	}

	data_len = get_u16_le((u8 *)p0 +1);	//获取数据长度

	p0 += 3;			//指向数据类型
	data_type = *p0;
	p0 ++;

	switch (data_type)
	{
		//蓝牙mac地址和软件版本,mac地址是十六进制，软件版本是字符串，需要分开解析和接收
		case 0x41:		
			if(data_len > sizeof(Bt_Info))	break;

			memset(Bt_Info,0,sizeof(Bt_Info));
			Bt_Mac_Transform(p0,Bt_Info,6);		//转换mac地址为字符串

			sprintf(debug_buf,"Recive the BT mac address is:%s\r\n",Bt_Info);
			UART_Send(USART3,(u8 *)debug_buf,strlen(debug_buf));
			printf("Have revice the BT mac address: %s\r\n",Bt_Info);

			p0 += 6;
		    strcat(Bt_Info,";BTSW=");
			memcpy(&Bt_Info[18],p0,data_len - 7);

			Test.HaveGetBtMac = 1;
			break;

		case 0x42:
			if(data_len > sizeof(Scan_Mac))	break;

			memset(Scan_Mac,0,sizeof(Scan_Mac));
			Bt_Mac_Transform(p0,Scan_Mac,data_len - 1);

			sprintf(debug_buf,"Scan the BT mac address is:%s\r\n",Scan_Mac);
			UART_Send(USART3,(u8 *)debug_buf,strlen(debug_buf));
			printf("Have revice the BT scan address: %s\r\n",Scan_Mac);
			
			Test.GetBtInfo = 1;
			Test.HaveGetBtScan = 1;
			break;	

		default:
			break;
	}
}

