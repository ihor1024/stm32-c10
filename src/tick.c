#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "main.h"
#include "misc.h"
#include "tick.h"

#define TICK_HZ         100
#define TICK_PERIOD	    (1000 / TICK_HZ) // ms

__IO uint tick_count = 0, us_count = 0;

void tickInit(void)
{
  TIM_TimeBaseInitTypeDef TIM2_TimeBaseStructure;
  TIM_OCInitTypeDef       TIM2_OCInitStructure;
  NVIC_InitTypeDef        NVIC_InitStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  /* Enable TIMER2 IRQ */
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Time Base configuration */
  /*
  TIM2CLK = 72MHz, Prescaler = 72, TIM2_CCR1 = 10000
  TIMER2 period is 100Hz
  */
  TIM2_TimeBaseStructure.TIM_Prescaler = SYSCLK - 1;
  TIM2_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM2_TimeBaseStructure.TIM_Period = TICK_PERIOD * 1000;
  TIM2_TimeBaseStructure.TIM_ClockDivision = 0x0;

  TIM_TimeBaseInit(TIM2, &TIM2_TimeBaseStructure);
  
  TIM2_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
  TIM2_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
  TIM2_OCInitStructure.TIM_Pulse = 0;
  TIM2_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

  TIM_OC1Init(TIM2, &TIM2_OCInitStructure);
  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
 
  TIM_Cmd(TIM2, ENABLE);
  TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
}

uint getTickMs()
{
  return tick_count * TICK_PERIOD;
}