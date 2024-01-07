#include <stm32f1xx.h>

#include "spi.h"

void spi_init() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// A4 - NSS
	// A5 - CLK
	// A6 - MISO
	// A7 - MOSI
	GPIOA->CRL |= GPIO_CRL_MODE4;
	GPIOA->CRL &= ~GPIO_CRL_CNF4_0;
	GPIOA->CRL |= GPIO_CRL_CNF4_1;

	GPIOA->CRL |= GPIO_CRL_MODE5;
	GPIOA->CRL &= ~GPIO_CRL_CNF5_0;
	GPIOA->CRL |= GPIO_CRL_CNF5_1;

	GPIOA->CRL &= ~GPIO_CRL_CNF6_0;
	GPIOA->CRL |= GPIO_CRL_CNF6_1;
	GPIOA->ODR |= GPIO_ODR_ODR6;

	GPIOA->CRL &= ~GPIO_CRL_CNF7_0;
	GPIOA->CRL |= GPIO_CRL_MODE7;
	GPIOA->CRL |= GPIO_CRL_CNF7_1;

	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
	SPI1->CR1 |= SPI_CR1_BR_Msk;
	SPI1->CR1 |= SPI_CR1_MSTR;

	SPI1->CR1 |= SPI_CR1_SPE;
}

unsigned char spi_write_read(const unsigned char byte)
{
	while(SPI1->SR & SPI_SR_BSY);
	while(!(SPI1->SR & SPI_SR_TXE));
	SPI1->DR = byte;
	while(SPI1->SR & SPI_SR_BSY);
	while(!(SPI1->SR & SPI_SR_RXNE));
	unsigned char data = SPI1->DR;
	return data;
}
