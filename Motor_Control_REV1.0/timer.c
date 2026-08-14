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

void TIM2_Stopwatch_Start(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 0);		//tim2 en set.

	// TIM2 CR1 설정: down count, one pulse
	Macro_Write_Block(TIM2->CR1, 0x3, 0x3, 3);
	//TIM2->CR1 = (0x1 << 4) | (0x1 << 3);	매크로만 쓰지 말고 비트로 직접 하는 것도 연습해야겟어...
	// PSC 초기값 설정 => 20usec tick이 되도록 설계 (50KHz)
	TIM2->PSC = TIMXCLK * TIM2_TICK/1000000 - 1;
	// TIM2->PSC = (int)(TIMXCLK/TIM2_FREQ +0.5) - 1;		//TIMXCLK/TIM2_FREQ하면 실수니까 +0.5하고 int형으로 보정.
	// TIM2->PSC = TIMXCLK * (TIM2_TICK/1000000.) - 1;		//오버플로우 방지하려면 실수형으로(. 추가) 만들어서 괄호 사용.
	// TIM2->PSC = 1920-1;
	//  ARR 초기값 설정 => 최대값 0xFFFF 설정
	TIM2->ARR = 0xffffffff;
	// UG 이벤트 발생
	Macro_Set_Bit(TIM2->EGR, 0);
	// TIM2 start
	Macro_Set_Bit(TIM2->CR1, 0);
}

unsigned int TIM2_Stopwatch_Stop(void)
{
	unsigned int time;

	// TIM2 stop
	Macro_Clear_Bit(TIM2->CR1, 0);
	// CNT 초기 설정값 (0xffff)와 현재 CNT의 펄스수 차이를 구하고
	// 그 펄스수 하나가 20usec이므로 20을 곱한값을 time에 저장
	time = (TIM2->ARR - TIM2->CNT) * TIM2_TICK;
	// 계산된 time 값을 리턴(단위는 usec)
	return time;
}

void TIM2_Delay(int time)		//시간 제약이 거의 없는 delay함수
{
	Macro_Set_Bit(RCC->APB1ENR, 0);

	// TIM2 CR1 설정: down count, one pulse
	TIM2->CR1 = (0x1 << 4) | (0x1 << 3);
	// PSC 초기값 설정 => 20usec tick이 되도록 설계 (50KHz)
	TIM2->PSC = TIMXCLK * (TIM2_TICK/1000000.) - 1;
	// ARR 초기값 설정 => 요청한 time msec에 해당하는 초기값 설정
	unsigned int pls = TIM2_1ms_Pls * time;		//pls의 최댓값이 셀 수 있는 최댓값. 42억/50-> 1억msec 정도.
	int n = pls / TIM2_MAX;
	int m = pls % TIM2_MAX;
	int i;

	for (i = 0; i < n; i++){
		TIM2->ARR = TIM2_MAX;
		Macro_Set_Bit(TIM2->EGR, 0);
		Macro_Clear_Bit(TIM2->SR, 0);
		Macro_Set_Bit(TIM2->CR1, 0);
		while(!Macro_Check_Bit_Set(TIM2->SR, 0));
	}
	TIM2->ARR = m;
	// UG 이벤트 발생
	Macro_Set_Bit(TIM2->EGR, 0);
	// UIF(Update Interrupt Pending) Clear
	Macro_Clear_Bit(TIM2->SR, 0);
	// TIM2 start
	Macro_Set_Bit(TIM2->CR1, 0);
	// Wait timeout
	while(!Macro_Check_Bit_Set(TIM2->SR, 0));


	// TIM2 Stop
	Macro_Clear_Bit(TIM2->CR1, 0);
}

void TIM4_Delay(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 2);

	// TIM2 CR1 설정: down count, one pulse
	TIM4->CR1 = (0x1 << 4) | (0x1 << 3);
	// PSC 초기값 설정 => 20usec tick이 되도록 설계 (50KHz)
	TIM4->PSC = TIMXCLK * (TIM2_TICK/1000000.) - 1;
	// ARR 초기값 설정 => 요청한 time msec에 해당하는 초기값 설정
	TIM4->ARR = time * TIM2_1ms_Pls;
	// UG 이벤트 발생
	Macro_Set_Bit(TIM4->EGR, 0);

	// UIF(Update Interrupt Pending) Clear
	Macro_Clear_Bit(TIM4->SR, 0);
	// TIM2 start
	Macro_Set_Bit(TIM4->CR1, 0);
	// Wait timeout
	while(!Macro_Check_Bit_Set(TIM4->SR, 0));
	// TIM2 Stop
	Macro_Clear_Bit(TIM4->CR1, 0);
}


void TIM4_Repeat(int time)
{
	Macro_Set_Bit(RCC->APB1ENR, 2);
	
	// TIM4 CR1: ARPE=0, down counter, repeat mode
	TIM4->CR1 = (0x0 << 7) | (0x1 << 4) | (0x0 << 3);
	// PSC(50KHz),  ARR(reload시 값) 설정
	TIM4->PSC = TIMXCLK * (TIM2_TICK/1000000.) - 1;
	TIM4->ARR = time * TIM2_1ms_Pls;
	// UG 이벤트 발생
	Macro_Set_Bit(TIM4->EGR, 0);
	// Update Interrupt Pending Clear
	Macro_Clear_Bit(TIM4->SR, 0);
	// TIM4 start
	Macro_Set_Bit(TIM4->CR1, 0);
}

int TIM4_Check_Timeout(void)
{
	// 타이머가 timeout 이면 1 리턴, 아니면 0 리턴
	if(Macro_Check_Bit_Set(TIM4->SR, 0)){
		Macro_Clear_Bit(TIM4->SR, 0);
		return 1;
	}
	return 0;
}

void TIM4_Stop(void)
{
	Macro_Clear_Bit(TIM4->CR1, 0);
}

void TIM4_Change_Value(int time)
{
	TIM4->ARR = 50 * time;
}
