#include <stm32f1xx.h>

#include "uart.h"

void uart_init()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// A9 - TX
	// A10 - RX
	GPIOA->CRH |= GPIO_CRH_MODE9;
	GPIOA->CRH &= ~GPIO_CRH_CNF9_0;
	GPIOA->CRH |= GPIO_CRH_CNF9_1;

	GPIOA->CRH &= ~GPIO_CRH_CNF10_0;
	GPIOA->CRH |= GPIO_CRH_CNF10_1;
	GPIOA->ODR |= GPIO_ODR_ODR10;

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
//	USART1->BRR = 13 << USART_BRR_DIV_Mantissa_Pos;
//	USART1->BRR = 39 << USART_BRR_DIV_Mantissa_Pos | 1; // 38400
	USART1->BRR = 156 << USART_BRR_DIV_Mantissa_Pos | 4; // 9600
//	USART1->BRR = 625 << USART_BRR_DIV_Mantissa_Pos;  // 2400
//	USART1->BRR = 0x20 << USART_BRR_DIV_Mantissa_Pos | 7; // ?
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;

	USART1->CR1 |= USART_CR1_UE;
}

void uart_write(const unsigned char byte)
{
	while(!(USART1->SR & USART_SR_TXE));
	USART1->DR = byte;
}

unsigned char uart_read()
{
	while(!(USART1->SR & USART_SR_RXNE));
	return USART1->DR;
}
