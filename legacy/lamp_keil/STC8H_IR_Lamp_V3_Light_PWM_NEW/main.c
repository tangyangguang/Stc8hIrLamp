#include "Config.h"
#include "STC8H.h"
#include "STC8H_PWM.h"


// =========================================== IO口定义 ===================================================

#define IO_IR_RECEIVER		P32		// IR红外接收头
#define IO_PWM_OUT			P10		// 控制CN5711的PWM输出

#define LED_IO				P11		// LED指示灯

#define LED_ON				1
#define LED_OFF 			0


#define PWM_FREQ 			2000    // PWM的频率
#define FN1_TIME_SEC		900		// 定时1秒数，15分钟
#define FN2_TIME_SEC		3600	// 定时2秒数，1小时

// =========================================== 初始化函数 ===================================================

// 初始化引脚
void IO_Init(){
	// 除P3.0和P3.1外，其余所有IO口上电后的状态均为高阻输入状态（手册P543)
	// 需要设置要用到的IO口的模式
//	P1M0 = 0x00; P1M1 = 0xfe; // P1.0准双向口，其他高阻输入
//    P3M0 = 0x00; P3M1 = 0xfb; // P3.2准双向口，其他高阻输入
	
	
	
	P1M1 = 0X00; P1M0 = 0X00; // 准双向口
	P3M1 = 0X00; P3M0 = 0X00; // 准双向口
	P5M1 = 0X00; P5M0 = 0X00; // 准双向口
	
	
	// P3.2准双向口，其他高阻输入
	P3M0 = 0x00; P3M1 = 0xfb; 
	
	// P1和P11设置为推挽输出
    P1M0 = 0x03; P1M1 = 0x00; 
}





// =========================================== PWM相关功能 ===================================================

// PWM初始化函数，使用的是P10口(PWM1P)
// 参数: channel - PWM通道, freq - PWM频率(Hz)
void PWM_Init(u8 channel, u16 freq)
{
	PWMx_InitDefine PWMx;
	u32 tmp;
	
	// 选择PWM1P的引脚为P10
	PWM1_USE_P10P11();
	
	// 计算周期值 (STC8H主频12MHz)
	// PWM周期 = (PWM_Period+1) / 12MHz
	tmp = MAIN_Fosc / freq;
	if(tmp > 0xFFFF) tmp = 0xFFFF;
	
	// 配置PWM参数
	PWMx.PWM_Mode    = CCMRn_PWM_MODE1;        // PWM模式1
	PWMx.PWM_Period  = tmp - 1;                // 周期时间，根据频率计算
	PWMx.PWM_Duty    = 0;                     // 初始占空比为0
	PWMx.PWM_DeadTime = 0;                    // 死区时间，PWM互补输出才需要
	PWMx.PWM_EnoSelect = ENO1P;               // 只使能PWM1P输出
	PWMx.PWM_CEN_Enable = ENABLE;             // 使能计数器
	PWMx.PWM_MainOutEnable = ENABLE;          // 主输出使能
	
	// 配置指定PWM通道
	PWM_Configuration(channel, &PWMx);
	  
	// 配置PWMA的基本参数
	PWM_Configuration(PWMA, &PWMx);
	
	
	// PWM1P_OUT_EN();  // 使能PWM1P输出
}

// PWM输出函数，使用的是P10口(PWM1P)
// 参数: channel - PWM通道, duty - 占空比(0-100)
void Set_PWM(u8 channel, u8 dutyPct)
{
	u32 tmp;
	u16 duty;

	tmp = MAIN_Fosc / PWM_FREQ ; // 计算占空比比例
	duty = dutyPct * tmp / 100;
	if(duty > tmp) duty = tmp;
	
	PWMA_Duty1(duty);
}

 

// =========================================== 变量定义 ===================================================

u8	PWM_Counter = 0;
u8	PWM_LOW_Start_Light_BRIGHTNESS = 0; // PWM脉宽，用于指定亮度等级，默认上一次的亮度，初始值中间值50

u16 time_50ms_count = 0;
u16 time_1sec_count = 0;

u16 PD_Mode_Delay_50ms_Counter = 0; // 延时进入省电模式的计数

// 延时，单位毫秒
void delay_ms(unsigned int ms)	 //@12MHz
{
	unsigned char data i, j;
	for(ms; ms; ms--){ 
		i = 16;
		j = 147;
		do
		{
			while (--j);
		} while (--i);
	}
}


// 进入掉电模式
void STC8_PD_Mode()
{
	// 掉电模式
	PCON |=0x02; 
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();	
}


// =========================================== 灯的控制函数 ===================================================


bit	Light_SW_State = 0;				// 0是关灯，1是开灯

u8 brightness_Levels[] = 
			{ 1, 2, 3, 5, 9, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90 };	// 亮度级别（PWM占空比）
		  //  1  2  3  4  5   6  7   8   9   10  11  12  13  14  15
u8 brightness_Levels_count = 15;	// 亮度级别的数量
u8 current_level_index = 8; 		// 当前的亮度级别（初始值为默认亮度级别）

u8  countdownEnable = 0;			// 是否开启倒计时
u8  countdown_Flashed = 0;			// 是否已经闪烁了
u16 countdownSecSetting = 0;		// 倒计时秒数



// *************************************
//
// 增加亮度：下一个亮度级别，开灯状态下才执行逻辑
// 
// *************************************
void Light_Next_Level()
{
	if(Light_SW_State == 1)
	{
		if(current_level_index < (brightness_Levels_count - 1))
		{
			current_level_index++;
		}
		PWM_LOW_Start_Light_BRIGHTNESS = brightness_Levels[current_level_index];
	}
}


// *************************************
//
// 减少亮度：上一个亮度级别, 开灯状态下才有效
// 
// *************************************
void Light_Prev_Level()
{
	if(Light_SW_State == 1)
	{
		if(current_level_index > 0)
		{
			current_level_index--;
		}
		PWM_LOW_Start_Light_BRIGHTNESS = brightness_Levels[current_level_index];
	}
}



// *************************************
//
// 开灯
// 
// *************************************
void Light_Power_On()
{
	// 初始化引脚
	IO_Init();  		 
	
	PWM1P_OUT_EN();  // 使能PWM1P输出
	
	
	// 清空状态: 关闭倒计时
	countdownEnable = 0;
	countdown_Flashed = 0;	
	countdownSecSetting = 0;
	
	// 开灯状态
	Light_SW_State = 1;
	
	/*
	// 计时器开始
	TR0 = 1;
	TR1 = 1;
		
	ET0 = 1;
	ET1 = 1;
	*/
}


// *************************************
//
// 关灯
// 
// *************************************
void Light_Power_Off()
{
	Light_SW_State	= 0;	// 关灯状态

	/*	
	// 停止计时器
	TR0 = 0;
	TR1 = 0;
	
	ET0 = 0;
	ET1 = 0;
	*/
	
	PWM1P_OUT_DIS();		// 禁止PWM1P输出
	IO_PWM_OUT = 0; 		// 低电平，关灯
	
	
	IO_PWM_OUT 		= 0;	// 停止输出, 关灯
	
	
	// P3.2准双向口，其他高阻输入
	P3M0 = 0x00; P3M1 = 0xfb; 
	
	// 设置为高阻输入，以便省电
	P1M0 = 0x00; P1M1 = 0xff;	
	
	
	delay_ms(50);
	
	// 进入省电模式
	STC8_PD_Mode();
}


// *************************************
//
// 开灯关灯切换
// 
// *************************************
void Light_Power_Toggle()
{
	if(Light_SW_State == 0)
	{
		Light_Power_On();
	}
	else
	{
		Light_Power_Off();
	}
}



// *************************************
//
// 设置定时关灯，开灯状态下才有效。参数是倒计时的秒数
// 
// *************************************

u16 Light_Timer_Sec_1 = FN1_TIME_SEC;	// 15分钟，对应 FN1 按键
u16 Light_Timer_Sec_2 = FN2_TIME_SEC;	// 60分钟，对应 FN2 按键

void Light_Turn_Off_Timer_Set(u16 sec)
{
	// 定时xx秒，开灯状态下才有效
	if(Light_SW_State == 1)
	{
		countdownEnable = 1;	
		countdown_Flashed = 0;	
		countdownSecSetting = sec;
		
		time_50ms_count = 0;
		time_1sec_count = 0;
	}
}

// *************************************
//
// 灯的循环执行逻辑
// 
// *************************************
void Light_Loop_Tick()
{
	// 关灯500ms后进入省电模式
	if(Light_SW_State == 0 && PD_Mode_Delay_50ms_Counter >= 10) 
	{
//		STC8_PD_Mode(); // 进入省电模式
	}
	
	// 定时关灯执行逻辑
	else if(Light_SW_State == 1 && countdownEnable == 1)
	{
		if(countdown_Flashed == 0 )
		{
			countdown_Flashed = 1;
			
			// 关一下灯再打开, 表示开始倒计时
			Light_SW_State = 0; 	
			PWM1P_OUT_DIS();		// 禁止PWM1P输出
			IO_PWM_OUT = 0; 		// 低电平，关灯
			
			delay_ms(100);	
			
			Light_SW_State = 1; 	
			PWM1P_OUT_EN();  // 使能PWM1P输出
		} 
	
		// 到了定时时间后关灯
		if(time_1sec_count >= countdownSecSetting)
		{
			Light_Power_Off();
		}
	}
}




// *************************************
//
// 定时器1  用于倒计时
// 
// *************************************


void Timer1_Isr(void) interrupt 3
{
	time_50ms_count++;
	if(time_50ms_count >= 20){
		time_1sec_count++;
		time_50ms_count = 0;
	}
	
	PD_Mode_Delay_50ms_Counter++;
}



void Timer1_Init(void)		//50毫秒@12.000MHz
{
	// 同时设置定时器0和1时，注意这两个寄存器需要正确设置！！
	// !!! 使用 TMOD = 0x00; 好像有问题，使用 TMOD &= 0x00; 才能正常工作！！！
	AUXR &= 0x3F;			//设置定时器0和1, 时钟12T模式
	TMOD &= 0x00;			//设置定时器0和1, 模式为16位自动重载模式
	
	// AUXR &= 0xBF;			//定时器时钟12T模式
	// TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0xB0;				//设置定时初始值
	TH1 = 0x3C;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
}



/**********************************************************************************************
***
*** 自己整理后的代码
***
*** 参考代码 https://blog.csdn.net/qq_61688416/article/details/132418785
** （STC8G红外遥控接收控制继电器案例）
***
***********************************************************************************************
**/


#define IR_RX_STATE_NONE				0 	// 未工作
#define IR_RX_STATE_RECEIVING		1		// 正在接收中


#define IR_RX_RAW_DATA_SIZE  					50   // 记录原始数据的数组大小

u8	IR_RX_State = 0; 			// 红外接收工作状态
u16	IR_RX_us_Duration = 0; // 微秒时长记录


u16 IR_RX_Time_Interval_10us = 0; // 微秒计时，记录两次红外接收引发的中断的时间间隔，单位10微秒
u16 rawData[IR_RX_RAW_DATA_SIZE] = {0}; 	// 记录原始数据，每一位数据的微秒数
u8	rawDataIndex = 0; 

u8  NEC_Data[4] = {0};	// 记录接收到的解码后的NEC数据:地址码、地址吗反码、按键码、按键码反码



/*
使用10us的定时，记录的数据就比较准确了，下面是得到的数据
	（经测试，使用定时3微秒以上的定时，才能得到准确的数据记录）
	0 = 1368   	
	1 = 117   	2 = 117   	3 = 117   	4 = 116   	
	5 = 117   	6 = 117   	7 = 117   	8 = 117   	
	9 = 226   	10 = 228   	11 = 225   	12 = 222   	
	13 = 225   	14 = 226   	15 = 225   	16 = 225   	
	17 = 225   	18 = 117   	19 = 225   	20 = 117   	
	21 = 117   	22 = 118   	23 = 225   	24 = 117   	
	25 = 116   	26 = 227   	27 = 115   	28 = 228   	
	29 = 225   	30 = 225   	31 = 115   	32 = 228   
*/
void Timer0_Init(void)		//10微秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0xF6;				//设置定时初始值
	TH0 = 0xFF;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
}


// 定时器0的中断处理, 每10微秒一次
void Timer0_Isr(void) interrupt 1
{	
	IR_RX_Time_Interval_10us++; // 微秒计时
	
	
	// 正在接收数据
	if(IR_RX_State == IR_RX_STATE_RECEIVING){ 
		
		IR_RX_us_Duration ++;  // 10微秒中断一次
		if(IR_RX_us_Duration > 11000){
			IR_RX_us_Duration = 0;
			
			IR_RX_State = IR_RX_STATE_NONE;
			IR_RX_Time_Interval_10us = 0;
			rawDataIndex = 0;
		}
		
	}
}





// 外部中断0的中断处理, 接收到红外信号时触发
void INT0_Isr() interrupt 0
{
	
	// 省电模式倒计时清零
	PD_Mode_Delay_50ms_Counter = 0;
	
	
	// 第一次进入中断（间隔为0），开始进入红外NEC编码接收流程
	if(IR_RX_State == IR_RX_STATE_NONE){
		IR_RX_Time_Interval_10us = 0;
		IR_RX_State = IR_RX_STATE_RECEIVING; 
	}
	
	// 正在接收数据
	else if(IR_RX_State == IR_RX_STATE_RECEIVING){ 
		
		// 每一步都先记录下来原始数据 (超出数组的大小就不再记录原始数据了)
		if(rawDataIndex < IR_RX_RAW_DATA_SIZE){  		
			rawData[rawDataIndex++] = IR_RX_Time_Interval_10us;
		}
		IR_RX_Time_Interval_10us = 0;		 // 微秒数记录清零
		
		// 清空超时时间记录
		IR_RX_us_Duration = 0;
	} 
}





/** 处理NEC格式的数据，进行NEC解码, 原始数据记录的单位是10微秒

		得到的是下面这样的数据（单位是10微秒）
		0 = 1372   	
		1 = 115   	2 = 117   	3 = 117   	4 = 117   	
		5 = 118   	6 = 117   	7 = 117   	8 = 117   	
		9 = 226   	10 = 226   	11 = 226   	12 = 225   	
		13 = 225   	14 = 226   	15 = 225   	16 = 225   	
		17 = 226   	18 = 117   	19 = 226   	20 = 117   	
		21 = 117   	22 = 117   	23 = 226   	24 = 117   	
		25 = 117   	26 = 225   	27 = 117   	28 = 225   	
		29 = 226   	30 = 226   	31 = 117   	32 = 225   	
		33 = 4029   	34 = 1145   	35 = 0     	36 = 0     

		[0], 			（一定有）13.5ms, 引导码(9ms LOW + 4.5ms HIGH)
		[1...8] 	（一定有）地址码 		8 Bit
		[9...16] 	（一定有）地址码反码	8 Bit
		[17...24]	（一定有）命令码			8 Bit
		[25...32]	（一定有）命令码反码	8 Bit
		
		再后面的就是重复码，本程序没有处理
	

 
		引导码：持续9ms 高电平，4.5ms低电平，作为启动信号；

		紧接着是32bit的数据，按照上述的NEC帧格式的顺序；最后以562.5μs脉冲高电平结尾，表示一帧消息传输结束。
		
		逻辑“0”：562.5μs高电平，562.5μs低电平，总时长为1.125ms
		逻辑“1”：562.5μs高电平，1.6875ms低电平，总时长为2.25ms

		
		
	返回0，表示收到的数据不符合NEC规则，没有有效数据，或者没有新数据；
  返回1，表示有新数据，且符合NEC规则，解码成功，可读取解码后的数据；	
*/
bit NEC_Decode(){
	u8 i, j, index = 0;
	
	// 没有新数据，返回解码失败
	if(rawData[0] == 0){
		return 0;
	}
	
	
	// 如果正在接收中，就等待接收完毕才能进行数据解码
	while(IR_RX_State == IR_RX_STATE_RECEIVING){ };
	
	
	// 正确的引导码是13.5ms
	if((rawData[0] > (1350-250)) && (rawData[0] < (1350+250))){

		for(i=0; i<4; i++){
			for(j=0; j<8; j++){
				index++; // 先++，从下标1开始才是数据
//				printf("%d ", rawData[index]);
				
				if(rawData[index] > (112 - 15) && rawData[index] < (112 + 15)){
					NEC_Data[i] &= ~(1 << j); // 这一位数据置0
				}
				else if(rawData[index] > (225 - 25) && rawData[index] < (225 + 25)){
					NEC_Data[i] |= (1 << j);	// 这一位数据置1
				}else{
					// 有任一个Bit的数据不符合0或1的规则，表示数据有误，返回解码失败
//					printf("faild 222 data. rawData[%d]=%d", index, rawData[index]);
	
					rawData[0] = 0;
					return 0;
				}
			}
		}
			
	}else{
		// 引导码不正确，表示数据有误，返回解码失败
//		printf("faild 111 bootcode. rawData[0]=%d \n", rawData[0]);
		for(i=0;i<32; i++){
//			printf("rawData[%d]=%d \n", (int)i, rawData[i]);
		}

		rawData[0] = 0;
		return 0; 
	}
	
	// 引导码和每一位数据都正确，清空状态，等待下一次接收，返回解码成功
	rawData[0] = 0; 
	
	return 1;
}



// LED闪烁一下
void LED_Flash(){
	LED_IO = LED_ON; // 指示灯闪烁一下
	delay_ms(50);
	LED_IO = LED_OFF;  // 指示灯闪烁一下
	
}



void main(){
		
	P_SW2 |= 0x80;  // 扩展寄存器(XFR)访问使能
	IO_Init();  		// 初始化引脚 
	
	PWM_Init(PWM1, PWM_FREQ);	// 初始化PWM，2KHz
	PWM1P_OUT_DIS();		// 禁止PWM1P输出，默认关灯
	IO_PWM_OUT = 0; 		// 默认关灯
	
	Timer0_Init();	// 初始化定时器0	
	Timer1_Init();	// 初始化定时器1
	
	IT0 = 1;      //使能INT0下降沿中断
	EX0 = 1;         //使能INT0中断

	EA=1;

	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	
	
	PWM_Counter = 0;
	Light_SW_State = 0; 	// 默认关灯
	
	LED_IO = LED_OFF; // 默认指示灯关闭

	
	
	
//	PWM_LOW_Start_Light_BRIGHTNESS = brightness_Levels[current_level_index]; // 默认亮度


	while(1)
	{
		
		
		// 灯的循环逻辑（倒计时关灯、省电模式）
		Light_Loop_Tick();
		
		
		if(NEC_Decode() == 1)
		{
			// 自制遥控器
			// [0]是地址码，[2]是命令码
			if(NEC_Data[0] == 0x01)
			{
				
				LED_Flash();  // LED闪烁一下
				
				if(NEC_Data[2] == 0x011)  			// 开关切换
				{
					Light_Power_Toggle();	
					if(Light_SW_State == 1){
						Set_PWM(PWM1, brightness_Levels[current_level_index]);
					}
				}
				else if(NEC_Data[2] == 0x22)		// 增加亮度，开灯状态下才有效
				{
					Light_Next_Level();	
					Set_PWM(PWM1, brightness_Levels[current_level_index]);
				}
				else if(NEC_Data[2] == 0x33)		// 减低亮度，开灯状态下才有效
				{
					Light_Prev_Level();	
					Set_PWM(PWM1, brightness_Levels[current_level_index]);
				}
				else if(NEC_Data[2] == 0x51)		// FN1，定时xx秒，开灯状态下才有效
				{
					Light_Turn_Off_Timer_Set(Light_Timer_Sec_1);
				}
				else if(NEC_Data[2] == 0x52)		// FN2，定时xx秒，开灯状态下才有效
				{
					Light_Turn_Off_Timer_Set(Light_Timer_Sec_2);
				}
				
				
			}
				
		}

	
	}

}

