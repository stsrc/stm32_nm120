#include "i2c_bitbang.h"
#include "stupid_delay.h"

// 4 - clk
// 5 - sda

static void clk_0()
{
	GPIOB->BSRR |= GPIO_BSRR_BR4;
}

static void clk_1()
{
	GPIOB->BSRR |= GPIO_BSRR_BS4;
}

static void sda_0()
{
	GPIOB->BSRR |= GPIO_BSRR_BR5;
}

static void sda_1()
{
	GPIOB->BSRR |= GPIO_BSRR_BS5;
}

void i2c_bitbang_init() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	GPIOB->CRL |= GPIO_CRL_MODE5_1 |
			GPIO_CRL_MODE4_1 |
			GPIO_CRL_CNF5_0 |
			GPIO_CRL_CNF4_0;
	sda_1();
	clk_1();
}

static void wait()
{
	delay_us(250);
}

static char sda_get()
{
	char value;

	GPIOB->CRL &= ~GPIO_CRL_MODE5_1;
	wait();

	value = (GPIOB->IDR & GPIO_IDR_IDR5) >> GPIO_IDR_IDR5_Pos;

	GPIOB->CRL |= GPIO_CRL_MODE5_1;
	wait();

	return value;
}

static void start_sequence()
{
	// start sequence
	sda_0();
	clk_1();
	wait();
}

static void stop_sequence()
{
	clk_0();
	wait();
	sda_0();
	wait();
	clk_1();
	wait();
	sda_1();
	wait();
}

static void write_address(const unsigned char address)
{
	for (int i = 0; i < 7; i++) {
		clk_0();
		wait();
		if (address & (1 << (6 - i))) {
			sda_1();
		} else {
			sda_0();
		}
		wait();
		clk_1();
		wait();
	}
}

static void write_byte(const unsigned char byte)
{
	for (int i = 0; i < 8; i++) {
		clk_0();
		wait();
		if (byte & (1 << (7 - i))) {
			sda_1();
		} else {
			sda_0();
		}
		wait();
		clk_1();
		wait();
	}
}

int i2c_bitbang_write(const unsigned char address,
		      const unsigned char *buffer,
		      const unsigned int size) {
	start_sequence();
	write_address(address);
	clk_0();
	wait();
	clk_1();
	wait();
	if (sda_get()) {
		stop_sequence();
		return -1;
	}
	stop_sequence();
	return 0;
}
