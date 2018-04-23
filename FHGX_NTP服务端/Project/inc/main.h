#ifndef __MAIN_H
#define __MAIN_H
#ifdef __cplusplus
 extern "C" {
#endif
#include "stm32f4xx.h"
#include "stm32f4x7_eth_bsp.h"
#include "serial_hand.h"
	 
//功能开关	-----------------------------------------
	 
//#define Device_FH1000 		//时间同步设备
#define Device_FHGX 		//测试仪同步设备
//#define	Gx_PTP_Device	 	//FHGX_PTP测试模块
//#define Device_Slave 	//FH1100PTP从
//#define PTP_Device	 	 
#define NTP_Server	 
//#define NTP_Client	
//#define NTP_Philips	 	 
//#define FreqMassage
	 
//功能开关	-----------------------------------------
	   
	 
#define myprintf printf	 
#define mLED1 GPIO_Pin_11
#define mLED2 GPIO_Pin_12
#define mLED3 GPIO_Pin_13
#define mLED4 GPIO_Pin_14
//LED1-运行
//LED2-同步
//LED3-PPS
//LED4-晶振驯服

#define Default_System_Time    1388592000

typedef struct FigStruct//CAN配置信息结构
{             
	uint32_t IPaddr;  		//本机IP
	uint32_t NETmark;			//本机子网掩码

	uint32_t GWaddr;			//本机网关
	uint32_t DstIpaddr;		//对端IP

	uint32_t Reidf;				//参考源
	
	uint8_t  WorkMode;		//ptp工作模式
	uint8_t  ip_mode;			//NTP网络模式(单播组播广播)
	uint8_t	 BroadcastInterval; //NTP广播间隔
	uint8_t	 tmp1;				//备用

	uint8_t  MACaddr[6];	//本机MAC地址
	uint8_t  MSorStep;		//一步法两步法	
	uint8_t  ClockDomain;	//时钟域
	

	uint8_t AnnounceInterval;	
	uint8_t AnnounceOutTime;	 
	uint8_t SyncInterval;			
	uint8_t PDelayInterval;		
	uint8_t DelayInterval;
	uint8_t clockClass;
	uint8_t priority1;
	uint8_t	tmp2;
}FigStructData;


void Time_Update(void);
void Delay(uint32_t nCount);
void STM_LEDon(uint16_t LEDx);
void STM_LEDoff(uint16_t LEDx);


#ifdef __cplusplus
}
#endif

#endif

