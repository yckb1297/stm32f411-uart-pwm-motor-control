#include "device_driver.h"
#include <stdio.h>

void GPIO_Init(void){
    Macro_Set_Bit(RCC->AHB1ENR, 0);	//GPIOA En
	Macro_Set_Bit(RCC->AHB1ENR, 3);	//GPIOC En

    Macro_Write_Block(GPIOA->MODER,0xf, 0x5, 0);	//PA0, PA1 General output
    Macro_Write_Block(GPIOA->OTYPER, 0x3, 0x0, 0);	//PA0, PA1 Push-pull
    
	Macro_Write_Block(GPIOC->MODER,0x3, 0x0, 26);	//key mode input
}