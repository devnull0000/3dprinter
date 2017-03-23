#pragma once

void uart_init(char const * dev_name);       //can throw in case of error, dev_name like /dev/ttyUSB1 
void uart_free();

void uart_write(void const * const data, int const size);   //synchronous
int  uart_read(void * const buf, int const buf_size);       //returns number of bytes read, asynchrounous

int uart_fd();
