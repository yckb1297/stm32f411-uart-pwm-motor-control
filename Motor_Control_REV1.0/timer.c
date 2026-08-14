#include "device_driver.h"
#include <stdio.h>

#define TIM2_TICK	20	//usec
#define TIM2_FREQ	(1000000./TIM2_TICK)	//Hz로 단위 통일하기 위해 1000000.을 곱해줌. 계산할 때 오버플로우 방지를 위해 괄호 쓰기.
#define TIM2_1ms_Pls	(TIM2_FREQ/1000.)
#define TIM2_MAX		(0xffffffff)

void TIM2_One_Shot(void){
	Macro_Set_Bit(RCC->APB1ENR, 0);		//TMR2 en set
	Macro_Write_Block(TIM2->CR1, 0x3, 0x3, 3);		//down count, one pulse
	TIM2->PSC = (int)(TIMXCLK/TIM2_FREQ +0.5) - 1;	//분주비 1920, 50kHz
	TIM2->ARR = 150000;
	Macro_Set_Bit(TIM2->EGR, 0);	//Update event Generation
	Macro_Clear_Bit(TIM2->SR, 0);	// UIF(Update Interrupt Pending) Clear
	Macro_Set_Bit(TIM2->CR1, 0);	//TIM2 Start
}

int TIM2_Check_Timeout(void){

	if(Macro_Check_Bit_Set(TIM2->SR, 0)){
		printf("Timeout !!!\n");
		Macro_Clear_Bit(TIM2->SR, 0);
		return 1;
	}
	else           
		return 0;
}

void TIM2_Stop(void)
{
	Macro_Clear_Bit(TIM2->CR1, 0);
}