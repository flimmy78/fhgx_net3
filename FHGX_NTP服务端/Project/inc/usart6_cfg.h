
#ifndef __USART6_CFG_H
#define __USART6_CFG_H

#include "ntp.h"
#include "main.h"
#include "flash_conf.h"




	 


 
struct ConfigPTPData //�������µ�ptp����֡
{
	unsigned char  frame_head;  	//*		0x2A
	unsigned char	 cmd_type;   		//0x01 ���Զ�ȷ������,0x02���Զ˿�ʼ����,0x03���Զ�ֹͣ����,,0x04�����ȷ������
	unsigned char  temp0;					//���� 0x00
	unsigned char	 temp1; 				//���� 0x00
	unsigned int	 Local_ip_32;		// Local ip 		4					   			
	unsigned int	 Gate_ip_32;		// Gate ip      4
	unsigned int   Mask_ip_32; 		// mask ip      4
	unsigned int	 temp2;		     	//���� 0x00000000
	unsigned int	 temp3;   			//���� 0x00000000
	unsigned int	 temp4; 			 	//���� 0x00000000   		
	unsigned int	 temp5; 				//���� 0x00000000
	unsigned char  ethernet_mode; //Э����� MAC��0x01  UDP��0x02
	unsigned char  E2E_mode;      //��·��ʽ E2E: 0x01  P2P��0x02
	unsigned char	 slaveOnly;     //���ӷ�ʽ: ֻ���� 0x00
	unsigned char  twoStep;       //һ���������� ֻ�������� 0x00
	unsigned char  temp10;				//���� 0x00
	unsigned char	 temp11;   			//���� 0x00
	unsigned char	 temp12;        //���� 0x00                                   
	unsigned char  frame_end;     //#		0x23
};



struct ConfigNTPData
{
	unsigned char  frame_head; 	  //*		0x2A
	unsigned char	 cmd_type;			//0x01 ���Զ�ȷ������,0x02���Զ˿�ʼ����,0x03���Զ�ֹͣ����,,0x04�����ȷ������
	unsigned char  temp0;					//���� 0x00	
	unsigned char	 refto;					//���� 0x00
	unsigned int	 Local_ip_32;		// Local ip 						
	unsigned int	 Server_ip_32;	// Server ip 	   			
	unsigned int	 Gate_ip_32;		// Gate ip 
	unsigned int   Mask_ip_32;		// mask ip     
	unsigned int	 testMark;	 		// ʱ��Դ��ʶ("TSSM"/"Erro")
	unsigned int	 temp3;					//���� 0x00000000 
	unsigned int	 temp4;		 			//���� 0x00000000
	unsigned char  temp5;				 	//���� 0x00 
	unsigned char  temp6; 				//���� 0x00 
	unsigned char	 temp7;					//���� 0x00 
	unsigned char  frame_end;     // #	 0x23
};











void Gx_HandlePTPFIG(void);
void Gx_HandleNTPFIG(void);
void Gx_HandleNTPFIGS(void);

#endif
