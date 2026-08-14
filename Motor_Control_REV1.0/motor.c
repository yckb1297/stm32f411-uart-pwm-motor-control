#include "device_driver.h"
#include <stdio.h>

void Toggle_Motor_State(enum Motor_state *state) {
    switch (*state) {
        case STOP:  // 정지 -> 정회전
            Motor_CW();
            *state = CW;
            break;

        case CW:    // 정회전 -> 역회전
            Motor_Short_Stop();
            Motor_CCW();
            *state = CCW;
            break;

        case CCW:   // 역회전 -> 정회전
            Motor_Short_Stop();
            Motor_CW();
            *state = CW;
            break;

        default:    // 예외 처리
            Motor_Stop();
            *state = STOP;
            break;
    }
}

void Motor_Stop(void){
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
}

void Motor_Short_Stop(void){
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x0, 0);
}

void Motor_CW(void){
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x1, 0);
}

void Motor_CCW(void){
    Macro_Write_Block(GPIOA->ODR, 0x3, 0x2, 0);
    TIM4_Delay(200);
}