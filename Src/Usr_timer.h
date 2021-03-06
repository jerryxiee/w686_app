#ifndef USR_TIMER_H
#define USR_TIMER_H

#include "usr_main.h"

#define ACTIVE_TIME 120

extern unsigned char baseTimeSec;
extern unsigned int  baseSecCnt;
extern unsigned int  AtDelayCnt; 

#ifndef USR_RTC
#define USR_RTC
typedef struct {
    unsigned char sec; /* [0, 59] */
    unsigned char min; /* [0,59]  */
    unsigned char hour; /* [0,23]  */
    unsigned char day; /* [1,31]  */
    unsigned char mon; /* [1,12] */
    unsigned char wday; /* [1,7] */
    unsigned short year; /* [0,127] */
}Rtc_st;
#endif

extern Rtc_st Rtc;
extern unsigned short IntervalTemp;
extern unsigned char ResetLeftCnt;
extern unsigned int  Timestamp;	
extern unsigned int  baseSecCnt;
extern unsigned char AT_CBC_IntervalTemp; 
extern unsigned char WaitRestart;
extern unsigned short Start_Fota_Rang;
extern unsigned short System_Rang_Data;

void TIMER_Init(void);
void delay_us(unsigned int us);
void delay_ms(unsigned int us);
void TIMER_AtDelay(unsigned int i);
void TIME_UpdateRtcByNtp(char *pSrc);
void SystemClock_Config(void);
uint32_t covBeijing2UnixTimeStp(Rtc_st *beijingTime,u8 TIMEZONE);
void RTC_Wake_Init(u16 sec);
#endif




