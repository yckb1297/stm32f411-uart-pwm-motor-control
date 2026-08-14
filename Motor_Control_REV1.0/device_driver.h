#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

//gpio_init.c

extern void GPIO_Init(void);

// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);

// SysTick.c

extern void SysTick_Run(unsigned int msec);
extern int SysTick_Check_Timeout(void);
extern unsigned int SysTick_Get_Time(void);
extern unsigned int SysTick_Get_Load_Time(void);
extern void SysTick_Stop(void);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// Key.c

extern int Key_Get_Pressed(void);

// Timer.c
extern void TIM2_One_Shot(void);
extern int TIM2_Check_Timeout(void);
extern void TIM2_Stop(void);

// Motor.c

enum Motor_state
{
	STOP = 0,
	CW,
	CCW,
};
extern void Toggle_Motor_State(enum Motor_state *state);
extern void Motor_Stop(void);
extern void Motor_Short_Stop(void);
extern void Motor_CW(void);
extern void Motor_CCW(void);