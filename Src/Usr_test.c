#include "usr_main.h"

TEST Test;
TEST_RESULT Test_Result;
TEST_RESULT Test_Result_Read;

uint8_t Test_Get_Mac[6]    = {0x59,0x01,0x00,0x41,0xD5,0x6F};
uint8_t Test_Get_Scan[6]   = {0x59,0x01,0x00,0x42,0xB6,0x5F};

void Test_Init(void)
{
    Test.TestStep = 0xFF;
    Test.WaitEnterTest = 20;
    UART_Send(USART3,"Device can enter Test mode by send Test command in 20s\r\n",56);
}

u8 Updata_TestResult(void)
{
    memset(&Test_Result_Read,0,sizeof(Test_Result_Read));

    EXFLASH_EraseSector(TESTRESULTADDR_0);
    EXFLASH_WriteBuffer((u8 *) &Test_Result,TESTRESULTADDR_0,sizeof(Test_Result));
    EXFLASH_ReadBuffer((u8 *) &Test_Result_Read,TESTRESULTADDR_0,sizeof(Test_Result_Read));

    if(memcmp(&Test_Result,&Test_Result_Read,sizeof(Test_Result)) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void Test_Receive(void)
{   
    char *p0 = NULL;
    char *p1 = NULL;
    char temp_buf[20] = {0};
    char read_buf[20] = {0};
    char send_buf[150] = {0};
    u8   step = 0;

    if((Test.WaitEnterTest > 0) && (strstr(Uart3Buf, "AT^TST")))
	{
		Test.TestStep = 0;
        Test.InTesting = 1;
        Flag.NeedBtPowerOn = 1;
        memset(&Test_Result,0,sizeof(Test_Result));
        EXFLASH_ReadBuffer((u8 *)&Test_Result,TESTRESULTADDR_0,sizeof(Test_Result));
	}

    //外部IMEI设置：AT^SETIMEI=123456789012345\r\n
    if(strstr(Uart3Buf, "AT^SETIMEI="))    
    {
        p0 = strstr(Uart3Buf, "AT^SETIMEI=");
        p0 += 11;
        p1 = strstr(p0, "\r\n");
        if(p1 - p0 != 15) 
        {
            printf("The IMEI data format error\r\n");
            UART_Send(USART3,"The IMEI data format error\r\n",29); 
            return;
        }  
        strncpy(temp_buf,p0,p1 - p0);
        strcat(temp_buf,"#");     
        EXFLASH_EraseSector(IMEIADDR);
        EXFLASH_WriteBuffer((u8 *)temp_buf,IMEIADDR,strlen(temp_buf));
        EXFLASH_ReadBuffer((u8 *)read_buf,IMEIADDR,strlen(temp_buf));   

        if(memcmp(temp_buf,read_buf,strlen(temp_buf)) == 0)
        {
            p1 = strstr(temp_buf, "#");
            *p1 = 0;                                             //删除“#”
            memset(Test_Result.GetAll,0,sizeof(Test_Result.GetAll));
            EXFLASH_ReadBuffer((u8 *)&Test_Result,TESTRESULTADDR_0,sizeof(Test_Result));
            p1 = strstr(Test_Result.GetAll, "\r\n");
            *p1 = 0; 
        
            if((Test.GetGsmCCID == 0) && (Test.NeedAckReadImei == 0))
            {
                Test.WaitCcidCnt = 15;   
            }

            Test.NeedAckWriteImei = 1;  
        }   
        else
        {
            UART_Send(USART3,"The IMEI Save Failed\r\n",23); 
        }
    }

    //读设置的IMEI
    if(strstr(Uart3Buf, "AT^READIMEI="))
    {
        EXFLASH_ReadBuffer((u8 *)read_buf,IMEIADDR,15);

        memset(Test_Result.GetAll,0,sizeof(Test_Result.GetAll));      
        EXFLASH_ReadBuffer((u8 *)&Test_Result,TESTRESULTADDR_0,sizeof(Test_Result));
 
        p1 = strstr(Test_Result.GetAll, "\r\n");
        *p1 = 0;
 
        if((Test.GetGsmCCID == 0) && (Test.NeedAckReadImei == 0))
        {
            Test.WaitCcidCnt = 15;   
        }

        Test.NeedAckReadImei = 1; 
    }

    if(Test.TestStep == 0xFF)      
    {
        return;
    }

    if(strstr(Uart3Buf, "AT^SET0="))
    {
        p0 = strstr(Uart3Buf, "AT^SET0=");
        p0 += 8;
        p1 = strstr(p0, "\r\n");
        if(p1 - p0 < sizeof(Test_Result.Get0))
        {
            memset(Test_Result.Get0,0,sizeof(Test_Result.Get0));
            strncpy(Test_Result.Get0,p0,p1 - p0);
            if( Updata_TestResult() )
            {
                sprintf(send_buf,"^DEV@SET0=%s\r\n",Test_Result.Get0);
                UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));              
            }
        }
    }
    else if(strstr(Uart3Buf, "AT^SET1="))
    {
        p0 = strstr(Uart3Buf, "AT^SET1=");
        p0 += 8;
        p1 = strstr(p0, "\r\n");
        if(p1 - p0 < sizeof(Test_Result.Get1))
        {
            memset(Test_Result.Get1,0,sizeof(Test_Result.Get1));
            memset(Test_Result.GetAll,0,sizeof(Test_Result.GetAll));
            strncpy(Test_Result.Get1,p0,p1 - p0);
            sprintf(Test_Result.GetAll,"IMEI=%s;ATI=%s;GET0=%s;GET2=%s;GET3=%s;DEVSW=%s;BTMAC=%s\r\n",\
            IMEI_MODULE,GsmRev,Test_Result.Get0,Test_Result.Get2,Test_Result.Get3,Edition_STD,Bt_Info);
            if( Updata_TestResult() )
            {
                sprintf(send_buf,"^DEV@SET1=%s\r\n",Test_Result.Get1);
                UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));              
            }
        }

        //测试完成，清除LED测试模式
        Clear_Leds();	

        if(*p0 == '0')
        {
            LED_NET_RED_ON;
            LED_SENSOR_RED_ON;
        }
        else if(*p0 == '1')
        {
            LED_NET_GREEN_ON;
            LED_SENSOR_GREEN_ON;
        }
        Test.ShowResultCnt = 200;     //通过LED显示测试结果持续200秒
        Test.TestOver = 1;
        Test.InTesting = 0;
    }
    else if(strstr(Uart3Buf, "AT^SET2="))
    {
        p0 = strstr(Uart3Buf, "AT^SET2=");
        p0 += 8;
        p1 = strstr(p0, "\r\n");
        if(p1 - p0 < sizeof(Test_Result.Get2))
        {
            memset(Test_Result.Get2,0,sizeof(Test_Result.Get2));
            strncpy(Test_Result.Get2,p0,p1 - p0);
            if( Updata_TestResult() )
            {
                sprintf(send_buf,"^DEV@SET2=%s\r\n",Test_Result.Get2);
                UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));              
            }
        }
    }
    else if(strstr(Uart3Buf, "AT^SET3="))
    {
        p0 = strstr(Uart3Buf, "AT^SET3=");
        p0 += 8;
        p1 = strstr(p0, "\r\n");
        if(p1 - p0 < sizeof(Test_Result.Get3))
        {
            memset(Test_Result.Get3,0,sizeof(Test_Result.Get3));
            strncpy(Test_Result.Get3,p0,p1 - p0);
            if( Updata_TestResult() )
            {
                sprintf(send_buf,"^DEV@SET3=%s\r\n",Test_Result.Get3);
                UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));              
            }
        }
    }
    else if(strstr(Uart3Buf, "AT^GET0="))
    {
        sprintf(send_buf,"^DEV@GET0=%s\r\n",Test_Result.Get0);
        UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));
    }
    else if(strstr(Uart3Buf, "AT^GET1="))
    {
        sprintf(send_buf,"^DEV@GET1=%s\r\n",read_buf);
        UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));
    }
    else if(strstr(Uart3Buf, "AT^GET2="))
    {
        sprintf(send_buf,"^DEV@GET2=%s\r\n",Test_Result.Get2);
        UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));
    }
    else if(strstr(Uart3Buf, "AT^GET3="))
    {
        sprintf(send_buf,"^DEV@GET3=%s\r\n",Test_Result.Get3);
        UART_Send(USART3,(u8 *)send_buf,strlen(send_buf));
    }

    else if(strstr(Uart3Buf, "AT^NOTE="))
    {
        Test.WaitTestCnt = 0;
        p0 = strstr(Uart3Buf, "AT^NOTE=");
        p0 += 8;
        step = (u8)atoi(p0);

        Test.TestOverStep = 0;

        switch (step)
        {
         case 1:
            Test.TestStep = 1;
            break;
         case 2:
            Test.TestStep = 2;
            break;   
         case 3:
            Test.TestStep = 3;
            break;     
         case 4:
            Test.TestStep = 4;
            break;   
         case 5:
            Test.TestStep = 5;
            break;
         case 6:
            Test.TestStep = 6;
            break;   
         case 7:
            Test.TestStep = 7;
            break;     
         case 8:
            Test.TestStep = 8;
            break;
         case 9:
            Test.TestStep = 9;
            break;
         case 10:
            Test.TestStep = 10;
            break;
         case 11:
            Test.TestStep = 11;
            break;
        default:
            break;
        }
        
    }
}

char SendDataTemp[500];

void Test_Handle(void)
{
    char read_buf[20] = {0};

    memset(SendDataTemp,0,sizeof(SendDataTemp));

    if(Test.NeedAckWriteImei && (Test.GetGsmCCID || Test.WaitCcidCnt == 0))
    {
        Test.NeedAckWriteImei = 0;
        EXFLASH_ReadBuffer((u8 *)read_buf,IMEIADDR,15);
        sprintf(SendDataTemp,"^DEV@SETIMEI=%s;%s;CCID=%s\r\n",read_buf,Test_Result.GetAll,CCID);
        UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp)); 
    }

    if(Test.NeedAckReadImei && (Test.GetGsmCCID || Test.WaitCcidCnt == 0))
    {
        Test.NeedAckReadImei = 0;
        EXFLASH_ReadBuffer((u8 *)read_buf,IMEIADDR,15);
        sprintf(SendDataTemp,"^DEV@SETIMEI=%s;%s;CCID=%s\r\n",read_buf,Test_Result.GetAll,CCID);
        UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp)); 
    }

    if((Test.WaitTestCnt > 0) || (Test.TestOver))
    {
        return;
    }

    Test.WaitTestCnt = 10;

    if(Test.TestOverStep != Test.TestStep)
    {
        switch (Test.TestStep)
        {
            case 1:                     //测试步骤1，输出软件版本及硬件版本    
                sprintf(SendDataTemp,"^TST@SW=%s;HW=%s\r\n",Edition_STD,HardWare_Edition);
                UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                Test.TestOverStep = Test.TestStep;
                
            break;

            case 2:                     //测试步骤2，输出模块版本及IMEI   
                if((Test.GetIMEI) && (Test.GetModuleAti))
                {
                    sprintf(SendDataTemp,"^DEV@IMEI=%s;Module ATI=%s\r\n",IMEI_MODULE,GsmRev);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                    Test.TestOverStep = Test.TestStep;
                }  
                else
                {
                    sprintf(SendDataTemp,"^NOTE@T0=%d\r\n",Test.TestStep);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));               /* code */
                }               
            break;

            case 3:                     //测试步骤3，外部flash测试结果
                if(Test.ExflashTestOver)
                {
                    if(Test.ExflashTestOk)
                    {                        
                        sprintf(SendDataTemp,"^EXFLASH@KEY=1\r\n");
                    }
                    else
                    {
                        sprintf(SendDataTemp,"^EXFLASH@KEY=0\r\n");
                    }
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                    Test.TestOverStep = Test.TestStep;
                }      
            break;

            case 4:                     //测试步骤4，CCID读取
                if(Test.GetGsmCCID) 
                {
                    sprintf(SendDataTemp,"^GSM@CCID=%s\r\n",CCID);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                    Test.TestOverStep = Test.TestStep;
                }      
                else
                {
                    sprintf(SendDataTemp,"^NOTE@T0=%d\r\n",Test.TestStep);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));               /* code */
                }
            break;

            case 5:                     //测试步骤5，sensor测试结果,除RI外
                sprintf(SendDataTemp,"^SENSOR@CO2=%d;TEMP=%.1f*C;HUMI=%.1f%%\r\n",co2_module_value,temperature_value,humidity_value);
                UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                Test.TestOverStep = Test.TestStep;
            break;

            case 6:                     //测试步骤6，RI测试结果
                if(Test.GetRiAction) 
                {
                    sprintf(SendDataTemp,"^RI@KEY=1\r\n");
                }
                else
                {
                    sprintf(SendDataTemp,"^RI@KEY=0\r\n");
                }
                UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                Test.TestOverStep = Test.TestStep;
                    
            break;

            case 7:                     //测试步骤7,读取关键参数,平台地址和端口，APN相关信息
                memset(&Fs, 0, sizeof(Fs));
                STMFLASH_Read(FLASH_SAVE_ADDR, (u32 *)&Fs, (u16)(sizeof(Fs))/4);
                sprintf(SendDataTemp,"^APN@APN=%s;USRNAME=%s;PASSWORD=%s\r\n",Fs.ApnName,Fs.GprsUserName,Fs.GprsPassWord);
                UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                Test.TestOverStep = Test.TestStep;    
            break;

            case 8:                     //测试步骤8,读取电池电压
                sprintf(SendDataTemp,"^BAT@VOLTAGE=%d\r\n",BatVoltage_Adc);
                UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                Test.TestOverStep = Test.TestStep;  
            break;       

            case 9:                     //测试步骤9,读取GSM信号强度
                if(Test.HaveChanegNet) 
                {
                    sprintf(SendDataTemp,"^GSM@CSQ=%s\r\n",Test.CsqValue);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                    Test.TestOverStep = Test.TestStep;
                }      
                else
                {
                    sprintf(SendDataTemp,"^NOTE@T0=%d\r\n",Test.TestStep);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));               /* code */
                }
            break;   

            case 10:            //测试步骤10,读取BT的mac地址(6个字节)+信号强度(1个字节)
                if(Test.GetBtInfo) 
                {
                    sprintf(SendDataTemp,"^BT@DEVMAC=%s;SCANMAC=%s\r\n",Bt_Info,Scan_Mac);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));
                    Test.TestOverStep = Test.TestStep;
                }      
                else
                {
                    sprintf(SendDataTemp,"^NOTE@T0=%d\r\n",Test.TestStep);
                    UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp));               /* code */
                }
            break;   

            case 11:                    //测试步骤11,读取外部flash中设置的IMEI
                EXFLASH_ReadBuffer((u8 *)read_buf,IMEIADDR,15);
                sprintf(SendDataTemp,"^EXFLASH@SETIMEI=%s\r\n",read_buf);
								UART_Send(USART3,(u8 *)SendDataTemp,strlen(SendDataTemp)); 
								Test.TestOverStep = Test.TestStep;
            break;   

            default:
                break;
        }
    }


    if(!Test.ExflashTestOver)
    {
        u8 test_buf[6] = {"Test"};
        u8 test_temp[6] = {0};
        
        Test.ExflashTestOver = 1;

        EXFLASH_EraseSector(0x00020000);
        EXFLASH_WriteBuffer(test_buf,0x00020000,4);
        EXFLASH_ReadBuffer(test_temp,0x00020000,4);
        
        if(memcmp(test_buf,test_temp,4) == 0)
        {
            Test.ExflashTestOk = 1;
        }
        EXFLASH_EraseSector(0x00020000);
    }

    if(Test.TestStep == 0)
	{
		UART_Send(USART3,"^NOTE@T0=?\r\n",12);
        return;
	}

    if(!Test.HaveGetBtMac)
    {
        UART_Send(USART4,Test_Get_Mac,sizeof(Test_Get_Mac));
        return;
    }

    if(!Test.HaveGetBtScan)
    {
        UART_Send(USART4,Test_Get_Scan,sizeof(Test_Get_Scan));
        return;
    }
}

