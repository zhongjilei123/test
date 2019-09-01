#include "sys.h"
#include "Analysis.h"
#include "stdlib.h"
#include "math.h"
#include "MangeSRAM.h"
#include "delay.h"
#include "sram.h"
#include "public.h"
#include <stdio.h>
/**
  ******************************************************************************
  * @file    Analysis
  * @author  
  * @version V1.0
  * @date    18-May-2019
  * @brief   main to use analysis  plt file
  ******************************************************************************/
extern USBH_HOST  USB_Host;
extern USB_OTG_CORE_HANDLE  USB_OTG_Core;
u16 RAM_Test(void); //�ⲿ������
u8  SRAMNotEmpty(u16  filelength);
u16	ReadSRAMChar(void);//��ȡSRAM��HPGL����
u16	ReadHPGLCmd(void);//��ȡHPGL�ļ��е�������
u16	ReadHPGLSP(void);//ִ�ж�ȡspָ�����һ���ַ�
u16	ReadHPGLPG(void);//��ȡPGָ���һ���ֽڣ��ж����ļ���������һ���������
u16	ReadHPGLPoint(Uint16	c0);//ִ�ж�ȡ����㣬����ΪPU��������PD
u16	ReadHPGL(void); //ִ�ж�ȡHPGLָ��
//u32 Time7_IQ_delay = 0;//�ڶ��ַ��ǵ���ʱ����ֵ
//u8 ll=0;

//�ڴ����
u16 RAM_Test(void)
{
	volatile u16 RAM_Section;
	volatile u16 Test_Data;
	volatile Uint16 *RAM_Addr;
	u16 RAM_Result,i;	
	Test_Data=0x0001;
	RAM_Addr=(volatile u16 *)0x68000000;
	while(1)
	{
		*RAM_Addr=Test_Data;
		RAM_Addr++;
		Test_Data++;
		for(i=0;i<10;i++)
		{__nop();}
		if(RAM_Addr==(volatile Uint16 *)0x6807FFFF)
			break;	
	}
	*RAM_Addr=0x2626;
	for(i=0;i<10;i++)
	{__nop();}	
	RAM_Addr=(volatile Uint16 *)0x68000000;
	Test_Data=0x0001;
	delay_us(2000);
	while(1)
	{
		RAM_Section=*RAM_Addr;
    if(RAM_Section==Test_Data)
		{	
			RAM_Addr++;
			Test_Data++;
			for(i=0;i<5;i++)
			{	__nop();}
			if(RAM_Addr==(volatile Uint16 *)0x6807FFFF)
			{	
				RAM_Result=1;
				break;
			}
		}				
		else
		{	
			RAM_Result=0;
			break;
		}	
	}
	return RAM_Result;			
}	
//����ڴ��Ƿ��㹻 ����1 ������0
u8 SRAMNotEmpty(Uint16  filelength)
{
	volatile u8 b_notempty;
	if(b_ReadStartHigh == b_WirteStartHigh)//���ĺ�д������һ�ߵ�
	{
		if((m_ReadStartPoint+filelength)<m_HPGLEndPoint)  //*m_ReadStartPoint:�и��ȡָ�� 
		 {	b_notempty = 1;}															//*m_HPGLEndPoint:hpgl��������ĵط�
		else
		 {	b_notempty = 0;}
	}
	else
	{
	   if(((Bank1_SRAM3_PLT_END-m_ReadStartPoint)+\
		     (m_HPGLEndPoint-Bank1_SRAM3_PLT_STA))>filelength)
		 {	b_notempty = 1;}
		 else
		 {	b_notempty = 0;}
	}
	return b_notempty;
}  

//��ȡSRAM��HPGL����
u16	ReadSRAMChar(void)
{	
	Uint16	c1;
	u32 DX;

	if(b_ReadHPGLPG == 0)
	{//û�ж����ļ�ĩβ��ָ��PGʱ
		if((m_SRAMAddr>=m_HPGLEndPoint)&&(b_ReadStartHigh==b_WirteStartHigh))
		{
			TIM7->CNT = 0;
			TIM_Cmd(TIM7,ENABLE); //ʹ�ܶ�ʱ��7
			while((m_SRAMAddr >= m_HPGLEndPoint)&&(b_ReadStartHigh == b_WirteStartHigh))
			{			
				if(Time7_IQ_delay >600)//��ʱ6��
				{
					TIM_Cmd(TIM7,DISABLE); //ʧ�ܶ�ʱ��7
					Time7_IQ_delay = 0;
					return HPGL_ENDCHAR;  //����PLT�����һ���ַ�
				}
				Delay_ms(2);
			}
			TIM_Cmd(TIM7,DISABLE); //ʧ�ܶ�ʱ��7
			Time7_IQ_delay = 0;
		}
	}
	else
	{//���Ѿ�����PGʱ���ļ��Ѿ����������˷����ļ���ȡ��ϱ�־
		if((m_SRAMAddr>=m_HPGLEndPoint)&&(b_ReadStartHigh==b_WirteStartHigh))
			return HPGL_NOCHAR;
	}
	if(b_ReadStartHigh)
	{	
		c1=((*m_SRAMAddr)&0xff00)>>8;
	}
	else
	{
		c1=(*m_SRAMAddr)&0x00ff;
	}
	m_SRAMAddr++;	
	if(m_SRAMAddr==Bank1_SRAM3_PLT_END)//�����˴洢���Ľ���λ��
	{
		m_SRAMAddr = Bank1_SRAM3_PLT_STA;//��λ ������ָ��ָ���׵�ַ
		if(b_ReadStartHigh)
		{	b_ReadStartHigh=0;}     
		else
		{	b_ReadStartHigh=1;}
	}
	if(m_WebEnable == 2 && USB_FileReadFinish == 0 && USBReadEnFlag == 1)
	{//U�����ļ�û��ȡ��ϣ���Ҫ�ж�RAM-PLT���Ƿ����㹻�Ŀռ�
		if(b_WirteStartHigh==b_ReadStartHigh)
		{//����õ�ʣ��RAM-PLT�Ŀռ��С
			if(m_HPGLEndPoint >= m_SRAMAddr)
			{	
			DX=(Bank1_SRAM3_PLT_END-Bank1_SRAM3_PLT_STA)+(Bank1_SRAM3_PLT_END-Bank1_SRAM3_PLT_STA)-(m_HPGLEndPoint-m_SRAMAddr);}
			else 
			{
				DX=m_SRAMAddr-m_HPGLEndPoint;
			}		
		}
		else
		{	
			  DX=(m_SRAMAddr-Bank1_SRAM3_PLT_STA)+(Bank1_SRAM3_PLT_END-m_HPGLEndPoint);
		}	
		if(DX > TCP_CLIENT_RX_BUFSIZE * 10)
		{//�ռ��㹻�����ж�ȡʣ�µ��ļ�
			USBReadEnFlag = 0;
			USBH_Process(&USB_OTG_Core, &USB_Host);	
			Delay_ms(4);
		}
	}
	return	c1;
}


//��ȡHPGL�ļ��е�������
Uint16	ReadHPGLCmd(void)
{	
	Uint16	c1;
	
	while(1)
	{	
		c1=ReadSRAMChar();//��ȡ��һ���ַ�
		if(c1=='P')
		{
			c1=ReadSRAMChar();
			switch(c1)
			{//���ݵڶ����ַ�ȷ������
				case	'U':	return 	HPGL_PU;
				case	'D':	return	HPGL_PD;
				//case	'A':	return	HPGL_PA;
				case	'G':	return	HPGL_PG;
				case    HPGL_ENDCHAR:
				{	return  HPGL_ENDCHAR;}
				default:break;
			}	
		}
		else if(c1=='S')
		{
			c1=ReadSRAMChar();
			if(c1=='P')			
			{	return	HPGL_SP;}
			if(c1==HPGL_ENDCHAR)
			{	return  HPGL_ENDCHAR;}
		}
		else if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
}

//��ȡPGָ���һ���ֽڣ��ж����ļ���������һ���������
Uint16	ReadHPGLPG(void)
{
	Uint16	c1;
	while(1)
	{	
		c1=ReadSRAMChar();
		if((c1!=' ')&&(c1!=0x0a)&&(c1!=0x0d))	
		{	break;}
	}

	b_ReadHPGLPG=1;//�������������ʱ�����Ѿ�����PG��������߾�ֱ�ӱ�־λ��1
	if(c1==';')	
	{	
		m_ReadStartPoint=m_SRAMAddr;
		c1=ReadSRAMChar();
	}
	while(1)
	{	
		if((c1!=' ')&&(c1!=0x0a)&&(c1!=0x0d)&&(c1!=';'))	
		{	break;}
		c1=ReadSRAMChar();
	}
	b_ReadHPGLPG=0;
	if(c1==HPGL_ENDCHAR)	
	{//������ļ�������
		m_ReadStartPoint=m_SRAMAddr;
		return	HPGL_ENDCHAR;
	}
	else if(c1 == HPGL_NOCHAR)
	{
		return	RETURN_OK;
	}
	else
	{	
		if(m_SRAMAddr==Bank1_SRAM3_PLT_STA)//�����˴洢���Ľ���λ��
		{
			m_SRAMAddr = Bank1_SRAM3_PLT_END - 1;//��λ ������ָ��ָ���׵�ַ
			if(b_ReadStartHigh)
			{	b_ReadStartHigh=0;}     
			else
			{	b_ReadStartHigh=1;}
		}
		else 
			m_SRAMAddr--;
		return	RETURN_OK;
	}
}

//ִ�ж�ȡ����㣬����ΪPU��������PD
Uint16	ReadHPGLPoint(Uint16	c0)
{	
	volatile	Uint8  	b_Minus;		//1Ϊ������
	volatile struct CUTPOINT  p1;
	volatile	float	fx,fy;
	//volatile  Uint8		b_OutWin;
	volatile	Uint16	c1;	
	volatile	Uint16 decimalPlace = 0;
	
repeatread:
//	b_OutWin=0;//�������иΧ		
	b_Minus=0;
	p1.XPoint=0;
	p1.YPoint=0;
	c1=ReadSRAMChar();//��ȡ�ļ��е��ַ�
	printf("%d",c1);
	switch(c1)
	{
		case ' ':
		{
			while(1)
			{//��ȡ�ڶ����ַ���ֱ���������ǿո��Ϊֹ
				c1=ReadSRAMChar();
				if(c1!=' ')
				{	break;}				
			}
			break;
		}
		case HPGL_ENDCHAR: //��ȡ���ַ�Ϊ�ļ������ַ�
		{	return HPGL_ENDCHAR;}
		case ';'://�����ֺš����С��س��ַ�
		case	0x0a: //����
		case	0x0d: //�ص����е���λ
		{	
//			m_LastHPGLCmd=c0;//������һ�ζ�ȡ���ַ�
			m_ReadStartPoint=m_SRAMAddr;	
			if(c0==HPGL_PU)	//��һ�ζ���������ΪPU
			{	return	RETURN_PU;}
			else
			{	return	RETURN_OK;}
		}
		case '-':
		{	b_Minus=1;}//������
		case '+':
		{//������
			c1=ReadSRAMChar();
			while(1)
			{	
				if(c1!=' ')		
				{	break;}
				else		
				{	c1=ReadSRAMChar();}
			}			
		}
		default: break;		
	}
	if(c1==HPGL_ENDCHAR)		
	{	return	HPGL_ENDCHAR;}	
	while(1)
	{//	��x������
		if((c1>='0')&&(c1<='9'))
		{//�������ַ���0~9
			p1.XPoint=p1.XPoint*10+c1-'0';//���ַ�ת��Ϊ����
			c1=ReadSRAMChar();//��ȡ��һ���ַ�
			if(c1==HPGL_ENDCHAR) 
			{	return HPGL_ENDCHAR;}	
			
		}
		else if((c1==0x0a)||(c1==0x0d))		
		{//�������ַ��ǻ��л�س�
			if(c1==HPGL_ENDCHAR)
			{	return HPGL_ENDCHAR;}	
		}
		else
		{	break;}				
	}
	if((c1!=' ')&&(c1!=','))
	{//�����Ĳ�����ȷ���ַ�	
		m_SRAMAddr--;
		return	HPGL_ENDCHAR;
	}
	
	
	fx=(float)p1.XPoint*m_MacPara.XStepFactor+m_XOrgPos;	   // tang 2019.5.21
	if(fx>(float)((m_XRange+5.0)*m_MacPara.XStepUnit))
	{//�ж��Ƿ񳬳��иΧ	
		fx=(float)(m_XRange+5.0)*m_MacPara.XStepUnit;
		printf("������Χ");
		//b_OutWin=1;
	}
	if(b_Minus)
	{//��
		fx=-fx;
		b_Minus = 0;
//		if(fabs(fx)>200.0f)
//		{	b_OutWin=1;}		
	}
	
	
  //X�����ȡ��������ȡ��һ���ַ�	
	c1=ReadSRAMChar();
	while(1)
	{//��ȡ��һ���ǿո��ַ�
		if(c1!=' ')	
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	if(c1=='+')
	{//���������
		b_Minus=0;
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	else if(c1=='-')
	{//����Ǹ���
		b_Minus=1;
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	p1.YPoint=0;
	while(1)
	{	
		if((c1<='9')&&(c1>='0'))
		{//�������ַ���0~9		
			p1.YPoint=p1.YPoint*10+c1-'0';//���ַ�ת��Ϊ����
			c1=ReadSRAMChar();//��ȡ��һ���ַ�
			if(c1==HPGL_ENDCHAR)	
			{	return	HPGL_ENDCHAR;}	
		}
		else if((c1==0x0a)||(c1==0x0d))		
		{//�������ǻ��л�ֱ�Ʊ��	
			c1=ReadSRAMChar();//��ȡ��һ���ַ�
			if(c1==HPGL_ENDCHAR)
			{	return	HPGL_ENDCHAR;}
		}
		else
		{	break;}			
	}
	while(1)
	{//��ȡ��һ���ǿո��ַ�
		if(c1!=' ')	
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)//��ȡ��һ���ַ�
		{	return	HPGL_ENDCHAR;}
	}
	if((c1!=';')&&(c1!=0x0a)&&(c1!=0x0d))
	{
	  	if(c1==',')
	  	{	
	  		if(c0==HPGL_PD)	
	  		{	b_CommaCmd=1;}//��һ���ַ�Ӧ��ΪPD������
	  		else if(c0==HPGL_PU)
				{	goto	repeatread;}//�����ǰ������PU����Ҫ��ȡ���һ��������ΪPU���˶�Ŀ��λ��
			}
	  	else
			{	return	HPGL_ENDCHAR;}
	}
	else
	{//��ǰ���߶�ȡ��� 
		m_ReadStartPoint=m_SRAMAddr; 
		b_CommaCmd=0;	
	}
	
	fy=(float)p1.YPoint*m_MacPara.YStepFactor+m_YOrgPos;	    //  TANG   2019.5.21  
	if(fy>(float)((m_YRange+5.0)*m_MacPara.YStepUnit))
	{//�ж��Ƿ񳬳��иΧ	
		fy=(float)(m_YRange+5.0)*m_MacPara.YStepUnit;
		printf("������Χ�����޷���");
		//b_OutWin=1;
	}
  	if(b_Minus)
	{//������
		fy=-fy;	
	}

	if(m_CurrentTool == PEN )  //��||m_CutSelect == 2
	{//��ǰ����
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.PenYOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.PenXOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == CUTTER || m_CurrentTool == HALFCUT)  //��1 || m_CutSelect == 0
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == CUTTER2 )  //��2|| m_CutSelect == 1
	{//��ǰ����
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.MillYOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.MillXOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == V_CUTTER )  //V������
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YVcutOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XVcutOffset*m_MacPara.XStepUnit);
	}
	else if(m_CurrentTool == PUNCH )  //���
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YPunchOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XPunchOffset*m_MacPara.XStepUnit);
	}
//	else if( m_CutSelect == 1)  //����
//	{//��ǰ����
//		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.MillYOffset*m_MacPara.YStepUnit);
//		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.MillXOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
//	}
	else
	{	
		m_ReadHPGLPoint.YPoint = (long)fy;
		m_ReadHPGLPoint.XPoint = (long)(fx - fy*m_MacPara.Vertica);
	}
	if(((u32)fx>m_MaxiumPoint.XPoint)&&(c0==HPGL_PD))
	{	m_MaxiumPoint.XPoint=(long)fx;}
	if(((u32)fy>m_MaxiumPoint.YPoint)&&(c0==HPGL_PD))	
	{	m_MaxiumPoint.YPoint=(long)fy;}
	
	
//	if(m_ReadHPGLPoint.YPoint>(float)((m_YRange+5.0)*m_MacPara.YStepUnit))//  TANG   2019.5.21  
//	{//�ж��Ƿ񳬳��иΧ	
//		m_ReadHPGLPoint.YPoint=(float)(m_YRange+5.0)*m_MacPara.YStepUnit;
//		//b_OutWin=1;
//	}
//  else if(m_ReadHPGLPoint.YPoint<0)
//	{
//		m_ReadHPGLPoint.YPoint =0;
//	}
//	   
//	if(m_ReadHPGLPoint.XPoint>(float)((m_XRange+5.0)*m_MacPara.XStepUnit))//  TANG   2019.5.21  
//	{//�ж��Ƿ񳬳��иΧ	
//		m_ReadHPGLPoint.XPoint=(float)(m_XRange+5.0)*m_MacPara.XStepUnit;
//		//b_OutWin=1;
//	}
//  else if(m_ReadHPGLPoint.XPoint<0)
//	{
//		m_ReadHPGLPoint.XPoint =0;
//	}
	
	
	
	
	
	
	return	RETURN_OK;
}

//ִ�ж�ȡSPָ����һ���ַ�
Uint16	ReadHPGLSP(void)
{
	volatile Uint16	c1,num;
	
	num=0;
	c1=ReadSRAMChar();//��ȡ��һ���ַ�
	while(1)
	{//��ȡ��һ���ǿո��ַ�
		if(c1!=' ')		
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	RETURN_ERR;}
	}	
	if(c1==HPGL_ENDCHAR)	
	{	return	RETURN_ERR;}
	while(1)
	{//��ȡSP����Ĺ�������
		if((c1<='9')&&(c1>='0'))
		{	
			num=num*10+c1-'0';
			c1=ReadSRAMChar();
			if(c1==HPGL_ENDCHAR)	
			{	return	RETURN_ERR;}
		}
		else
		{	break;}
	}
	while(1)
	{//��ȡ��һ���ǿո��ַ�
		if(c1!=' ')
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	RETURN_ERR;}
	}
	if((c1!=';')&&(c1!=0x0a)&&(c1!=0x0d))	
	{	return	RETURN_ERR;}
	m_ReadStartPoint=m_SRAMAddr;
	switch(num)
	{//����Ϊ��ͬ�ĵ�������
		case	2://�뵶
		if(m_MacPara.HalfCutPre)
		{	if(m_CurrentTool==HALFCUT)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=HALFCUT;		
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}
		case	3://��
		//case	4:
		{	
			m_CurrentCutterFlag=1;
			VIBRATION_ON(); 
			if(m_CurrentTool==CUTTER)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=CUTTER;			
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}
		case 4:
		{	
			m_CurrentCutterFlag=2;
//			CUTTER_ON(); 
			if(m_CurrentTool==CUTTER2)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=CUTTER2;			
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}			
//		case	4://��2
//		{	
//			if(m_CurrentTool==MILLCUT)
//			{	return	RETURN_ERR;}
//			else
//			{	
//				m_NewTools=MILLCUT;			
//				b_ToolChanged=1;
//				b_ToolChanged1=1;
//			}
//			break;
//		}
		case  0:
		case	1:
		{	
			if(m_CurrentTool==PEN)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=PEN;			
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}	
		case	5:
		{	//V������
			if(m_CurrentTool==V_CUTTER)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=V_CUTTER;					
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}	
		case	6:
		{	//���
			if(m_CurrentTool==PUNCH)
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=PUNCH;					
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}	
		default://?
		{	
			if(m_CurrentTool==PEN)	
			{	return	RETURN_ERR;}
			else
			{	
				m_NewTools=PEN;			
				b_ToolChanged=1;
				b_ToolChanged1=1;
			}
			break;
		}
    }
	return	RETURN_OK;						
}
 //ִ�ж�ȡHPGLָ��
Uint16	ReadHPGL(void)
{
	volatile Uint16	c0;
	while(1)
	{
		if(b_CommaCmd)	//��һ�ζ�ȡ�����һ���ַ�ʶ�Ƕ��ţ���֮ǰ����PD����
		{	c0=HPGL_PD;}
		else
		{	c0=ReadHPGLCmd();} //��ȡ����
		if(c0==HPGL_PU)//||((m_LastHPGLCmd==HPGL_PU)&&(c0==HPGL_PA))
		{
			c0=ReadHPGLPoint(HPGL_PU);//��ȡ����
			if(c0==RETURN_PU)	//��ȡ������Ϊ̧��	
			{	return	HPGL_PU;}
			if(c0==HPGL_ENDCHAR)	//��ȡ������Ϊ%
			{	return	HPGL_ENDCHAR;}
			if(m_PD_Count==1)
			{//ֻ��һ���и��
					{	
						b_ToolChanged1=0;
						m_NextPoint2=m_ReadHPGLPoint;
					}					
			}
			else if(m_PD_Count==0)
			{//û�е�
				{	
					b_ToolChanged1=0;
					m_NextPoint=m_ReadHPGLPoint; //����PLT�ļ�ʱ�����õ��������ʱ��ű���
				}
			}
			if(c0!=RETURN_OK)	
			{	continue;}
			else	
			{	return HPGL_PU;}
	    }
		else if(c0==HPGL_SP)
		{//��ȡ������Ϊ�������͵�ѡ��
			c0=ReadHPGLSP();	//��ȡ���ߵ�����
			if(c0!=RETURN_OK)	
			{	continue;}
			else			
			{	return HPGL_SP;}
		}
		else if(c0==HPGL_PG)
		{//��ȡ��������һ��������
			c0=ReadHPGLPG();//��ȡ��һ�ֽ� ������һ�ֽ��ж��Ƿ�������
			if(c0==HPGL_ENDCHAR)
			{	return HPGL_ENDCHAR;}
			else
				
			{
       m_Partial_Request=1; //�ְ汾����
			 return HPGL_PAGE;
			}
		}		
		else if(c0==HPGL_ENDCHAR)//��ȡ���������ļ���������
		{	return	HPGL_ENDCHAR;}
		else if(c0==HPGL_PD)//||((m_LastHPGLCmd==HPGL_PD)&&(c0==HPGL_PA))
		{	
			c0=ReadHPGLPoint(HPGL_PD);	//��ȡ����
			if(c0==HPGL_ENDCHAR)	//��ȡ������Ϊ̧��
			{	return	HPGL_ENDCHAR;}
			if(c0!=RETURN_OK)	
			{	continue;}
			m_PD_Count++;
			if(m_PD_Count==2)
			{
				m_NextPoint2=m_ReadHPGLPoint;
				return	HPGL_PD;
			}
			else if(m_PD_Count==1)
			{//��һ����
				m_NextPoint=m_ReadHPGLPoint;
				continue;
			}	
		}
	}
}	

//�жϵ�ǰ��ȡ��Ϣ�Ƿ�Ϊ�ļ���ʼ�ַ�IN
u8	HPGLCheckIN(void)
{
	volatile Uint16	char1,t1;	
	t1=100;
	while(1)
	{	
		char1=ReadSRAMChar();		//��ȡHPGL�ļ����ַ�
		if(char1==HPGL_ENDCHAR)	
		{//��ȡ���ַ�Ϊ�����ַ�
			m_ReadStartPoint=m_SRAMAddr;				
			return	0;
		}
		else if(t1==100)				
		{//��ʼѰ��
			if(char1=='I')		
			{	t1=200;}
			else				
			{	t1=100;}
		}
		else if(t1==200)				
		{//�Ѿ��ҵ�I
			if(char1=='N')//Ӧ��ΪN	
			{	t1=300;}		
			else if(char1=='I')	
			{	t1=200;}
			else 				
			{	t1=100;}
		}
		else if(t1==300)				
		{//�Ѿ��ҵ�IN	
			if((char1==';')||(char1==0x0a )||(char1==0x0d)||(char1==' '))
			{	
				t1=800;	
				m_ReadStartPoint=m_SRAMAddr;
				return 1;
			}
			else if(char1=='I')	
			{	t1=200;}
		}	
	}	
}






				


	










	
	
	
	
























