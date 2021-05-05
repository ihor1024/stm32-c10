#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "pwm.h"

void initPwm(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef TIM3_TimeBaseStructure;
  TIM_OCInitTypeDef TIM3_OCInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /*-------------------------------------------------------------------
  TIM3CLK=72MHz, prescaler=2, 24MHz
  PWM duty rate is TIM3_CCR2/(TIM_Period+1)
  -------------------------------------------------------------------*/
  TIM3_TimeBaseStructure.TIM_Prescaler = 2;
  TIM3_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM3_TimeBaseStructure.TIM_Period = 192;
  TIM3_TimeBaseStructure.TIM_ClockDivision = 0x0;
  TIM3_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM3, &TIM3_TimeBaseStructure);
  
  TIM3_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM3_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM3_OCInitStructure.TIM_Pulse = 192 / 2;
  TIM3_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
         
  TIM_OC1Init(TIM3, &TIM3_OCInitStructure);
  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
  TIM_Cmd(TIM3, ENABLE);
}