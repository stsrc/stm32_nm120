#include <stm32f1xx.h>

#include "spi.h"

void spi_init() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	// A5 - CLK
	// A6 - MISO
	// A7 - MOSI
	GPIOA->CRL |= GPIO_CRL_MODE5;
	GPIOA->CRL &= ~GPIO_CRL_CNF5_0;
	GPIOA->CRL |= GPIO_CRL_CNF5_1;

	GPIOA->CRL &= ~GPIO_CRL_CNF6_0;
	GPIOA->CRL |= GPIO_CRL_CNF6_1;
	GPIOA->ODR |= GPIO_ODR_ODR6;

	GPIOA->CRL &= ~GPIO_CRL_CNF7_0;
	GPIOA->CRL |= GPIO_CRL_MODE7;
	GPIOA->CRL |= GPIO_CRL_CNF7_1;

	// C4 - WP
	// C5 - CS
	GPIOC->CRL &= ~GPIO_CRL_CNF4_0;
	GPIOC->CRL |= GPIO_CRL_MODE4;

	GPIOC->CRL &= ~GPIO_CRL_CNF5_0;
	GPIOC->CRL |= GPIO_CRL_MODE5;

	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 |= SPI_CR1_BR_Msk;
	SPI1->CR1 |= SPI_CR1_MSTR;

	SPI1->CR1 |= SPI_CR1_SPE;
}

void spi_write(const unsigned char byte)
{
	while(SPI1->SR & SPI_SR_BSY);
	while(!(SPI1->SR & SPI_SR_TXE));
	SPI1->DR = byte;
}

unsigned char spi_read()
{
	spi_write(0xaa);
	while(!(SPI1->SR & SPI_SR_RXNE));
	return SPI1->DR;
}
