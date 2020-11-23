#include "usr_main.h"

#define USR_TEST_PLAM		0		//使用测试服

AT_TYPE AtType; 					//给AtType赋值的函数要在没有AT指令通信时调用，
									//赋值语句后要有break或return,以免影响同函数其它对AtType的赋值
AT_ERROR AtError;


unsigned char InitCmdTimes;			//初始AT指令时某条指令出错次数，重试三次失败，跳过该指令
unsigned char AtErrorTimes;			//计数某些AT指令连续出错次数，超过一定次数时需要重启操作
unsigned short GprsDataLen;		    //发送GPRS数据长度
unsigned char CheckSimError;		//CPIN检查SIM卡出错次数，连续三次出错会重启模块


char BatValue[5];
unsigned char Rssi; //gsm信号强度原始数据
unsigned short BatVoltage;		//电池电压，这里使用模块供电电压作为电池电压计算电池剩余电量
char CsqValue[12];
char MccMnc[7];
char GsmRev[50];				//GSM模块版本



//向GSM内核发送AT指令前，打包要发送的AT指令
void AT_SendPacket(AT_TYPE temType, char *pDst)
{
	switch (temType)
	{
	case AT_AT:									//测试AT指令
		strcpy(pDst, "AT\r\n");
		break;
	case AT_CPIN:								//查询SIM卡状态
		strcpy(pDst, "AT+CPIN?\r\n");
		break;
	case AT_ATE:								//设置指令回显
		strcpy(pDst, "ATE0\r\n");
		break;
	case AT_CFGRI:								//配置RI脚触发方式
		strcpy(pDst, "AT+CFGRI=1\r\n");
		break;
	case AT_CSQ:								//查询信号强度
		strcpy(pDst, "AT+CSQ\r\n");
		break;
	case AT_CMNB_1:								//设置网络类型，1是仅使用CAT-M
		strcpy(pDst, "AT+CMNB=1\r\n");
		break;
	case AT_CMNB_3:								//设置网络类型，3是CAT-M和NB都使用
		strcpy(pDst, "AT+CMNB=3\r\n");
		break;
	case AT_CMNB_CK:								//查询设置网络类型
		strcpy(pDst, "AT+CMNB?\r\n");
		break;
	case AT_CSCLK:								//开启低功耗模式
		strcpy(pDst, "AT+CSCLK=1\r\n");
		break;
	case AT_CGDCONT:							//设置APN
		#if JP_AT_USE
		sprintf(pDst, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",Fs.ApnName);	
		#else
		strcpy(pDst, "AT\r\n");
		#endif
		break;
	case AT_CNCFG:								//设置APN用户名和密码
		#if JP_AT_USE
		sprintf(pDst, "AT+CGAUTH=1,3,\"%s\",\"%s\"\r\n",Fs.GprsUserName,Fs.GprsPassWord);	
		#else
		strcpy(pDst, "AT\r\n");
		#endif
		break;
	case AT_SLEDS_1:							//设置模块网络LED未注册网络闪烁状态
		strcpy(pDst, "AT+SLEDS=1,64,1000\r\n");	
		break;
	case AT_SLEDS_2:							//设置模块注册上网络LED未开启PPP通道闪烁状态
		strcpy(pDst, "AT+SLEDS=2,64,3000\r\n");	
		break;
	case AT_SLEDS_3:							//设置模块注开启PPP通道闪烁状态
		strcpy(pDst, "AT+SLEDS=3,64,30000\r\n");	
		break;
	case AT_SLEDS:
		strcpy(pDst, "AT+SLEDS?\r\n");	
		break;	
	case AT_CPSI:								//查询基站，MCCMNC等信息
		strcpy(pDst, "AT+CPSI?\r\n");
		break;
	case AT_CCID:								//驱动SIM卡CCID
		strcpy(pDst, "AT+CCID\r\n");
		break;
	case AT_ATI:								//查询模块固件版本
		strcpy(pDst, "AT+SIMCOMATI\r\n");
		break;
	case AT_CNTP_SET:							//设置NTP服务器
		strcpy(pDst, "AT+CNTP=\"ntp1.aliyun.com\",32\r\n");
		break;
	case AT_CNTP:								//同步网络时间
		strcpy(pDst, "AT+CNTP\r\n");
		break;
	case AT_CNDS:								//设置服务域为CS + PS
		strcpy(pDst, "AT+CNDS=2\r\n");
		At_Timeout_Cnt = 100; 
		break;
	case AT_CNDS_CK:							//查询服务域设置
		strcpy(pDst, "AT+CNDS?\r\n");
		break;
	case AT_PSOFTSIM:						//设置使用软卡
		strcpy(pDst, "AT+PSOFTSIM=start,1\r\n");
		break;
	case AT_PSOFTSIM_CK:						//查询使用的是软卡还是硬卡
		strcpy(pDst, "AT+PSOFTSIM=start?\r\n");
		break;	
	case AT_COPS_SET:							//强制注网到NB
		strcpy(pDst, "AT+COPS=1,2,46000\r\n");
		At_Timeout_Cnt = 100; 
		break;
	case AT_CIMI:								//请求国际移动订户身份
		strcpy(pDst, "AT+CIMI\r\n");
		break;
	case AT_CNETLIGHT:							//关闭网络指示灯
		strcpy(pDst, "AT+CNETLIGHT=0\r\n");
		break;
	case AT_CNETLIGHT_ON:						//开启网络指示灯
		strcpy(pDst, "AT+CNETLIGHT=1\r\n");
		break;
	case AT_CCLK:								//查询网络时间
		strcpy(pDst, "AT+CCLK?\r\n");
		break;
	case AT_CGATT:								//网络附着状态
		strcpy(pDst, "AT+CGATT?\r\n");
		break;
	case AT_CGREG:								//查询注网状态
		strcpy(pDst, "AT+CGREG?\r\n");
		break;		
	case AT_CNACT:								//激活场景
		strcpy(pDst, "AT+CNACT=0,1\r\n");
		break;
	case AT_CFSWFILE:							//下载证书文件
		strcpy(pDst, "AT+CFSWFILE=3,\"ca.crt\",0,1358,5000\r\n");
		break;
	case AT_CFSTERM:							//释放缓存文件
		strcpy(pDst, "AT+CFSTERM\r\n");
		break;
	case AT_SMCONF_URL:							//设置MQTT服务器地址和端口
		#if USR_TEST_PLAM
		sprintf(pDst, "AT+SMCONF=\"URL\",\"221.110.245.99\",%s\r\n",Fs.IpPort);	
		#else
		sprintf(pDst, "AT+SMCONF=\"URL\",\"device2.iotpf.mb.softbank.jp\",%s\r\n",Fs.IpPort);	
		#endif
		break;	
	case AT_SMCONF_KEEP:						//设置保持时间
		strcpy(pDst, "AT+SMCONF=\"KEEPTIME\",120\r\n");
		break;
	case AT_SMCONF_CLEANSS: 
		strcpy(pDst, "AT+SMCONF=\"CLEANSS\",1\r\n");
		break;
	case AT_SMCONF_CLIENTID:					//设置clentid
		sprintf(pDst, "AT+SMCONF=\"CLIENTID\",\"%s\"\r\n",IMEI); //20150708_5
		break;
	case AT_SMCONF_PASSWORD:					//设置登入密码  
		strcpy(pDst, "AT+SMCONF=\"PASSWORD\",\"UjvJri4-48fYPAjL#\"\r\n");
		break;
	case AT_SMCONF_QOS: 
		strcpy(pDst, "AT+SMCONF=\"QOS\",0\r\n");
		break;
	case AT_SMCONF_USERNAME:					//设置登入用户名
		#if USR_TEST_PLAM
		sprintf(pDst, "AT+SMCONF=\"USERNAME\",\"C58D391E4-%s\"\r\n",IMEI);
		#else
		sprintf(pDst, "AT+SMCONF=\"USERNAME\",\"CDA68B264-%s\"\r\n",IMEI);
		#endif
		break;
	case AT_CSSLCFG:							//设置服务器使用的是TLS1.2方式连接
		strcpy(pDst, "AT+CSSLCFG=\"sslversion\",0,3\r\n");
		break;
	case AT_CSSLCFG_CONVERT:					//转换CA证书
		strcpy(pDst, "AT+CSSLCFG=CONVERT,2,ca.crt\r\n");
		break;
	case AT_SMSSL:								//设置CA证书
		strcpy(pDst, "AT+SMSSL=1,\"ca.crt\",\"\"\r\n");
		break;
	case AT_SMCONN:								//连接到mqtt服务器
		printf("Connect to serivce...\r\n");
		strcpy(pDst, "AT+SMCONN\r\n");
		At_Timeout_Cnt = 300; 					//连接时间有时需要等待很久，这里最多等待150秒
		break;
	case AT_SMPUB:								//发布消息
	  	if(GprsType != BKDATA)		
		{
			GprsDataLen = Mqtt_SendPacket(GprsType);
			#if USR_TEST_PLAM
			sprintf(pDst,"AT+SMPUB=\"/oneM2M/req/C58D391E4-%s/CSE1000/json\",%d,1,1\r\n",IMEI,GprsDataLen);
			#else
			sprintf(pDst,"AT+SMPUB=\"/oneM2M/req/CDA68B264-%s/CSE1000/json\",%d,1,1\r\n",IMEI,GprsDataLen);
			#endif
		}					
		else
		{
			GprsDataLen = Breakpointleng;
			#if USR_TEST_PLAM
			sprintf(pDst,"AT+SMPUB=\"/oneM2M/req/C58D391E4-%s/CSE1000/json\",%d,1,1\r\n",IMEI,GprsDataLen);
			#else
			sprintf(pDst,"AT+SMPUB=\"/oneM2M/req/CDA68B264-%s/CSE1000/json\",%d,1,1\r\n",IMEI,GprsDataLen);
			#endif
		}
		At_Timeout_Cnt = 300; 					//连接时间有时需要等待很久，这里最多等待150秒
		break;
	case AT_GPRSEND:
		memcpy(pDst, GprsSendBuf, GprsDataLen);
		At_Timeout_Cnt = 300;
		break;	
	case AT_SMSUB:								//订阅消息
		#if USR_TEST_PLAM
		sprintf(pDst, "AT+SMSUB=\"/oneM2M/req/CSE1000/C58D391E4-%s/json\",1\r\n",IMEI);
		#else
		sprintf(pDst, "AT+SMSUB=\"/oneM2M/req/CDA68B264-%s/CSE1000/json\",1\r\n",IMEI);
		#endif
		break;
	case AT_SMUNSUB:							//取消订阅消息
		#if USR_TEST_PLAM
		sprintf(pDst, "AT+SMUNSUB=\"/oneM2M/req/CSE1000/C58D391E4-%s/json\"\r\n",IMEI);
		#else
		sprintf(pDst, "AT+SMUNSUB=\"/oneM2M/req/CDA68B264-%s/CSE1000/json\"\r\n",IMEI);
		#endif
		break;
	case AT_SMDISC:								//断开MQTT连接
		strcpy(pDst, "AT+SMDISC\r\n");
		break;
	case AT_DISCNACT:							//断开无线连接（场景去激活）
		strcpy(pDst, "AT+CNACT=0,0\r\n");
		break;
	case AT_CPOWD:								//关机
		strcpy(pDst, "AT+CPOWD=1\r\n");
		break;
	case AT_CBANDCFG:							//锁定指定频段
		strcpy(pDst, "AT+CBANDCFG=\"CAT-M\",1,8,9\r\n");
		break;
	case AT_CBC:								//查询电池电量
		strcpy(pDst, "AT+CBC\r\n");
		break;
	case AT_GSN:								//查询模块IMEI
		strcpy(pDst, "AT+GSN\r\n");
		break;
	case AT_CFUN_0:								//进入飞行模式
		strcpy(pDst, "AT+CFUN=0\r\n");
		break;	
	case AT_CFUN_1:								//进入正常模式
		strcpy(pDst, "AT+CFUN=1\r\n");
		break;
	case AT_CFUN_4:								//进入正常模式
		strcpy(pDst, "AT+CFUN=4\r\n");
		break;
	case AT_CNVW:								//设置休眠功耗为低功耗
		strcpy(pDst, "at+cnvw=2,\"/datatx/config_tcp_tick_sleep_time\",353030\r\n");
		break;	
	case AT_PSOFTSIM_OTA:						//OTA换卡指令，该指令在需要跟换softsim卡时使用
		strcpy(pDst, "AT+PSOFTSIM=start_ota\r\n");
		break;
	case AT_PSOFTSIM_LIST:						//查询softsim列表
		strcpy(pDst, "AT+PSOFTSIM=get_profile_list\r\n");
		break;

	case AT_HTTPTOFS:							//http获取指定网页文件
		sprintf(pDst, "AT+HTTPTOFS=\"%s%s\",\"/custapp/DownloadApp.bin\",500,10\r\n",FsUpg.AppIpAdress,FsUpg.AppFilePath);
		break;
	case AT_HTTPTOFSRL:							//查询文件下载状态和进度
		strcpy(pDst, "AT+HTTPTOFSRL?\r\n");
		break;	
	case AT_CFSINIT:							//初始化文件系统，为读取数据做准备
		strcpy(pDst, "AT+CFSINIT\r\n");
		break;
	case AT_CFSRFILE:							//读取文件内容
		sprintf(pDst, "AT+CFSRFILE=0,\"DownloadApp.bin\",1,1024,%d\r\n",1024*UpgInfo.UpgPacketNums);
		break;

	case AT_FILE_WRITE:							//软银根证书写入
		strcpy(pDst,\
"-----BEGIN CERTIFICATE-----\r\n\
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\r\n\
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\r\n\
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\r\n\
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\r\n\
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\r\n\
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\r\n\
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\r\n\
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\r\n\
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\r\n\
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\r\n\
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\r\n\
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\r\n\
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\r\n\
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\r\n\
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\r\n\
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\r\n\
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\r\n\
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\r\n\
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\r\n\
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\r\n\
-----END CERTIFICATE-----\r\n");	
	break;

	default:
		break;
	}
}

unsigned char AT_InitReceive(AT_TYPE *temType, char *pSrc)
{
//	char *ptem = NULL;
//	char *p1 = NULL;
	unsigned char back = 0;
	if (strstr(pSrc, "OK") || InitCmdTimes >= 7)
	{
		back = 1;
		InitCmdTimes = 0;
		AtDelayCnt = 0;
		if (*temType == AT_ATE)
		{
			*temType = AT_AT;
		}
		else if (*temType == AT_AT)
		{
		// 	*temType = AT_CFUN_4;
		// }
		// else if (*temType == AT_CFUN_4)
		// {
		// 	*temType = AT_CFUN_1;
		// }
		// else if (*temType == AT_CFUN_1)
		// {
			*temType = AT_ATI;
		}
		else if (*temType == AT_ATI)
		{
	#if JP_AT_USE
		 	*temType = AT_CGDCONT;
		}
		else if (*temType == AT_CGDCONT)
		{
		 	*temType = AT_CNCFG;
		}
		else if (*temType == AT_CNCFG)
		{
			*temType = AT_CMNB_1;
		}
		else if (*temType == AT_CMNB_1)
		{	
	#else
		*temType = AT_CMNB_3;
		}
		else if (*temType == AT_CMNB_3)
		{	
	#endif
			*temType = AT_CFGRI;
		}
		else if (*temType == AT_CFGRI)
		{
			*temType = AT_CSCLK;
		}
		else if (*temType == AT_CSCLK)
		{
			*temType = AT_SLEDS_1;
		}
		else if (*temType == AT_SLEDS_1)
		{
			*temType = AT_SLEDS_2;
		}
		else if (*temType == AT_SLEDS_2)
		{
			*temType = AT_SLEDS_3;
		}
		else if (*temType == AT_SLEDS_3)
		{
			*temType = AT_CNETLIGHT_ON;
		}

		else if (*temType == AT_CNETLIGHT_ON)
		{
			*temType = AT_SLEDS;
		}
	
		else if (*temType == AT_SLEDS)
		{
			*temType = AT_CCID;
		}
		else if (*temType == AT_CCID)
		{
			*temType = AT_CNVW;
		}
		else if (*temType == AT_CNVW)
		{
			#if USE_SOFTSIM
			*temType = AT_CMNB_CK;
			#else
			*temType = AT_CGREG;
			#endif
			Flag.AtInitCmd = 0;
			Flag.AtInitFinish = 1;
		}
	}
	else if (strstr(pSrc, "ERROR"))
	{
		back = 1;
		AtDelayCnt = 5;
		InitCmdTimes++;
	}
	return back;
}

//处理GSM内核回应AT指令的数据
unsigned char AT_Receive(AT_TYPE *temType, char *pSrc)
{
	char *ptem,*p1;
	unsigned short i;
	char temp[10] = {0};
	unsigned char back = 0;
	static unsigned char error1 = 0, error3 = 0;
	switch (*temType)
	{
	case AT_CPIN:
		if (strstr(pSrc, "READY"))
		{
			AtDelayCnt = 0;
			back = 1;
			*temType = AT_NULL;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
			CheckSimError++;
			if (CheckSimError > 3)
			{
				CheckSimError = 0;
				NeedModuleReset = NO_SIMCARD;
			}
		}
		break;

	case AT_ATI:
		//提取模块的版本信息
		if ((p1 = strstr(pSrc, "APRev:")) != NULL)
		{
			p1 += 6;
			memset(GsmRev,0,sizeof(GsmRev));
			ptem = strstr(p1, "\r\n");

			if(ptem - p1 > sizeof(GsmRev))
			{
				strncpy(GsmRev,p1,ptem - p1);
				Test.GetModuleAti = 1;
			}
		}
		//提起模块的IMEI
		if ((p1 = strstr(pSrc, "IMEI:")) != NULL)
		{
			p1 += 5;
			memset(IMEI,0,sizeof(IMEI));
			ptem = strstr(p1, "\r\n");

			if(ptem - p1 > sizeof(IMEI))
			{
				strncpy(IMEI,p1,ptem - p1);
				Test.GetIMEI = 1;
			}
		}	
		break;

	case AT_CCID:
		if ((p1 = strstr(pSrc, "OK")) != NULL)
		{
			Flag.HaveGetCCID = 1;
			Test.GetGsmCCID = 1;

			p1 = strstr(pSrc, "\r\n");

			memset(CCID, '\0', 21);
			p1 += 2;
			strncpy(CCID, p1, 20);		
		}

		back = 1;
		AtDelayCnt = 0;
		*temType = AT_NULL;
		break;
	case AT_CNTP_SET:
		if ((p1 = strstr(pSrc, "OK")) != NULL)
		{
			AtDelayCnt = 0;
			*temType = AT_CNTP;
			back = 1;		
		}
		else
		{
			AtDelayCnt = 0;
			*temType = AT_NULL;
			back = 1;
		}
		break;

	case AT_CNTP:
		*temType = AT_NULL;
		AtDelayCnt = 0;
		back = 1;

		break;

	case AT_CCLK:
		//+CCLK: "20/09/03,18:12:37+32"，
		if ((p1 = strstr(pSrc, "+CCLK:")) != NULL)
		{
			if ((p1 = strstr(pSrc, "80/01")) != NULL) //不保存初始时间
			{
				AtDelayCnt = 0;
				*temType = AT_NULL;
				back = 1;
				break;
			}
			TIME_UpdateRtcByNtp(pSrc);

			AtDelayCnt = 0;
			*temType = AT_NULL;
			back = 1;
		}
		else if (strstr(pSrc, "OK"))
		{
			back = 1;
		}
		else
		{
			AtDelayCnt = 0;
			*temType = AT_NULL;
			back = 1;
		}
		break;

	case AT_CGREG:
		if (strstr(pSrc, "+CGREG:"))
		{
			if (strstr(pSrc, ",1") || strstr(pSrc, ",5"))
			{

				if ((Flag.GprsConnectOk)||(ConnectDelayCnt > 0))				//如果网络连接，那只是周期性检查网络，不处理
				{
					*temType = AT_NULL;
				}
				else if(!Flag.IsContextAct)
				{
					printf("Have register on the net,ready connect service...\r\n");
					*temType = AT_CNACT;			//网络未连接，激活场景
				}
				else
				{
					*temType = AT_CSSLCFG;
				}
				
				Flag.PsSignalOk = 1; 
				AtDelayCnt = 0;
				AtError.PsSingalEorCnt = 0; 
											
			}
			else
			{
				*temType = AT_NULL;
				Flag.PsSignalOk = 0; 			
				AtError.PsSingalEorCnt++;	

				if (AtError.PsSingalEorCnt > 15) 
				{
					AtError.PsSingalEorCnt = 0;

					if (!Flag.InNoSignNoRstMd)				//如果不是处于认定没有信号的地方，重启模块
					{
						NeedModuleReset = CANT_ATTACH_NET;
					}		
				}

				if (Flag.Insleeping && Flag.InNoSignNoRstMd) //设备处于没有信号的地方，查询没有网络，直接进入休眠
				{
					ActiveTimer = 4;
	//				Flag.SendHandInsleeping = 0;
				}
			}
		}
		else if (strstr(pSrc, "OK"))
		{
			*temType = AT_NULL;
		}

		Flag.PsSignalChk = 0;
		back = 1;

		break;


	case AT_CNACT:
		if(strstr(pSrc, "ACTIVE"))
		{
			*temType = AT_CSSLCFG;

			Flag.IsContextAct = 1;
			AtDelayCnt = 0;
			back = 1;
		}
		else if (strstr(pSrc, "OK"))
		{
			back = 0;
		}
		else if ((strstr(pSrc, "ERROR"))||(strstr(pSrc, "DEACTIVE")))
		{
			if (Flag.IsContextAct) //场景已经激活
			{
				*temType = AT_SMCONN;
			}
			else
			{
				*temType = AT_NULL;
				AtErrorTimes++;
				if (AtErrorTimes > 5)
				{
					NeedModuleReset = 8;
				}					
			}
			AtDelayCnt = 0;
			back = 1;
		}

		break;

	case AT_CFSINIT:
		if(Flag.IsUpgrate)
		{
			if (strstr(pSrc, "OK"))
			{
				*temType = AT_CFSRFILE;
				AtDelayCnt = 0;
				back = 1;
			}

			else if (strstr(pSrc, "ERROR"))
			{
				*temType = AT_NULL;
				Flag.IsUpgrate = 0;
				back = 1;
			}
		}
		else
		{
			// if (strstr(pSrc, "OK"))
			// {
				*temType = AT_CFSWFILE;
				AtDelayCnt = 0;
				back = 1;
			// }

			// else if (strstr(pSrc, "ERROR"))
			// {
			// 	*temType = AT_NULL;
			// 	AtErrorTimes++;
			// 	if (AtErrorTimes > 10)
			// 	{
			// 		NeedModuleReset = CONNECT_SERVICE_FAILED;
			// 	}
			// 	back = 1;
			// }
		}

		break;

	case AT_CSSLCFG:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_CFSINIT;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_CFSWFILE:
		if (strstr(pSrc, "DOWNLOAD"))
		{
			*temType = AT_FILE_WRITE;
			AtDelayCnt = 0;
			back = 1;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_FILE_WRITE:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_CFSTERM;
			AtDelayCnt = 0;
			back = 1;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_CFSTERM:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_URL;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONF_URL:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_QOS;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONF_QOS:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_KEEP;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;	

	case AT_SMCONF_KEEP:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_CLEANSS;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;	


	case AT_SMCONF_CLEANSS:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_CLIENTID;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONF_CLIENTID:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_PASSWORD;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONF_PASSWORD:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONF_USERNAME;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONF_USERNAME:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_CSSLCFG_CONVERT;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_CSSLCFG_CONVERT:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMSSL;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMSSL:
		if (strstr(pSrc, "OK"))
		{
			*temType = AT_SMCONN;
			AtDelayCnt = 0;
			back = 1;
		}

		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtErrorTimes++;
			if (AtErrorTimes > 10)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_SMCONN:
		if (strstr(pSrc, "OK"))
		{
			printf("Connect to service success\r\n");
			Flag.GprsConnectOk = 1;
			AtErrorTimes = 0;				//只有在成功连接到服务器后，才清零
			*temType = AT_SMSUB;
			error3 = 0;
			back = 1;
		}

		else
		{
			ConnectDelayCnt = 15;
			printf("Connect to service failed,ready disactive app network\r\n");
			*temType = AT_NULL;
			error3++;
			if (error3 > 5)
			{
				error3 = 0;
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		At_Timeout_Cnt = 0; 

		break;


	case AT_SMPUB:

		if (strstr(pSrc, ">"))
		{
			*temType = AT_GPRSEND;
			AtDelayCnt = 0;
			error1 = 0;
			back = 1;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			Flag.NeedCloseGprs = 1;

			if (DATA == GprsType) 
			{
				GprsDataLen = Mqtt_SendPacket(GprsType);
				EXFLSAH_SaveBreakPoint();
			}
			
			error1++;
			if (error1 > 5)
			{
				error1 = 0;
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}

			back = 1;
		}
		At_Timeout_Cnt = 0; 

		break;

	case AT_GPRSEND:
		if (strstr(pSrc, "OK"))
		{
			printf("Data send complete\r\n");
			//如果是休眠期间周期性唤醒时上传数据成功，ActiveTimer = 2即刻进入休眠
			if((Flag.Insleeping) || (GprsSend.posCnt == 0))
			{
				ActiveTimer = 3;
			}
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}
		else if(strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			printf("Data send failed!\r\n");
			if (DATA == GprsType) 
			{
				GprsDataLen = Mqtt_SendPacket(GprsType);
				EXFLSAH_SaveBreakPoint();
			}
			Flag.NeedCloseGprs = 1;
			AtDelayCnt = 0;
			back = 1;
		}
		
		if(WaitRestart > 0)
		{
			WaitRestart = 1;	
		}
		break;

	case AT_SMSUB:
		if (strstr(pSrc, "OK") || strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}
		break;


	case AT_SMDISC:
		if (strstr(pSrc, "OK"))
		{
			printf("Disconnect to service\r\n");
			Flag.GprsConnectOk = 0;
			Flag.ConNet = 0; 

			*temType = AT_NULL;
			AtDelayCnt = 5;	
			back = 1;
		}
		else
		{
			*temType = AT_DISCNACT; //AT_SMDISC关不掉的，用AT_DISCNACT来关
			AtDelayCnt = 0;
			back = 1;
		}
		break;


	case AT_DISCNACT:
		if (strstr(pSrc, "OK"))
		{
			printf("App network disactived\r\n");
			ConnectDelayCnt = 5;
			Flag.GprsConnectOk = 0;
			Flag.ConNet = 0; 
			Flag.IsContextAct = 0;

			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			printf("App network disactive failed!\r\n");

			Flag.GprsConnectOk = 0;
			Flag.ConNet = 0; 
			Flag.IsContextAct = 0;

			AtErrorTimes++;
			if (AtErrorTimes > 3)
			{
				NeedModuleReset = CONNECT_SERVICE_FAILED;
			}
			back = 1;
		}
		break;

	case AT_CPSI:
	{
		//+CPSI: LTE NB-IOT,Online,460-00,0x1D2B,47393342,9,EUTRAN-BAND8,3686,0,0,-10,-62,-52,18
		if ((p1 = strstr(pSrc, "+CPSI:")) != NULL)
		{
			AtDelayCnt = 0;
			back = 1;
			*temType = AT_NULL;

			ptem = strstr(pSrc, ",");

			if(ptem != NULL)
			{
				ptem ++;
				ptem = strstr(ptem, ",");			//指针指向第二个逗号
				p1 = ptem + 1;						//p1指向MCC的第一个数

				//判断MCCMNC格式是否为xxx-xx
				ptem = strstr(ptem, "-");
				if((ptem - p1) != 3)
				{
					break;
				}
			}

			if (ptem != NULL)
			{
				strncpy(MccMnc, p1, 3);					//拷贝3位的MCC
				p1 += 4;
				strncpy(&MccMnc[3], p1, 2);				//拷贝2位的MNC
			}
		}
		else 
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}
	}
	break;


	case AT_CBC:
		if (strstr(pSrc, "+CBC:"))
		{
			static u8 low_bat_times = 0;

			*temType = AT_NULL;

			ptem = strrchr(pSrc, ',') + 1;
			memset(BatValue, '\0', 5);
			strncpy(BatValue, ptem, 4);
			
			BatVoltage = (u16)Usr_Atoi(BatValue);
			Test.GetBatVoltage = 1;
			if (strcmp(BatValue, "3450") < 0)
			{
				AT_CBC_IntervalTemp = 5;		//检测到电池电量低时，加快检测周期，尽快确认状态
				low_bat_times ++;
				if(low_bat_times > 5)
				{
					Flag.BattLow = 1;
				}
			}			
			else if((Flag.BattLow)&&(strcmp(BatValue, "3650") > 0))
			{
				AT_CBC_IntervalTemp = 20;
				low_bat_times = 0;
				Flag.BattLow = 0;
			}			
			back = 1;
		}
		else if (strstr(pSrc, "OK"))
		{
			back = 1;
		}
		else if (strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			back = 1;
		}
		break;

	case AT_GSN:
		if (strstr(pSrc, "OK"))
		{
			ptem = strstr(pSrc,"\r\n");
			if(ptem != NULL)
			{
				ptem += 2;
				if((*ptem >= '0')&&(*ptem <= '9'))
				{
					memset(IMEI,0,16);
					if(strlen(IMEI_MANUAL) == 15)
					{
						strcpy(IMEI,IMEI_MANUAL);
					}
					else
					{
						strncpy(IMEI,ptem,15);
						if(strcmp(Fs.DeviceImei,IMEI) != 0)
						{
							strncpy(Fs.DeviceImei,IMEI,sizeof(Fs.DeviceImei));
							Flag.NeedUpdateFs = 1;
						}
					}
				}
			}

			*temType = AT_NULL;		
			back = 1;
		}
		else
		{
			*temType = AT_NULL;
			back = 1;
		}
		break;

	case AT_CSQ:
		if ((p1 = strstr(pSrc, "+CSQ:")) != NULL)
		{
			memset(CsqValue, '\0', 12);
			memset(temp, '\0', 10);
			p1 += 5;
			if (*p1 == ' ')
				p1++;
			ptem = strchr(p1, ',');
			strncpy(temp, p1, (ptem - p1));
			i = Usr_Atoi(temp);
			Rssi = i;
			if (i == 99)
			{
				strcpy(CsqValue, "csq unknown");
			}
			else
			{
				i = 113 - (2 * i);

				CsqValue[0] = '-';
				Itoa(i, CsqValue + 1);
				strcat(CsqValue, "dBm");
				Test.GetGsmCsq = 1;
			}
			*temType = AT_NULL;
			back = 1;
		}
		else if (strstr(pSrc, "OK") || strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			back = 1;
		}

		break;
	case AT_ATE:
		if (strstr(pSrc, "OK") || strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}

		break;

	case AT_AT:
		if (strstr(pSrc, "OK") || strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;
		}
		break;

	case AT_CMNB_3:
		if(strstr(pSrc, "OK"))
		{
			*temType = AT_CNDS_CK;
		}
		else 
		{
			*temType = AT_NULL;
		}
		AtDelayCnt = 0;
		back = 1;
		
		break;

	case AT_CMNB_CK:
		if(strstr(pSrc, "+CMNB: 3"))
		{
			*temType = AT_CNDS_CK;
		}
		else 
		{
			*temType = AT_CMNB_3;
		}
		AtDelayCnt = 0;
		back = 1;
		
		break;

	case AT_CNDS:
		if(strstr(pSrc, "OK"))
		{
			*temType = AT_PSOFTSIM_CK;
		}
		else 
		{
			*temType = AT_NULL;
		}
		AtDelayCnt = 0;
		back = 1;
		
		break;

	//+CNDS: 2
	case AT_CNDS_CK:
		if(strstr(pSrc, "+CNDS: 2"))
		{
			*temType = AT_PSOFTSIM_CK;
		}
		else 
		{
			*temType = AT_CNDS;
		}
		AtDelayCnt = 0;
		back = 1;

		break;

	case AT_PSOFTSIM:
		if(strstr(pSrc, "OK"))
		{
			*temType = AT_NULL;
		}
		else 
		{
			*temType = AT_NULL;
		}
		AtDelayCnt = 0;
		back = 1;
		
		break;

	case AT_PSOFTSIM_CK:
		if(strstr(pSrc, "+PSOFTSIM:1"))
		{
			*temType = AT_NULL;
		}
		else 
		{
			*temType = AT_PSOFTSIM;
		}
		AtDelayCnt = 0;
		back = 1;
		break;

	case AT_PSOFTSIM_OTA:
		if(strstr(pSrc, "+PSOFTSIM:3,refresh"))
		{
			*temType = AT_PSOFTSIM_LIST;
			AtDelayCnt = 0;
			back = 1;
		}
		else if(strstr(pSrc, "OK"))
		{
			*temType = AT_PSOFTSIM_LIST;
			AtDelayCnt = 0;
			back = 1;
		}
		else if(strstr(pSrc, "ERROR"))
		{
			*temType = AT_NULL;
			AtDelayCnt = 0;
			back = 1;			
		}

		break;

	case AT_PSOFTSIM_LIST:

		*temType = AT_NULL;
		AtDelayCnt = 0;
		back = 1;
		break;

	case AT_HTTPTOFS:	
		if(strstr(pSrc, "OK"))
		{
			Flag.IsUpgrate = 1;
			*temType = AT_NULL;
		}
		else 
		{
			*temType = AT_NULL;
		}
		AtDelayCnt = 0;
		back = 1;
		break;	

	case AT_HTTPTOFSRL:							//查询文件下载状态和进度
		*temType = AT_NULL;
		AtDelayCnt = 0;
		back = 1;
		break;


	case AT_CFSRFILE:

		if((strstr(pSrc, "+CFSRFILE:"))&&(UpgInfo.AppReadComplete == 0))
		{
			WIRELESS_UpgradeReceive(pSrc);		
		}	
		else						//文件已经读取完成
		{
			*temType = AT_NULL;
			UpgInfo.UpgrateFail = 1;
		}
		AtDelayCnt = 0;
		back = 1;	
		break;

	default:
		break;
	}

	if (*temType == AT_NULL)
	{
		AtDelayCnt = 0;
	}
	return back;
}


void Flag_check(void)
{
	if (AtType != AT_NULL)
	{
		return;
	}
	//模块不响应AT指令时 只发AT测试	
	if (AtTimeOutCnt)
	{
		AtType = AT_AT;
		return;
	}

	if (Flag.SendAtWithoutRDY)
	{
		Flag.SendAtWithoutRDY = 0;
		Flag.PsSignalOk = 0; 
		Flag.WaitAtAck = FALSE;
		Flag.HaveSmsReady = 1;		//将SMSREADY设置为1，避免后面SMSREADY报上来，又重新初始化一遍

		AtType = AT_ATE; 	//开始初始化模块
		Flag.AtInitCmd = 1;
		printf("Haven't recvive SMS Ready from GSM module,start AT command without SMS Ready\r\n");
		return;
	}

	if(Flag.AtInitFinish == 0)				//模块没有初始化完成前，不查询下面内容
	{
		return;
	}

	if (Flag.NeedCloseGprs)
	{
		Flag.NeedCloseGprs = 0;
		AtType = AT_SMDISC;
		printf("\r\nDisconnect the connect to service!\r\n");
		return;
	}

	if(UpgInfo.NeedCheckUploadState)
	{
		UpgInfo.NeedCheckUploadState = 0;
		AtType = AT_HTTPTOFSRL;
		return;
	}

	if(UpgInfo.AppDownloadOk)
	{
		UpgInfo.AppDownloadOk = 0;
		AtType = AT_CFSINIT;
		return;		
	}

	if(Flag.IsUpgrate)
	{
		return;
	}

	//定时检查ps是否注册
	if (Flag.PsSignalChk && !Flag.IsUpgrate)
	{
		AtType = AT_CGREG;
		return;
	}

	//联网之后查询到网络注册丢失，断开连接
	if (!Flag.PsSignalOk && Flag.GprsConnectOk)
	{
		AtType = AT_SMDISC;
		printf("\r\nDisconnect the Service because of attch to net failed!\r\n");
		return;
	}

	//查电量
	if (Flag.BatChk && !Flag.IsUpgrate)
	{
		Flag.BatChk = 0;
		AtType = AT_CBC;
		return;
	}

	//查信号强度
	if (Flag.CsqChk && !Flag.IsUpgrate)
	{
		Flag.CsqChk = 0;
		AtType = AT_CSQ;
		return;
	}

	if(Flag.NeedSetNtp && Flag.PsSignalOk)
	{
		Flag.NeedSetNtp = 0;
		AtType = AT_CNTP_SET;
		return;
	}

	if (Flag.NtpGetCCLK)
	{
		Flag.NtpGetCCLK = 0;
		AtType = AT_CCLK;
		return;
	}

	if (Flag.NeedGetIMEI)
	{
		Flag.NeedGetIMEI = 0;
		AtType = AT_GSN;
		return;
	}



	if (Flag.NeedcheckCCID)
	{
		Flag.NeedcheckCCID = 0;
		AtType = AT_CCID;
		return;
	}

	if (Flag.NeedCheckSIM && !Flag.PsSignalOk)
	{
		Flag.NeedCheckSIM = 0;
		AtType = AT_CPIN;
		return;
	}

	if (Flag.NeedChangeSoftSim  && Flag.GprsConnectOk)
	{
		Flag.NeedChangeSoftSim = 0;
		AtType = AT_PSOFTSIM_OTA;
		return;
	}

	//网络激活状态，并且已经给服务器应答开始升级指令，可以开始升级
	if((UpgInfo.NeedUpdata)&&(Flag.IsContextAct)&&(!Flag.NeedResponseFrist))
	{
		UpgInfo.NeedUpdata = 0;
		AtType = AT_HTTPTOFS;
		return;
	}

	if (Test.NeedCheckATI)
	{
		Test.NeedCheckATI = 0;
		AtType = AT_ATI;
		return;
	}

}

