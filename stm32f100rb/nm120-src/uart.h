#ifndef _UART_H_
#define _UART_H_

void uart_init();
void uart_write(const unsigned char byte);
unsigned char uart_read();

#endif
