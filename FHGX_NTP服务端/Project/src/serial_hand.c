#include "ptpd.h"
#include "ntp.h"
#include "main.h"
#include "netconf.h"
#include "share.h"

extern sysTime sTime;
extern RunTimeOpts rtOpts;
extern PtpClock  G_ptpClock;
extern char ntpsendf;
unsigned char leap61;			//������
unsigned char leap59;			//������
unsigned char leapwring;	//����澯
unsigned char leapNum;		//�����־�ļ���



//FHGX��ͬ������*********************************************************************************
#if defined Device_FHGX

SyncData 		  Syncbuf;							//��ȡ����ͬ����Ϣ��Buff
unsigned char Rxbuffer[50];					//���ڽ���Buff
unsigned char rec_SYNIndex=0;				//Buffָ��
unsigned char rec_SYNdata_flag = 0;	//ͬ����Ϣ��־


void USART1_IRQHandler(void)//godin ptp���մ���ʱ����Ϣ
{	
	static  unsigned char rec_SYNend_flag = 0;
	static	unsigned char synvalid=1;	//ģ�������־
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)//����Ƿ���ָ�����жϷ���
	{
		unsigned char rev;
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);//��������жϱ��	
		rev = USART_ReceiveData(USART1);	//�����ڻ��Զ�����жϱ��
		if(rev == '#') 					 //'#'��������
			{
				rec_SYNend_flag = 1; //�������ѽ����
			}
		else if(rev == '*') 		 //'*'֡��ʼ��
			{
				rec_SYNIndex 		= 0; //��0
				rec_SYNend_flag = 0;
			}
		else if(rec_SYNIndex > sizeof(Rxbuffer)) //���ճ��ȳ�����ȷ����
			{
				rec_SYNIndex =  sizeof(Rxbuffer); 	 //Buffָ��ָ�����һλ
			}
		Rxbuffer[rec_SYNIndex++] = rev;
		if(rec_SYNend_flag == 1)	
		{
			if((rec_SYNIndex==18)&&synvalid) 					//���Ⱥϸ�	
				{
					if(strncmp((const char *)&Rxbuffer[5],"CEB",3) == 0)						//NTP,ͨ��2
						{
							if(Rxbuffer[16]==0x30)	//������
								{
									leapNum= 0;
								}
							else if(Rxbuffer[16]==0x31)	//������
								{
									leap61 = 1;	
									leap59 = 0;
									leapNum++;
								}
							else if(Rxbuffer[16]==0x32)
								{
									leap61 = 0;
									leap59 = 1;
									leapNum++;
								}
						}						
				}	
			else if((rec_SYNIndex==35 )&&synvalid) 			//���Ⱥϸ�	
				{
			#ifndef NTP_Server
					if(strncmp((const char *)&Rxbuffer[5],"NET",3) == 0)		//NTP���Զ˺�PTPͨ��1
			#else
					if(strncmp((const char *)&Rxbuffer[5],"SYN",3) == 0)		//NTP�����ͨ��2
			#endif 	
					{
						memcpy(&Syncbuf,Rxbuffer,sizeof(SyncData));
						rec_SYNdata_flag=1;
						GPIO_ResetBits(GPIOD,mLED3);//Ϩ��LED3(PPS�յ���˸Ч��)
					}				
				}
		#ifdef NTP_Server
			else if(rec_SYNIndex==11 ) 	//���Ⱥϸ�	
				{
					if(strncmp((const char *)&Rxbuffer[5],"CEL",3) == 0)					  	//CEL���
						{
							myprintf("CEL\n");
							if(Rxbuffer[9]==0x30)	//������
								{
									leap61 = 0;
									leap59 = 0;	
									leapNum= 0;
									synvalid=1;
								}
							else if(Rxbuffer[9]==0x31)			//������
								{
									leap61 = 1;	
									leap59 = 0;	
									leapNum= 3;
									synvalid=0;
								}
							else if(Rxbuffer[9]==0x32)
								{
									leap61 = 0;
									leap59 = 1;	
									leapNum= 3;
									synvalid=0;
								}
								myprintf("%d,%d,leapNum:%d\n",leap61,leap59,leapNum);	
						}				
				}
		#endif 	
			
		}
	}
}

static long prmtread_decimal(unsigned char *p,unsigned char n)		//��GXͬ��������
{
	long r;
	unsigned char i;
	unsigned char pol;

	for(i=0;i<n;p++)							//step1���ҵ�n������
	{
		if(*p==',')
			{i++; }
	}

	pol=1;											
	for(n=0;*p!=','&&*p!='#';p++)	//step2ɨ�������ʶ�����ִ���
	{
		if(*p==' ')
			{}			   
		else if(*p=='-')	
			{pol=0;	}									//��������
		else if(*p>='0'&&*p<='9')
			{n++;	}										//�õ����ִ���	
	}
	p--;													//ָ��
	r=0;													//step3��ASCII�����ַ�תΪ������
	if(pol)		//����
	{
		for(i=0;i<n;i++)
		{
			switch(i)
			{
				case 0:r=*p-0x30;break;
				case 1:r=(*p-0x30)*10+r;break;
				case 2:r=(*p-0x30)*100+r;break;
				case 3:r=(*p-0x30)*1000+r;break;
				case 4:r=(*p-0x30)*10000+r;break;	  
				case 5:r=(*p-0x30)*100000+r;break;
				case 6:r=(*p-0x30)*1000000+r;break;
				case 7:r=(*p-0x30)*10000000+r;break;
				case 8:r=(*p-0x30)*100000000+r;break;
				case 9:r=(*p-0x30)*1000000000+r;break; 
				default:	break;
			}		
			p--;
		}
	}
	else 		//����		 
	{
		for(i=0;i<n;i++)
		{
			switch(i)
			{
				case 0:r=0-(*p-0x30);break;
				case 1:r=r-((*p-0x30)*10);break;
				case 2:r=r-((*p-0x30)*100);break;
				case 3:r=r-((*p-0x30)*1000);break;
				case 4:r=r-((*p-0x30)*10000);break;	  
				case 5:r=r-((*p-0x30)*100000);break;
				case 6:r=r-((*p-0x30)*1000000);break;
				case 7:r=r-((*p-0x30)*10000000);break;
				case 8:r=r-((*p-0x30)*100000000);break;
				case 9:r=r-((*p-0x30)*1000000000);break;  
				default:	break;
			}		
			p--;
		}
	}
	return r;
}


void Hand_serialSync(void)
{
	if(rec_SYNdata_flag == 1) //���ڱ���յ���ʱ����
		{
			
			tTime CurrentTime;
			rec_SYNdata_flag = 0;//��־��0
			if(Syncbuf.sync_state==0)
				return;
			CurrentTime.usYear = prmtread_decimal((unsigned char *)&Syncbuf,2);
			CurrentTime.ucMon  = prmtread_decimal((unsigned char *)&Syncbuf,3);//�˴��µ�ֵΪ1��12
			CurrentTime.ucMday = prmtread_decimal((unsigned char *)&Syncbuf,4);
			CurrentTime.ucHour = prmtread_decimal((unsigned char *)&Syncbuf,5);
			CurrentTime.ucMin  = prmtread_decimal((unsigned char *)&Syncbuf,6);
			CurrentTime.ucSec  = prmtread_decimal((unsigned char *)&Syncbuf,7);
			//myprintf("%d-%d-%d,%d.%.d.%d\n",CurrentTime.usYear,CurrentTime.ucMon,CurrentTime.ucMday,CurrentTime.ucHour,CurrentTime.ucMin,CurrentTime.ucSec);
			sTime.serailTime.seconds = Serial_Htime(&CurrentTime); //UTCʱ��ת��������
			sTime.serailTime.nanoseconds = 0;		
			offset_time(&sTime);		 //���㴮��ʱ��ͱ���ʱ���ƫ��
			abjClock(sTime.SubTime); //����ƫ��
			ntpsendf=1;
		}
}




#endif



//FHGX��ͬ������*********************************************************************************
void handleap(void)//FH1000���մ���ʱ����Ϣ
{

	if(leapNum >= 3)//����3������Ԥ��
	{
		
		TimeInternal ngettime;
		struct ptptime_t timeupdata;
		getTime(&ngettime);
		//����ʱ�����
		if(leap61 != 0)//������
		{
			if((ngettime.seconds%60) == 1)//g_CFL_ucSec2���յ�ģ������֡��һ��ʱ��ÿ����һ��
			{
				timeupdata.tv_sec = -1;
				timeupdata.tv_nsec = 0;
				ETH_PTPTime_UpdateOffset(&timeupdata);
				leap61 = 0;	
				leapNum = 0;
				printf("61 OK\n");
			}
		}
		else if(leap59 != 0)//������
		{
			if((ngettime.seconds%60) == 59)
			{
				timeupdata.tv_sec = 1;//
				timeupdata.tv_nsec = 0;
				ETH_PTPTime_UpdateOffset(&timeupdata);
				leap59 = 0;
				leapNum = 0;
				printf("59 OK\n");
			}
		}
	}
}




