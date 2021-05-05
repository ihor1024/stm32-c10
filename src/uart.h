#ifndef __UART_H__
#define __UART_H__

#include "typedef.h"

void uartInit(void);
void uartPoll(void);
uint uartRecv(void* data, uint size);
uint uartSend(void* data, uint size);

#endif //__UART_H__