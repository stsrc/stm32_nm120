#include <stm32f1xx.h>
#include <core_cm3.h>
#include <stdlib.h>

#include "nm120.h"
#include "stupid_delay.h"
#include "winbond_w25q32bv.h"
#include "uart.h"

#define LED_port GPIOC
#define LED_Blue (1 << 8)
#define LED_Green (1 << 9)
#define GPIO_setBit(PORT, PIN) (PORT->BSRR |= PIN)
#define GPIO_clearBit(PORT, PIN) (PORT->BSRR |= (PIN << 0x10))

static void init_blue_led() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	LED_port->CRH |= GPIO_CRH_MODE8_0;
	LED_port->CRH &= ~GPIO_CRH_CNF8;
}

static void init_green_led() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	LED_port->CRH |= GPIO_CRH_MODE9_0;
	LED_port->CRH &= ~GPIO_CRH_CNF9;
}

static void configure_clock() {
	RCC->CR |= RCC_CR_HSEON; //External high speed clock enabled;
	while(!(RCC->CR & RCC_CR_HSERDY)); //External high-speed clock ready flag check

	RCC->CFGR |= RCC_CFGR_PLLXTPRE; //LSB of division factor PREDIV1
	RCC->CFGR |= RCC_CFGR_PLLSRC; //PLL entry clock source => clock from prediv1
	RCC->CFGR |= RCC_CFGR_PLLMULL_2;

	RCC->CR |= RCC_CR_PLLON; //PLL enabled;
	while(!(RCC->CR & RCC_CR_PLLRDY)); //PLL clock ready flag

	RCC->CR &= ~RCC_CR_CSSON; //Clock security system disabled
	RCC->CR &= ~RCC_CR_CSSON; //External high-speed clock not bypassed

	RCC->CFGR &= ~RCC_CFGR_SW_Msk; //select PLL as input
	RCC->CFGR |= RCC_CFGR_SW_1;
	while(!((RCC->CFGR & RCC_CFGR_SWS_Msk) == RCC_CFGR_SWS_1)); //wait until PLL is selected

	SystemCoreClock = 24000000;
}

int main(void) {
	configure_clock();
	init_blue_led();
	init_green_led();
	delay_init();
	nm120_init();
//	winbond_init();
//	uart_init();

/*	winbond_dump_memory_start();
	unsigned int address = 0;
	while(address++ != 0x400000) {
		GPIO_setBit(LED_port, LED_Blue);
		unsigned char data = winbond_dump_memory();
		GPIO_clearBit(LED_port, LED_Blue);
		uart_write(data);
	}
	GPIO_setBit(LED_port, LED_Green);
	while(1);*/
	while(1) {
		GPIO_setBit(LED_port, LED_Blue);
		delay_ms(100);
		GPIO_clearBit(LED_port, LED_Blue);
		delay_ms(100);
	}
	return 0;
}
