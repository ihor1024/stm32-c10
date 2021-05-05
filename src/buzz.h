#ifndef __BUZZ_H__
#define __BUZZ_H__

#include "typedef.h"

void buzzInit(void);
void buzzSuccess(void);
void buzzError(void);
void buzzAlarm(void);
void buzzPoll(void);
bool buzzArming();
void buzzWait(void);

#endif //__BUZZ_H__