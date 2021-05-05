#include <string.h>
#include <stdarg.h>
#include "misc.h"
#include "stm32f10x_usart.h"
#include "uart.h"
#include "tick.h"
#include "typedef.h"
#include "com.h"

#define buflen      2048
#define cmdbuflen   32

static __IO int rdpt = 0, wrpt = 0;
static u8 buf[buflen], cmdbuf[cmdbuflen];

void uartInit(void)
{
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable USART1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  /* USART1 TX, RX IO*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;      //USART1 TX
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;      //USART1 RX
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Enable USART1 ISR */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Reset USART1 */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART1, &USART_InitStructure);

  /* Enable USART1 Receive and Transmit interrupts */
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);
}

//while (!(USART1->SR & USART_FLAG_TXE));
//while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval : None
  */
void USART1_IRQHandler(void)
{
  u8 d;

  if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    d = USART_ReceiveData(USART1);
    int n = (wrpt + 1) % buflen;
    if (n != rdpt) {
      buf[wrpt++] = d;
      if (wrpt == buflen)
        wrpt = 0;
    }
  }
  if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
     USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
}

int com_uart_check_cmd(void* cmd, int size)
{
  return communication_check_cmd(cmd, size, COMM_MODE_SERIAL, 0);
}

void com_uart_proc_cmd(void)
{
  communication_proc_cmd();
}

void uartPoll(void)
{
  int i, n;

  u32 st = getTickMs();
  // wait remain command packet, command packet size always < FIFO size = 16bytes.
  do {
    n = wrpt; n -= rdpt;
    if (n < 0)
      n = buflen + n;
    n = MIN(n, cmdbuflen);
  } while (n > 0 && n < 16 && getTickMs() - st < 20);

  int rdpt_temp = rdpt;
  for ( i = 0; i < n; i++ ) {
    cmdbuf[i] = buf[rdpt_temp];

    rdpt_temp++;

    if (rdpt_temp == buflen)
      rdpt_temp = 0;
  }

  n = com_uart_check_cmd(cmdbuf, n);
  for (i = 0; i < n; i++) {
    rdpt++;

    if (rdpt == buflen)
      rdpt = 0;
  }

  // maybe cmd is pending
  if (n > 1)
    com_uart_proc_cmd();
}

void sendByte(u8 c)
{
  USART_SendData(USART1, c);
  while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

uint uartSend(void* data, uint size)
{
  uint   i;

  //RX485_TX();

  for (i = 0; i < size; i++)
    sendByte(((u8*)data)[i]);

  //RX485_RX();
  rdpt = wrpt = 0;

  return i;
}

bool recvByte(u8* c)
{
  int n = rdpt;
  if (n == wrpt)
    return false;

  *c = buf[rdpt];

  rdpt++;

  if (rdpt == buflen)
    rdpt = 0;

  return true;
}

uint uartRecv(void* data, uint size)
{
  uint i, starttime = getTickMs();

  for (i = 0; i < size; i++) {
    while (rdpt == wrpt)
      if (getTickMs() - starttime > 7000)
        return i;

    ((u8*)data)[i] = buf[rdpt];

    rdpt++;

    if (rdpt == buflen)
      rdpt = 0;

    starttime = getTickMs();
  }

  return i;
}

void sendString(char *str)
{
  uartSend(str, strlen(str));
}