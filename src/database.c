#include <string.h>
#include "database.h"
#include "typedef.h"
#include "flash.h"
#include "rtc.h"
#include "buzz.h"
#include "com.h"

#define EMPTY_DATA (-1UL)

#define DbGlogPos2Virtual(nRealPos) (((nRealPos) - gGLogStartPos + \
  dbLicense.nGlogMaxCount + 1) % (dbLicense.nGlogMaxCount + 1))

AppTemps apptemps;
AppVars appvars;

void __dbSetupTotal_apply()
{
  //uartInit();
}

static inline u32 DbSetupCsum(void* pSetup, int nSize)
{
  DWORD* pdw = (DWORD*)pSetup;
  DWORD dwCheckSum = FLASH_CHECKSUM_SIGN;
  int i;

  for (i=0; i<nSize/4; i++)
  {
    dwCheckSum += *pdw;
    pdw++;
  }

  return ~dwCheckSum;
}

bool DbSetupTotalWrite(void)
{
  dbLicense.dwCheckSum = 0;
  dbLicense.dwCheckSum = DbSetupCsum(&dbLicense, sizeof(DBLICENSE));

  dbSetupTotal.dwCheckSum = 0;
  dbSetupTotal.dwCheckSum = DbSetupCsum(&dbSetupTotal, sizeof(DBSETUP_TOTAL));
  
  return true;
}

int DbUInfoGetPosition(int nID)
{
  int i;

  for (i = 0; i < MAX_ENROLL_COUNT; i++)
    if (gpUInfoData[i].dwID == nID)
      break;

  if (i == MAX_ENROLL_COUNT)
    return -1;

  return i;
}

bool DbCardEnroll(int uid, uint card_id)
{
  int nUPos;

  if (uid < MIN_ID || uid > MAX_ID)
    return false;

  if ((nUPos = DbUInfoGetPosition(uid)) < 0) {
    if ((nUPos = DbUInfoGetPosition(EMPTY_DATA)) < 0)
      return false;

    memset(&gpUInfoData[nUPos], 0, sizeof(USER_INFO));
    gpUInfoData[nUPos].dwID = EMPTY_DATA;
  }
  gpUInfoData[nUPos].dwID = uid;
  gpUInfoData[nUPos].dwCard = card_id;

  return true;
}

void _Db_AppVarsInit(void)
{
  memset(&appvars, 0, sizeof(appvars));

  gpDbLicense = &dbLicenseW;
  gpDbSetupTotal = &dbSetupTotalW;
  gpUInfoData = gpUInfoDataW;

  g_dwPrevRtcGet = -1000UL;
}

bool DbCardVerify(DWORD dwCard, int* pnID)
{
  for (int i = 0; i < MAX_ENROLL_COUNT; i++) {
    if (gpUInfoData[i].dwID != EMPTY_DATA && gpUInfoData[i].dwCard == dwCard) {
      *pnID = gpUInfoData[i].dwID;
      return true;
    }
  }
  return false;
}

bool DbLicenseApply(void)
{
  bool bRet = true;

  if (DbSetupCsum(&dbLicense, sizeof(DBLICENSE))) {//to default
    memset(&dbLicense, 0, sizeof(DBLICENSE));
    strcpy(dbLicense.szSerialNumber, LNSC_SERIAL);
    strcpy(dbLicense.szProductCode, LNSC_SOFTNAME);
    strcpy(dbLicense.szProductName, LNSC_PRODUCT_NAME);
    strcpy(dbLicense.szFirstDate, LNSC_FIRST_DATE);
    strcpy(dbLicense.szManufacturerName, LNSC_COMP_NAME);
    dbLicense.nGlogMaxCount = MAX_ALOG_COUNT;
    bRet = false;
  }
  dbLicense.dwReleaseTime = FW_BUILDNUM;
  return bRet;
}

void DbSetupTotalNew2Default(void)
{
  memset(&dbSetupTotal, 0, sizeof(DBSETUP_TOTAL));
  dbSetupTotal.dwMachineID = 1;
  dbSetupTotal.dwCommPassword = 0;
}

BOOL DbGLogDeleteAll(void)
{
  dbSetupTotal.nGLogRdPos = 0;
  gGLogStartPos = 0;
  gGLogPos = 0;
  return flashWrite(SFLASH_ALOG_START, (void*)-1, sizeof(ALOG_INFO) * (dbLicense.nGlogMaxCount + 1));
}

void DbGLogCalcPos(void)
{
  int nPos = 0;

  while (nPos <= dbLicense.nGlogMaxCount) {
    if (gpALogs[nPos].dwSeconds == (DWORD)EMPTY_DATA)
      break;
    nPos++;
  }

  if (!nPos) {
    while (nPos <= dbLicense.nGlogMaxCount) {
      if (gpALogs[nPos].dwSeconds != (DWORD)EMPTY_DATA)
        break;
      nPos++;
    }
    gGLogStartPos = nPos % (dbLicense.nGlogMaxCount + 1);

    while (nPos <= dbLicense.nGlogMaxCount) {
      if (gpALogs[nPos].dwSeconds == (DWORD)EMPTY_DATA)
        break;
      nPos++;
    }
    gGLogPos = nPos % (dbLicense.nGlogMaxCount + 1);
  } else {
    if (nPos > dbLicense.nGlogMaxCount) { // except
      DbGLogDeleteAll();
    } else {
      gGLogPos = nPos;
      while (nPos <= dbLicense.nGlogMaxCount) {
        if (gpALogs[nPos].dwSeconds != (DWORD)EMPTY_DATA)
          break;
        nPos++;
      }
      gGLogStartPos = nPos % (dbLicense.nGlogMaxCount + 1);
    }
  }

  nPos = gGLogPos;

  while (nPos != gGLogStartPos) {
    if (gpALogs[nPos].dwSeconds != (DWORD)EMPTY_DATA) {
      DbGLogDeleteAll();
      break;
    }
    nPos++;
    if (nPos > dbLicense.nGlogMaxCount)
      nPos = 0;
  }
  if (DbGlogPos2Virtual(gGLogPos) < DbGlogPos2Virtual(dbSetupTotal.nGLogRdPos))
    DbGLogDeleteAll();
}

BOOL DbUInfoDelete(int nUPos)
{
  if (nUPos < 0 || nUPos >= MAX_ENROLL_COUNT)
    return FALSE;

  memset(&gpUInfoData[nUPos], (BYTE)EMPTY_DATA, sizeof(USER_INFO));
  return TRUE;
}

void DbUInfoCheckIntegrity(void)
{
  for (int i = 0; i < MAX_ENROLL_COUNT; i++)
    if (gpUInfoData[i].dwID != EMPTY_DATA && gpUInfoData[i].dwCard == EMPTY_DATA)
      DbUInfoDelete(i);
}

void _Db_Bank1Load(void)
{
  if(_Bank1Loaded)
    return;

  _Bank1Loaded = TRUE;

  memcpy(&bank1_memory[0], (void*)APPDATA_MAP0, BANK1_MEMORY_SIZE);

  gpDbLicense = &dbLicenseW;
  gpDbSetupTotal = &dbSetupTotalW;
  gpUInfoData = gpUInfoDataW;
}

void _Db_Bank1Save(void)
{
  if(!_Bank1Loaded)
    return;

  _Bank1Loaded = FALSE;
  DbSetupTotalWrite();
  flashWrite((void*)APPDATA_MAP0, bank1_memory, BANK1_MEMORY_SIZE);
  gpDbLicense = &dbLicenseR;
  gpDbSetupTotal = &dbSetupTotalR;
  gpUInfoData = gpUInfoDataR;
}

bool DbSetupTotalRead(void)
{
  _Db_Bank1Load();
  bool bRet = DbLicenseApply();
  if (!bRet || (DbSetupCsum(&dbSetupTotal, sizeof(DBSETUP_TOTAL)) != 0)) {
    DbSetupTotalNew2Default();
    bRet = FALSE;
  }

  DbGLogCalcPos();

  DbUInfoCheckIntegrity();
  _Db_Bank1Save();

  return bRet;
}

#define GLOG_EMPTY_COUNT	1000
void DbGlogMakeEmpty(void)
{
  if (DbGLogGetExtraCount() == 0) {
    bool bCorrectRdPos;
    int nPos = gGLogPos + GLOG_EMPTY_COUNT;

    if (nPos <= dbLicense.nGlogMaxCount) {
      flashWrite(SFLASH_ALOG_START + gGLogPos + 1, (void*)-1, sizeof(ALOG_INFO) * GLOG_EMPTY_COUNT);
      bCorrectRdPos = (dbSetupTotal.nGLogRdPos >= gGLogStartPos && 
        dbSetupTotal.nGLogRdPos <= nPos);
    } else {
      flashWrite(SFLASH_ALOG_START + gGLogPos + 1, (void*)-1, sizeof(ALOG_INFO) * (dbLicense.nGlogMaxCount - gGLogPos));
      flashWrite(SFLASH_ALOG_START, (void*)-1, sizeof(ALOG_INFO) * (nPos - dbLicense.nGlogMaxCount));
      bCorrectRdPos = (dbSetupTotal.nGLogRdPos >= gGLogStartPos || 
        dbSetupTotal.nGLogRdPos <= nPos - dbLicense.nGlogMaxCount); 
    }
    gGLogStartPos = (nPos + 1) % (dbLicense.nGlogMaxCount + 1);
    if (bCorrectRdPos) {
      bool bPrevLoaded = _Bank1Loaded;
      _Db_Bank1Load();
      dbSetupTotal.nGLogRdPos = gGLogStartPos;
      if (!bPrevLoaded)
        _Db_Bank1Save();
    }
  }
}

BOOL DbGLogSaveOne(int nPos, ALOG_INFO* pAlogInfo)
{
  return flashWrite((void*)(SFLASH_ALOG_START + nPos), pAlogInfo, sizeof(ALOG_INFO));
}

int DbGLogGetCountUnRead(void)
{
  return (gGLogPos - dbSetupTotal.nGLogRdPos + dbLicense.nGlogMaxCount + 1) % 
    (dbLicense.nGlogMaxCount + 1);
}

int DbGLogGetExtraCount(void)
{
  return gGLogStartPos < gGLogPos ?
    (gGLogStartPos + dbLicense.nGlogMaxCount - gGLogPos) : (gGLogStartPos - gGLogPos - 1);
}

void uiLogGlogAdd(int uid)
{
  DbGlogMakeEmpty();

  gALogInfo.dwSeconds = rtcGetSeconds();
  gALogInfo.wId = (u32)uid;

  DbGLogSaveOne(gGLogPos, &gALogInfo);
  gGLogPos = (gGLogPos + 1) % (dbLicense.nGlogMaxCount + 1);

  if (dbSetupTotal.nGLogWrn) {
    if (DbGLogGetExtraCount() < dbSetupTotal.nGLogWrn) {
      /* Arm alarm */
      buzzAlarm();
    }
  }

  if (gbPollingMode == POLLING_MODE_EVENT) {
    comm_mode = COMM_MODE_SERIAL;
    Com_SendCmd(0x3001, 0, uid);
  }
}

BOOL DbDeleteUser(int nID)
{
  int nUPos;

  if ((nUPos = DbUInfoGetPosition(nID)) < 0)
    return false;

  DbUInfoDelete(nUPos);

  return true;
}

bool DbGlogIsReverify(int nID)
{
  if (!dbSetupTotal.nReverifyTime) {
    return false;
  } else {
    int nPos = gGLogPos;
    uint dwSecondsNow = rtcGetSeconds(), dwSecondsLast;

    while (nPos != gGLogStartPos) {
      nPos--;
      if (nPos < 0) nPos = dbLicense.nGlogMaxCount;
      dwSecondsLast = gpALogs[nPos].dwSeconds;

      if (dwSecondsNow > dwSecondsLast + dbSetupTotal.nReverifyTime * 60)
        return false;

      if (gpALogs[nPos].wId == (uint)nID)
        return true;
    }

    return false;
  }
}