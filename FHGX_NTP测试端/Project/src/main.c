#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "main.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "string.h"
#include "serial_debug.h"
#include "ptpd.h"
#include "share.h"
#include "sys.h"
#include "lcdmessage.h"
#include "usart6_cfg.h"
/************************************调试段码输出printf*******************************************/
#define SYSTEMTICK_PERIOD_MS  10
#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))
#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000
struct __FILE 
{ 
	int handle1; /* Add whatever you need here */ 
};
FILE __stdout;
FILE __stdin;
int fputc(int ch, FILE *f)
{
	return ITM_SendChar(ch);
}

void cpuidGetId(void);
void Read_ConfigData(FigStructData *FigData);
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms *///本地时间
uint32_t LocalTime1 = 0;
uint32_t timingdelay;
volatile sysTime sTime;
volatile Servo Mservo; //用于本地时钟和GPS对时的调整分量指数
Filter	 MofM_filt;			//调整时间数据集的数字滤波器
FigStructData GlobalConfig;
uint32_t mcuID[3];
extern uint8_t CAN1_Mode_Init(void);
extern void ARM_to_FPGA(void);
extern PtpClock G_ptpClock;
extern RunTimeOpts rtOpts;
extern uint8_t synflags;
int 	 oldseconds = 0;


//LED1-运行
//LED2-同步
//LED3-PPS
//LED4-晶振驯服
		
int main(void)
{
	Mservo.ai	  = 16;	//积分时间常数I	 = 16	
	Mservo.ap		= 2;	//比例系数P		   = 2
	MofM_filt.n = 0;				
	MofM_filt.s = 0;		//alpha 0.1-0.4 
	sTime.noAdjust 		 = FALSE;			//允许调整系统时钟
	sTime.noResetClock = FALSE;			//允许重置系统时钟
	RCC_Configuration();
	Read_ConfigData(&GlobalConfig); //注意：需要先读写FLASH，在开硬件中断前
	GPIO_Configuration();
	UART1_Configuration();
	UART6_Configuration();
	EXTI_Configuration();
	NVIC_Configuration();									
	ETH_GPIO_Config();
	ETH_MACDMA_Config();
 	LwIP_Init();
	//PTPd_Init();								
	NTP_Init();
	STM_LEDon(mLED1); 				 			//初始化完成，点亮LED1
	printf("waiting synflags...\n");
  while (1)
		{ 
			handleap(); 				
			Hand_serialSync(); 
			Gx_HandleNTPFIG();
			LwIP_Periodic_Handle(LocalTime);
			NTPClientLoop();
		}
}



void Delay(uint32_t nCount)
{
  timingdelay = LocalTime + nCount;  
  while(timingdelay > LocalTime)
		{     
		}
}

void Time_Update(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}


void Read_ConfigData(FigStructData *FigData)//godin flash配置信息读取
{
	int32_t readflash[10] = {0},i;
	FLASH_Read(FLASH_SAVE_ADDR1,readflash,10);//读出信息到readflash中
	if(readflash[0] == 0xffffffff)//默认配置
		{
			FigData->IPaddr		 = 0xc0a80100|13;	//192.168.1.13 默认IP
			FigData->NETmark	 = 0xffffff00;	
			FigData->GWaddr		 = 0xc0a80101;	
			FigData->DstIpaddr = 0xc0a80100|11;	//192.168.1.11
			FigData->Reidf		 = 0x0;
			FigData->MACaddr[0] = 0x06;
			FigData->MACaddr[1] = 0x04;
			FigData->MACaddr[2] = 0x00;
			FigData->MACaddr[3] = 0xFF;
			cpuidGetId(); 										//读取芯片ID唯一编号，用于取随机MAC地址
			FigData->MACaddr[4] = rand()%255; //通过随机数获取mac地址
			FigData->MACaddr[5] = rand()%255; //通过随机数获取mac地址
			FigData->Reidf			=	0x0;
			FigData->tmp1				=	0x0;
			FigData->MSorStep           = 0; 												//两步发
			FigData->ClockDomain 				= DEFAULT_DOMAIN_NUMBER;		//默认时钟域0
			FigData->WorkMode		 				=	0;												//默认E2E-MAC; 
			FigData->ip_mode	   				= IPMODE_MULTICAST;					//默认组播
			FigData->BroadcastInterval 	= 0;												//广播间隔
			FigData->AnnounceInterval 	= DEFAULT_ANNOUNCE_INTERVAL;				//1
			FigData->AnnounceOutTime		= DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT; //6
			FigData->SyncInterval				=	DEFAULT_SYNC_INTERVAL; 						//0
			FigData->PDelayInterval			= DEFAULT_PDELAYREQ_INTERVAL; 			//1
			FigData->DelayInterval			= DEFAULT_DELAYREQ_INTERVAL; 				//0
			FigData->clockClass 				= 0; 																//时钟等级，用于填写闰秒总数
			FigData->priority1 					= DEFAULT_PRIORITY1; 								//128
			memcpy((int8_t *)readflash,(int8_t *)FigData,20);
			readflash[5]	=	((FigData->WorkMode)<<24)|((FigData->ip_mode)<<16)|((FigData->BroadcastInterval)<<8)|(FigData->tmp1);
			readflash[6]	=	((FigData->MACaddr[0])<<24)|((FigData->MACaddr[1])<<16)|((FigData->MACaddr[2])<<8)|(FigData->MACaddr[3]);
			readflash[7]	=	((FigData->MACaddr[4])<<24)|((FigData->MACaddr[5])<<16)|((FigData->MSorStep)<<8)|(FigData->ClockDomain);
			readflash[8]	=	((FigData->AnnounceInterval)<<24)|((FigData->AnnounceOutTime)<<16)|((FigData->SyncInterval)<<8)|(FigData->PDelayInterval);
			readflash[9]	=	((FigData->DelayInterval)<<24)|((FigData->clockClass)<<16)|((FigData->priority1)<<8)|(FigData->tmp2);
			FLASH_Write(FLASH_SAVE_ADDR1,readflash,10);
			for(i=0;i<1000;i++){}//等待烧写结束	
		}
	else 
		{ //读取到新的配置
			FigData->IPaddr 	= readflash[0];
			FigData->NETmark 	= readflash[1];
			FigData->GWaddr 	= readflash[2];
			FigData->DstIpaddr= readflash[3];
			FigData->Reidf		=	readflash[4];
			
			
			FigData->MACaddr[0] = 0x06;
			FigData->MACaddr[1] = 0x04;
			FigData->MACaddr[2] = 0x00;
			FigData->MACaddr[3] = 0xFF;
			cpuidGetId(); 										//读取芯片ID唯一编号，用于取随机MAC地址
			FigData->MACaddr[4] = rand()%255; //通过随机数获取mac地址
			FigData->MACaddr[5] = rand()%255; //通过随机数获取mac地址
		
			FigData->MSorStep           = 0; 												//两步发
			FigData->ClockDomain 				= DEFAULT_DOMAIN_NUMBER;		//默认时钟域0
			//FigData->WorkMode		 				=	0;												//默认E2E-MAC; 
			FigData->ip_mode	   				= IPMODE_MULTICAST;					//默认组播
			FigData->BroadcastInterval 	= 0;												//广播间隔
			FigData->AnnounceInterval 	= DEFAULT_ANNOUNCE_INTERVAL;				//1
			FigData->AnnounceOutTime		= DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT; //6
			FigData->SyncInterval				=	DEFAULT_SYNC_INTERVAL; 						//0
			FigData->PDelayInterval			= DEFAULT_PDELAYREQ_INTERVAL; 			//1
			FigData->DelayInterval			= DEFAULT_DELAYREQ_INTERVAL; 				//0
			FigData->clockClass 				= 0; 																//时钟等级，用于填写闰秒总数
			FigData->priority1 					= DEFAULT_PRIORITY1; 								//128

			FigData->WorkMode		= (uint8_t)((readflash[5]&0xff000000)>>24);			//ptp工作模式
			FigData->tmp1				= (uint8_t)((readflash[5]&0xff));			//参考时间
			
		}
	printf("FigData->IPaddr=0x%04X\n",FigData->IPaddr);
	printf("FigData->GWaddr=0x%04X\n",FigData->GWaddr);
	printf("FigData->NETmark=0x%04X\n",FigData->NETmark);
	//printf("FigData->MAC=0x%06X\n",FigData->MACaddr);
	printf("FigData->ntpservaddr=0x%04X\n",FigData->DstIpaddr);
	printf("FigData->Reidf=%d\n",FigData->Reidf);
	printf("UTC_Offset=%d\n",FigData->clockClass);	
	printf("tmp1=%d\n",FigData->tmp1);	
			
}


void cpuidGetId(void)
{
	mcuID[0] = *(__IO u32*)(0x1FFF7A10);
	mcuID[1] = *(__IO u32*)(0x1FFF7A14);
	mcuID[2] = *(__IO u32*)(0x1FFF7A18);
	srand((mcuID[0]+mcuID[1])+mcuID[2]); //用作随机数生成的种子
}

void STM_LEDon(uint16_t LEDx)
{
	GPIO_SetBits(GPIOD,LEDx);
}

void STM_LEDoff(uint16_t LEDx)
{
	GPIO_ResetBits(GPIOD,LEDx);
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  while (1)
  {}
}
#endif


