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
u16 RAM_Test(void); //外部检测程序
u8  SRAMNotEmpty(u16  filelength);
u16	ReadSRAMChar(void);//读取SRAM的HPGL数据
u16	ReadHPGLCmd(void);//读取HPGL文件中的命令字
u16	ReadHPGLSP(void);//执行读取sp指令的下一个字符
u16	ReadHPGLPG(void);//读取PG指令后一个字节，判断是文件结束还是一个样板结束
u16	ReadHPGLPoint(Uint16	c0);//执行读取坐标点，参数为PU，或者是PD
u16	ReadHPGL(void); //执行读取HPGL指令
//u32 Time7_IQ_delay = 0;//在读字符是的延时计数值
//u8 ll=0;

//内存测试
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
//检测内存是否足够 够：1 不够：0
u8 SRAMNotEmpty(Uint16  filelength)
{
	volatile u8 b_notempty;
	if(b_ReadStartHigh == b_WirteStartHigh)//读的和写的是在一边的
	{
		if((m_ReadStartPoint+filelength)<m_HPGLEndPoint)  //*m_ReadStartPoint:切割读取指针 
		 {	b_notempty = 1;}															//*m_HPGLEndPoint:hpgl结束储存的地方
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

//读取SRAM的HPGL数据
u16	ReadSRAMChar(void)
{	
	Uint16	c1;
	u32 DX;

	if(b_ReadHPGLPG == 0)
	{//没有读到文件末尾的指令PG时
		if((m_SRAMAddr>=m_HPGLEndPoint)&&(b_ReadStartHigh==b_WirteStartHigh))
		{
			TIM7->CNT = 0;
			TIM_Cmd(TIM7,ENABLE); //使能定时器7
			while((m_SRAMAddr >= m_HPGLEndPoint)&&(b_ReadStartHigh == b_WirteStartHigh))
			{			
				if(Time7_IQ_delay >600)//延时6秒
				{
					TIM_Cmd(TIM7,DISABLE); //失能定时器7
					Time7_IQ_delay = 0;
					return HPGL_ENDCHAR;  //反馈PLT的最后一个字符
				}
				Delay_ms(2);
			}
			TIM_Cmd(TIM7,DISABLE); //失能定时器7
			Time7_IQ_delay = 0;
		}
	}
	else
	{//当已经读到PG时，文件已经读到最后，因此返回文件读取完毕标志
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
	if(m_SRAMAddr==Bank1_SRAM3_PLT_END)//读到了存储区的结束位置
	{
		m_SRAMAddr = Bank1_SRAM3_PLT_STA;//复位 将读的指针指向首地址
		if(b_ReadStartHigh)
		{	b_ReadStartHigh=0;}     
		else
		{	b_ReadStartHigh=1;}
	}
	if(m_WebEnable == 2 && USB_FileReadFinish == 0 && USBReadEnFlag == 1)
	{//U盘中文件没读取完毕，需要判断RAM-PLT中是否有足够的空间
		if(b_WirteStartHigh==b_ReadStartHigh)
		{//计算得到剩余RAM-PLT的空间大小
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
		{//空间足够，进行读取剩下的文件
			USBReadEnFlag = 0;
			USBH_Process(&USB_OTG_Core, &USB_Host);	
			Delay_ms(4);
		}
	}
	return	c1;
}


//读取HPGL文件中的命令字
Uint16	ReadHPGLCmd(void)
{	
	Uint16	c1;
	
	while(1)
	{	
		c1=ReadSRAMChar();//读取第一个字符
		if(c1=='P')
		{
			c1=ReadSRAMChar();
			switch(c1)
			{//根据第二个字符确定命令
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

//读取PG指令后一个字节，判断是文件结束还是一个样板结束
Uint16	ReadHPGLPG(void)
{
	Uint16	c1;
	while(1)
	{	
		c1=ReadSRAMChar();
		if((c1!=' ')&&(c1!=0x0a)&&(c1!=0x0d))	
		{	break;}
	}

	b_ReadHPGLPG=1;//这个函数进来的时候是已经读到PG了所以这边就直接标志位置1
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
	{//如果是文件结束符
		m_ReadStartPoint=m_SRAMAddr;
		return	HPGL_ENDCHAR;
	}
	else if(c1 == HPGL_NOCHAR)
	{
		return	RETURN_OK;
	}
	else
	{	
		if(m_SRAMAddr==Bank1_SRAM3_PLT_STA)//读到了存储区的结束位置
		{
			m_SRAMAddr = Bank1_SRAM3_PLT_END - 1;//复位 讲读的指针指向首地址
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

//执行读取坐标点，参数为PU，或者是PD
Uint16	ReadHPGLPoint(Uint16	c0)
{	
	volatile	Uint8  	b_Minus;		//1为负坐标
	volatile struct CUTPOINT  p1;
	volatile	float	fx,fy;
	//volatile  Uint8		b_OutWin;
	volatile	Uint16	c1;	
	volatile	Uint16 decimalPlace = 0;
	
repeatread:
//	b_OutWin=0;//不超出切割范围		
	b_Minus=0;
	p1.XPoint=0;
	p1.YPoint=0;
	c1=ReadSRAMChar();//读取文件中的字符
	printf("%d",c1);
	switch(c1)
	{
		case ' ':
		{
			while(1)
			{//读取第二个字符，直到读到不是空格符为止
				c1=ReadSRAMChar();
				if(c1!=' ')
				{	break;}				
			}
			break;
		}
		case HPGL_ENDCHAR: //读取的字符为文件结束字符
		{	return HPGL_ENDCHAR;}
		case ';'://读到分号、换行、回车字符
		case	0x0a: //换行
		case	0x0d: //回到本行的首位
		{	
//			m_LastHPGLCmd=c0;//保存上一次读取的字符
			m_ReadStartPoint=m_SRAMAddr;	
			if(c0==HPGL_PU)	//上一次读到的命令为PU
			{	return	RETURN_PU;}
			else
			{	return	RETURN_OK;}
		}
		case '-':
		{	b_Minus=1;}//负坐标
		case '+':
		{//正坐标
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
	{//	读x轴坐标
		if((c1>='0')&&(c1<='9'))
		{//读到的字符是0~9
			p1.XPoint=p1.XPoint*10+c1-'0';//将字符转化为数字
			c1=ReadSRAMChar();//读取下一个字符
			if(c1==HPGL_ENDCHAR) 
			{	return HPGL_ENDCHAR;}	
			
		}
		else if((c1==0x0a)||(c1==0x0d))		
		{//读到的字符是换行或回车
			if(c1==HPGL_ENDCHAR)
			{	return HPGL_ENDCHAR;}	
		}
		else
		{	break;}				
	}
	if((c1!=' ')&&(c1!=','))
	{//读到的不是正确的字符	
		m_SRAMAddr--;
		return	HPGL_ENDCHAR;
	}
	
	
	fx=(float)p1.XPoint*m_MacPara.XStepFactor+m_XOrgPos;	   // tang 2019.5.21
	if(fx>(float)((m_XRange+5.0)*m_MacPara.XStepUnit))
	{//判断是否超出切割范围	
		fx=(float)(m_XRange+5.0)*m_MacPara.XStepUnit;
		printf("超出范围");
		//b_OutWin=1;
	}
	if(b_Minus)
	{//负
		fx=-fx;
		b_Minus = 0;
//		if(fabs(fx)>200.0f)
//		{	b_OutWin=1;}		
	}
	
	
  //X坐标读取结束，读取下一个字符	
	c1=ReadSRAMChar();
	while(1)
	{//读取下一个非空格字符
		if(c1!=' ')	
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	if(c1=='+')
	{//如果是正号
		b_Minus=0;
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	else if(c1=='-')
	{//如果是负号
		b_Minus=1;
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	HPGL_ENDCHAR;}
	}
	p1.YPoint=0;
	while(1)
	{	
		if((c1<='9')&&(c1>='0'))
		{//读到的字符是0~9		
			p1.YPoint=p1.YPoint*10+c1-'0';//将字符转换为数字
			c1=ReadSRAMChar();//读取下一个字符
			if(c1==HPGL_ENDCHAR)	
			{	return	HPGL_ENDCHAR;}	
		}
		else if((c1==0x0a)||(c1==0x0d))		
		{//读到的是换行或垂直制表符	
			c1=ReadSRAMChar();//读取下一个字符
			if(c1==HPGL_ENDCHAR)
			{	return	HPGL_ENDCHAR;}
		}
		else
		{	break;}			
	}
	while(1)
	{//读取下一个非空格字符
		if(c1!=' ')	
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)//读取下一个字符
		{	return	HPGL_ENDCHAR;}
	}
	if((c1!=';')&&(c1!=0x0a)&&(c1!=0x0d))
	{
	  	if(c1==',')
	  	{	
	  		if(c0==HPGL_PD)	
	  		{	b_CommaCmd=1;}//下一个字符应该为PD的坐标
	  		else if(c0==HPGL_PU)
				{	goto	repeatread;}//如果当前命令是PU，需要读取最后一个坐标作为PU的运动目标位置
			}
	  	else
			{	return	HPGL_ENDCHAR;}
	}
	else
	{//当前曲线读取完毕 
		m_ReadStartPoint=m_SRAMAddr; 
		b_CommaCmd=0;	
	}
	
	fy=(float)p1.YPoint*m_MacPara.YStepFactor+m_YOrgPos;	    //  TANG   2019.5.21  
	if(fy>(float)((m_YRange+5.0)*m_MacPara.YStepUnit))
	{//判断是否超出切割范围	
		fy=(float)(m_YRange+5.0)*m_MacPara.YStepUnit;
		printf("超出范围进行限幅！");
		//b_OutWin=1;
	}
  	if(b_Minus)
	{//负方向
		fy=-fy;	
	}

	if(m_CurrentTool == PEN )  //笔||m_CutSelect == 2
	{//当前工具
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.PenYOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.PenXOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == CUTTER || m_CurrentTool == HALFCUT)  //刀1 || m_CutSelect == 0
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == CUTTER2 )  //刀2|| m_CutSelect == 1
	{//当前工具
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.MillYOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.MillXOffset*m_MacPara.XStepUnit - fy*m_MacPara.Vertica);
	}
	else if(m_CurrentTool == V_CUTTER )  //V冲气缸
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YVcutOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XVcutOffset*m_MacPara.XStepUnit);
	}
	else if(m_CurrentTool == PUNCH )  //打孔
	{	
		m_ReadHPGLPoint.YPoint = (long)(fy + m_MacPara.YPunchOffset*m_MacPara.YStepUnit);
		m_ReadHPGLPoint.XPoint = (long)(fx + m_MacPara.XPunchOffset*m_MacPara.XStepUnit);
	}
//	else if( m_CutSelect == 1)  //备用
//	{//当前工具
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
//	{//判断是否超出切割范围	
//		m_ReadHPGLPoint.YPoint=(float)(m_YRange+5.0)*m_MacPara.YStepUnit;
//		//b_OutWin=1;
//	}
//  else if(m_ReadHPGLPoint.YPoint<0)
//	{
//		m_ReadHPGLPoint.YPoint =0;
//	}
//	   
//	if(m_ReadHPGLPoint.XPoint>(float)((m_XRange+5.0)*m_MacPara.XStepUnit))//  TANG   2019.5.21  
//	{//判断是否超出切割范围	
//		m_ReadHPGLPoint.XPoint=(float)(m_XRange+5.0)*m_MacPara.XStepUnit;
//		//b_OutWin=1;
//	}
//  else if(m_ReadHPGLPoint.XPoint<0)
//	{
//		m_ReadHPGLPoint.XPoint =0;
//	}
	
	
	
	
	
	
	return	RETURN_OK;
}

//执行读取SP指令下一个字符
Uint16	ReadHPGLSP(void)
{
	volatile Uint16	c1,num;
	
	num=0;
	c1=ReadSRAMChar();//读取下一个字符
	while(1)
	{//读取下一个非空格字符
		if(c1!=' ')		
		{	break;}
		c1=ReadSRAMChar();
		if(c1==HPGL_ENDCHAR)	
		{	return	RETURN_ERR;}
	}	
	if(c1==HPGL_ENDCHAR)	
	{	return	RETURN_ERR;}
	while(1)
	{//读取SP后面的工具类型
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
	{//读取下一个非空格字符
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
	{//参数为不同的刀具类型
		case	2://半刀
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
		case	3://刀
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
//		case	4://刀2
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
		{	//V冲气缸
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
		{	//打孔
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
 //执行读取HPGL指令
Uint16	ReadHPGL(void)
{
	volatile Uint16	c0;
	while(1)
	{
		if(b_CommaCmd)	//上一次读取的最后一个字符识是逗号，且之前的是PD坐标
		{	c0=HPGL_PD;}
		else
		{	c0=ReadHPGLCmd();} //读取命令
		if(c0==HPGL_PU)//||((m_LastHPGLCmd==HPGL_PU)&&(c0==HPGL_PA))
		{
			c0=ReadHPGLPoint(HPGL_PU);//读取坐标
			if(c0==RETURN_PU)	//读取的命令为抬刀	
			{	return	HPGL_PU;}
			if(c0==HPGL_ENDCHAR)	//读取的命令为%
			{	return	HPGL_ENDCHAR;}
			if(m_PD_Count==1)
			{//只有一个切割点
					{	
						b_ToolChanged1=0;
						m_NextPoint2=m_ReadHPGLPoint;
					}					
			}
			else if(m_PD_Count==0)
			{//没有点
				{	
					b_ToolChanged1=0;
					m_NextPoint=m_ReadHPGLPoint; //解析PLT文件时解析得到坐标的临时存放变量
				}
			}
			if(c0!=RETURN_OK)	
			{	continue;}
			else	
			{	return HPGL_PU;}
	    }
		else if(c0==HPGL_SP)
		{//读取的命令为刀具类型的选择
			c0=ReadHPGLSP();	//读取刀具的类型
			if(c0!=RETURN_OK)	
			{	continue;}
			else			
			{	return HPGL_SP;}
		}
		else if(c0==HPGL_PG)
		{//读取的命令是一结束命令
			c0=ReadHPGLPG();//读取下一字节 根据下一字节判断是否还有样板
			if(c0==HPGL_ENDCHAR)
			{	return HPGL_ENDCHAR;}
			else
				
			{
       m_Partial_Request=1; //分版本请求
			 return HPGL_PAGE;
			}
		}		
		else if(c0==HPGL_ENDCHAR)//读取的命令是文件结束命令
		{	return	HPGL_ENDCHAR;}
		else if(c0==HPGL_PD)//||((m_LastHPGLCmd==HPGL_PD)&&(c0==HPGL_PA))
		{	
			c0=ReadHPGLPoint(HPGL_PD);	//读取坐标
			if(c0==HPGL_ENDCHAR)	//读取的命令为抬刀
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
			{//有一个点
				m_NextPoint=m_ReadHPGLPoint;
				continue;
			}	
		}
	}
}	

//判断当前读取信息是否为文件起始字符IN
u8	HPGLCheckIN(void)
{
	volatile Uint16	char1,t1;	
	t1=100;
	while(1)
	{	
		char1=ReadSRAMChar();		//读取HPGL文件的字符
		if(char1==HPGL_ENDCHAR)	
		{//读取的字符为结束字符
			m_ReadStartPoint=m_SRAMAddr;				
			return	0;
		}
		else if(t1==100)				
		{//开始寻找
			if(char1=='I')		
			{	t1=200;}
			else				
			{	t1=100;}
		}
		else if(t1==200)				
		{//已经找到I
			if(char1=='N')//应该为N	
			{	t1=300;}		
			else if(char1=='I')	
			{	t1=200;}
			else 				
			{	t1=100;}
		}
		else if(t1==300)				
		{//已经找到IN	
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






				


	










	
	
	
	
























