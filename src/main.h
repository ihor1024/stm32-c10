#ifndef __MAIN_H__
#define __MAIN_H__

#define SYSCLK  72   //System clock is 72MHz

void pushTx(u8* buf);
void pushRx(u8 data);

#endif //__MAIN_H__