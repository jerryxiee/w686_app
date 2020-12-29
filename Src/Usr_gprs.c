#include "usr_main.h"

GPRS_TYPE GprsType;
GPRS_SEND GprsSend;

unsigned short ConnectDelayCnt; //由于网络问题，重连服务器中间等待时间
unsigned char Upd_command_len;	//远程升级时，发送数据包长度
unsigned short At_Timeout_Cnt;	//等待AT超时的时间，用于等待非标准超时时间时使用
unsigned int ActiveTimer;
char UserIDBuf[16];
char GprsSendBuf[DATABUFLEN];
char UpgradeSendBuf[UPDRADELEN];
char CCID[30];
char AtSendbuf[SCIBUFLEN]; /*定义一个数组存储发送数据*/
char GprsContent[GPRSCONTLEN];
char IMEI[16];					//系统最终会使用的IMEI
char IMEI_MODULE[16];			//GSM模块的IMEI
char IMEI_ID[16];				//自行设定的IMEI
char RespServiceBuf[100];		//用于应答平台的内容
int Usr_Atoi(char *pSrc)
{
	signed int i = 0;
	signed char f = 1;

	if ('-' == *pSrc)
	{
		f = -1;
		pSrc++;
	}
	while ((*pSrc >= '0') && (*pSrc <= '9'))
	{
		i *= 10;
		i += *pSrc - '0';
		pSrc++;
	}
	i *= f;

	return i;
}

u16 HEX2BCD(u8 Val_HEX)
{
	u16 Val_BCD;
	u8 temp;
	temp = Val_HEX % 100;
	Val_BCD = ((u16)Val_HEX) / 100 << 8;
	Val_BCD = Val_BCD | temp / 10 << 4;
	Val_BCD = Val_BCD | temp % 10;
	return Val_BCD;
}

u32 HEX2BCD_FOR_U32(u16 Val_HEX)
{
	u32 Val_BCD;
	u32 temp;
	temp = Val_HEX % 10000;
	Val_BCD = (u32)(Val_HEX) / 10000 << 16;
	Val_BCD = Val_BCD | temp / 1000 << 12;
	temp = Val_HEX % 1000;
	Val_BCD = Val_BCD | temp / 100 << 8;
	temp = Val_HEX % 100;
	Val_BCD = Val_BCD | temp / 10 << 4;
	Val_BCD = Val_BCD | temp % 10;
	return Val_BCD;
}

void ChangeToUpper(char *pSrc, unsigned char maxLen)
{
	unsigned char I = 0;
	do
	{
		if (pSrc[I] >= 'a' && pSrc[I] <= 'z')
			pSrc[I] = pSrc[I] - 32;
		I++;
		maxLen--;
	} while (pSrc[I] != '\0' && pSrc[I] != '*' && maxLen);
}

void Ascii2BCD(char *pSrc, unsigned char *pDst)
{
	char i, f, k = 0, temp;

	for (i = 0; i < 2; i++)
	{
		temp = 0;
		f = 1;
		if (pSrc[i] >= '0' && pSrc[i] <= '9')
			temp = pSrc[i] - 0x30;

		else
			f = 0;

		if (f)
		{
			if (0 == k % 2)
			{
				*pDst = (unsigned char)(temp * 10);
			}
			else
			{
				*pDst += (unsigned char)temp;
			}
			k++;
		}
	}
}

u16 Ascii2BCD_u16(char *pSrc, unsigned char len)
{
	unsigned char i;
	u16 result = 0, temp = 0;

	for (i = 0; i < len; i++)
	{
		result *= 10;
		if (pSrc[i] >= '0' && pSrc[i] <= '9')
			temp = pSrc[i] - 0x30;
		else
			return 0;

		result += temp;
	}

	return result;
}

//将字符串"41 01 83 07 FF 00"转换到hex值
u32 Ascii2Hex(char *pSrc, unsigned char srcLen)
{
	char i;
	u32 Data_leng = 0, temp = 0;
	for (i = 0; i < srcLen; i++)
	{
		Data_leng <<= 4;
		if (pSrc[i] >= '0' && pSrc[i] <= '9')
			temp = pSrc[i] - 0x30;
		else if (pSrc[i] >= 'a' && pSrc[i] <= 'f')
			temp = pSrc[i] - 0x57;
		else if (pSrc[i] >= 'A' && pSrc[i] <= 'F')
			temp = pSrc[i] - 0x37;
		else
			return 0xFF;
		Data_leng += temp;
	}
	return Data_leng;
}
//将指定长度的字符串转换为HEX
void StrAscii2Hex(char *pSrc, char *pDst, u16 Srclen)
{
	char f, temp;
	u16 Dstlen = 0, i;
	if (Srclen % 2 != 0)
		return;

	for (i = 0; i < Srclen; i++)
	{
		temp = 0;
		f = 1;
		if (pSrc[i] >= '0' && pSrc[i] <= '9')
			temp = pSrc[i] - 0x30;
		else if (pSrc[i] >= 'a' && pSrc[i] <= 'f')
			temp = pSrc[i] - 0x57;
		else if (pSrc[i] >= 'A' && pSrc[i] <= 'F')
			temp = pSrc[i] - 0x37;
		else
			f = 0;

		if (f)
		{
			if (0 == i % 2)
			{
				pDst[Dstlen] = (unsigned char)(temp << 4);
			}
			else
			{
				pDst[Dstlen] += (unsigned char)temp;
				Dstlen++;
			}
		}
	}
}

void U32ToBCDStrAscii(u32 Src, char *pDst)
{
	u8 data_temp = 0;
	u32 BCD_temp = 0, Mask = 0;
	u8 Frist_not_0 = 0, i = 0, j = 7;
	BCD_temp = HEX2BCD_FOR_U32((u16)Src);
	i = 8;
	Mask = 0xF0000000;
	while (i--)
	{
		data_temp = (u8)((BCD_temp & Mask) >> i * 4);
		if ((data_temp != 0) || (Frist_not_0 == 1))
		{
			pDst[7 - j] = data_temp + 0x30;
			Frist_not_0 = 1;
			j--;
		}
		Mask = Mask >> 4;
	}
}
void Hex2StrAscii(char *pSrc, char *pDst, u16 Srclen)
{
	char f, temp, temp0;
	u16 Dstlen = 0, i, j = 0;

	Dstlen = Srclen * 2;
	f = 1;
	for (i = 0; i < Dstlen; i++)
	{

		if (0 == i % 2)
		{
			temp0 = pSrc[j] & 0xF0;
			temp0 = temp0 >> 4;
			if (temp0 <= 0x9)
				temp = temp0 + 0x30;
			else if ((temp0 >= 0x0A) && (temp0 <= 0x0F))
				temp = temp0 + 0x37;
			else
				f = 0;
		}
		else
		{
			temp0 = pSrc[j] & 0x0F;
			if (temp0 <= 0x9)
				temp = temp0 + 0x30;
			else if ((temp0 >= 0x0A) && (temp0 <= 0x0F))
				temp = temp0 + 0x37;
			else
				f = 0;

			j++;
		}

		if (f)
			pDst[i] = temp;
	}
}

//将i个字节的pSrcAcsii码转换成BCD码存放在pDst
void Acsii2Bcd(char *pSrc, char *pDst, unsigned char i)
{
	unsigned char j = 0;
	unsigned char l = 0;

	if (0 == i || NULL == pSrc || NULL == pDst)
	{
		return;
	}

	ChangeToUpper(pSrc, i);

	while (i--)
	{
		if (pSrc[j] >= '0' && pSrc[j] <= '9')
		{
			l = pSrc[j] - '0';
		}
		else if (pSrc[j] >= 'A' && pSrc[j] <= 'F')
		{
			l = pSrc[j] - 'A' + 10;
		}
		else
		{
			return;
		}

		if (0 == (j % 2))
		{
			*pDst = l << 4;
		}
		else
		{
			*pDst++ |= l;
		}

		j++;
	}
}

//将十进制的字符串转换为十六进制的字符串,最大转换0xFFFFFFFF对应的十进制长度
void BcdStr2HexStr(char *pSrc, char *pDst)
{
	u32 TempData = 0;

	TempData = (u32)atoi(pSrc);
	sprintf(pDst,"%x",TempData);
}

u16 Mqtt_SendPacket(GPRS_TYPE switch_tmp)
{
	u16 data_len = 0;
	u8 bat_percente = 0;
	char Temp[50] = {0};

	memset(GprsSendBuf, '\0', DATABUFLEN);
	memset(GprsContent, '\0', GPRSCONTLEN);

	//电池电量低于3.0v认为是没电，高于4.1v认为是满电，中间电量按照电压线性计算
	if (BatVoltage < 3000)
		bat_percente = 0;
	else
		BatVoltage -= 3000;

	if (BatVoltage > 1100)
		bat_percente = 100;
	else
		bat_percente = BatVoltage / 11;
	switch (switch_tmp)
	{
	case LOGIN:

		strcat(GprsContent, "\"a0\":"); //时间戳
		sprintf(Temp, "\"%d\",", Timestamp);
		strcat(GprsContent, Temp);

		// strcat(GprsContent, "a1:"); //时区
		// strcat(GprsContent, "\"GMT+8\",");

		// strcat(GprsContent, "a2:"); //公司名称
		// strcat(GprsContent, "\"IOTBANK\",");

		strcat(GprsContent, "\"a3\":"); //产品名称
		strcat(GprsContent, "\"W686A\",");

		strcat(GprsContent, "\"a4\":"); //IMEI
		sprintf(Temp, "\"%s\",", IMEI);
		strcat(GprsContent, Temp);

		strcat(GprsContent, "\"a5\":"); //CCID
		sprintf(Temp, "\"%s\",", CCID);
		strcat(GprsContent, Temp);

		strcat(GprsContent, "\"a8\":"); //固件版本
		sprintf(Temp, "\"%s\"", Edition_STD);
		strcat(GprsContent, Temp);

		sprintf(GprsSendBuf,
				"{\"to\":\"IoT/imei:%s/default\",\"op\":1,\"pc\":{\"m2m:cin\":{\"con\":{%s}}},\
\"fr\":\"IoT/imei:%s\",\"rqi\":\"imei:%s_20200623185648_001\",\"ty\":4,\"rt\":2}",
				IMEI, GprsContent, IMEI, IMEI);
		break;

	case DATA:

		strcat(GprsContent, "\"a0\":"); //时间戳
		sprintf(Temp, "\"%d\",", Timestamp);
		strcat(GprsContent, Temp);

		// strcat(GprsContent, "\"b0\":"); //电池电量
		// sprintf(Temp, "\"%d\",", bat_percente);
		// strcat(GprsContent, Temp);

		// strcat(GprsContent, "\"b1\":"); //充电状态
		// sprintf(Temp, "\"%s\",", NotSupport);
		// strcat(GprsContent, Temp);

		strcat(GprsContent, "\"s1\":"); //湿度
		sprintf(Temp, "\"%.1f\",", humidity_value);
		strcat(GprsContent, Temp);

		strcat(GprsContent, "\"s2\":"); //二氧化碳浓度
		sprintf(Temp, "\"%d\",", co2_module_value);
		strcat(GprsContent, Temp);

		strcat(GprsContent, "\"s3\":"); //温度
		sprintf(Temp, "\"%.1f\",", temperature_value);
		strcat(GprsContent, Temp);

		strcat(GprsContent, "\"p3\":"); //基站
		sprintf(Temp, "\"%s_%s_%s_%s\"", Cid,Lac,Mcc,Mnc);
		strcat(GprsContent, Temp);

		// strcat(GprsContent, "s4:"); //红外
		// sprintf(Temp, "\"%d\",", ir_value);
		// strcat(GprsContent, Temp);

		// strcat(GprsContent, "s5:"); //tvoc
		// sprintf(Temp, "\"%d\"", ccs811_tvoc_value);
		// strcat(GprsContent, Temp);

		sprintf(GprsSendBuf,
				"{\"to\":\"IoT/imei:%s/default\",\"op\":1,\"pc\":{\"m2m:cin\":{\"con\":{%s}}},\
\"fr\":\"IoT/imei:%s\",\"rqi\":\"imei:%s_20200623185648_001\",\"ty\":4,\"rt\":2}",
				IMEI, GprsContent, IMEI, IMEI);
		//有些数据打包发送之后，需要清零重新计数
		ir_value = 0;
		break;

	case UPGRESULT:
		strcat(GprsContent, "upgrade result:"); 
		if(Flag.UpgrateFailed)
		{
			Flag.UpgrateFailed = 0;
			strcat(GprsContent, FsUpg.HttpError);
		}
		else
		{
			strcat(GprsContent, "upgrade finish!");
		}
		sprintf(GprsSendBuf,
				"{\"to\":\"IoT/imei:%s/default\",\"op\":1,\"pc\":{\"m2m:cin\":{\"con\":\"%s\"}},\
\"fr\":\"IoT/imei:%s\",\"rqi\":\"imei:%s_20200623185648_001\",\"ty\":4,\"rt\":2}",
				IMEI, GprsContent, IMEI, IMEI);
		break;

		case RESPONSE:
		sprintf(GprsSendBuf,
				"{\"to\":\"IoT/imei:%s/default\",\"op\":1,\"pc\":{\"m2m:cin\":{\"con\":\"{%s}\"}},\
\"fr\":\"IoT/imei:%s\",\"rqi\":\"imei:%s_20200623185648_001\",\"ty\":4,\"rt\":2}",
				IMEI, RespServiceBuf, IMEI, IMEI);	
		break;
	default:
		break;
	}
	data_len = strlen(GprsSendBuf);
	return data_len;
}

void GPRS_Send_Handle(void)
{
	//如果是没有发送登入包需要发送登入包时，需要时间和CCID都已经获取到。
	//登入包发送之后，就不需要这个两个条件
	if ((!Flag.HaveGetCCID || !Flag.HaveSynRtc) && (Flag.NeedLogIn))
	{
		return;
	}

	if (Flag.NeedLogIn)
	{
		Flag.NeedLogIn = 0;
		AtType = AT_SMPUB;
		GprsType = LOGIN;
		printf("Send login data to mqtt service\r\n");
		return;
	}

	if ((GprsSend.posCnt > 0) && (GprsSend.posFlag))
	{
		AtType = AT_SMPUB;
		GprsType = DATA;
		GprsSend.posCnt = 0;
		GprsSend.posFlag = 0;
		printf("Send sensor data to mqtt service\r\n");
		return;
	}

	if (EXFLASH_ReadBreakPoint())
	{
		AtType = AT_SMPUB;
		GprsType = BKDATA;
		Flag.IsSendingBk = 1;
		return;
	}

#if 0								//暂时先关闭结果上报
	if(Flag.NeedSendUpgResult)
	{
		AtType = AT_SMPUB;
		GprsType = UPGRESULT;
		Flag.NeedSendUpgResult = 0;	
		return;	
	}
#endif

	if(Flag.NeedSendResponse)
	{
		AtType = AT_SMPUB;
		GprsType = RESPONSE;
		Flag.NeedSendResponse = 0;	
		Flag.NeedResponseFrist = 0;
		return;			
	}
}

//定位包保存
void GPRS_SaveBreakPoint(void)
{

	if ((Flag.GprsConnectOk == 0 || Flag.PsSignalOk == 0) && Flag.ReadySaveBreak && GprsSend.posCnt > 0)
	{
		Flag.ReadySaveBreak = 0;

		if (GprsSend.posCnt > 0)
		{
			GprsSend.posCnt--;
		}
		GprsType = DATA;
		Mqtt_SendPacket(GprsType);
		EXFLSAH_SaveBreakPoint();
	}
}

//例:将"41424344" 转换为"ABCD"
void HexStrToStr(const char *pSrc, char *pDst)
{
	u8 tmp;

	while ('\0' != *pSrc)
	{
		tmp = 0;

		if ((*pSrc >= '0') && (*pSrc <= '9'))
		{
			tmp = *pSrc - '0';
		}
		else if ((*pSrc >= 'a') && (*pSrc <= 'z'))
		{
			tmp = *pSrc - 'a' + 10;
		}
		else if ((*pSrc >= 'A') && (*pSrc <= 'Z'))
		{
			tmp = *pSrc - 'A' + 10;
		}
		tmp <<= 4;
		pSrc++;

		if ((*pSrc >= '0') && (*pSrc <= '9'))
		{
			tmp |= *pSrc - '0';
		}
		else if ((*pSrc >= 'a') && (*pSrc <= 'z'))
		{
			tmp |= *pSrc - 'a' + 10;
		}
		else if ((*pSrc >= 'A') && (*pSrc <= 'Z'))
		{
			tmp |= *pSrc - 'A' + 10;
		}

		*pDst++ = tmp;
		pSrc++;
	}
}

void Itoa(unsigned char src, char dst[])
{
	unsigned char i = 0;
	int tmp = src;

	while ((tmp /= 10) > 0)
	{
		i++;
	}
	i++;
	dst[i--] = '\0';

	do
	{
		dst[i] = src % 10 + '0';
		src /= 10;
	} while (i--);
}

void WIRELESS_GprsReceive(char *pSrc, u16 len)
{
	char *p0 = NULL;
	char *p1 = NULL;
	char fota_type = 0;
	char gprs_content[100] = {0};

	p0 = strstr(pSrc,"\"con\":");
	p0 += 7;
	if(p0 != NULL)
	{
		p1 = strstr(p0,"}}");
		if(p1 != NULL)
		{
			Flag.NeedSendResponse = 1;
			memset(RespServiceBuf,0,sizeof(RespServiceBuf));

			strncpy(gprs_content, p0, p1 - p0 - 1);
			printf("\r\nRecvice the data form service: %s\r\n",gprs_content);


			//重启和关机命令
			if((p0 = strstr(gprs_content,"c8")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 ++;
				if(*p0 == '1')			
				{
					Flag.NeedDeviceRst = 1;
					Flag.NeedResponseFrist = 1;	
					strcpy(RespServiceBuf,"\\\"c8\\\":1");
				}
				else if(*p0 == '0')
				{
					Flag.NeedShutDown = 1;	
					Flag.NeedResponseFrist = 1;		
					strcpy(RespServiceBuf,"\\\"c8\\\":0");		
				}
				else
				{
					printf("\r\nc8 data format incorrect!\r\n");
				//	strcpy(RespServiceBuf,"c8 data format incorrect!");	
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//设置FOTA升级允许
			else if((p0 = strstr(gprs_content,"c17")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 ++;

				if(*p0 == '1')				//开启FOTA
				{
					Fs.FotaSwitch = 0x01;
					Flag.NeedUpdateFs = 1;
					strcpy(RespServiceBuf,"\\\"c17\\\":1");
				}
				else if(*p0 == '0')
				{
					Fs.FotaSwitch = 0xAA;
					Flag.NeedUpdateFs = 1;		
					strcpy(RespServiceBuf,"\\\"c17\\\":0");			
				}
				else
				{
					printf("\r\nc17 data format incorrect!\r\n");
					strcpy(RespServiceBuf,"c17 data format error!");	
				}
			}

			//远程升级：c18:4-W686IB_V0.0.1.bin-9caf17aecfcf907bc85ad1c187cb255b
			else if((p0 = strstr(gprs_content,"c18")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 += 3;
				fota_type = *p0;

				if((fota_type != '1')&&(fota_type != '4'))
				{
					printf("c18 data format error!\r\n");
					UpgInfo.UpgrateFail = 1;
					strcpy(FsUpg.HttpError,"Command error!");
					return;					
				}


				Flag.NeedSendResponse = 0;		//这里使用远程升级结果回复，不需要通用结果重复回复

				if(Fs.FotaSwitch == 0xAA)		//如果不允许远程升级，上报服务器
				{
					printf("FOTA Switch off,Not updata the device!\r\n");
					UpgInfo.UpgrateFail = 1;
					strcpy(FsUpg.HttpError,"Fota switch off,Upgrade failed");
					return;
				}

				p0 = strstr(p0,"-");
				p0 ++;
				p1 = strstr(p0,"-");
				//获取文件名称
				memset(FsUpg.AppFilePath,0,sizeof(FsUpg.AppFilePath));
				strcpy(FsUpg.AppFilePath,"/fw/");
				strncat(FsUpg.AppFilePath,p0,p1 - p0);
				
				#if 1			//判断升级文件名的合法性，避免升级文件错误
				if((memcmp(p0,"W686",4) !=0 )||(strstr(FsUpg.AppFilePath,".bin") == NULL))
				{
					printf("Illegal file name!\r\n");
					UpgInfo.UpgrateFail = 1;
					strcpy(FsUpg.HttpError,"Illegal file name!");
					return;
				}
				#endif

				//提取MD5校验
				p0 = p1 + 1;
				memset(Md5FileAsc,0,sizeof(Md5FileAsc));
				memcpy(Md5FileAsc,p0,32);

				//设置升级服务器域名和端口
				memset(FsUpg.AppIpAdress,0,sizeof(FsUpg.AppIpAdress));
//				strcpy(FsUpg.AppIpAdress,"http://stg-fota.mamosearch.com:80");		//最早的测试服
//				strcpy(FsUpg.AppIpAdress,"http://fota.mamoair.net:80");			//正式服务器
				strcpy(FsUpg.AppIpAdress,"http://stg-fota.mamoair.net:80");		//中间一版测试服务器

				MD5Init(&Upgmd5);  					//初始化MD5

				if(fota_type == '4')
				{
					UpgInfo.NeedUpdata = 1;				//需要开始升级
					Flag.NeedResponseFrist = 1;			//需要首先应答平台消息后在开始升级
					Flag.NeedSendResponse = 1;
					UpgInfo.RetryCnt = 2;				//升级失败重复次数

					printf("Need upgrade the device,upgrade file name is: %s\r\n",FsUpg.AppFilePath);
					strncpy(RespServiceBuf,gprs_content + 1,strlen(gprs_content) - 2);	//去掉前后的“{}”
				}
				else if(fota_type == '1')
				{
					Flag.NeedSendResponse = 1;
					UpgInfo.NeedWaitUpgrade = 1;
					strncpy(RespServiceBuf,gprs_content + 1,strlen(gprs_content) - 2);
				}
				

			}

			//修改传感器数据检测时间间隔
			else if((p0 = strstr(gprs_content,"c21")) != NULL)
			{
				unsigned short Interval_ck_Temp = 0;

				p0 = strstr(p0,":");
				p0 ++;

				p1 = strstr(p0,"}");

				if(p1 - p0 <= 7)			
				{
					Interval_ck_Temp = Ascii2BCD_u16(p0, p1-p0);
					if(Interval_ck_Temp < 5)			//19B自身采样间隔为5秒一次，小于这个值没什么意义
					{
						Flag.NeedSendResponse = 0;		//数据格式错误不回复
						return;
					}

					Fs.SensorCkInterval = Interval_ck_Temp;

					Flag.NeedUpdateFs = 1;	
					sprintf(RespServiceBuf,"\\\"c21\\\":%d",Fs.SensorCkInterval);
				}
				else
				{
					printf("\r\nc21 data format incorrect!\r\n");
					Flag.NeedSendResponse = 0;			//数据格式错误不回复
				}
			}

			//修改上传时间间隔
			else if((p0 = strstr(gprs_content,"c22")) != NULL)
			{
				unsigned short Interval_Temp = 0;

				p0 = strstr(p0,":");
				p0 ++;

	//			p1 = strstr(p0,"\"}}");
				p1 = strstr(p0,"}");

				if(p1 - p0 <= 7)			
				{
					Interval_Temp = Ascii2BCD_u16(p0, p1-p0);
					if(Interval_Temp < 5)
					{
//						strcpy(RespServiceBuf,"c22 data format incorrect!");
						Flag.NeedSendResponse = 0;		//数据格式错误不回复
						return;
					}

					Fs.Interval = Interval_Temp;
					IntervalTemp = Interval_Temp;

					Flag.NeedUpdateFs = 1;	
					sprintf(RespServiceBuf,"\\\"c22\\\":%d",Fs.Interval);
				}
				else
				{
					printf("\r\nc22 data format incorrect!\r\n");
//					strcpy(RespServiceBuf,"c22 data format incorrect!");	
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//恢复出厂设置
			else if((p0 = strstr(gprs_content,"c23")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 ++;

				if(*p0 == '1')				//开启FOTA
				{
					Flag.NeedClrValueFile = 1;
					Flag.NeedResponseFrist = 1;	
					strcpy(RespServiceBuf,"\\\"c23\\\":1");
				}
				else if(*p0 == '0')
				{
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
				else
				{
					printf("\r\nc8 data format incorrect!\r\n");
				//	strcpy(RespServiceBuf,"c8 data format incorrect!");	
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//修改二氧化碳sensor阈值，格式：c24:1000,1500
			else if((p0 = strstr(gprs_content,"c24")) != NULL)
			{
				unsigned short WarnThresholdTemp = 0;
				unsigned short AlarmThresholdTemp = 0;

				p0 = strstr(p0,":");
				p0 += 3;

				p1 = strstr(p0,",");

				if(p1 - p0 <= 5)			
				{
					WarnThresholdTemp = Ascii2BCD_u16(p0, p1-p0);

					p0 = p1 + 1;
					p1 = strstr(p0,"\\\"}");

					if(p1 - p0 <= 5)	
					{
						AlarmThresholdTemp = Ascii2BCD_u16(p0, p1-p0);
					}


					if(WarnThresholdTemp >= AlarmThresholdTemp)		//数据格式不合法
					{
//						strcpy(RespServiceBuf,"c22 data format incorrect!");
						Flag.NeedSendResponse = 0;		//数据格式错误不回复
						return;
					}

					Fs.Co2WarnThreshold = WarnThresholdTemp;
					Fs.Co2AlarmThreshold = AlarmThresholdTemp;

					Flag.NeedUpdateFs = 1;	
					sprintf(RespServiceBuf,"\\\"c24\\\":\\\"%d,%d\\\"",Fs.Co2WarnThreshold,Fs.Co2AlarmThreshold);
				}
				else
				{
					printf("\r\nc24 data format incorrect!\r\n");
//					strcpy(RespServiceBuf,"c22 data format incorrect!");	
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//需要校准二氧化碳传感器
			else if((p0 = strstr(gprs_content,"c25")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 ++;

				if(*p0 == '1')				
				{
					Flag.NeedCalibrateCo2 = 1;
					Flag.NeedResponseFrist = 1;	
					strcpy(RespServiceBuf,"\\\"c25\\\":1");
				}
				else if(*p0 == '0')
				{
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
				else
				{
					printf("\r\nc25 data format incorrect!\r\n");
				//	strcpy(RespServiceBuf,"c8 data format incorrect!");	
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//上报设备参数
			else if((p0 = strstr(gprs_content,"c26")) != NULL)
			{
				u8 CalibrateState_temp = 0;

				p0 = strstr(p0,":");
				p0 ++;

				if(*p0 == '1')				
				{
					if(Fs.AutoCalibrateState == 1)
					{
						CalibrateState_temp = 0;
					}
					else
					{
						CalibrateState_temp = 1;
					}

					Flag.NeedResponseFrist = 1;	
					sprintf(RespServiceBuf,"\\\"c21\\\":%d,\\\"c22\\\":%d,\\\"c24\\\":%d,%d,\\\"c27\\\":%d",Fs.SensorCkInterval,Fs.Interval,Fs.Co2WarnThreshold,Fs.Co2AlarmThreshold,CalibrateState_temp);
				}
				else if(*p0 == '0')
				{
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
				else
				{
					printf("\r\nc26 data format incorrect!\r\n");
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//二氧化碳传感器自动校准关闭/开启
			else if((p0 = strstr(gprs_content,"c27")) != NULL)
			{
				p0 = strstr(p0,":");
				p0 ++;

				if(*p0 == '0')				
				{
					Fs.AutoCalibrateState = 1;	
					Flag.NeedCloseAutoCalib = 1;
					Flag.NeedUpdateFs = 1;	
					strcpy(RespServiceBuf,"\\\"c27\\\":0");
				}
				else if(*p0 == '1')
				{
					Fs.AutoCalibrateState = 0xAA;	
					Flag.NeedOpenAutoCalib = 1;
					Flag.NeedUpdateFs = 1;	
					strcpy(RespServiceBuf,"\\\"c27\\\":1");
				}
				else
				{
					printf("\r\nc27 data format incorrect!\r\n");
					Flag.NeedSendResponse = 0;		//数据格式错误不回复
				}
			}

			//设置APN,Usr name,password
			//c28:device2.iotpf.mb.softbank.jp,8883
			else if((p0 = strstr(gprs_content,"c28")) != NULL)
			{

			}
		}
		else
		{
			printf("\r\nRecvice data format incorrect!\r\n");
//			strcpy(RespServiceBuf,"Parameter format incorrect!");	
			Flag.NeedSendResponse = 0;		//数据格式错误不回复
			return;
		}
	}
}

void WIRELESS_Handle(void)
{
	u16 at_timeout_temp;

	if (At_Timeout_Cnt > 0)
	{
		at_timeout_temp = At_Timeout_Cnt;
	}
	else if (Flag.AtInitCmd)
	{
		at_timeout_temp = 10; //初始化指令通常不需要等待很久
	}
	else
	{
		at_timeout_temp = 75;
	}

	if(NoSimCardCnt >= 3)		//如果没有SIM卡，不执行任何AT指令动作
	{
		return;
	}


	//AT指令延时时间到仍没回复，复位模块
	if (AtType != AT_NULL && Flag.WaitAtAck && !AtDelayCnt)
	{
		delay_ms(500);
		printf("\r\n---->wait AT response cnt:");
		printf("%d(%d)\r\n", ResetCnt, AtType);

		if (++ResetCnt > at_timeout_temp) //延时1分钟
		{
			ResetCnt = 0;
			Flag.PsSignalOk = 0;
			if ((DATA == GprsType) || (LOGIN == GprsType))
			{
				GprsDataLen = Mqtt_SendPacket(GprsType);
				EXFLSAH_SaveBreakPoint();
			}
			//3次AT无回应重启模块
			if (++AtTimeOutCnt > 2)
			{
				AtTimeOutCnt = 0;
				At_Timeout_Cnt = 0;
				NeedModuleReset = MODUAL_NOACK;
			}

			AtType = AT_NULL;
			Flag.WaitAtAck = 0;
			AtDelayCnt = 0;
		}
	}
	else
	{
		ResetCnt = 0;
	}

	//AT指令错误处理
	//增加Flag.ModuleWakeUp判断，避免设备在发送休眠指令后，以及唤醒前，程序还是会进入AT_Eorror()发送
	if (Flag.ModuleOn && AtType == AT_NULL && !Flag.WaitAtAck && Flag.WakeUpMode)
	{
		Flag_check();
	}

	//AT指令发送
	if (Flag.ModuleOn && AtType != AT_NULL && !Flag.WaitAtAck)
	{
		if (!AtDelayCnt)
		{

			AT_SendPacket(AtType, AtSendbuf);
			UART_Send(AT_PORT, (u8 *)AtSendbuf, strlen(AtSendbuf));

			AtDelayCnt = 2;
			Flag.WaitAtAck = TRUE;

			printf("\r\nsend AT(%d):\r\n", AtType);
			printf(AtSendbuf);
			memset(AtSendbuf, '\0', SCIBUFLEN);
		}
	}

	if (Flag.PsSignalOk && AtType == AT_NULL && Flag.GprsConnectOk && !Flag.IsUpgrate)
	{
		GPRS_Send_Handle();	
	}
}
