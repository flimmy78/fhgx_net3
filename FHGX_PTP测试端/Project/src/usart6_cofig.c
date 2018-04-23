#include "usart6_cfg.h"
extern FigStructData GlobalConfig;
extern RunTimeOpts rtOpts;
extern PtpClock  G_ptpClock;

//extern unsigned int test_fjm;
//extern sysTime sTime;
////extern struct ADDressInfo addrInfo;
//extern Servo Mservo;
//extern Filter MofM_filt;
//extern TimeInternal NtplocalTime;
//static void Mfilter(Integer32 * nsec_current, Filter * filt);
//unsigned char synflags;
////Integer32 abjbuf[5];


#ifdef Gx_PTP_Device
struct ConfigPTPData Gx_PTPFIG;
//struct Gx_Flags Gx_flags;

static unsigned char Gx_PTPFIG_OK;

//��������֡�ͷ�����Ϣ����ʾ(PTP)
void USART6_IRQHandler(void)
{
	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
		{
			uint8_t Res;
			static int16_t Revlen=0;
			Res =USART_ReceiveData(USART6);//��ȡ���յ�������
			if(Gx_PTPFIG_OK!=1)
				{
					if(Res==0x2a)
						{		
							Revlen=0;
						}
					else if(Res==0x23)
						{
							if((Revlen==39)&&(Gx_PTPFIG.frame_head==0x2a)&&(Gx_PTPFIG.cmd_type==0x1))
								{
									Gx_PTPFIG_OK=1;
								}
							else
								{
									Revlen=0;
								}
						}	
					*((unsigned char*)(&Gx_PTPFIG)+Revlen) = Res;		
					Revlen=(Revlen+1)%40;	
					
				}
			USART_ClearITPendingBit(USART6,USART_IT_RXNE);//��������жϱ��(����ʱ���Ѿ����ˣ����Ǳ����)
		}
}

void Gx_HandlePTPFIG(void)
{
	if(Gx_PTPFIG_OK == 1)  //ptp����������Ϣ������ɱ��(USART6����)
	{
		uint8_t mode=0;
		printf("cmd_type=%d",Gx_PTPFIG.cmd_type);
		if (G_ptpClock.portState != PTP_INITIALIZING)
			{	
				if(Gx_PTPFIG.E2E_mode==TRUE && Gx_PTPFIG.ethernet_mode==TRUE) 
					{
						mode = 0;//E2E MAC
						rtOpts.transport 			= IEEE_802_3;	//0:E2E-MAC
						rtOpts.delayMechanism = E2E; 				
					}
				else if(Gx_PTPFIG.E2E_mode==FALSE && Gx_PTPFIG.ethernet_mode==TRUE) 
					{
						mode = 1;//P2P MAC
						rtOpts.transport 			= IEEE_802_3;	//1:P2P-MAC
						rtOpts.delayMechanism = P2P; 
					}
				else if(Gx_PTPFIG.E2E_mode==TRUE  && Gx_PTPFIG.ethernet_mode==FALSE) 
					{
						mode = 2;//E2E UDP
						rtOpts.transport 			= UDP_IPV4;		//2:E2E-UDP
						rtOpts.delayMechanism = E2E; 
					}
				else if(Gx_PTPFIG.E2E_mode==FALSE && Gx_PTPFIG.ethernet_mode==FALSE) 
					{
						mode = 3;//P2P UDP
						rtOpts.transport 			= UDP_IPV4;		//3:P2P-UDP
						rtOpts.delayMechanism = P2P; 
					}		
				myprintf("mode chang %d\n",mode);	
				if (G_ptpClock.portState == PTP_SLAVE)
					{	
						toState(PTP_SLAVE, &rtOpts, &G_ptpClock);		//Э��״̬ת��->��ʼ��״̬	
					}
				//if((GlobalConfig.IPaddr!=Gx_PTPFIG.Local_ip_32)||(GlobalConfig.GWaddr!=Gx_PTPFIG.Gate_ip_32))
				if(1)	
					{
						int i=0;
						int32_t config_tmp[10]={0};	
						GlobalConfig.WorkMode = mode;
						GlobalConfig.IPaddr   = Gx_PTPFIG.Local_ip_32;
						GlobalConfig.GWaddr   = Gx_PTPFIG.Gate_ip_32;
						GlobalConfig.NETmark  = Gx_PTPFIG.Mask_ip_32;
						GlobalConfig.DstIpaddr= 0; 
						memcpy((int8_t *)config_tmp,(int8_t *)&GlobalConfig,sizeof(FigStructData));
						config_tmp[5]	=	((GlobalConfig.WorkMode)<<24)|((GlobalConfig.ip_mode)<<16)|((GlobalConfig.BroadcastInterval)<<8)|(GlobalConfig.tmp1);
						NVIC_DisableIRQ(ETH_IRQn);		//дFLASHǰ�ر��ж�
						NVIC_DisableIRQ(USART1_IRQn);	
						NVIC_DisableIRQ(EXTI1_IRQn);
						NVIC_DisableIRQ(USART6_IRQn);
						FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,10);
						for(i=0;i<10000;i++){}//�ȴ�	
						NVIC_SystemReset();
					}
			}
		Gx_PTPFIG_OK = 0; //������ڽ�����ɱ��
	}
}
#endif

//#ifdef Gx_NTP_Client
//struct ConfigNTPData Gx_NTPFIGC;
//struct Gx_Flags Gx_flags;
//unsigned char Gx_NTPIndex;
//unsigned char Gx_NTPFIG_OK;

//void USART6_IRQHandler(void)
//{
//	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
//	{//���յ����ݲ����ж� 
//		if(Gx_NTPIndex==0)//һ��ָ�������ƶ�����λ��a[i] i++;
//		{
//			memset(&Gx_NTPFIGC,0,sizeof(Gx_NTPFIGC));
//		}
//		
//		//�����յ��ĵ�һ�����ݻ���0x23
//		*((unsigned char*)(&Gx_NTPFIGC)+Gx_NTPIndex) = USART_ReceiveData(USART6);
////		printf("0x%02x",*((unsigned char*)(&Gx_NTPFIGC)+Gx_NTPIndex));
//		Gx_NTPIndex++;
//		if(Gx_NTPIndex==1)
//		{
//			if(Gx_NTPFIGC.frame_head != 0x2a)//�յ��ĵ�һ���ֽڲ�����"*"
//				Gx_NTPIndex=0;
//		}
//		else if(Gx_NTPIndex == sizeof(Gx_NTPFIGC))
//		{
//			if(Gx_NTPFIGC.frame_end == 0x23)
//			{
//				Gx_NTPFIG_OK = 1;
//				Gx_NTPIndex=0;
//			}
////			printf("\n");
//		}
//		USART_ClearITPendingBit(USART6,USART_IT_RXNE);//��������жϱ��	
//	}
//}

//void Gx_HandleNTPFIGC(void)
//{
//	if(Gx_NTPFIG_OK == 1) //NTP���ڽ�����ɱ��(USART6)
//	{
//		int i;
//		int config_tmp[5] ={0};
//		switch(Gx_NTPFIGC.cmd_type)
//		{
//			case 0x01:
//				config_tmp[0] = Gx_NTPFIGC.Local_ip_32;
//				config_tmp[1] = Gx_NTPFIGC.Gate_ip_32;
//				config_tmp[2] = Gx_NTPFIGC.Mask_ip_32;
//				config_tmp[3] = Gx_NTPFIGC.Server_ip_32;
//				config_tmp[4] = Gx_NTPFIGC.testMark;//TSSM�ֶ�
//				
//				test_fjm = Gx_NTPFIGC.testMark; //TSSM
//				FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,5); //д��flash
//			
//				addrInfo.ptpmode = 0;
//				addrInfo.ipaddr  = Gx_NTPFIGC.Local_ip_32;
//				addrInfo.gwaddr  = Gx_NTPFIGC.Gate_ip_32;
//				addrInfo.netmark = Gx_NTPFIGC.Mask_ip_32;
//				addrInfo.ntpservaddr = Gx_NTPFIGC.Server_ip_32;
//				addrInfo.flags = 1; //�������֡�յ�������ɣ�������while������ģ��
//				for(i=0;i<10000;i++){}//�ȴ�
////				for(i=0;i<10000;i++){}//�ȴ�
//				printf("write flash ntp client\n");
//				NVIC_SystemReset();//�������;
//				break;
//					
//			case 0x02:
//				Gx_flags.GX_NTPStopC = 1;
//				break;
//			
//			case 0x03:
//				Gx_flags.GX_NTPStopC = 0;
//				break;
//			
//			default:
//				break;
//		}
//		Gx_NTPFIG_OK = 0;
//	}
//}

//#endif

//#ifdef Gx_NTP_Server
//struct ConfigNTPData Gx_NTPFIGS;
//struct Gx_Flags Gx_flags;
//unsigned char Gx_NTPIndex;
//unsigned char Gx_NTPFIG_OK;

//void USART6_IRQHandler(void) //����������Ϣ
//{
//	if(USART_GetITStatus(USART6,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
//	{//���յ����ݲ����ж� 
//		if(Gx_NTPIndex==0)//һ��ָ�������ƶ�����λ��a[i] i++;
//		{
//			memset(&Gx_NTPFIGS,0,sizeof(Gx_NTPFIGS));
//		}
//		
//		//�����յ��ĵ�һ�����ݻ���0x2a
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
//		USART_ClearITPendingBit(USART6,USART_IT_RXNE);//��������жϱ��	
//	}
//}

//void Gx_HandleNTPFIGS(void)
//{
//	if(Gx_NTPFIG_OK == 1) //NTP����֡�������
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
//				FLASH_Write(FLASH_SAVE_ADDR1,config_tmp,3);//��ntp server������д��flash��
//				
//				addrInfo.ptpmode = 0;
//				addrInfo.ipaddr  = Gx_NTPFIGS.Local_ip_32;
//				addrInfo.gwaddr  = Gx_NTPFIGS.Gate_ip_32;
//				addrInfo.netmark = Gx_NTPFIGS.Mask_ip_32;
//				addrInfo.ntpservaddr = Gx_NTPFIGS.Mask_ip_32;
//				addrInfo.flags = 1;//������������ptpd ntp(������ϵͳ)

//				for(i=0;i<10000;i++){}//�ȴ�
//					printf("ntp server SystemReset\n");
//				NVIC_SystemReset();//�������ϵͳ;
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









