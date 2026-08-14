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

#if 1 	//REV1.1
/*
모터 방향 버튼 제어, 3초 이상 누르면 정지
	- 버튼을 떼지 않아도 3초가 지나면 정지

모터 속도 PWM, UART로 제어

이벤트 처리에 인터럽트 사용

루프에 갇히지 않도록 설계
*/

/* Global Variable Definition */
volatile KEY_STATE key_state = 0U;		//Key Press/Release INT flag

volatile int TIM2_Expired = 0U;			//TIM2 Expired INT flag
volatile int TIM4_Expired = 0U;			//TIM4 Expired INT flag

volatile int Uart_Data_In = 0U;			//UART INT flag
volatile MOTOR_SPEED curr_motor_speed = SPD_5;

volatile MOTOR_STATE next_state = 0;


void Main(void)
{
	/* 초기화 */
	Sys_Init(115200);
	GPIO_Init();
	TIM5_Out_Init();					//PWM Enable
	printf("\nMotor_test_rev1.1\n");
			
	/* Variable Definition */
	MOTOR_STATE motor_state = STOP;		// 모터 상태 정지로 초기화
	int is_timer_started = 0; 			// 타이머가 현재 동작 중인지, 1: 동작중(타임아웃 이전)

	/* Interrupt Enable */
	Key_ISR_Enable(1);					//Key Rising/Fallind Edge INT
	Uart2_RX_Interrupt_Enable();		//Uart RX INT Enable
	//TIM2, TIM4는 타이머 실행 함수에 INT를 함께 허용해놓아서 다로 Enable함수를 호춣하지 않음.

	TIM5_PWM_Generation(curr_motor_speed);	//PWM 기본 duty 설정: 70
	
	for (;;) {

		//Falling Edge INT
		if (key_state == IS_PRESSED) {
			key_state = KEY_IDLE;
			printf("button pressed\n");
			
			TIM2_3sec_Timer_Start();		// 3초 타이머 시작
			is_timer_started = 1;
		}
		//Rising Edge INT
		else if (key_state == IS_RELEASED) {
			key_state = KEY_IDLE;
			printf("button released\n");
			
			if (is_timer_started){			//타임아웃 X, 타이머 동작중
				TIM2_Stop();
				is_timer_started = 0;

				Toggle_Motor_State(&motor_state);
			}
		}

		//Timer Expired
		if (TIM2_Expired) {
			printf("is timeout\n");

			TIM2_Expired = 0;
			is_timer_started = 0;

			Motor_Stop();
			motor_state = STOP;
		}

		if (TIM4_Expired){
			TIM4_Expired = 0;

			Toggle_with_Short_Stop(&motor_state, next_state);
		}
		
		//UART2 RX Interrupt
		if (Uart_Data_In){
			Uart_Data_In = 0;

			TIM5_PWM_Generation(curr_motor_speed);
		}
	}
}
#endif

#if 0 	//REV1.01
/*
REV1.1로 기능 확정 이전에
확장성을 고려하여 코드 정리
*/

void Main(void)
{
	Sys_Init(115200);
	GPIO_Init();
	printf("Motor test_3sec_stop_v2\n");
	Key_ISR_Enable(1);	//버튼이 눌리면 인터럽트
	
	MOTOR_STATE motor_state = STOP;				//모터 상태 정지로 초기화
	BTN_STATE curr_btn_state = IS_RELEASED; 	//이전 버튼 상태 초기화: released
	BTN_STATE prev_btn_state = curr_btn_state;	//현재 버튼 상태 초기화: released
	int is_timer_started = 0; // 타이머가 현재 동작 중인지, 1: 동작중(타임아웃 이전)

	for (;;) {

		prev_btn_state = curr_btn_state;	//버튼 상태 반영
		curr_btn_state = Key_Get_Pressed();	//버튼 상태 반영
		int is_timer_timeout = TIM2_Check_Timeout();

		//Falling Edge(Button Pressed)
		if (prev_btn_state == IS_RELEASED && curr_btn_state == IS_PRESSED) {
			printf("button pressed\n");

			TIM2_3sec_Timer_Start();      // 3초 타이머 시작
			is_timer_started = 1;
		}
		//Rising Edge(Button Released)
		else if (prev_btn_state == IS_PRESSED && curr_btn_state == IS_RELEASED) {
			printf("button released\n");
			
			if (is_timer_started){	//타임아웃 X, 타이머 동작중
				TIM2_Stop();
				is_timer_started = 0;

				Toggle_Motor_State(&motor_state);
			}
		}

		//Timer Expired
		if (is_timer_timeout) {
			printf("is timeout\n");
			is_timer_started = 0;

			Motor_Stop();
			motor_state = STOP;
		}
	}
}
#endif

#if 0 	//REV1.0
/*모터 방향 버튼 제어, Pressed 상태도 3초 이상 누르면 정지
버튼을 떼지 않아도 3초가 지나면 끝나도록.
루프에 갇히지 않게.
*/

void Main(void)
{
	Sys_Init(115200);
	GPIO_Init();
	printf("Motor test_3sec_stop_v2\n");

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
			is_timer_started = 0;

			Motor_Stop();
			state = STOP;
		}
	}
}

#endif
