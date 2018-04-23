#include "usart6_cfg.h"
extern FigStructData GlobalConfig;
extern RunTimeOpts rtOpts;
extern PtpClock  G_ptpClock;
#include "ntp.h"


extern unsigned int TEST_TSSM_FLAG;
//extern sysTime sTime;
////extern struct ADDressInfo addrInfo;
//extern Servo Mservo;
//extern Filter MofM_filt;
//extern TimeInternal NtplocalTime;
//static void Mfilter(Integer32 * nsec_current, Filter * filt);
//unsigned char synflags;
////Integer32 abjbuf[5];


//#ifdef Gx_PTP_Device
//static const uint8_t FIG_Lenth = 40;
//struct ConfigPTPData Gx_PTPFIG;
//static unsigned char Gx_PTPFIG_OK;

//void USART6_IRQHandler(void)
//{
//	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//检查是否是指定的中断发生
//		{
//			uint8_t Res;
//			static int16_t Revlen=0;
//			Res =USART_ReceiveData(USART6);//读取接收到的数据
//			if(Gx_PTPFIG_OK!=1)
//				{
//					if(Res==0x2a)
//						{		
//							Revlen=0;
//						}
//					else if(Res==0x23)
//						{
//							if((Revlen==(FIG_Lenth-1))&&(Gx_PTPFIG.frame_head==0x2a)&&(Gx_PTPFIG.cmd_type==0x1))
//								{
//									Gx_PTPFIG_OK=1;
//								}
//							else
//								{
//									Revlen=0;
//								}
//						}	
//					*((unsigned char*)(&Gx_PTPFIG)+Revlen) = Res;		
//					Revlen=(Revlen+1)%FIG_Lenth;	
//					
//				}
//			USART_ClearITPendingBit(USART6,USART_IT_RXNE);//清除接收中断标记(读的时候已经清了，不是必须的)
//		}
//}

//void Gx_HandlePTPFIG(void)
//{
//	if(Gx_PTPFIG_OK == 1)  //ptp串口配置信息接收完成标记(USART6接收)
//	{
//		uint8_t mode=0;
//		printf("cmd_type=%d",Gx_PTPFIG.cmd_type);
//		if (G_ptpClock.portState != PTP_INITIALIZING)
//			{	
//				if(Gx_PTPFIG.E2E_mode==TRUE && Gx_PTPFIG.ethernet_mode==TRUE) 
//					{
//						mode = 0;//E2E MAC
//						rtOpts.transport 			= IEEE_802_3;	//0:E2E-MAC
//						rtOpts.delayMechanism = E2E; 				
//					}
//				else if(Gx_PTPFIG.E2E_mode==FALSE && Gx_PTPFIG.ethernet_mode==TRUE) 
//					{
//						mode = 1;//P2P MAC
//						rtOpts.transport 			= IEEE_802_3;	//1:P2P-MAC
//						rtOpts.delayMechanism = P2P; 
//					}
//				else if(Gx_PTPFIG.E2E_mode==TRUE  && Gx_PTPFIG.ethernet_mode==FALSE) 
//					{
//						mode = 2;//E2E UDP
//						rtOpts.transport 			= UDP_IPV4;		//2:E2E-UDP
//						rtOpts.delayMechanism = E2E; 
//					}
//				else if(Gx_PTPFIG.E2E_mode==FALSE && Gx_PTPFIG.ethernet_mode==FALSE) 
//					{
//						mode = 3;//P2P UDP
//						rtOpts.transport 			= UDP_IPV4;		//3:P2P-UDP
//						rtOpts.delayMechanism = P2P; 
//					}			
//				if (G_ptpClock.portState == PTP_SLAVE)
//					{	
//						toState(PTP_SLAVE, &rtOpts, &G_ptpClock);		//协议状态转换->初始化状态	
//					}
//				if((GlobalConfig.IPaddr!=Gx_PTPFIG.Local_ip_32)||(GlobalConfig.GWaddr!=Gx_PTPFIG.Gate_ip_32))
//					{
//						int i=0;
//						int32_t config_tmp[10]={0};	
//						GlobalConfig.WorkMode = mode;
//						GlobalConfig.IPaddr   = Gx_PTPFIG.Local_ip_32;
//						GlobalConfig.GWaddr   = Gx_PTPFIG.Gate_ip_32;
//						GlobalConfig.NETmark  = Gx_PTPFIG.Mask_ip_32;
//						GlobalConfig.DstIpaddr= 0; 
//						memcpy((int8_t *)config_tmp,(int8_t *)&GlobalConfig,sizeof(FigStructData));
//						config_tmp[5]	=	((GlobalConfig.WorkMode)<<24)|((GlobalConfig.ip_mode)<<16)|((GlobalConfig.BroadcastInterval)<<8)|(GlobalConfig.tmp1);
//						NVIC_DisableIRQ(ETH_IRQn);		//写FLASH前关闭中断
//						NVIC_DisableIRQ(USART1_IRQn);	
//						NVIC_DisableIRQ(EXTI1_IRQn);
//						NVIC_DisableIRQ(USART6_IRQn);
//						FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,10);
//						for(i=0;i<10000;i++){}//等待	
//						NVIC_SystemReset();
//					}
//			}
//		Gx_PTPFIG_OK = 0; //清楚串口接收完成标记
//	}
//}
//#endif

#ifdef NTP_Client
static const uint8_t FIG_Lenth = 36;
struct ConfigNTPData Gx_NTPFIGC;
unsigned char Gx_NTPFIG_OK;

void USART6_IRQHandler(void)
{
	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//检查是否是指定的中断发生
		{
			uint8_t Res;
			static int16_t Revlen=0;
			Res =USART_ReceiveData(USART6);//读取接收到的数据
			if(Gx_NTPFIG_OK!=1)
				{
					if(Res==0x2a)
						{		
							Revlen=0;
						}
					else if(Res==0x23)
						{
							myprintf("OK(%d)%x",Revlen,Res);
							if((Revlen==(FIG_Lenth-1))&&(Gx_NTPFIGC.frame_head==0x2a)&&(Gx_NTPFIGC.cmd_type==0x1))
								{
									Gx_NTPFIG_OK=1;
								}
							else
								{
									Revlen=0;
								}
						}	
					//myprintf("(%d)%x\n",Revlen,Res);
					*((unsigned char*)(&Gx_NTPFIGC)+Revlen) = Res;		
					Revlen=(Revlen+1)%FIG_Lenth;	
				}
			USART_ClearITPendingBit(USART6,USART_IT_RXNE);//清除接收中断标记(读的时候已经清了，不是必须的)
		}
}

void Gx_HandleNTPFIG(void)
{
	if(Gx_NTPFIG_OK == 1) //NTP串口接收完成标记(USART6)
		{
			int i=0;
			int32_t config_tmp[6]={0};			
			GlobalConfig.Reidf 		= Gx_NTPFIGC.testMark; //参考时间源
			GlobalConfig.tmp1			= Gx_NTPFIGC.refto; 	 //参考时标
			GlobalConfig.IPaddr   = Gx_NTPFIGC.Local_ip_32;
			GlobalConfig.GWaddr   = Gx_NTPFIGC.Gate_ip_32;
			GlobalConfig.NETmark  = Gx_NTPFIGC.Mask_ip_32;
			GlobalConfig.DstIpaddr= Gx_NTPFIGC.Server_ip_32; 
			memcpy((int8_t *)config_tmp,(int8_t *)&GlobalConfig,20);
			config_tmp[5]	=	((GlobalConfig.WorkMode)<<24)|((GlobalConfig.ip_mode)<<16)|((GlobalConfig.BroadcastInterval)<<8)|(GlobalConfig.tmp1);
			NVIC_DisableIRQ(ETH_IRQn);		//写FLASH前关闭中断
			NVIC_DisableIRQ(USART1_IRQn);	
			NVIC_DisableIRQ(EXTI1_IRQn);
			NVIC_DisableIRQ(USART6_IRQn);
			FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,6);
			for(i=0;i<10000;i++){}//等待	
			NVIC_SystemReset();
			Gx_NTPFIG_OK = 0;
		}
}

#endif

//#ifdef Gx_NTP_Server
//struct ConfigNTPData Gx_NTPFIGS;
//struct Gx_Flags Gx_flags;
//unsigned char Gx_NTPIndex;
//unsigned char Gx_NTPFIG_OK;

//void USART6_IRQHandler(void) //接收配置信息
//{
//	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//检查是否是指定的中断发生
//	{//接收到数据产生中断 
//		if(Gx_NTPIndex==0)//一个指向数据移动到的位置a[i] i++;
//		{
//			memset(&Gx_NTPFIGS,0,sizeof(Gx_NTPFIGS));
//		}
//		
//		//这里收到的第一个数据还是0x2a
//		*((unsigned char*)(&Gx_NTPFIGS)+Gx_NTPIndex) = USART_ReceiveData(USART6);
////		printf("%d.",*((unsigned char*)(&Gx_NTPFIGS)+Gx_NTPIndex));
//		Gx_NTPIndex++;
//		
//		if(Gx_NTPIndex == 1)
//		{
//			if(Gx_NTPFIGS.frame_head != 0x2a) //"*"
//				Gx_NTPIndex=0;
//		}
//		else if(Gx_NTPIndex == sizeof(Gx_NTPFIGS))
//		{
//			if(Gx_NTPFIGS.frame_end == 0x23)//"#"
//					Gx_NTPFIG_OK = 1;
////			printf("\n");
//		}
//		USART_ClearITPendingBit(USART6,USART_IT_RXNE);//清除接收中断标记	
//	}
//}

//void Gx_HandleNTPFIGS(void)
//{
//	if(Gx_NTPFIG_OK == 1) //NTP配置帧接收完成
//	{
//		int i;
//		int32_t config_tmp[5] ={0};
//		switch(Gx_NTPFIGS.cmd_type)
//		{
//			case 0x04:
//				config_tmp[0] = Gx_NTPFIGS.Local_ip_32;
//				config_tmp[1] = Gx_NTPFIGS.Gate_ip_32;
//				config_tmp[2] = Gx_NTPFIGS.Mask_ip_32;
//				
//				FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,3);//将ntp server的配置写入flash中
//				
//				addrInfo.ptpmode = 0;
//				addrInfo.ipaddr  = Gx_NTPFIGS.Local_ip_32;
//				addrInfo.gwaddr  = Gx_NTPFIGS.Gate_ip_32;
//				addrInfo.netmark = Gx_NTPFIGS.Mask_ip_32;
//				addrInfo.ntpservaddr = Gx_NTPFIGS.Mask_ip_32;
//				addrInfo.flags = 1;//标记重启网络和ptpd ntp(不重启系统)

//				for(i=0;i<10000;i++){}//等待
//					printf("ntp server SystemReset\n");
//				NVIC_SystemReset();//软件重启系统;
//				break;
//					
//			default:
//				break;
//		}	
//		Gx_NTPIndex  = 0;
//		Gx_NTPFIG_OK = 0;
//	}
//}
//#endif









