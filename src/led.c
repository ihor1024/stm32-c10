#include "stm32f10x_gpio.h"
#include "typedef.h"
#include "led.h"
#include "tick.h"

#define GREEN_OFF		GPIO_ResetBits(GPIOB, GPIO_Pin_14)
#define GREEN_ON		GPIO_SetBits(GPIOB, GPIO_Pin_14)
#define RED_OFF			GPIO_ResetBits(GPIOC, GPIO_Pin_6)
#define RED_ON			GPIO_SetBits(GPIOC, GPIO_Pin_6)

#define LED_TIMEOUT 	3000

static u32 ok_start, ok_timeout, no_start, no_timeout;

enum { LED_GREEN = BIT(0), LED_RED = BIT(1) };

/*
 *	Switch off leds
 */
static inline void ledOn(int what)
{
	if (what & LED_GREEN)
		GREEN_ON;
	if (what & LED_RED)
		RED_ON;
}

/*
*	Switch on leds
*/
static inline void ledOff(int what)
{
	if (what & LED_GREEN)
		GREEN_OFF;
	if (what & LED_RED)
		RED_OFF;
}

void ledInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// PB14 : GREEN LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// PC6 : RED LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	ok_timeout = no_timeout = -1;

	// Switch off leds
	ledOff(LED_GREEN | LED_RED);
}

void ledPoll(void)
{
	u32 t = getTickMs();
	if (ok_timeout != -1 && t - ok_start >= ok_timeout) {
		ledOff(LED_GREEN);
    ok_timeout = -1;
	}

	if (no_timeout != -1 && t - no_start >= no_timeout) {
		ledOff(LED_RED);
    no_timeout = -1;
  }
  
	if (ok_timeout == -1 && no_timeout == -1) {
		if (t % 1000 == 500)
			ledOff(LED_GREEN);
		else if (t % 1000 == 0)
			ledOn(LED_GREEN);
	}
}

static void enableTimeout(int what, int timeout/* = LED_TIMEOUT */)
{
	if (what & LED_RED) {
		ledOff(LED_RED);
		if (timeout) {
			no_start = getTickMs();
			no_timeout = timeout;
      ledOn(LED_RED);
      ledOff(LED_GREEN);
		} else {
			no_start = 0;
			no_timeout = -1;
		}
	}
	if (what & LED_GREEN) {
		ledOff(LED_GREEN);
		if (timeout) {
			ok_start = getTickMs();
			ok_timeout = timeout;
			ledOn(LED_GREEN);
      ledOff(LED_RED);
		} else {
			ok_start = 0;
			ok_timeout = -1;
		}
	}
}
	
void ledSuccess(void)
{
	enableTimeout(LED_GREEN, LED_TIMEOUT);
}

void ledError(void)
{
	enableTimeout(LED_RED, LED_TIMEOUT);
}