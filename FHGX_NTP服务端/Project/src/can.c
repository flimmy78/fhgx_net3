//////////////////////////////////////////////////////////////////////////////////	 
//创建日期:2016/5/26
//版本：V1.0
//ZX						  
//修改记录：1）2016.06.21修改接受完成条件为CAN1_RX1_STA&0xffff0000	
//修改记录：2）2016.11.09延迟上电后，读板卡地址的时间，并且修改CAN，滤波器设置
////////////////////////////////////////////////////////////////////////////////// 
#include "can.h"
#include "main.h"
#include "lcdmessage.h"
//bit15，	接收完成标志
//
//bit13~0，	接收到的有效字节数目	

//uint16_t BoardADDR;
uint16_t ReadBoardADDR__(void)
{
	uint16_t read_addr;
	read_addr  =	GPIO_ReadInputData(GPIOE);	
	read_addr &= 	0x7F; //保留低8位
	return read_addr;
}

uint32_t CAN1_SEND_ID;

u8 CAN1_RX0_BUF[CAN1_RX0_LEN]; 	//CAN接收BUF,最大CAN1_RX0_LEN个字节.
u16 CAN1_RX0_STA=0;       			//接收状态标记	

 /**
  * @brief  CAN1初始化
  * @param  无
  * @retval 初始化成功->0
  */
u8 CAN1_Mode_Init(void)
{
		u8 READ_SLOT;
		u8 READ_PORT;
		uint32_t MASK_ID_H,MASK_ID_L,i;
		uint16_t BoardADDR =0xffff;
		uint16_t temp			 =0xffff;
		uint8_t key=1;

	
  	GPIO_InitTypeDef 				GPIO_InitStructure; 
	  CAN_InitTypeDef       	CAN_InitStructure;
  	CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
	
		
		for (i = 0; i < 100000000; i++);//延迟了最少6ns*100 000 000=600ms之后再读板卡地址
		while(key)
			{
				BoardADDR=ReadBoardADDR__();//读插槽网口地址
				printf("read boardaddr 1st %x\n",BoardADDR);
				for (i = 0; i < 100000; i++);
				temp=ReadBoardADDR__();			//读插槽网口地址
				printf("read boardaddr 2nd %x\n",temp);
				if((BoardADDR==temp)&&(BoardADDR!=0xffff))
					key=0;
			}
		READ_SLOT=(BoardADDR>>3)+2;
		READ_PORT=BoardADDR&0x7;
		MASK_ID_H=((6<<8)|(READ_SLOT<<4)|READ_PORT)<<3;
		MASK_ID_L=(0x0904<<3)	;
		CAN1_SEND_ID=((8<<24)|(READ_SLOT<<20)|(READ_PORT<<16)|0x0940);
    //使能相关时钟
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能PORTB时钟	    
  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟	
    //初始化GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11| GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	  //引脚复用映射配置
	  GPIO_PinAFConfig(GPIOA,GPIO_PinSource11,GPIO_AF_CAN1); 
	  GPIO_PinAFConfig(GPIOA,GPIO_PinSource12,GPIO_AF_CAN1); 

		CAN_InitStructure.CAN_TTCM=DISABLE;			   //MCR-TTCM  关闭时间触发通信模式使能
		CAN_InitStructure.CAN_ABOM=DISABLE;			   //MCR-ABOM  关闭自动离线管理 
		CAN_InitStructure.CAN_AWUM=DISABLE;			   //MCR-AWUM  关闭自动唤醒模式
		CAN_InitStructure.CAN_NART=ENABLE;			   //MCR-NART  开启报文自动重传	  
		CAN_InitStructure.CAN_RFLM=DISABLE;			   //MCR-RFLM  接收FIFO 不锁定 溢出时新报文会覆盖原有报文  
		CAN_InitStructure.CAN_TXFP=DISABLE;			   //MCR-TXFP  发送FIFO优先级 取决于报文标示符 
		CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  //正常工作模式
		CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;		  //BTR-SJW 重新同步跳跃宽度 1个时间单元
		CAN_InitStructure.CAN_BS1=CAN_BS1_4tq;		  //BTR-TS1 时间段1 占用了4个时间单元
		CAN_InitStructure.CAN_BS2=CAN_BS1_2tq;		  //BTR-TS1 时间段2 占用了2个时间单元	   
		CAN_InitStructure.CAN_Prescaler =6;		  		//BTR-BRP 波特率分频器  定义了时间单元的时间长度 (CAN 时钟频率为 APB 1 = 42 MHz) 42/(1+4+2)/6=1 Mbps
		
  	CAN_Init(CAN1, &CAN_InitStructure); // 初始化CAN1 
		//配置过滤器0
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //过滤器0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 	//掩码模式
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 	//32位 																				
  	CAN_FilterInitStructure.CAN_FilterIdHigh=MASK_ID_H;      				//只筛选CAN1_MASK_ID的报文
  	CAN_FilterInitStructure.CAN_FilterIdLow=MASK_ID_L;																				
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0xFFFF;     				//32位MASK（设置为1的位数，表示与FilterId相同即筛选位成功）
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0xF800;							//0xffff右移了3位
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//过滤器0关联到FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //激活过滤器0
  	CAN_FilterInit(&CAN_FilterInitStructure);//滤波器初始化		
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0消息挂号中断允许.		    
  
		return 0;
}   
 /**
  * @brief  CAN1_RX0消息挂号中断服务函数，接受POWERPC每秒动态信息
  * @param  无
  * @retval 无
  */		 

void CAN1_RX0_IRQHandler(void)
{
	CanRxMsg RxMessage;
	u32 i, FRAME_LEN,FRAME_NUMS,bufoff,FRAME_REV; 
	//u8 sendmsg[8]={0};
	if((CAN1_RX0_STA&0xff00)!=0)		//接收完成标志	2016.06.21修改接受完成条件为CAN1_RX1_STA&0xffff0000
		CAN1->RF0R |= CAN_RF0R_RFOM0; 		//清FIFO_0挂号消息
	else
	{
		CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
		if(RxMessage.RTR)
			{
				CAN1_RX0_STA|=0x200;
			}
		else
			{
				FRAME_NUMS=RxMessage.ExtId&0x0f;     	//帧序号
				bufoff=FRAME_NUMS<<3;			//计算BUF存放的地址偏移量
				FRAME_LEN=(RxMessage.ExtId&0xf0)>>4; 	//报文包帧数量
				FRAME_REV=(CAN1_RX0_STA&0xff)>>3;		//收到帧数=(字节数/8)
				if(4==FRAME_LEN)							//
					{
						if(FRAME_REV==FRAME_LEN)		//2016.08.15修改：1.报文包帧数量=帧序号=收到帧数2.报文包帧数量=帧序号=0接受完成	
							CAN1_RX0_STA|=0x100;
					}	
				else if(FRAME_NUMS==0)							//如果收到报文包头，重新开始接受
					CAN1_RX0_STA=0;
				for(i=0;i<RxMessage.DLC;i++)				
					{
						CAN1_RX0_BUF[i+(bufoff)]=RxMessage.Data[i];
						CAN1_RX0_STA++;
						if((CAN1_RX0_STA&0xff)>(CAN1_RX0_LEN-1))
							CAN1_RX0_STA=0;//接收数据错误,重新开始接收	
					}	
			}
	
	}		
}


 /**
  * @brief  CAN发送邮箱状态查询
	* @param  CANx:CAN口x
	* @retval 0,有空邮箱; 1:无空邮箱;
  */	
uint8_t CAN_GETMAILFLAG(CAN_TypeDef* CANx)
{
  uint8_t transmit_mailbox = 0;
  /* Check the parameters */
  assert_param(IS_CAN_ALL_PERIPH(CANx));
  /* Select one empty transmit mailbox */
  if ((CANx->TSR&CAN_TSR_TME0) == CAN_TSR_TME0)
  {
    transmit_mailbox = 0;
  }
  else if ((CANx->TSR&CAN_TSR_TME1) == CAN_TSR_TME1)
  {
    transmit_mailbox = 1;
  }
  else if ((CANx->TSR&CAN_TSR_TME2) == CAN_TSR_TME2)
  {
    transmit_mailbox = 2;
  }
  else
  {
    transmit_mailbox = CAN_TxStatus_NoMailBox;
  }
  if (transmit_mailbox != CAN_TxStatus_NoMailBox)
		return 0;
	else
		return 1;
}


 /**
  * @brief  CAN1发送数据函数

  * @param  msg:数据指针,最大为8个字节
	* @param  ext_id:发送的CAN扩展ID
	* @param	len:数据长度(最大为8)
	* @retval 0,成功; 1:发送失败，2:总线繁忙/故障;
  */	
u8 CAN1_Send_Msg(u8* msg,u32 ext_id,u8 len)
{	
  u8 mbox;
  u32 i=0;
  CanTxMsg TxMessage;                   //定义结构体TxMessage 
	TxMessage.ExtId=ext_id;	 							//设置扩展标示符（29位）      	0 to 0x1FFFFFFF.  
  TxMessage.IDE=CAN_Id_Extended;		  	//使用扩展格式
  TxMessage.RTR=CAN_RTR_Data;		  			//消息类型为数据帧
  TxMessage.DLC=len;							 			//数据长度(规约中约定为固定8)
  for(i=0;i<len;i++)
		{
			TxMessage.Data[i]=msg[i];	
		}
	while(CAN_GETMAILFLAG(CAN1))
		{
			i++;	//等待邮箱
			if(i>=0XFFFF)
				{
					printf("can send no mailbox!\n");
					return 2;
				}						
		}
	mbox= CAN_Transmit(CAN1, &TxMessage); 
  i=0;
  while(CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)
		{
			i++;	//等待发送结束
			if(i>=0XFFFF)
				{
					printf("can send time put!\n");
					return 1;
				}
		}
	return 0;		
}



void CAN1_Send_u32msg(u32* u32msg,u32 ext_id)
{	
	u8 msg[8];
	msg[0] = ((u32msg[0])&0xff000000)>>24;
	msg[1] = ((u32msg[0])&0xff0000)>>16;
	msg[2] = ((u32msg[0])&0xff00)>>8;
	msg[3] = ((u32msg[0])&0xff);
	msg[4] = ((u32msg[1])&0xff000000)>>24;
	msg[5] = ((u32msg[1])&0xff0000)>>16;
	msg[6] = ((u32msg[1])&0xff00)>>8;
	msg[7] = ((u32msg[1])&0xff);
  CAN1_Send_Msg(msg,ext_id,8);
}



