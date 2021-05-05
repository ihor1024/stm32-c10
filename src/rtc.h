#ifndef __RTC_H__
#define __RTC_H__

#include "typedef.h"

void initRtc(void);
uint rtcGetSeconds();
void rtcSetSeconds(uint seconds);

#endif //__RTC_H__