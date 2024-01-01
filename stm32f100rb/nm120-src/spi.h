#ifndef _SPI_H_
#define _SPI_H_

void spi_init(void);
void spi_write(const unsigned char byte);
unsigned char spi_read();

#endif
