#include "stm32f10x_rcc.h"
#include "stm32f10x_wwdg.h"
#include "wdg.h"

void initWdg(void)
{
  /* Check if the system has resumed from WWDG reset */
  if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET) {
    /* WWDGRST flag set */

    /* Clear reset flags */
    RCC_ClearFlag();
  } else {
    /* WWDGRST flag is not set */
  }

  /* WWDG configuration */
  /* Enable WWDG clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);

  /* On other devices, WWDG clock counter = (PCLK1(36MHz)/4096)/8 = 1099 Hz(~910us) */
  WWDG_SetPrescaler(WWDG_Prescaler_8);

  /* Set Window value to 80; WWDG counter should be refreshed only when the counter 
  is below 80 (and greater than 64) otherwise a reset will be generated */
  WWDG_SetWindowValue(127);

  /* Enable WWDG and set counter value to 127, WWDG timeout = ~910 us * 64 = 58.25 ms
  In this case the refresh window is: ~910 us * (127 - 127) = 0 ms < refresh window < ~910 us * 64 = 58.25ms
  */
  WWDG_Enable(127);
}

inline void wdgRefresh(void)
{
  WWDG_SetCounter(127);
}