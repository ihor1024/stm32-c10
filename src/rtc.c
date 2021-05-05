#include "stm32f10x_rtc.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "rtc.h"

static const int days_month[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
static int ymd2days(int yy, int mm, int dd) // 00/01/01 is 1
{
  int i, n = 0;

  if (yy > 0)
    n = yy * 365 + ((yy - 1) / 4 + 1);

  for (i = 1; i < mm; i++)
    n += days_month[i-1];

  if (yy % 4 == 0)
    if (mm > 2)
      n += 1;
  n += dd;

  return n;
}

static void days2ymd(int days, int* yy, int* mm, int* dd) // 00/01/01 is 1
{
  int temp;

  *yy = 0;
  *mm = 1;

  while (1) {
    temp = 365;
    if (*yy % 4 == 0)
      temp++;

    if (days <= temp)
      break;

    days -= temp;
    (*yy)++;
  }

  while (1) {
    temp = days_month[*mm - 1];
    if (*mm == 2 && *yy % 4 == 0)
      temp++;

    if (days <= temp)
      break;

    days -= temp;
    (*mm)++;
  }

  *dd = days;
}

static void rtcConfigure(void)
{ 
  /* 使能 PWR 和 BKP 的时钟 */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
  
  /* 允许访问BKP区域 */
  PWR_BackupAccessCmd(ENABLE);

  /* 复位BKP */
  BKP_DeInit();

  /* 使能RTC外部时钟 */
  RCC_LSEConfig(RCC_LSE_ON);
  /* 等待RTC外部时钟就绪 */
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

  /* 选择RTC外部时钟为RTC时钟 */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

  /* 使能RTC时钟 */
  RCC_RTCCLKCmd(ENABLE);

  /* 等待RTC寄存器同步 */
  RTC_WaitForSynchro();

  /* 等待写RTC寄存器完成 */
  RTC_WaitForLastTask();
  
  /* 使能RTC秒中断 */  
  //RTC_ITConfig(RTC_IT_SEC, ENABLE);

  /* 等待写RTC寄存器完成 */
  //RTC_WaitForLastTask();
  
  /* 设置RTC预分频 */
  RTC_SetPrescaler(32767);
  
  /* 等待写RTC寄存器完成 */
  RTC_WaitForLastTask();
}

void initRtc(void)
{
  //判断保存在备份寄存器的RTC标志是否已经被配置过
  if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5) {
    //RTC not yet configured....
    
    //RTC初始化
    //rtcConfigure();
    
    //RTC configured....

    //设置RTC时钟参数
    //Time_Adjust();										            

    //RTC设置后，将已配置标志写入备份数据寄存器
    BKP_WriteBackupRegister(BKP_DR1, 0xA5A5); 
  } else {
    rtcConfigure();
    //检查是否掉电重启
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET) { 
      //Power On Reset occurred....
    } else if(RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET) { //检查是否reset复位
      //External Reset occurred....
    }
    //No need to configure RTC....

    //等待RTC寄存器被同步
    RTC_WaitForSynchro();

    //使能秒中断
    //RTC_ITConfig(RTC_IT_SEC, ENABLE);			

    //等待写入完成
    RTC_WaitForLastTask();
  }

  //清除复位标志
  RCC_ClearFlag();
}

uint ymdhms2seconds(int yy, int mm, int dd, int h, int m, int s)
{
  uint t;

  t = (uint)(ymd2days(yy, mm, dd) - 1) * 24 * 3600;
  t += h * 3600;
  t += m * 60;
  t += s;

  return t;
}

void seconds2ymdhms(uint seconds, int* yy, int* mm, int* dd, int* h, int* m, int* s)
{
  uint t = seconds % (3600 * 24);

  *h = t / 3600; t %= 3600;
  *m = t / 60;
  *s = t % 60;

  days2ymd(seconds / 3600 / 24 + 1, yy, mm, dd);
}

uint rtcGetSeconds()
{
  return RTC_GetCounter();
}

void rtcSetSeconds(uint seconds)
{
  rtcConfigure();
  RTC_WaitForLastTask();
  RTC_SetCounter(seconds);
  RTC_WaitForLastTask();
}

int getWeekDay(int yy, int mm, int dd)
{
  return (ymd2days(yy, mm, dd) + 5) % 7;
}

int getDaysForMonth(int yy, int mm)
{
  int n;

  n = days_month[mm - 1];
  if (mm == 2 && yy % 4 == 0)
    n++;

  return n;
}