#ifndef __TICK_H__
#define __TICK_H__

#include "typedef.h"

extern __IO uint tick_count, us_count;

void tickInit(void);
uint getTickMs();

#endif //__TICK_H__