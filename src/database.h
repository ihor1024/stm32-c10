#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "stm32f10x.h"
#include "typedef.h"
#include "flash.h"

#define MAX_ENROLL_COUNT  200
#define MAX_ALOG_COUNT    13500

#define MIN_ID					  1
#define MAX_ID					  99999999

#define FLASH_CHECKSUM_SIGN 0x20201227
#define LICENSE_STR_LEN		32

#define LNSC_SERIAL				"C10-201227-01"
#define LNSC_SOFTNAME			"C10 v1.00"
#define LNSC_PRODUCT_NAME	"C10 ATT"
#define LNSC_FIRST_DATE		"2020/12/27"
#define LNSC_COMP_NAME		" Co.,Ltd."
#define FW_BUILDNUM				0x20201227
#define MODEL_FIRMWARE_MAGIC 0x0C100001

typedef struct {
  u32	  dwCheckSum;
  u32	  dwBuildNumber;
  char	szSerialNumber[LICENSE_STR_LEN];
  char	szProductCode[LICENSE_STR_LEN];

  char	szFirstDate[LICENSE_STR_LEN];
  char	szManufacturerName[LICENSE_STR_LEN];
  char	szProductName[LICENSE_STR_LEN];
  char	szProductType[LICENSE_STR_LEN];

  u32	  dwReleaseTime;
  s32   nGlogMaxCount;
  u8    reserved[12];
} DBLICENSE; //220

typedef struct {
  u32   dwCheckSum;
  s32		nGLogRdPos;

  s32   nGLogWrn;
  s32   nReverifyTime;

  u32	  dwMachineID;
  u32	  dwCommPassword;

  s32	  nAutoSleepMinute;
  u8    reserved[8];
} DBSETUP_TOTAL; //36

typedef struct {
  u32	  dwID;
  u32	  dwCard;
} USER_INFO; //8

typedef struct {
  u32	  dwSeconds;
  u32	  wId;
} ALOG_INFO; //8

#define BANK1_MEMORY_SIZE	(2 * 1024)

#pragma pack(4)
typedef struct {
  BYTE  bank1_memory[BANK1_MEMORY_SIZE];
  ALOG_INFO gALogInfo;
  u8 flash_page_buf[FLASH_PAGE_SIZE];
} AppTemps;

typedef struct {
  u32   gTimerCount;
  u32   gPrevCounter;

  //database
  DBLICENSE*  gpDbLicense;
  DBSETUP_TOTAL*  gpDbSetupTotal;
  USER_INFO*  gpUInfoData;
  s32   gGLogPos;
  s32   gGLogStartPos;
  bool _Bank1Loaded;
  u32	  g_dwPrevRtcGet;
  bool  g_uibDisableDevice;
  u32	  g_uiTimeLastAction;
  bool	g_bIsSleepMode;
  bool  g_uiCommandProc;
  bool	g_uiPowerOffFlag;
  bool  g_uiClearForLogWrn;

  struct {
    u32	dwCardID;
    u32	dwTimeDisableDevice;
  } g_uiProcStatus;
} AppVars;
#pragma pack()

#define gpDbLicense		    appvars.gpDbLicense
#define gpDbSetupTotal	  appvars.gpDbSetupTotal
#define gpUInfoData		    appvars.gpUInfoData
#define gGLogPos			    appvars.gGLogPos
#define gGLogStartPos	    appvars.gGLogStartPos
#define _Bank1Loaded	    appvars._Bank1Loaded
#define g_dwPrevRtcGet		appvars.g_dwPrevRtcGet
#define _Bank1Loaded			appvars._Bank1Loaded
#define g_uiProcStatus		appvars.g_uiProcStatus
#define g_uibDisableDevice appvars.g_uibDisableDevice

#define dbLicense			    (*gpDbLicense)
#define dbSetupTotal	    (*gpDbSetupTotal)

#define bank1_memory			apptemps.bank1_memory
#define gALogInfo				  apptemps.gALogInfo
#define flash_page_buf    apptemps.flash_page_buf

/* Memory map */
#define FLASH_START_ADDRESS 0x08000000
#define FIRMWARE_SIZE     0x5000

#define APPDATA_MAP0			(FLASH_START_ADDRESS + FIRMWARE_SIZE)
#define APPDATA_MSET_SIZE	0x100
#define dbLicenseR				(*(DBLICENSE*)APPDATA_MAP0)
#define dbSetupTotalR			(*(DBSETUP_TOTAL*) (&dbLicenseR + 1))
#define gpUInfoDataR			((USER_INFO*)(APPDATA_MAP0 + APPDATA_MSET_SIZE))

#define APPDATA_MAP1			(APPDATA_MAP0 + BANK1_MEMORY_SIZE)
#define SFLASH_ALOG_START	((ALOG_INFO*)APPDATA_MAP1)
#define gpALogs					  SFLASH_ALOG_START

#define dbLicenseW				(*(DBLICENSE*)bank1_memory)
#define dbSetupTotalW			(*(DBSETUP_TOTAL*)(&dbLicenseW + 1))
#define gpUInfoDataW			((USER_INFO*)(bank1_memory + APPDATA_MSET_SIZE))

extern AppTemps	  apptemps;
extern AppVars		appvars;

BOOL DbDeleteUser(int nID);
int DbGLogGetCountUnRead(void);
bool DbCardEnroll(int uid, uint card_id);
void _Db_Bank1Load(void);
void _Db_Bank1Save(void);
int DbGLogGetExtraCount(void);
bool DbCardVerify(DWORD dwCard, int* pnID);
bool DbGlogIsReverify(int nID);
void uiLogGlogAdd(int uid);
void _Db_AppVarsInit(void);
bool DbSetupTotalRead(void);

#endif //__DATABASE_H__