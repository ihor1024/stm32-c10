#include "typedef.h"
#include "proc.h"
#include "database.h"
#include "rfid.h"
#include "buzz.h"
#include "led.h"
#include "uart.h"

bool uiProcVerifyCard(void)
{
  int nID;
  bool ret = false;

  if (DbCardVerify(g_uiProcStatus.dwCardID, &nID)) {
    if (!DbGlogIsReverify(nID)) {
      uiLogGlogAdd(nID);
      buzzSuccess();
    } else {
      buzzAlarm();
    }
    ledSuccess();
    ret = true;
  } else {
    buzzError();
    ledError();
  }
  return ret;
}

void mainProc(void)
{
  _Db_AppVarsInit();
  DbSetupTotalRead();

  g_uibDisableDevice = false;
  buzzSuccess();

  while (1) {
    uartPoll();
    if (!g_uibDisableDevice && (g_uiProcStatus.dwCardID = uiCardCapture()))
      uiProcVerifyCard();
  }
}