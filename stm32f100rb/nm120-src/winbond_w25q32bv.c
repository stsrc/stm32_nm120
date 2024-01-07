#include <stm32f1xx.h>

#include "winbond_w25q32bv.h"
#include "spi.h"
#include "stupid_delay.h"
void winbond_init()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

	// C4 - WP
	// C5 - CS
	GPIOC->CRL &= ~GPIO_CRL_CNF4_0;
	GPIOC->CRL |= GPIO_CRL_MODE4;

	GPIOC->CRL &= ~GPIO_CRL_CNF5_0;
	GPIOC->CRL |= GPIO_CRL_MODE5;

	spi_init();
}

void winbond_get_ids(unsigned char *man_id, unsigned char *dev_id)
{
	spi_write_read(0x90);
	spi_write_read(0x00);
	spi_write_read(0x00);
	spi_write_read(0x00);
	*man_id = spi_write_read(0x00);
	*dev_id = spi_write_read(0x00);
}

void winbond_dump_memory_start()
{
	spi_write_read(0x03);
	spi_write_read(0);
	spi_write_read(0);
	spi_write_read(0);
}

unsigned char winbond_dump_memory()
{
	return spi_write_read(0);
}
