#ifndef USR_TEST_H
#define USR_TEST_H

#define TESTRESULTADDR_0      0x00030000
#define TESTRESULTADDR_1      0x00030000 + 0x20
#define TESTRESULTADDR_2      0x00030000 + 0x40
#define TESTRESULTADDR_3      0x00030000 + 0x60
#ifndef _TEST_
#define _TEST_
typedef struct{
	unsigned char 	InTesting:1;		//设备处于测试中
	unsigned char 	TestReady:1;		//进入测试模式后，需要应答测试准备好
	unsigned char 	TestOver:1;			//测试结束
    unsigned char   GetIMEI:1;          //获取到了IMEI
    unsigned char   GetModuleAti:1;     //获取到了GSM模块版本信息
    unsigned char   ExflashTestOk:1;    //外部flash读写测试OK
    unsigned char   GetSht31Data:1;     //获取到温湿度传感器数据
    unsigned char   GetCo2Date:1;       //获取到二氧化碳传感器数据
    unsigned char   GetGsmCsq:1;        //获取到GSM模块的csq数据
    unsigned char   GetGsmCCID:1;       //获取到GSM模块的CCID
    unsigned char   GetBatVoltage:1;    //获取到电池电压
    unsigned char   GetRiAction:1;      //检测到RI触发
    unsigned char   GetDeviceInfo:1;    //获取到设备配置信息
    unsigned char   GsmModuleOk:1;      //GSM模块测试OK（可以正常识别SIM及搜索到CSQ）
    unsigned char   ExflashTestOver:1;  //外部flash读写测试完成
    unsigned char   NeedCheckATI:1;     //需要查询模块的ATI

    unsigned char   WaitTestCnt;        //Test_Handle执行周期
	unsigned char 	WaitEnterTest;		//开机后等待外部发送指令进入测试模式的窗口时间
    unsigned char 	WaitRiAction;		//等待
	unsigned char 	TestStep;			//测试到哪一步，这个值需要初始化为0xFF
    unsigned char   TestOverStep;       //测试完成到哪一步，配合TestStep来实现不重复发送测试结果
    unsigned char   ShowResultCnt;      //通过LED灯显示测试结果保持时间
}TEST;
#endif

extern TEST Test;

void Test_Init(void);
void Test_Handle(void);
void Test_Receive(void);
#endif
