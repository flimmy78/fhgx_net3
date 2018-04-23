#include "ptpd.h"
#include "ntp.h"
#include "main.h"
#include "netconf.h"
#include "share.h"

extern sysTime sTime;
extern RunTimeOpts rtOpts;
extern PtpClock  G_ptpClock;

unsigned char leap61;			//正闰秒
unsigned char leap59;			//负闰秒
unsigned char leapwring;	//闰秒预告
unsigned char leapNum;		//闰秒标志的计数


//FH1100的同步函数*********************************************************************************
#if defined Device_FH1000
struct Sync_UartData sync_UartData;			//串口报文结构
unsigned char SyncIndex = 0;						//串口报文指针
unsigned char SyncUart_OK;							//串口报文接受标识

void USART1_IRQHandler(void)						//FH1000接收串口时间信息
{	
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)//检查是否是指定的中断发生
	{	
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);//清除接收中断标记	
		if(SyncIndex==0)//一个指向数据移动到的位置a[i] i++;
		{
				memset(&sync_UartData,0,sizeof(sync_UartData));
		}
		//这里收到的第一个数据是0x23
		*( (unsigned char*)&sync_UartData + SyncIndex) = USART_ReceiveData(USART1);
		SyncIndex++;
		if(SyncIndex == 1)//判断收到头的第一个字段
		{		
			//判断帧的起始字符(#)0x23
			if( sync_UartData.frame_head != 0x23)
			{
				SyncIndex--;	
			}
		}
		//接收到的数据大小等于帧头的大小帧头接收完成
		if(SyncIndex==sizeof(sync_UartData))
		{	//结尾标记(0x0d和0x0a)同时满足
			if((sync_UartData.end_flag2==0x0a)&&(sync_UartData.end_flag1==0x0d))
			{
				//printf("\n");
				SyncUart_OK = 1;//标记uart收到数据
				SyncIndex = 0;//地址指针偏移量清0
				GPIO_ResetBits(GPIOD,mLED3);//熄灭LED3(PPS收到闪烁效果)
			}
		}
		else if(SyncIndex>sizeof(sync_UartData))
		{
			SyncIndex = 0;
		}
	}
}

void Hand_serialSync(void)//FH1000串口处理
{
	if(SyncUart_OK == 1)//串口标记收到对时报文
	{
		unsigned char state_temp;
		tTime sulocaltime;								//UTC时间
		TimeInternal time;
		SyncUart_OK = 0;
		state_temp = sync_UartData.state_flag4 - 0x30;
		if(state_temp >=0x0a)
		{
			leapwring = 1;									//闰秒预告
		}
		sTime.SubTime.seconds = 0;
		sTime.SubTime.nanoseconds = 0;
		getTime(&time);										//取ETH的硬件时间
		state_temp =  sync_UartData.state_flag1 - 0x30;//ascii的0对应的十六进制数是0x30
		if(state_temp != 0)
				leapNum++;		 //闰秒预报的计数
		if(state_temp == 2)//正闰秒
			{
				leap61 = 1;
				leap59 = 0;
			}
		else if(state_temp == 3)//负闰秒
			{
				leap59 = 1;
				leap61 = 0;
			}
		else
			{//正常状态不闰秒
				leap61  = 0;
				leap59  = 0;
				leapNum = 0;
			}
		//串口报文转化成UTC时间
		sulocaltime.usYear=(sync_UartData.year_4-0x30)*1000+(sync_UartData.year_3-0x30)*100+((sync_UartData.year_2 - 0x30 )*10+(sync_UartData.year_1-0x30));
		sulocaltime.ucMon =(sync_UartData.month_2 - 0x30)*10+(sync_UartData.month_1-0x30);//此处月的值为1－12，系统ttime的月值应该是0－11.在后面的timetoseconds函数中，此处的月值减了1.
		sulocaltime.ucMday=(sync_UartData.day_2 - 0x30)*10+(sync_UartData.day_1- 0x30);
		sulocaltime.ucHour=(sync_UartData.hour_2 - 0x30)*10+(sync_UartData.hour_1-0x30);
		sulocaltime.ucMin =(sync_UartData.min_2 - 0x30)*10+(sync_UartData.min_1- 0x30);
		sulocaltime.ucSec =(sync_UartData.sec_2 - 0x30)*10+(sync_UartData.sec_1- 0x30);
		//系统的串口时间
		sTime.serailTime.seconds = Serial_Htime(&sulocaltime);		//UTC时间转化成整秒
		sTime.serailTime.nanoseconds = 0;													//ns部分置0
			
		offset_time(&sTime);			//计算串口时间和本地时间的偏差	
		abjClock(sTime.SubTime); 	//调整偏差
	}
}


#endif
//FH1100的同步函数*********************************************************************************



//FHGX的同步函数*********************************************************************************
#if defined Device_FHGX

SyncData 		  Syncbuf;							//提取串口同步信息的Buff
unsigned char Rxbuffer[50];					//串口接收Buff
unsigned char rec_SYNIndex=0;				//Buff指针
unsigned char rec_SYNdata_flag = 0;	//同步信息标志


void USART1_IRQHandler(void)//godin ptp接收串口时间信息
{	
	static unsigned char rec_SYNend_flag = 0;
	if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)//检查是否是指定的中断发生
	{
		unsigned char rev;
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);//清除接收中断标记	
		rev = USART_ReceiveData(USART1);	//读串口会自动清除中断标记
		if(rev == '#') 					 //'#'语句结束符
			{
				rec_SYNend_flag = 1; //标记语句已结结束
			}
		else if(rev == '*') 		 //'*'帧起始符
			{
				rec_SYNIndex 		= 0; //清0
				rec_SYNend_flag = 0;
			}
		else if(rec_SYNIndex > sizeof(Rxbuffer)) //接收长度超过正确长度
			{
				rec_SYNIndex =  sizeof(Rxbuffer); 	 //Buff指针指向最后一位
			}
		Rxbuffer[rec_SYNIndex++] = rev;
		if(rec_SYNend_flag == 1)	
		{
			if(rec_SYNIndex==18) 					//长度合格	
				{
					if(strncmp((const char *)&Rxbuffer[5],"CEB",3) == 0)						//NTP,通道2
						{
							if(Rxbuffer[16]==0x30)	//无闰秒
								{
									leapNum= 0;
								}
							else if(Rxbuffer[16]==0x31)	//正闰秒
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
			else if(rec_SYNIndex==35 ) 			//长度合格	
				{
			#ifndef NTP_Server
					if(strncmp((const char *)&Rxbuffer[5],"NET",3) == 0)		//NTP测试端和PTP通道1
			#else
					if(strncmp((const char *)&Rxbuffer[5],"SYN",3) == 0)		//NTP服务端通道2
			#endif 	
					{
						memcpy(&Syncbuf,Rxbuffer,sizeof(SyncData));
						rec_SYNdata_flag=1;
						GPIO_ResetBits(GPIOD,mLED3);//熄灭LED3(PPS收到闪烁效果)
					}				
				}
		#ifdef NTP_Server
			else if(rec_SYNIndex==11 ) 	//长度合格	
				{
					if(strncmp((const char *)&Rxbuffer[5],"CEL",3) == 0)					  	//CEL语句
						{
							if(Rxbuffer[9]==0x30)	//无闰秒
								{
									leap61 = 0;
									leap59 = 0;	
									leapNum= 0;
								}
							else if(Rxbuffer[9]==0x31)			//正闰秒
								{
									leap61 = 1;	
									leap59 = 0;	
									leapNum= 3;
								}
							else if(Rxbuffer[9]==0x32)
								{
									leap61 = 0;
									leap59 = 1;	
									leapNum= 3;
								}
						}				
				}
		#endif 	
			
		}
	}
}

static long prmtread_decimal(unsigned char *p,unsigned char n)		//解GX同步报文用
{
	long r;
	unsigned char i;
	unsigned char pol;

	for(i=0;i<n;p++)							//step1查找第n个逗号
	{
		if(*p==',')
			{i++; }
	}

	pol=1;											
	for(n=0;*p!=','&&*p!='#';p++)	//step2扫描特殊标识和数字串长
	{
		if(*p==' ')
			{}			   
		else if(*p=='-')	
			{pol=0;	}									//正负符号
		else if(*p>='0'&&*p<='9')
			{n++;	}										//得到数字串长	
	}
	p--;													//指针
	r=0;													//step3将ASCII数字字符转为长整型
	if(pol)		//正数
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
	else 		//负数		 
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
	if(rec_SYNdata_flag == 1) //串口标记收到对时报文
		{
			tTime CurrentTime;
			rec_SYNdata_flag = 0;//标志清0
			if(Syncbuf.sync_state==0)
				return;
			CurrentTime.usYear = prmtread_decimal((unsigned char *)&Syncbuf,2);
			CurrentTime.ucMon  = prmtread_decimal((unsigned char *)&Syncbuf,3);//此处月的值为1－12
			CurrentTime.ucMday = prmtread_decimal((unsigned char *)&Syncbuf,4);
			CurrentTime.ucHour = prmtread_decimal((unsigned char *)&Syncbuf,5);
			CurrentTime.ucMin  = prmtread_decimal((unsigned char *)&Syncbuf,6);
			CurrentTime.ucSec  = prmtread_decimal((unsigned char *)&Syncbuf,7);
			//printf("%d-%d-%d,%d.%.d.%d\n",CurrentTime.usYear,CurrentTime.ucMon,CurrentTime.ucMday,CurrentTime.ucHour,CurrentTime.ucMin,CurrentTime.ucSec);
			sTime.serailTime.seconds = Serial_Htime(&CurrentTime); //UTC时间转化成整秒
			sTime.serailTime.nanoseconds = 0;		
			offset_time(&sTime);		 //计算串口时间和本地时间的偏差
			abjClock(sTime.SubTime); //调整偏差
		}
}




#endif



//FHGX的同步函数*********************************************************************************
void handleap(void)//FH1000接收串口时间信息
{

	if(leapNum >= 3)//连续3次闰秒预告
	{
		TimeInternal time1;
		struct ptptime_t timeupdata;
		getTime(&time1);
		//闰秒时间调整
		if(leap61 != 0)//正闰秒
		{
			if((time1.seconds%60) == 1)//g_CFL_ucSec2是收到模拟闰秒帧的一个时间每秒会加一次
			{
				timeupdata.tv_sec = -1;
				timeupdata.tv_nsec = 0;
				ETH_PTPTime_UpdateOffset(&timeupdata);
				leap61 = 0;	
				leapNum = 0;
				printf("leap61\n");
			}
		}
		else if(leap59 != 0)//负闰秒
		{
			if((time1.seconds%60) == 59)
			{
				timeupdata.tv_sec = 1;//
				timeupdata.tv_nsec = 0;
				ETH_PTPTime_UpdateOffset(&timeupdata);
				leap59 = 0;
				leapNum = 0;
				printf("leap59\n");
			}
		}
	}
}




