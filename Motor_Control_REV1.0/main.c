#include "device_driver.h"
#include <stdio.h>

static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	Uart1_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
}

#if 1 //모터 방향 버튼 제어, 3초 이상 누르면 정지
// 버튼을 떼지 않아도 3초가 지나면 끝나도록.
// 루프에 갇히지 않게.

void Main(void)
{
	Sys_Init(115200);
	GPIO_Init();
	printf("Motor test_3sec_stop_v2\n");
#if 1
/*
 * [리팩토링 노트 - 이벤트 처리 구조 개선]
 *
* 1. 기존 코드(#else)의 문제
 *    - '버튼 눌림' 조건문 안에 '타이머 만료'와 '버튼 뗌' 검사를 같이 넣어서 정상적으로 동작하지 않았음.
 *    - 버튼을 누른 순간에 바로 버튼을 뗐는지, 3초가 지났는지를 검사하다 보니
 *      버튼 뗌 감지가 아예 안 되거나 타이머 오작동이 발생함.
 *
 * 2. 수정 코드(#if 1)의 내용
 *    - 버튼 눌림(Press), 버튼 뗌(Release), 타이머 만료(Timeout)를 
 *      각각 별개의 if문으로 빼서 독립적으로 작동하게 함.
 *    - 이제 버튼을 누르고 있을 때, 뗐을 때, 3초 타임아웃이 발생했을 때
 *      서로 방해하지 않고 각각 제때 감지되어 정상 동작함.
 */
	enum Motor_state state = STOP;

	int prev_btn = 0;	//이전 버튼 상태, Release: 0, Press: 1
	int is_timer_started = 0; // 타이머가 현재 동작 중인지

	for (;;) {

		int current_btn = !Macro_Extract_Area(GPIOC->IDR, 0x1, 13);
		int is_timeout = TIM2_Check_Timeout();

		//Falling Edge(Button Pressed)
		if (prev_btn == 0 && current_btn) {
			prev_btn = 1;
			printf("button pressed\n");

			TIM2_One_Shot();      // 3초 타이머 시작
			is_timer_started = 1;
		}
		//Rising Edge(Button Released)
		else if (prev_btn == 1 && current_btn == 0) {
			prev_btn = 0;
			printf("button released\n");

			if (is_timer_started) {
				is_timer_started = 0;
				TIM2_Stop();

				Toggle_Motor_State(&state);
			}
		}

		//Timer Expired
		if (is_timeout) {
			printf("is timeout\n");
			is_timer_started = 0; // 타이머 종료 처리

			Motor_Stop();
			state = STOP;
		}
	}

#else
	enum Motor_state state = STOP;

	int prev_btn = 0;
	int is_timer_started = 0;

	for (;;){

		int current_btn = !Macro_Extract_Area(GPIOC->IDR, 0x1, 13);
		printf("current_btn = %d\n", current_btn);
		int is_timeout = TIM2_Check_Timeout();
		printf("is_timeout: %d\n", is_timeout);

		if(prev_btn == 0 && current_btn){
			printf("button pressed\n");
			TIM2_One_Shot();
			is_timer_started = 1;

			if(is_timer_started){
				is_timer_started = 0;
				printf("timer_started\n");

				if(is_timeout){
					is_timeout = 0;
					prev_btn = 1;

					Motor_Stop();
					state = STOP;
				}
				else if (current_btn == 0){ 

					prev_btn = 1;

					if(state == STOP){
						Motor_CW();
						state = CW;
					}
					else{
						if(state == CW){
							Motor_Short_Stop();
							Motor_CW();
							state = CCW;
						}
						else if(state == CCW){
							Motor_Short_Stop();
							Motor_CW();
							state = CW;
						}
					}
				}
			}
		}
		else if (prev_btn &&current_btn == 0){
			prev_btn = 0;
			printf("button released\n");
		}
	}
#endif
}

#endif

#if 0 //모터 방향 UART 제어
void Main(void)
{
	Sys_Init(115200);
	printf("Motor test_UART\n");
	Uart1_Init(115200);

    Macro_Set_Bit(RCC->AHB1ENR, 0);	//GPIOA En
	Macro_Set_Bit(RCC->AHB1ENR, 3);	//GPIOC En

    Macro_Write_Block(GPIOA->MODER,0xf, 0x5, 0);	//PA0, PA1 General output
    Macro_Write_Block(GPIOA->OTYPER, 0x3, 0x0, 0);	//PA0, PA1 Push-pull
	Macro_Write_Block(GPIOC->MODER,0x3, 0x0, 26);	//key mode input
	
	char x;

	for (;;){

		while(!Macro_Check_Bit_Set(USART1->SR, 7));
		USART1->DR = x;
		while(!Macro_Check_Bit_Set(USART1->SR, 5));
		x = USART1->DR;
		
		if(x == 'S' || x == 's'){
			Macro_Write_Block(GPIOA->ODR, 0x3, 0x3, 0);
			printf("정지\n");
		}
		else if(x == 'F' || x == 'f'){
			Macro_Write_Block(GPIOA->ODR, 0x3, 0x3, 0);
			TIM4_Delay(500);
			Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
			printf("정방향\n");
		}
		else if(x == 'B' || x == 'b'){
			Macro_Write_Block(GPIOA->ODR, 0x3, 0x3, 0);
			TIM4_Delay(500);
			Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
			printf("역방향\n");
		}
	}	
}
#endif

#if 0 //모터 방향 버튼 제어, 3초 이상 누르면 정지
void Main(void)
{
	Sys_Init(115200);
	printf("Motor test_3sec_stop\n");
	Uart1_Init(115200);

    Macro_Set_Bit(RCC->AHB1ENR, 0);	//GPIOA En
	Macro_Set_Bit(RCC->AHB1ENR, 3);	//GPIOC En

    Macro_Write_Block(GPIOA->MODER,0xf, 0x5, 0);	//PA0, PA1 General output
    Macro_Write_Block(GPIOA->OTYPER, 0x3, 0x0, 0);	//PA0, PA1 Push-pull
	Macro_Write_Block(GPIOC->MODER,0x3, 0x0, 26);	//key mode input

	int CW = 0;	//0: CW, 1: CCW
	int btn_flag = 0;
	unsigned int time = 0;


	for (;;){
		if (Macro_Check_Bit_Clear(GPIOC->IDR, 13))
		{
			TIM2_Stopwatch_Start();
			while(!Macro_Check_Bit_Set(GPIOC->IDR, 13));
			time = TIM2_Stopwatch_Stop();
			printf("time: %d", time);
			btn_flag = 1;

			if(btn_flag == 1){
				btn_flag = 0;

				if(time >= 3000000){
				Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
				printf("3초 초과");
				}
				else{
					printf("3초 미만");

					if(CW == 0){		//정방향
						printf("정방향");
						Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
						TIM2_Delay(1000);
						Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
						CW = 1;
					}
					else if(CW == 1){	//역방향
						printf("역방향");
						Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
						TIM2_Delay(1000);
						Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
						CW = 0;
					}
				}
				time = 0;
			}
		}		
	}
}	
#endif


#if 0 //모터 방향 버튼 제어
void Main(void)
{
	Sys_Init(115200);
	printf("Motor test\n");
	Uart1_Init(115200);

    Macro_Set_Bit(RCC->AHB1ENR, 0);	//GPIOA En
	Macro_Set_Bit(RCC->AHB1ENR, 3);	//GPIOC En

    Macro_Write_Block(GPIOA->MODER,0xf, 0x5, 0);	//PA0, PA1 General output
    Macro_Write_Block(GPIOA->OTYPER, 0x3, 0x0, 0);	//PA0, PA1 Push-pull
	Macro_Write_Block(GPIOC->MODER,0x3, 0x0, 26);	//key mode input

	int lock = 0;

	for (;;){
			if((lock == 0) && (Macro_Check_Bit_Clear(GPIOC->IDR, 13))){
				Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
				TIM2_Delay(1000);
				Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
				lock = 1;
			}
			else if((lock == 1) && (Macro_Check_Bit_Clear(GPIOC->IDR, 13))){
				Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
				TIM2_Delay(1000);
				Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
				lock = 0;
			}
	}

}	

#endif