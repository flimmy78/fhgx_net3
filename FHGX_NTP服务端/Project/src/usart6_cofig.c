#include "usart6_cfg.h"
extern FigStructData GlobalConfig;
extern RunTimeOpts rtOpts;
extern PtpClock  G_ptpClock;
#include "netconf.h"
#include "ntp.h"
#include "main.h"
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
//	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
//		{
//			uint8_t Res;
//			static int16_t Revlen=0;
//			Res =USART_ReceiveData(USART6);//��ȡ���յ�������
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
//			USART_ClearITPendingBit(USART6,USART_IT_RXNE);//��������жϱ��(����ʱ���Ѿ����ˣ����Ǳ����)
//		}
//}

//void Gx_HandlePTPFIG(void)
//{
//	if(Gx_PTPFIG_OK == 1)  //ptp����������Ϣ������ɱ��(USART6����)
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
//						toState(PTP_SLAVE, &rtOpts, &G_ptpClock);		//Э��״̬ת��->��ʼ��״̬	
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
//						NVIC_DisableIRQ(ETH_IRQn);		//дFLASHǰ�ر��ж�
//						NVIC_DisableIRQ(USART1_IRQn);	
//						NVIC_DisableIRQ(EXTI1_IRQn);
//						NVIC_DisableIRQ(USART6_IRQn);
//						FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,10);
//						for(i=0;i<10000;i++){}//�ȴ�	
//						NVIC_SystemReset();
//					}
//			}
//		Gx_PTPFIG_OK = 0; //������ڽ�����ɱ��
//	}
//}
//#endif

#ifdef NTP_Server
static const uint8_t FIG_Lenth = 36;
struct ConfigNTPData Gx_NTPFIGC;
unsigned char Gx_NTPFIG_OK;

void USART6_IRQHandler(void)
{
	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
		{
			uint8_t Res;
			static int16_t Revlen=0;
			Res =USART_ReceiveData(USART6);//��ȡ���յ�������
			if(Gx_NTPFIG_OK!=1)
				{
					if(Res==0x2a)
						{		
							Revlen=0;
						}
					else if(Res==0x23)
						{
							myprintf("OK(%d)%x",Revlen,Res);
							if((Revlen==(FIG_Lenth-1))&&(Gx_NTPFIGC.frame_head==0x2a)&&(Gx_NTPFIGC.cmd_type==0x04))
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
			USART_ClearITPendingBit(USART6,USART_IT_RXNE);//��������жϱ��(����ʱ���Ѿ����ˣ����Ǳ����)
		}
}

void Gx_HandleNTPFIG(void)
{
	if(Gx_NTPFIG_OK == 1) //NTP���ڽ�����ɱ��(USART6)
		{
			int i=0;
			int32_t config_tmp[6]={0};			
			GlobalConfig.Reidf 		= Gx_NTPFIGC.testMark; //�ο�ʱ��Դ
			GlobalConfig.tmp1			= Gx_NTPFIGC.refto; 	 //�ο�ʱ��
			GlobalConfig.IPaddr   = Gx_NTPFIGC.Local_ip_32;
			GlobalConfig.GWaddr   = Gx_NTPFIGC.Gate_ip_32;
			GlobalConfig.NETmark  = Gx_NTPFIGC.Mask_ip_32;
			GlobalConfig.DstIpaddr= Gx_NTPFIGC.Server_ip_32; 
			memcpy((int8_t *)config_tmp,(int8_t *)&GlobalConfig,20);
			config_tmp[5]	=	((GlobalConfig.WorkMode)<<24)|((GlobalConfig.ip_mode)<<16)|((GlobalConfig.BroadcastInterval)<<8)|(GlobalConfig.tmp1);
			NVIC_DisableIRQ(ETH_IRQn);		//дFLASHǰ�ر��ж�
			NVIC_DisableIRQ(USART1_IRQn);	
			NVIC_DisableIRQ(EXTI1_IRQn);
			NVIC_DisableIRQ(USART6_IRQn);
			FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,6);
			for(i=0;i<10000;i++){}//�ȴ�	
			NVIC_SystemReset();
			Gx_NTPFIG_OK = 0;
		}
}

#endif










