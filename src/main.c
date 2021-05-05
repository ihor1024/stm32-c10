#include "stm32f10x.h"
#include "core_cm3.h"
#include <stdarg.h>
#include <string.h>
#include "typedef.h"
#include "platform_config.h"
#include "misc.h"
#include "main.h"
#include "rfid.h"
#include "led.h"
#include "wdg.h"
#include "rtc.h"
#include "uart.h"
#include "proc.h"
#include "tick.h"
#include "buzz.h"

static void initPeriph(void)
{
  /* Init RTC */
  //initRtc();

#ifdef USE_WDG
  /* Init watchdog timer */
  initWdg();
#endif

  /* Tick ther timer in 10ms */
  tickInit();

  /* Init RFID */
  rfidInit();

  /* Init LEDs */
  ledInit();

  /* Init buzzer */
  buzzInit();

  /* Init UART */
  uartInit();
}

int main(void)
{
  /* System Clocks Configuration */
  SystemInit(); 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
    | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  DBGMCU->CR |= DBGMCU_CR_DBG_WWDG_STOP;

  /* System clock ticks in 1us */
  SysTick_Config(SYSCLK);

  initPeriph();

  /* Loop main proc */
  mainProc();
}