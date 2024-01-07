#include <stm32f1xx.h>

#include "i2c.h"

void i2c_init() {
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	GPIOB->CRL |= GPIO_CRL_MODE7_1 |
			GPIO_CRL_MODE6_1 |
			GPIO_CRL_CNF7 |
			GPIO_CRL_CNF6;

	// 24MHz clock
	I2C1->CR2 |= I2C_CR2_FREQ_4 | I2C_CR2_FREQ_3;
	I2C1->CCR |= 256; // 256, 240 is 100kHz
	// 25, because calculations
	I2C1->TRISE = 25;
	I2C1->CR1 |= I2C_CR1_PE;
}

static void i2c_start(const unsigned char address,
		      const unsigned char ack) {
	I2C1->CR1 &= ~I2C_CR1_STOP;
	if (ack) {
		I2C1->CR1 |= I2C_CR1_ACK;
	}
	I2C1->CR1 |= I2C_CR1_START;
	while(!(I2C1->SR1 & I2C_SR1_SB));
	I2C1->DR = address;
	while(!(I2C1->SR1 & I2C_SR1_ADDR));
	while(!(I2C1->SR2 & I2C_SR2_MSL));
}

void i2c_write(const unsigned char address,
	       const unsigned char *buffer,
	       const unsigned int size) {
	i2c_start(address, 0U);
	for (unsigned int i = 0U; i < size; i++) {
		I2C1->DR = buffer[i];
		while(!(I2C1->SR1 & I2C_SR1_BTF));
		while(!(I2C1->SR1 & I2C_SR1_TXE));
	}
	I2C1->CR1 |= I2C_CR1_STOP;
	while (I2C1->SR2 & I2C_SR2_MSL);
}

void i2c_read(const unsigned char address,
              unsigned char *buffer,
              const unsigned int size) {
	i2c_start(address, 1U);
	if (size == 1U) {
		I2C1->CR1 &= ~I2C_CR1_ACK;
		I2C1->CR1 |= I2C_CR1_STOP;
		while (!(I2C1->SR1 & I2C_SR1_RXNE));
		buffer[0] = I2C1->DR;
	} else {
		for (unsigned int i = 0U; i < size; i++) {
			while (!(I2C1->SR1 & I2C_SR1_RXNE));
			buffer[i] = I2C1->DR;
			if (size - i == 2U) {
				I2C1->CR1 &= ~I2C_CR1_ACK;
				I2C1->CR1 |= I2C_CR1_STOP;
			}
		}
	}
}
