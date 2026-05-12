#include "STC8H.h"
#include	"IR_PWM_NEC_Sender.h"










// 使用PWM方式发送指定数据
/**
	@txData_us_Values 	要发送的数据，已经转换后的使用微秒us表示的数据内容（一个值表示一个数据bit，+号表示高电平，-号表示低电平，数值表示时长，单位微秒us）
	@len 数组长度
*/
void IR_PWM_Send(int txData_us_Values[],  unsigned int len)
{
	unsigned char i;
	int dataByte;
	
	// 发送数据
	for(i=0; i < len; i++){
		dataByte = txData_us_Values[i];
		
		if(dataByte == 0){
			continue;
		}
		
		IR_Emitter_38K_Flag = (dataByte > 0);    		// 高电平需要38kHz载波
		IR_Emitter_Set_Count = (dataByte>0 ? dataByte : (0-dataByte)) / 13;  // 定时器0设定的是13us触发一次，所以除以13达到设定的时长
		IR_Emitter_Count = 0; 
		
		TR0 = 1;                 												// 定时器0开启计时
		while(IR_Emitter_Count < IR_Emitter_Set_Count); // 在此等待，直到进入中断次数达到设定次数，即达到了设定的时长
		TR0 = 0;                										 		// 定时器0关闭计时
	}
	
	
	// 关闭红外发射
	IR_EMITTER_IO = IR_EMITTER_OFF;                  
}




