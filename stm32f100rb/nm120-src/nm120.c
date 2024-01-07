#include <stm32f1xx.h>
#include "nm120.h"
#include "i2c.h"
#include "stupid_delay.h"

#define LED_port GPIOC
#define LED_Green (1 << 9)
#define GPIO_setBit(PORT, PIN) (PORT->BSRR |= PIN)
#define GPIO_clearBit(PORT, PIN) (PORT->BSRR |= (PIN << 0x10))

#define ADDR 0xCEU
#define REG_CHIP_ID 0x03fc

static unsigned int nm120_i2c_read_u32(const unsigned short reg)
{
	unsigned char result[4];
	unsigned char buffer[2] = { reg >> 8, reg & 0xff };
	i2c_write(ADDR, buffer, sizeof(buffer));
	i2c_read(ADDR | 1, result, sizeof(result));
	return result[0] | result[1] << 8 | result[2] << 16 | result[3] << 24;
}

static unsigned char nm120_i2c_read_u8(const unsigned char reg)
{
	unsigned char result;
	unsigned char buffer[] = {0, reg};
	i2c_write(ADDR, buffer, sizeof(buffer));
	i2c_read(ADDR | 1, &result, sizeof(result));
	return result;
}

static void nm120_i2c_write_u8(const unsigned char reg,
			       const unsigned char val)
{
	unsigned char buffer[] = {0, reg, val};
	i2c_write(ADDR, buffer, sizeof(buffer));
}

static void nm120_i2c_write_u32(const unsigned short reg,
				const unsigned int val)
{
	unsigned char buffer[] = {
		reg >> 8,
		reg & 0xff,
	        val & 0xff,
		val >> 8 & 0xff,
		val >> 16 & 0xff,
		val >> 24
	};
	i2c_write(ADDR, buffer, sizeof(buffer));
}

static void check_reg_u8(const unsigned char reg, const unsigned char val)
{
	unsigned char temp = nm120_i2c_read_u8(reg);
	if (temp != val) {
		GPIO_setBit(LED_port, LED_Green);
		while(1);
	}
}

static void check_reg_u32(const unsigned short reg, const unsigned int val)
{
	unsigned int temp = nm120_i2c_read_u32(reg);
	if (temp != val) {
		GPIO_setBit(LED_port, LED_Green);
		while(1);
	}
}

void nm120_init() {
	i2c_init();
	if (nm120_chip_id() != 0x120b0) {
		GPIO_setBit(LED_port, LED_Green);
		while(1);
	}
	const char first_init[] = {
		0x06, 0xc8,
		0x07, 0x40,
		0x0a, 0xeb,
		0x0b, 0x11,
		0x0c, 0x10,
		0x0d, 0x88,
		0x10, 0x04,
		0x11, 0x30,
		0x12, 0x30,
		0x15, 0xaa,
		0x16, 0x03,
		0x17, 0x80,
		0x18, 0x67,
		0x19, 0xd4,
		0x1a, 0x44,
		0x1c, 0x10,
		0x1d, 0xee,
		0x1e, 0x99,
		0x21, 0xc5,
		0x22, 0x91,
		0x24, 0x01,
		0x2b, 0x91,
		0x2d, 0x01,
		0x2f, 0x80,
		0x31, 0x00,
		0x33, 0x00,
		0x38, 0x00,
		0x39, 0x2f,
		0x3a, 0x00,
		0x3b, 0x00
	};
	for(unsigned int i = 0; i < sizeof(first_init); i += 2) {
		nm120_i2c_write_u8(first_init[i], first_init[i + 1]);
		check_reg_u8(first_init[i], first_init[i + 1]);
	}
	unsigned char temp = nm120_i2c_read_u8(0x36);
	temp &= 0x7f;
	nm120_i2c_write_u8(0x36, temp);
	check_reg_u8(0x36, temp);
	nm120_i2c_write_u32(0x01 << 8 | 0x64,
	   	            0x00 << 24 | 0x00 << 16 | 0x08 << 8 | 0x00);
	check_reg_u32(0x01 << 8 | 0x64, 0x00 << 24 | 0x00 << 16 | 0x08 << 8 | 0x00);
	nm120_i2c_write_u32(0x01 << 8 | 0xc0,
		            0x2d << 24 | 0x8c << 16 | 0x19 << 8 | 0xc7);
	delay_ms(100);
	check_reg_u32(0x01 << 8 | 0xc0, 0x2d << 24 | 0x8c << 16 | 0x19 << 8 | 0xc7);
	const unsigned char second_init[] = {
		0x0e, 0x45,
		0x1b, 0x0e,
		0x23, 0xff,
		0x26, 0x82,
		0x28, 0x00,
		0x30, 0xdf,
		0x32, 0xdf,
		0x34, 0x68,
		0x35, 0x18
	};
	for (unsigned int i = 0; i < sizeof(second_init); i += 2) {
		nm120_i2c_write_u8(second_init[i], second_init[i + 1]);
		check_reg_u8(second_init[i], second_init[i + 1]);
	}
	nm120_i2c_write_u8(0x06, 0x48);
	check_reg_u8(0x06, 0x48);
	nm120_i2c_write_u8(0x0a, 0xf8);
	check_reg_u8(0x0a, 0xf8);
	nm120_i2c_write_u8(0x00, 0x9f);
	check_reg_u8(0x00, 0x9f);
	unsigned int long_temp = nm120_i2c_read_u32(0x0104);
	nm120_i2c_write_u32(0x0104, long_temp | 0x01);
	temp = nm120_i2c_read_u8(0x07);
	nm120_i2c_write_u8(0x07, temp | 0x40);
	check_reg_u8(0x07, temp | 0x40);
	temp = nm120_i2c_read_u8(0x0b);
	nm120_i2c_write_u8(0x0b, temp | 0x10);
	check_reg_u8(0x0b, temp | 0x10);
	nm120_i2c_write_u8(0x34, 0x60);
	check_reg_u8(0x34, 0x60);
	nm120_i2c_write_u8(0x35, 0x18);
	check_reg_u8(0x35, 0x18);
}

unsigned int nm120_chip_id()
{
	return nm120_i2c_read_u32(REG_CHIP_ID);
}

