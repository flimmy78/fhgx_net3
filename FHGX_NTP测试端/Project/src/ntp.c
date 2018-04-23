#include "ntp.h"
#include "main.h"
#include "share.h"
extern FigStructData GlobalConfig;

extern unsigned char leap59; 
extern unsigned char leap61;
extern unsigned char leapwarning;
extern unsigned char leapflag;
extern unsigned char synflags;//�Ѿ�ͬ�����
char ntpsendf = 0;

unsigned int NTPFRAC(unsigned int x)  
{	
	return (4294 * (x) +  ((1981 * (x)) >> 11) + ((2911*(x))>>28));	//����ȷ
}

void getNtpTime( ntp_time* t_ntpTime)
{	
	TimeInternal t_psRxTime;
	getTime(&t_psRxTime);
	t_ntpTime->seconds = t_psRxTime.seconds + JAN_1970;
	t_ntpTime->fraction = NTPFRAC( t_psRxTime.nanoseconds /1000);
}


#ifdef NTP_Client
unsigned int TEST_TSSM_FLAG;
struct udp_pcb *ntpc_pcb;
ntp_msg send_pack;//�ͷ��˷����Ľṹ�����
ntp_msg revNtpData;
TimeData send_ntp_inf;
ntp_time T1_Time;//NTP�ͷ��˷���ʱ��T1
ntp_time Org_Time;//�ͷ��˷���Դʱ��
unsigned int SendNum = 0;
static ntp_time NNTime;

static void client_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
               struct ip_addr *addr, u16_t port)
 {   
	ntp_time rx_time;
  if(p->len != NTP_PCK_LEN)
  {
    pbuf_free(p);
	 	return;
  }
	if(synflags!=1)
	{
    pbuf_free(p);
	 	return;
  }
	getNtpTime(&rx_time);//�յ�NTP���������ص����ݰ���¼ʱ��
	printf("client recv %d!!\n",rx_time.seconds%60);
	MEMCPY(&revNtpData, p->payload, NTP_PCK_LEN);
	pbuf_free(p);
	
	if( ((revNtpData.status & LI_ALARM) == LI_ALARM )||
		(revNtpData.stratum ==0 )||(revNtpData.stratum > NTP_MAXSTRATUM))
		{
			//return;
		} 	
	memset(&send_ntp_inf,0,sizeof(send_ntp_inf)); 
#if 0//godin test must be use local temp time;t1ʱ��Ӧ���ñ��ر����ʱ��������ý��յ��ķ������ظ���orgtime
  if(TEST_TSSM_FLAG == Test_LOC_FERID)
		{
			send_ntp_inf.T1secondsFrom1900 = T1_temp.seconds;
			send_ntp_inf.T1fraction 			 = T1_temp.fraction;
		}
	else
		{
			send_ntp_inf.T1secondsFrom1900 = (unsigned int)ntohl(revNtpData.orgtime.seconds) ;	
			send_ntp_inf.T1fraction        = (unsigned int)ntohl(revNtpData.orgtime.fraction);
		}
#endif	//end 0
	
  send_ntp_inf.T1secondsFrom1900  = T1_Time.seconds;
	send_ntp_inf.T1fraction 			  = T1_Time.fraction;
	send_ntp_inf.T2secondsFrom1900 	= (unsigned int)ntohl(revNtpData.rxtime.seconds);
	send_ntp_inf.T2fraction 				= (unsigned int)ntohl(revNtpData.rxtime.fraction);
	send_ntp_inf.T3secondsFrom1900 	= (unsigned int)ntohl(revNtpData.txtime.seconds) ;
	send_ntp_inf.T3fraction 				= (unsigned int)ntohl(revNtpData.txtime.fraction);
	send_ntp_inf.T4secondsFrom1900 	= rx_time.seconds;
	send_ntp_inf.T4fraction 				= rx_time.fraction;
	send_ntp_inf.fjm 								= revNtpData.refid;
	send_ntp_inf.CFL_leapflag 			= revNtpData.status;//�����LI ,VN,MODE����Ϣ������,
  SendNum++;
	send_ntp_inf.frame_head 	= 0xeb;
  send_ntp_inf.frame_head1 	= 0x90;
	send_ntp_inf.frame_head2	= 0xeb;
  send_ntp_inf.frame_head3	= 0x90;
  send_ntp_inf.data_type 		= 0x01;
	send_ntp_inf.updateNum 		= SendNum;
	send_ntp_inf.frame_end0 	= 0xef;
	send_ntp_inf.frame_end1		= 0x9f;
	send_ntp_inf.frame_end2		= 0xef;
	send_ntp_inf.frame_end3 	= 0x9f;
	if(synflags)                                       
	{
		UARTSend((unsigned char *)&send_ntp_inf,sizeof(send_ntp_inf));//���͵�PC
	}	
}

void Ntp_ClientInit(unsigned int ip)
{
	struct ip_addr ipaddr;
  ipaddr.addr = htonl(ip);//�����ֽ���ת��Ϊ�����ֽ���(���ڴ��ڴ�����IP��ַ�ǵ��ֽ���ǰ)	
	ntpc_pcb = udp_new();
	udp_bind(ntpc_pcb, IP_ADDR_ANY,NTP_PORT);//
	udp_connect(ntpc_pcb, &ipaddr, NTP_PORT);// ���ӶԶ�
	udp_recv(ntpc_pcb,client_recv, NULL);		//client_recv���ղ�������������ص����ݰ�
	printf("ntp client init\n");
}

void NTPClientLoop(void)//NTP�ͷ���ѭ�����͵�������
{	
	struct pbuf *psend = NULL;
	ntp_time NtpgetTime;
	getNtpTime(&NtpgetTime);//����ƫ��
	if((ntpsendf == 1)&& synflags )//1s ����ƫ��
	{
		if(abs(NtpgetTime.seconds - NNTime.seconds)<1)
			return;
		ntpsendf = 0;
		NNTime.seconds 		= NtpgetTime.seconds;
		
		memset(&send_pack,0,NTP_PCK_LEN);
		send_pack.status 	= DEFAULT_STATUS;
		send_pack.stratum = DEFAULT_STRATUM;//0
		send_pack.poll 		= DEFAULT_POLL;	
		send_pack.precision 	= DEFAULT_PREC;
		send_pack.rootdelay 	= DEFAULT_RTDELAY;
		send_pack.dispersion 	= DEFAULT_DISPER;
		send_pack.refid 			= GlobalConfig.Reidf;
		
		psend = pbuf_alloc(PBUF_TRANSPORT, NTP_PCK_LEN, PBUF_RAM);
		if(psend == NULL)
		{
			pbuf_free(psend);
			return;
		}		
		getNtpTime(&Org_Time);	          
		T1_Time.seconds  = Org_Time.seconds;
		T1_Time.fraction = Org_Time.fraction;

		//if(send_pack.refid != Test_LOC_FERID)//�������TSSM �ͽ�T1��ʱ���õ�ǰ������ʱ��
		if(GlobalConfig.tmp1==0)//��T1��ʱ���õ�ǰ������ʱ��
		{
			send_pack.txtime.seconds = htonl(Org_Time.seconds);
			send_pack.txtime.fraction= htonl(Org_Time.fraction);
		}
		MEMCPY(psend->payload,&send_pack, NTP_PCK_LEN);
		udp_send(ntpc_pcb,psend);				
		pbuf_free(psend);
		myprintf("ntp Client sent %d %x\n",Org_Time.seconds%60,send_pack.refid);
	}
}

#endif //end NTP_Client



//#ifdef NTP_Server
//struct udp_pcb 	*ntps_pcb;
//extern unsigned char leap61;
//extern unsigned char leap59;
//extern unsigned char leapwring;

////NTP����������Ntp���ݰ�������
//void server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
//    ip_addr_t *addr, u16_t port)
//{
//		struct pbuf *reply_pbuf = NULL;
//		ntp_time rx_time, tx_time;
//		ntp_msg query, reply;
//		ip_addr_t Dstaddr;
//		 
//		//���������յ���Ϣʱ��ȡһ��ʱ��
//		getNtpTime(&rx_time);
//		if(p->len != NTP_PCK_LEN)//�ж��յ������ݳ����Ƿ����
//		{
//			pbuf_free(p);
//			return;
//		}
//		memset(&reply,0,NTP_PCK_LEN);//��ջظ�����
//		MEMCPY(&query,p->payload,NTP_PCK_LEN);//
//		
//		pbuf_free(p);
//		
//		if(leapwring)
//		{
//			reply.status  = LI_ALARM; //δͬ��
//		}
//		if(leap61)
//		{
//			reply.status = LI_PLUSSEC;
//		}
//		else if(leap59)
//		{
//			reply.status = LI_MINUSSEC;
//		}
//		else
//			reply.status = LI_NOWARNING;//����ʱ��
//		
//		reply.status |= (query.status & VERSIONMASK);//���յ���query.statut�ֶθ���reply //�汾��
//	
//		//send_pack.status = DEFAULT_STATUS;�ͷ��˷��͵���0xdbδͬ��
//		switch(query.status & MODEMASK)//�жϷ���query����������״̬
//		{	
//			case ACT_MOD:
//				reply.status |= PAS_MOD; 
//				break;
//			case CLIENT_MOD:
//			case BDC_MOD: 
//				if(GlobalConfig.ip_mode == 2) //0(����) 1(�鲥) 2(�㲥)//godin ֧�ֹ㲥ģʽ
//					reply.status |=	BDC_MOD;//�㲥ģʽ
//				else
//					reply.status |= SERVER_MOD;//�ͷ���Ϊ3,�����������ֶ�Ϊ4
//				break;
//			default:
//				return;
//		}

//		//�������
//		reply.stratum 	 = LOC_STRATUM;//ʱ�Ӳ�ͱ���ʱ�Ӿ���
//		reply.poll 			 = query.poll; //������Ϣ��������
//		reply.precision  = LOC_PRECISION;//����ʱ�Ӿ���
//		reply.rootdelay  = LOC_RTDELAY;
//		reply.dispersion = LOC_DISPER;
//		reply.refid 		 = LOC_FERID;		 //

//		if(query.refid == Test_LOC_FERID)//����������refid��Test_LOC_FERID������ͬ��
//		{
//			reply.refid = Test_LOC_FERID;
//		}
//		//T1
//		reply.orgtime.seconds		= query.txtime.seconds; //NTP����ʱ��T1
//		reply.orgtime.fraction	= query.txtime.fraction;//
//		//T2
//		reply.rxtime.seconds 		= htonl(rx_time.seconds) ;//query�Ľ���ʱ��T2
//		reply.rxtime.fraction 	= htonl(rx_time.fraction);//
//		
//		reply_pbuf = pbuf_alloc(PBUF_TRANSPORT,NTP_PCK_LEN,PBUF_RAM);//����һ��reply_pbuf���ڷ���reply
//		if(reply_pbuf == NULL) 
//		{
//			return;	
//		}
//		
//		getNtpTime(&tx_time);//�ظ�ʱ��
//		//T3
//		reply.reftime.seconds		= htonl(tx_time.seconds);//�������ݰ�ʱ��
//    reply.reftime.fraction	= htonl(tx_time.fraction);
//		//T4
//		reply.txtime.seconds		= htonl(tx_time.seconds);//��ʱ�û������ݰ�ʱ�����
//		reply.txtime.fraction		= htonl(tx_time.fraction);//
//		
//		MEMCPY(reply_pbuf->payload, &reply, NTP_PCK_LEN);
//		
//		if(GlobalConfig.ip_mode == 2)
//		{
//			Dstaddr.addr = htonl(0xffffffff); //�㲥
//			udp_sendto(pcb, reply_pbuf, &Dstaddr, port);
//		}
//		else
//			udp_sendto(pcb, reply_pbuf, addr, port);//˭�����һ��͸�˭
//		
//		pbuf_free(reply_pbuf);
//		//printf("ntp server send time %d\n",tx_time.seconds%60);
//}

//void NTP_ServerInit(void)
//{
//	ntps_pcb = udp_new();
//	udp_bind(ntps_pcb,IP_ADDR_ANY,NTP_PORT);
//	udp_recv(ntps_pcb, server_recv, NULL);
//	printf("ntp Server init\n");
//}	
//#endif	//end NTP_Server


//��ʼ��NTP������
void NTP_Init(void)
{
#ifdef NTP_Server   //godin ntp����������
	NTP_ServerInit();
#elif defined(NTP_Client)
	Ntp_ClientInit(GlobalConfig.DstIpaddr);
	myprintf("DstIp:%x",GlobalConfig.DstIpaddr);
#endif
	printf("---------NTP init finish---------\r\n");
}

