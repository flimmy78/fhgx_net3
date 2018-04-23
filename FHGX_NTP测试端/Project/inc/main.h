#ifndef __MAIN_H
#define __MAIN_H
#ifdef __cplusplus
 extern "C" {
#endif
#include "stm32f4xx.h"
#include "stm32f4x7_eth_bsp.h"
#include "serial_hand.h"
	 
//���ܿ���	-----------------------------------------
	 
//#define Device_FH1000 		//ʱ��ͬ���豸
#define Device_FHGX 		//������ͬ���豸
//#define	Gx_PTP_Device	 	//FHGX_PTP����ģ��
//#define Device_Slave 	//FH1100PTP��
//#define PTP_Device	 	 
//#define NTP_Server	 
#define NTP_Client	
//#define NTP_Philips	 	 
//#define FreqMassage
	 
//���ܿ���	-----------------------------------------
	   
	 
#define myprintf printf	 
#define mLED1 GPIO_Pin_11
#define mLED2 GPIO_Pin_12
#define mLED3 GPIO_Pin_13
#define mLED4 GPIO_Pin_14
//LED1-����
//LED2-ͬ��
//LED3-PPS
//LED4-����ѱ��

#define Default_System_Time    1388592000

typedef struct FigStruct//CAN������Ϣ�ṹ
{             
	uint32_t IPaddr;  		//����IP
	uint32_t NETmark;			//������������

	uint32_t GWaddr;			//��������
	uint32_t DstIpaddr;		//�Զ�IP

	uint32_t Reidf;				//�ο�Դ
	
	uint8_t  WorkMode;		//ptp����ģʽ
	uint8_t  ip_mode;			//NTP����ģʽ(�����鲥�㲥)
	uint8_t	 BroadcastInterval; //NTP�㲥���
	uint8_t	 tmp1;				//����

	uint8_t  MACaddr[6];	//����MAC��ַ
	uint8_t  MSorStep;		//һ����������	
	uint8_t  ClockDomain;	//ʱ����
	

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

