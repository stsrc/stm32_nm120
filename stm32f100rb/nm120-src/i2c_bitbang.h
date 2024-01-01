#ifndef _I2C_BITBANG_H_
#define _I2C_BITBANG_H_
void i2c_bitbang_init();
int i2c_bitbang_write(const unsigned char address,
		      const unsigned char *buffer,
		      const unsigned int size);
#endif
