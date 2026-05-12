#include "type_def.h"
#include "STC8H.H"
#include "intrins.h" // _nop_() 函数



/**
NEC编码是红外遥控最常用的编码方式，其特点包括：

载波频率：NEC编码的载波频率为38 kHz。
编码方式：NEC协议采用脉冲间隔的方式编码每一位数据，用不同数据位的时间间隔来表示不同的逻辑位。
命令帧结构：
起始位：每个序列均以9ms的脉冲开始，随后是4.5ms的空闲间隔。
地址码 + 地址码反码：起始位之后会传输4个字节共32Bit的数据位，分别是地址码+地址码反码+命令码+命令码反码。其中，地址码和命令码是有效的，而地址码反码和命令码反码用于校验。
结束位：结束位为末尾的562.5μs的有效脉冲。
重复码：即使一直按住遥控器上的一个键，命令帧也只会发送一次。只要按键保持按下状态，就会每110毫秒发送一次重复码。重复码的组成为9ms的AGC脉冲+2.25ms的空闲间隔+560μs的脉冲。
脉冲宽度调制：NEC协议通过脉冲串之间的时间间隔来实现信号的调制。逻辑“0”是由0.56ms的38KHZ载波和0.560ms的无载波间隔组成；逻辑“1”是由0.56ms的38KHZ载波和1.68ms的无载波间隔组成。
NEC编码的红外信号传输协议是目前红外遥控使用最普遍的信号传输协议，它采用脉冲间距编码来表示信号





在遥控器上按某个键时，传输的消息将按顺序包含以下内容：

引导码：持续9ms 高电平，4.5ms低电平，作为启动信号；
紧接着是32bit的数据，按照上述的NEC帧格式的顺序；最后以562.5μs脉冲高电平结尾，表示一帧消息传输结束。


引导码	  表示开始发送数据
数据码0	表示数据为0
数据码1	表示数据为1
重复码  	表示重复上次的数据
结束码 	在发送完数据帧或重复帧之后的占位符


引导码：
	由9ms高电平闪亮和4.5ms低电平不亮构成，共13.5ms
	表示数据帧的开始
	
数据码0
	由0.5625ms高电平闪亮和0.5625ms低电平不亮构成，共1.125ms
	表示数据0
	
数据码1
	由0.5625ms高电平闪亮和1.6875ms（3x0.5625）低电平不亮构成，共2.25ms
	表示数据1

重复码
	由9ms高电平闪亮和2.25ms低电平不亮构成，共11.25ms
	表示重复之前的数据
	
结束位
	由0.5625ms高电平闪亮构成
	用于数据帧或重复码之后的占位
	
	
数据结构
	标准NEC编码为4byte（32bit）数据构成，
	分别是1byte地址+1byte地址反码+1byte数据+1byte数据反码

但目前很多厂商并不使用标准的NEC数据结构，可能有5byte数据，可能全部4byte都用于数据传输等等
	

https://blog.csdn.net/qq_36470994/article/details/127414629

*/


/**
// 按键使用下拉电阻，按下是1，松开是0
   下拉的按键需要把IO口设置为高阻输入
#define BUTTON_ON  1
#define BUTTON_OFF 0
*/


/* 按键使用上拉电阻，按下是0，松开是1
** 上拉的按键 不需要 把IO口设置为高阻输入，使用准双向口或高阻输入都可以，好像使用准双向口时掉电模式会更省电
** 经测试：
**	 使用高阻输入（P3M0 = 0x00; P3M1 = 0x0f; //P30 P31 P32 P33 设置为高阻输入） 这时进入掉电模式约0.019mA
**   使用准双向口，进入掉电模式约0.016mA
*/
#define BUTTON_ON  0
#define BUTTON_OFF 1



/* 
** 这3个按键需要接10K的上拉电阻
*/
#define POWER_SW_IO  			P32 // 按钮，遥控键：开关
#define INCREASE_BRIGHTNESS_IO  P33 // 按钮，遥控键：加亮
#define DECREASE_BRIGHTNESS_IO  P36	// 按钮，遥控键：减亮

#define SW_FN1_IO				P37 // 遥控按钮，功能键1
#define SW_FN2_IO				P30 // 遥控按钮，功能键2

/**
 接S8050（J3Y）三极管，NPN类型
*/
#define IR_EMITTER_ON  1   // 发射红外
#define IR_EMITTER_OFF 0   // 关闭红外

/**
 接S8050（J3Y）三极管，NPN类型
*/
#define LED_ON  1   
#define LED_OFF 0   



#define IR_EMITTER_IO	P10  // 定义红外发射的管脚
#define LED_IO			P11  // 定义LED指示灯的管脚



// 我的红外小遥控灯的按键命令定义
#define POWER_SW_IR_CODE 			0x11	// 红外命令，遥控键：开关
#define INCREASE_BRIGHTNESS_IR_CODE 0x22	// 红外命令，遥控键：加亮
#define DECREASE_BRIGHTNESS_IR_CODE 0x33	// 红外命令，遥控键：减亮

#define IR_CODE_FN1 0x51	// 红外命令，遥控键：FN1
#define IR_CODE_FN2 0x52	// 红外命令，遥控键：FN2




bit IR_Emitter_38K_Flag = 0; 			// 是否发射38k波形标志位
unsigned int IR_Emitter_Count=0;  		// 中断计数
unsigned int IR_Emitter_Set_Count = 0; 	// 设置中断计数次数


unsigned int stopModeTimeCount_13us = 0; 
unsigned int  btnTimeCount_13us = 0;



// 发射红外
void IR_Emitter_ON(){
	IR_EMITTER_IO = IR_EMITTER_ON;   
	LED_IO = LED_ON;
}


// 关闭红外
void IR_Emitter_OFF(){
	IR_EMITTER_IO = IR_EMITTER_OFF;   
	LED_IO = LED_OFF;
}


// 切换红外开关
void IR_Emitter_Toggle(){
	IR_EMITTER_IO = ~IR_EMITTER_IO;   
	LED_IO = ~LED_IO;
}


/************************ IO口配置 ****************************/
// 初始化引脚
void IO_Init(){
	P1M1 = 0X00; P1M0 = 0X00; // 准双向口
	P3M1 = 0X00; P3M0 = 0X00; // 准双向口
	P5M1 = 0X00; P5M0 = 0X00; // 准双向口

	// P55口接了红外发射LED，使用了S8550三极管（基极加1K电阻）
  // 默认的准双向口模式，可以驱动三极管控制红外发射LED
	// 改为推挽输出，效果更好了（使用的时2025纽扣电池，遥控距离明显变远了）
	// 最后改为：不设置推挽输出，基极也不加电阻了
//	P5M0 = 0x20; P5M1 = 0x00;  // P55 设置为推挽输出

	// P10, P11 设置为推挽输出
    P1M0 = 0x03; P1M1 = 0x00; 

}


void Timer0_Init(void)		//13微秒@12.000MHz
{
	AUXR |= 0x80;			//定时器时钟1T模式
	//TMOD &= 0xF0;			//设置定时器模式
	TMOD = 0x00;			//设置定时器模式
	TL0 = 0x64;				//设置定时初始值
	TH0 = 0xFF;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
}



void Timer0_Isr(void) interrupt 1
{
	stopModeTimeCount_13us++;
	btnTimeCount_13us ++;

	if(IR_Emitter_Set_Count > 0){
		IR_Emitter_Count++;   // 每中断一次加1
		
		if(IR_Emitter_38K_Flag)   // 发射38k波形标志位
		{
			// IR_EMITTER_IO = ~IR_EMITTER_IO; //有发射标志，则发射38Khz的矩形波
			IR_Emitter_Toggle();
		}
		else  // 否则不发送 相当于发射编码中的低电平
		{
			// IR_EMITTER_IO = IR_EMITTER_OFF; 
			IR_Emitter_OFF();
		}
	}
}



// 延时，单位毫秒
void delay_ms(unsigned int ms)	 //@12MHz
{
	unsigned char data i, j;
	for(ms; ms; ms--)
	{ 
		i = 16;
		j = 147;
		do
		{
			while (--j);
		} while (--i);
	}
}


u8 btnFlag = 0; // 按键是否按下的标志
u8 INT_TX_IRCode = 0;



void INT0_Isr() interrupt 0  // P32   开关
{
	btnFlag = 1;
	btnTimeCount_13us = 0; 
	INT_TX_IRCode = POWER_SW_IR_CODE;
}




void INT1_Isr() interrupt 2  // P33,  加亮
{
	btnFlag = 2;
	btnTimeCount_13us = 0; 
	INT_TX_IRCode = INCREASE_BRIGHTNESS_IR_CODE;
}


void INT2_Isr() interrupt 10  // P54,  减亮
{
	btnFlag = 3;
	btnTimeCount_13us = 0; 
	INT_TX_IRCode = DECREASE_BRIGHTNESS_IR_CODE; 
}

      

void INT3_Isr() interrupt 11	// P37, FN1
{
   btnFlag = 4;
	btnTimeCount_13us = 0; 
	INT_TX_IRCode = IR_CODE_FN1; 
}



void INT4_Isr() interrupt 16	// P30, FN2
{
    btnFlag = 5;
	btnTimeCount_13us = 0; 
	INT_TX_IRCode = IR_CODE_FN2; 
}
              


 // 红外发送一字节数据

void IR_NEC_Send_Byte(unsigned char ircode)              
{
	unsigned char i;
	
	for(i=0;i<8;i++)             // 一字节八位，循环八次
	{
		IR_Emitter_Set_Count = 43;	// 0.56ms高电平，需要进43次定时器1中断（560/13=43）
		IR_Emitter_38K_Flag = 1;	// 发射38KHz载波标志
		IR_Emitter_Count = 0;		// count置0，从这时起记录进入定时器1中断的次数

		while(IR_Emitter_Count < IR_Emitter_Set_Count);  // 在此等待，直到进入中断次数达到43次

		IR_Emitter_Set_Count = 0;               
		IR_Emitter_38K_Flag = 0;              
		IR_Emitter_Count = 0;               
 
	
		
		if(ircode&0x01)          // 数据是从最低位开始发送的，最低位是1则要进130次中断
		{
			IR_Emitter_Set_Count = 130;       // 1.69ms低电平，进中断总次数130（1690/13=130）
		}
		else                     // 最低位是0，则要进43次定时器1中断
		{
			IR_Emitter_Set_Count = 43;        // 0.565ms低电平，进中断总次数43（565/13=43）
		}

		IR_Emitter_38K_Flag = 0;              // 低电平，不需要38kHz载波
		IR_Emitter_Count = 0;
		
		while(IR_Emitter_Count < IR_Emitter_Set_Count);
		
		IR_Emitter_Set_Count = 0;               
		IR_Emitter_38K_Flag = 0;              
		IR_Emitter_Count = 0;     
		
		
		ircode = ircode >> 1;    // 将数据右移一位，即从低位到高位发送
	}
}


/**
https://blog.csdn.net/Mark_md/article/details/115053032

命令帧由 起始位 + 地址码8位 + 地址码反码8位 + 命令码8位 + 命令码反码8位  组成
标准NEC编码为4byte（32bit）数据构成，
分别是 1byte地址 + 1byte地址反码 + 1byte数据 + 1byte数据反码

但目前很多厂商并不使用标准的NEC数据结构，可能有5byte数据，可能全部4byte都用于数据传输等等
*/

/**
  * @param address 发送的地址
  * @param command 发送的命令/数据
  * @retval 无
  */
void IR_NEC_Send(unsigned char address,unsigned char command)
{
	//
	// 引导码中的9ms高电平，9000/13=692
	IR_Emitter_Set_Count = 692;               
	IR_Emitter_38K_Flag = 1;                  // 高电平需要38kHz载波
	IR_Emitter_Count = 0;               
 
	while(IR_Emitter_Count < IR_Emitter_Set_Count); // 在此等待，直到进入中断次数达到设定次数
	
	IR_Emitter_Set_Count = 0;               
	IR_Emitter_38K_Flag = 0;              
	IR_Emitter_Count = 0;               
 
	
	//
	// 引导码中4.5ms低电平，4500/13=346
	IR_Emitter_Set_Count = 346;               
	IR_Emitter_38K_Flag = 0;                  // 低电平不需要38kHz载波
	IR_Emitter_Count = 0;
 
	while(IR_Emitter_Count < IR_Emitter_Set_Count); // 在此等待，直到进入中断次数达到设定次数

	IR_Emitter_Set_Count = 0;               
	IR_Emitter_38K_Flag = 0;              
	IR_Emitter_Count = 0;       
	
	
	//
	// 发送数据
	IR_NEC_Send_Byte(address);						// 发送地址码
	IR_NEC_Send_Byte(~address);						// 发送地址码反码
	IR_NEC_Send_Byte(command);						// 发送命令码
	IR_NEC_Send_Byte(~command);						// 发送命令码反码


	//
	// 0.56ms高电平，560/13=43
	IR_Emitter_Set_Count = 43;        
	IR_Emitter_38K_Flag = 1;          // 高电平需要38kHz载波
	IR_Emitter_Count = 0;
              												
	while(IR_Emitter_Count < IR_Emitter_Set_Count); 
 	
	IR_Emitter_Set_Count = 0;               
	IR_Emitter_38K_Flag = 0;              
	IR_Emitter_Count = 0;       
	

	//
	// (NEC协议中的停止码)0.56ms低电平
	IR_Emitter_Set_Count = 43;               
	IR_Emitter_38K_Flag = 0;
	IR_Emitter_Count = 0;
	
	while(IR_Emitter_Count < IR_Emitter_Set_Count);
 
	IR_Emitter_Set_Count = 0;               
	IR_Emitter_38K_Flag = 0;              
	IR_Emitter_Count = 0;       


	//
	// 关闭红外发射
	// IR_EMITTER_IO = IR_EMITTER_OFF;     
	IR_Emitter_OFF();	
}





u8 NEC_Code_Address = 0x01; // 本遥控器的地址码


void main(){
	u8 btnValue = 0;

	EAXSFR();		// 扩展寄存器访问使能
	IO_Init();  	// 初始化引脚 
	Timer0_Init(); 	//初始化定时器0
	
	IT0 = 1;	    //使能INT0下降沿中断, P32
	EX0 = 1;    	//使能INT0中断

	IT1 = 1;		//使能INT1下降沿中断, P33
	EX1 = 1;    	//使能INT1中断

	INTCLKO = 0x10;  	//使能INT2下降沿中断
	INTCLKO |= 0x20;	//使能INT3下降沿中断
	INTCLKO |= 0x40;    //使能INT4下降沿中断

	
	EA = 1; 			// 打开中断总开关
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	
	// 关闭才能进入掉电模式
	IR_EMITTER_IO = IR_EMITTER_OFF;
	LED_IO = LED_OFF; // 默认关闭LED
	
	// https://www.stcaimcu.com/forum.php?mod=viewthread&tid=1732&highlight=%E6%8E%89%E7%94%B5%E6%A8%A1%E5%BC%8F&page=1&extra=#pid11168
	// 最好在主程序进入睡眠，至少要3个空操作（NOP）
	// 进入掉电模式
	PCON |= 0x02;
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
			
			
	while(1){
			
		if(btnFlag != 0){
			
			// 根据按键编码获取IO的按钮值 
			if(btnFlag == 1){
				btnValue = POWER_SW_IO;
			}
			else if(btnFlag == 2){
				btnValue = INCREASE_BRIGHTNESS_IO;
			}
			else if(btnFlag == 3){
				btnValue = DECREASE_BRIGHTNESS_IO;
			}
			else if(btnFlag == 4){
				btnValue = SW_FN1_IO;
			}
			else if(btnFlag == 5){
				btnValue = SW_FN2_IO;
			}
			
			// 根据按钮发射相应的红外信号
			if( (btnValue == BUTTON_ON) && (btnTimeCount_13us>= 500) ){ // 使用中断的方式，也需要按键消抖
				IR_NEC_Send(NEC_Code_Address, INT_TX_IRCode);  // 发射红外命令

				btnFlag = 0; 				// 清空按钮状态
				btnTimeCount_13us = 0;		// 计时清零
				stopModeTimeCount_13us = 0;	// 计时清零
			}	
		}
		
		
		// 超过一段时间后（毫秒），进入掉电模式，好省电
		// 必须要延后一段时间掉电才行，要留出红外发射的时间，否则可能红外命令没发送完就掉电了
		// 通过外部中断可以唤醒
		if(stopModeTimeCount_13us >= 30000){
			stopModeTimeCount_13us = 0;
			
			// 进入掉电模式，测试进入掉电模式后大约0.02mA（测试空闲模式好像没有明显省电）
			// 使用USB连接电脑时，进入掉电模式大约0.02mA
			// 使用2节7号电池，进入掉电模式大约0.4uA，
			PCON |=0x02;
			_nop_();
			_nop_();
			_nop_();
			_nop_();
			_nop_();			
		}
			
	}

}
