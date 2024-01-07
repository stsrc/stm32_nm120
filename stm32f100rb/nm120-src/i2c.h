#ifndef _I2C_H_
#define _I2C_H_
void i2c_init();
void i2c_read(const unsigned char address,
              unsigned char *buffer,
              const unsigned int size);
void i2c_write(const unsigned char address,
	       const unsigned char *buffer,
	       const unsigned int size);
#endif
