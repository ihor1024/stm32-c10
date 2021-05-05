#include "stm32f10x_gpio.h"
#include "typedef.h"
#include "buzz.h"
#include "tick.h"

#define BUZZ_OFF	GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define BUZZ_ON		GPIO_SetBits(GPIOB, GPIO_Pin_12)

static vu32 Tmr_buz_count, Tmr_buz_prev_time, Tmr_buz_prev_flag;
static vu16 Tmr_buz_interval_on, Tmr_buz_interval_off;

void buzzInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Activate PB12(BUZZER) as output */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	Tmr_buz_count = Tmr_buz_prev_time = Tmr_buz_prev_flag = 
		Tmr_buz_interval_on = Tmr_buz_interval_off = 0;
	BUZZ_OFF;
}

void buzzAlarm(void)
{
  Tmr_buz_count = 0;
  Tmr_buz_interval_on = 30;
  Tmr_buz_interval_off = 30;
  Tmr_buz_prev_time = getTickMs();
  Tmr_buz_prev_flag = 1;
  BUZZ_ON;
  Tmr_buz_count = 1;
}

void buzzSuccess(void)
{
	Tmr_buz_count = 0;
	Tmr_buz_count = 0;
	Tmr_buz_interval_on = 30;
	Tmr_buz_interval_off = 30;
	Tmr_buz_prev_time = getTickMs();
	Tmr_buz_prev_flag = 1;
	BUZZ_ON;
	Tmr_buz_count = 5;
}

void buzzError(void)
{
	Tmr_buz_count = 0;
	Tmr_buz_interval_on = 70;
	Tmr_buz_interval_off = 20;
	Tmr_buz_prev_time = getTickMs();
	Tmr_buz_prev_flag = 1;
	BUZZ_ON;
	Tmr_buz_count = 3;
}

void buzzPoll(void)
{
	u32 cur_time = getTickMs();
	if (Tmr_buz_count > 0 && cur_time - Tmr_buz_prev_time >
		(Tmr_buz_prev_flag ? Tmr_buz_interval_on : Tmr_buz_interval_off)) {
			Tmr_buz_prev_time = cur_time;
			if (Tmr_buz_prev_flag) {
				Tmr_buz_prev_flag = 0;
				BUZZ_OFF;
			} else {
				Tmr_buz_prev_flag = 1;
				BUZZ_ON;
			}
			if (--Tmr_buz_count == 0)
				return;
	}
}

inline bool buzzArming(void)
{
  return (Tmr_buz_count > 0);
}

inline void buzzWait(void)
{
  while (buzzArming());
}
