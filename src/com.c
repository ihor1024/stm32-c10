#include <string.h>
#include "typedef.h"
#include "uart.h"
#include "wdg.h"
#include "rtc.h"
#include "com.h"
#include "errno.h"
#include "database.h"
#include "tick.h"
#include "rfid.h"
#include "buzz.h"

DWORD dbComm_Password = 0, dbComm_machineID = 1;

int gbPollingMode = POLLING_MODE_NONE;

#define COMM_PWD_TCPIP    dbComm_Password
#define COMM_PWD_SERIAL   0
#define COMM_MTU          65536

#define RES_ACK           1
#define RES_NACK          0

#define CARD_READ_TIMEOUT 5000

int comm_mode = COMM_MODE_NONE;
int comm_sub_mode = -1;

BOOL comm_send_cmd(WORD cmd);
long comm_recv_ack(DWORD* pData);
BOOL comm_send_data(void* pbuf, int nsize);

enum {
  CMD_INTERNAL_CHECK_LIVE = 0x51,
  CMD_INTERNAL_CHECK_PWD
};

enum {
  OEM_CMD_OPEN = 0x1001,
  OEM_CMD_CLOSE,
  OEM_CMD_ENUMERATE,
  OEM_CMD_ENUMERATE_READ_USERINFO,
  OEM_CMD_ENUMERATE_WRITE_USERINFO,
  OEM_CMD_CAPTUREIMG,
  OEM_CMD_ISFINGER,
  OEM_CMD_IMAGERAW_GET,
  OEM_CMD_IMAGE256_GET,
  OEM_CMD_IMAGE256_PUT,
  OEM_CMD_TEMPLATE_ENROLL,
  OEM_CMD_TEMPLATE_IDENTIFY,
  OEM_CMD_TEMPLATE_VERIFY,
  OEM_CMD_TEMPLATE_INDEX_READ,
  OEM_CMD_TEMPLATE_INDEX_WRITE,
  OEM_CMD_DELETE,
  OEM_CMD_DELETE_ID,
  OEM_CMD_DELETEALL,
  OEM_CMD_ENROLL_START,
  OEM_CMD_ENROLL_NTH,
  OEM_CMD_ENROLL_END,
  OEM_CMD_IMAGE256_IDENTIFY,
  OEM_CMD_IMAGE256_VERIFY,

  OEM_CMD_ENUM_ENROLL_COUNT,
  OEM_CMD_ENUM_SEARCH_POS,
  OEM_CMD_ENUM_CHECK_POS,
  OEM_CMD_ENUM_SEARCH_ID,
  OEM_CMD_ENUM_CHECK_ID,
  OEM_CMD_ENROLL_CARD,
  OEM_CMD_READ_CARD_NUM,

  OEM_CMD_LOG_GET_COUNT,
  OEM_CMD_LOG_GET,
  OEM_CMD_LOG_GET_ALL,
  OEM_CMD_LOG_DELETE_ALL,

  OEM_CMD_POLLING_MODE,
  OEM_CMD_HEATER_SET,
  OEM_CMD_LED_OUT,
  OEM_CMD_BEEP_OUT,
  OEM_CMD_TRIGGER,
  OEM_CMD_LAST_SPOOF,
  OEM_CMD_SEND_WIEGAND,
  OEM_CMD_GET_DENIED_WIEGAND,
  OEM_CMD_SET_DENIED_WIEGAND,
  OEM_CMD_GET_KEEPALIVE_TIME,
  OEM_CMD_SET_KEEPALIVE_TIME,
  OEM_CMD_GET_TIMEZONE,
  OEM_CMD_SET_TIMEZONE,
  OEM_CMD_GET_USERTZ,
  OEM_CMD_SET_USERTZ,

  OEM_CMD_RESTART = 0x2001,
  OEM_CMD_FW_UPGRADE,
  OEM_CMD_SETTINGS_GET,
  OEM_CMD_SETTINGS_SET,
  OEM_CMD_MAC_SET,
  OEM_CMD_DEVICE_TIME_GET,
  OEM_CMD_DEVICE_TIME_SET,
  OEM_CMD_ALL_LOAD_ENROLL,
  OEM_CMD_ALL_SAVE_ENROLL,
  OEM_CMD_ALL_LOAD_LOG,
  OEM_CMD_ALL_SAVE_LOG,

  OEM_CMD_GET_SN = 0x2105,
};

void comm_crypt(void* pbuf, int nsize, DWORD dwCommPassword)
{
  int i;
  DWORD dwPwd = dwCommPassword;
  BYTE *pData = (BYTE*)pbuf;
  BYTE *pKey = (BYTE*)&dwPwd;
  for (i = 0; i < nsize; i++) *pData++ ^= pKey[i % 4];
}

BOOL comm_send(void* pbuf, int nsize)
{
  BOOL bRet = FALSE;

  comm_crypt(pbuf, nsize, COMM_PWD_SERIAL);
  bRet = (uartSend(pbuf, nsize) == nsize);
  comm_crypt(pbuf, nsize, COMM_PWD_SERIAL);

  return bRet;
}

BOOL comm_recv(void* pbuf, int nsize)
{
  BOOL bRet = FALSE;

  bRet = (uartRecv(pbuf, nsize) == nsize);
  if (bRet)
    comm_crypt(pbuf, nsize, COMM_PWD_SERIAL);
  return bRet;
}

#define STX1        0x55
#define STX2        0xAA
#define STX3        0x5A
#define STX4        0xA5

typedef struct {    //    cmd       ack       csum        reserve
  BYTE    head1;    //  55=STX1    AA=STX2    5A=STX3     A5=STX4
  BYTE    head2;    //  AA=STX2    55=STX1    A5=STX4     5A=STX3
  WORD    devid;
  DWORD   data;
  WORD    wPadding;
  WORD    csum;
} STAFF; // 12 BYTE

#define STAFF_SIZE    sizeof(STAFF)

#define CMD_ACK   1
#define CMD_NACK  0

#define DEVID     (dbComm_machineID)

WORD comm_csum(void* pbuf, int nsize)
{
  WORD csum = 0;
  int i;

  for (i = 0; i < nsize; i++)
    csum += ((BYTE*)pbuf)[i];

  return csum;
}

BOOL comm_check_staff(STAFF* pstaff, BYTE head1, BYTE head2)
{
  return (pstaff->head1 == head1 &&
    pstaff->head2 == head2 &&
    pstaff->devid == DEVID &&
    pstaff->csum == comm_csum(pstaff, STAFF_SIZE - 2));
}

#define comm_check_cmd(pstaff)  comm_check_staff(pstaff, STX1, STX2)
#define comm_check_ack(pstaff)  comm_check_staff(pstaff, STX2, STX1)
#define comm_check_csum(pstaff) comm_check_staff(pstaff, STX3, STX4)
#define comm_check_nack(pstaff) comm_check_staff(pstaff, STX4, STX3)

void comm_make_staff(STAFF* pstaff, BYTE head1, BYTE head2)
{
  pstaff->head1 = head1;
  pstaff->head2 = head2;
  pstaff->devid = DEVID;
  pstaff->csum = comm_csum(pstaff, STAFF_SIZE - 2);
}

#define comm_make_cmd(pstaff)   comm_make_staff(pstaff, STX1, STX2)
#define comm_make_ack(pstaff)   comm_make_staff(pstaff, STX2, STX1)
#define comm_make_csum(pstaff)  comm_make_staff(pstaff, STX3, STX4)
#define comm_make_nack(pstaff)  comm_make_staff(pstaff, STX4, STX3)
#define comm_make_resp(ack, pstaff)  (ack == CMD_ACK ? comm_make_ack(pstaff) : comm_make_nack(pstaff))

BOOL comm_send_cmd(WORD cmd)
{
  STAFF staff;

  staff.data = cmd;
  comm_make_cmd(&staff);

  return comm_send(&staff, STAFF_SIZE);
}

BOOL comm_send_ack(WORD ack, DWORD dwData)
{
  STAFF staff;

  staff.data = dwData;
  comm_make_resp(ack, &staff);

  return comm_send(&staff, STAFF_SIZE);
}

BOOL Com_SendData(void* pData, int nsize);
long comm_recv_ack(DWORD* pData)
{
  STAFF staff;
  long nRet = 0;

  if (!comm_recv(&staff, STAFF_SIZE))
    return 0;

  if (comm_check_ack(&staff))
    nRet = RES_ACK;
  else if (comm_check_nack(&staff))
    nRet = RES_NACK;

  if (pData)
    *pData = staff.data;

  return nRet;
}

BOOL comm_recv_ack_and_check()
{
  return (comm_recv_ack(NULL) == RES_ACK);
}

BOOL comm_send_data(void* pbuf, int nsize)
{
  STAFF staff;

  staff.data = comm_csum(pbuf, nsize);
  comm_make_csum(&staff);

  return (comm_send(pbuf, nsize) && comm_send(&staff, STAFF_SIZE));
}

BOOL comm_recv_data(void* pbuf, int nsize)
{
  STAFF staff;

  return (comm_recv(pbuf, nsize) &&
    comm_recv(&staff, STAFF_SIZE) &&
    comm_check_csum(&staff) &&
    staff.data == comm_csum(pbuf, nsize));
}

BOOL hcCommanProc(int nCMD);
BOOL Com_CommanProc(CMDPKT* vCMD);
int Com_CheckCMD(CMDPKT* pvCMD);

STAFF com_pending_staff;
BOOL com_pending_staff_valid = FALSE, com_pending_cmdpkt_valid = FALSE;
CMDPKT com_pending_cmdpkt;

int communication_check_cmd(void* cmd, int size, int channel, int sub_channel)
{
  if (size < STAFF_SIZE && size < CMDSIZE)
    return 0; //Don't drop invalid packet

  if (size >= CMDSIZE) {
    memcpy(&com_pending_cmdpkt, cmd, CMDSIZE);
    if(Com_CheckCMD(&com_pending_cmdpkt)) {
      com_pending_cmdpkt_valid = TRUE;
      comm_mode = channel;
      comm_sub_mode = sub_channel;
      return CMDSIZE; //Drop cmd packet
    }
  }

  if (size >= STAFF_SIZE) {
    memcpy(&com_pending_staff, cmd, STAFF_SIZE);
    comm_crypt(&com_pending_staff, STAFF_SIZE, COMM_PWD_SERIAL);
    if(comm_check_cmd(&com_pending_staff)) {
      com_pending_staff_valid = TRUE;
      comm_mode = channel;
      comm_sub_mode = sub_channel;
      return STAFF_SIZE; //Drop staff packet
    }
  }

  return 1; //Drop invalid packet
}

void communication_proc_cmd(void)
{
  if (com_pending_staff_valid) {
    hcCommanProc(com_pending_staff.data);
    com_pending_staff_valid = FALSE;
    comm_mode = COMM_MODE_NONE;
    comm_sub_mode = -1;
  }
  if (com_pending_cmdpkt_valid) {
    Com_CommanProc(&com_pending_cmdpkt);
    com_pending_cmdpkt_valid = FALSE;
    comm_mode = COMM_MODE_NONE;
    comm_sub_mode = -1;
  }
}

BOOL ComCheckPwd()
{
  DWORD  dwCommPwd;

  if (!comm_recv_data(&dwCommPwd, 4))
    return FALSE;

  return comm_send_ack(dbSetupTotal.dwCommPassword == dwCommPwd ?
    CMD_ACK : CMD_NACK, MODEL_FIRMWARE_MAGIC);
}

BOOL hcCommanProc(int nCMD)
{
  if (nCMD < 0x4000 || nCMD > 0x40FF)
    if (!comm_send_ack(CMD_ACK, 0))
      return FALSE;

  switch (nCMD) {
  case CMD_INTERNAL_CHECK_PWD:  
    ComCheckPwd();
    break;
  }

  return TRUE;
}


//////////////////////////////////////////////////////////////////////////
// classic_comm_send function is the same as comm_send function 
// except it doesn't perform any encryption operation.
BOOL classic_comm_send(void* pbuf, int nsize)
{
  return (uartSend(pbuf, nsize) == nsize);
}

// Same as comm_recv, except it doesn't crypt.
BOOL classic_comm_recv(void* pbuf, int nsize)
{
  return (uartRecv(pbuf, nsize) == nsize);
}

int Com_CheckCMD(CMDPKT* pvCMD)
{
  int i;
  BYTE byData[CMDSIZE];
  WORD wChkSum = 0;

  memcpy((void*)byData, (void*)pvCMD, CMDSIZE);
  if (byData[0] != STX1)
    return CMD_ANS_NACK;

  if (byData[1] != STX2)
    return CMD_ANS_NACK;

  if (pvCMD->MachineID != DEVID)
    return CMD_ANS_NACK;

  for (i = 0; i < CMDSIZE - 2; i++)
    wChkSum += byData[i];

  if(pvCMD->ChkSum != wChkSum)
    return CMD_ANS_NACK;

  return CMD_ANS_ACK;
}

BOOL Com_SendCmdAck(WORD wResult)
{
  WORD vSum;
  ACKPKT vACK;

  vACK.Head1 = STX4; vSum = vACK.Head1;
  vACK.Head2 = STX3; vSum += vACK.Head2;
  vACK.MachineID = DEVID; 
  vSum += LOBYTE(DEVID);
  vSum += HIBYTE(DEVID);
  vACK.Response = wResult; vSum += LOBYTE(wResult); vSum += HIBYTE(wResult);
  vACK.ChkSum = vSum;

  if (!classic_comm_send(&vACK, ACKSIZE))
    return FALSE;

  return TRUE;
}

void Com_SendExeResult(WORD wResult, DWORD dwOutParam)
{
  WORD vSum;

  RESULTPKT vRESULT = {0, 0, 0, 0, 0, 0, 0};

  vRESULT.Head1 = STX2; vSum = vRESULT.Head1;
  vRESULT.Head2 = STX1; vSum += vRESULT.Head2;
  vRESULT.MachineID = DEVID; 
  vSum += LOBYTE(DEVID);
  vSum += HIBYTE(DEVID);
  vRESULT.Ret = wResult; vSum += LOBYTE(wResult);vSum += HIBYTE(wResult);
  vRESULT.OutParam = dwOutParam; vSum += LOBYTE(LOWORD(dwOutParam)); vSum += LOBYTE(HIWORD(dwOutParam));
  vSum += HIBYTE(LOWORD(dwOutParam));vSum += HIBYTE(HIWORD(dwOutParam));
  vRESULT.ChkSum = vSum; 

  classic_comm_send(&vRESULT, EXERETSIZE);
}

BYTE __Com_Buf[COMM_BLOCK_SIZE + 6];
BOOL Com_RecData(void* pData, int nsize)
{
  int i;
  WORD CheckSum;
  int RecSize = nsize + 6;
  BYTE*  pRecBuff = __Com_Buf;

  if (!classic_comm_recv(pRecBuff, RecSize))
    return FALSE;

  if (pRecBuff[0] != STX3)
    return FALSE;

  CheckSum = pRecBuff[0];

  if (pRecBuff[1] != STX4)
    return FALSE;

  CheckSum += pRecBuff[1];

  if (*(WORD*)(pRecBuff + 2) != (WORD)DEVID)
    return FALSE;

  CheckSum += pRecBuff[2];
  CheckSum += pRecBuff[3];

  for (i = 4; i < RecSize - 2; i++)
    CheckSum += pRecBuff[i];

  if (CheckSum != *(WORD*)&pRecBuff[RecSize - 2])
    return FALSE;

  memcpy(pData, pRecBuff + 4, nsize);
  return TRUE;
}

BOOL Com_RecvCmdAck(WORD* pResult)
{
  ACKPKT vACK;
  if (!classic_comm_recv(&vACK, ACKSIZE))
    return FALSE;

  if (pResult)
    *pResult = vACK.Response;
  return TRUE;
}

BOOL Com_RecvCmdAckCheck(/*DWORD dwTimeOut = 2000*/)
{
  WORD wResult;
  return (Com_RecvCmdAck(&wResult) && wResult != CMD_NACK);
}

void Com_SendCmd(WORD aCmd, WORD aInParam, DWORD aTransSize)
{
  DWORD vSum;
  CMDPKT  vCMD;

  vCMD.Head1 = STX1;
  vSum = vCMD.Head1;

  vCMD.Head2 = STX2;
  vSum += (WORD)vCMD.Head2;

  vCMD.MachineID = (WORD)DEVID;
  vSum += HIBYTE(vCMD.MachineID); vSum += LOBYTE(vCMD.MachineID);

  vCMD.Reserved = 0x1979;
  vSum += LOBYTE(vCMD.Reserved); vSum += HIBYTE(vCMD.Reserved);

  vCMD.Command = aCmd;
  vSum += HIBYTE(vCMD.Command); vSum += LOBYTE(vCMD.Command);

  vCMD.Length = aTransSize;
  vSum += HIBYTE(HIWORD(vCMD.Length)); vSum += LOBYTE(HIWORD(vCMD.Length));
  vSum += HIBYTE(LOWORD(vCMD.Length)); vSum += LOBYTE(LOWORD(vCMD.Length));

  vCMD.InParam = aInParam;
  vSum += HIBYTE(vCMD.InParam); vSum += LOBYTE(vCMD.InParam);

  vCMD.ChkSum = (WORD)vSum;

  classic_comm_send(&vCMD, CMDSIZE);
}

BOOL Com_SendData(void* pData, int nsize)
{
  int i;
  WORD CheckSum;
  int SendSize = nsize + 6;
  BYTE*  pSendBuff = __Com_Buf;

  pSendBuff[0] = STX4;
  CheckSum = pSendBuff[0];

  pSendBuff[1] = STX3;
  CheckSum += pSendBuff[1];

  *(WORD*)(pSendBuff + 2) = (WORD)DEVID;
  CheckSum += pSendBuff[2];
  CheckSum += pSendBuff[3];

  memcpy( pSendBuff + 4, pData, nsize );
  for ( i = 0; i < nsize; i++ ) CheckSum += pSendBuff[i+4];
  *(WORD*)&pSendBuff[4+nsize] = CheckSum;

  return classic_comm_send(pSendBuff, SendSize);
}

int Com_RecBigData(void* pData, DWORD dwSize)
{
  int i;
  WORD CheckSum;
  BYTE* pRecBuff = __Com_Buf;
  BYTE* pPoint = (BYTE*)pData;
  int Bytes, Remain = (int)dwSize;

  if (pRecBuff == NULL)
    return -1;

  Bytes = dwSize;
  while (Remain > 0) {
    if (Remain > COMM_BLOCK_SIZE)
      Bytes = COMM_BLOCK_SIZE;
    else
      Bytes = Remain;

    if (!classic_comm_recv(pRecBuff, Bytes + 6))
      return -1;

    if (pRecBuff[0] != STX3)
      return -1;

    CheckSum = pRecBuff[0];

    if (pRecBuff[1] != STX4)
      return -1;

    CheckSum += pRecBuff[1];

    if (*(WORD*)(pRecBuff + 2) != (WORD)DEVID)
      return -1;

    CheckSum += pRecBuff[2];
    CheckSum += pRecBuff[3];

    for (i = 0; i < Bytes; i++)
      CheckSum += pRecBuff[i + 4];

    if (CheckSum != *(WORD*)&pRecBuff[Bytes + 4])
      return -1;

    memcpy(pPoint, pRecBuff + 4, Bytes);
    pPoint += Bytes;
    Remain -= Bytes;
  }
  return 1;
}

int Com_SendBigData(void* pData, DWORD dwSize)
{
  DWORD i;
  WORD CheckSum;
  BYTE *pSendBuff = __Com_Buf, *pPoint = (BYTE*)pData;
  int Remain = dwSize, Bytes;

  if (pSendBuff == NULL)
    return -1;

  while (Remain > 0) {
    if (Remain > COMM_BLOCK_SIZE)
      Bytes = COMM_BLOCK_SIZE;
    else
      Bytes = Remain;

    pSendBuff[0] = STX3;
    CheckSum = pSendBuff[0];

    pSendBuff[1] = STX4;
    CheckSum += pSendBuff[1];

    *(WORD*)(pSendBuff + 2) = (WORD)DEVID;
    CheckSum += pSendBuff[2];
    CheckSum += pSendBuff[3];

    memcpy( pSendBuff + 4, pPoint, Bytes );
    for (i = 0; i < (DWORD)Bytes; i++) CheckSum += (WORD)pSendBuff[i+4];
    *(WORD*)&pSendBuff[4+Bytes] = CheckSum;

    if (!classic_comm_send(pSendBuff, Bytes + 6))
      return -1;

    pPoint += Bytes;
    Remain -= Bytes;
  }
  return 1;
}

BOOL ComInternalCheckPwd(CMDPKT *vCMD)
{
  if (vCMD->InParam == 0 &&
    dbSetupTotal.dwCommPassword != 0 &&
    dbSetupTotal.dwCommPassword != vCMD->Length) {
    Com_SendExeResult(EXE_RES_FALSE, 0);
    return FALSE;
  }

  Com_SendExeResult(EXE_RES_TRUE, 0);
  return TRUE;
}

BOOL OemCmdOpen(CMDPKT *vCMD)
{
  Com_SendExeResult(FP_ERR_SUCCESS, 0);
  return TRUE;
}

BOOL OemCmdClose(CMDPKT* vCMD)
{
  Com_SendExeResult(FP_ERR_SUCCESS, 0);
  return TRUE;
}

BOOL OemCmdDeleteId(CMDPKT *vCMD)
{
  DbDeleteUser(vCMD->Length);

  Com_SendExeResult(FP_ERR_SUCCESS, 0);
  return TRUE;
}

BOOL OemCmdDeviceTimeGet(CMDPKT* vCMD)
{
  Com_SendExeResult(FP_ERR_SUCCESS, rtcGetSeconds());
  return TRUE;
}

BOOL OemCmdReadCardNum(CMDPKT* vCMD)
{
  uint t = getTickMs();
  uint cardNum = 0;
  do {
    cardNum = uiCardCapture();
    if (cardNum) {
      buzzSuccess();
      Com_SendExeResult(FP_ERR_SUCCESS, cardNum);
      return TRUE;
    }
  } while (getTickMs() - t < CARD_READ_TIMEOUT);
  Com_SendExeResult(FP_ERR_PARAM, 0);
  return FALSE;
}

BOOL OemCmdDeviceTimeSet(CMDPKT* vCMD)
{
  rtcSetSeconds(vCMD->Length);
  Com_SendExeResult(FP_ERR_SUCCESS, 0);
  return TRUE;
}

BOOL OemCmdLogGet(CMDPKT *vCMD)
{
  int nNewCount, nGetCount;
  int nTransCount = vCMD->Length;

  while (1) {
    nNewCount = DbGLogGetCountUnRead();
    Com_SendExeResult(FP_ERR_SUCCESS, nNewCount);

    if (nNewCount == 0)
      break;

    nGetCount = nNewCount;
    if (nGetCount > nTransCount)
      nGetCount = nTransCount;

    Com_SendBigData(&gpALogs[dbSetupTotal.nGLogRdPos], nGetCount * sizeof(ALOG_INFO));
    if (!Com_RecData(&nGetCount, 4))
      break;

    if (nGetCount == -1)
      break;

    dbSetupTotal.nGLogRdPos += nGetCount;
    dbSetupTotal.nGLogRdPos %= (dbLicense.nGlogMaxCount + 1);
  }

  return true;
}

BOOL OemCmdPollingMode(CMDPKT *vCMD)
{
  gbPollingMode = (int)vCMD->InParam;

  Com_SendExeResult(FP_ERR_SUCCESS, 0);
  return TRUE;
}

BOOL OemCmdEnrollCard(CMDPKT* vCMD)
{
  struct CardInfo {
    u32 uid;
    u32 card_num;
  } info;

  if (!Com_RecData(&info, sizeof(info)))
    return TRUE;

  Com_SendExeResult(DbCardEnroll(info.uid, info.card_num) ?
    FP_ERR_SUCCESS : FP_ERR_PARAM, 0);
  return TRUE;
}

bool Com_CommanProc(CMDPKT* vCMD)
{
  if (!Com_SendCmdAck(CMD_ACK))
    return false;

  bool bBank1Saved = !_Bank1Loaded;
  _Db_Bank1Load();

  switch (vCMD->Command) {
  case CMD_INTERNAL_CHECK_LIVE:
    break;
  case CMD_INTERNAL_CHECK_PWD:  
    ComInternalCheckPwd(vCMD);
    break;
  case OEM_CMD_OPEN:
    OemCmdOpen(vCMD);
    break;
  case OEM_CMD_CLOSE:
    OemCmdClose(vCMD);
    break;
  case OEM_CMD_DELETE_ID:
    OemCmdDeleteId(vCMD);
    break;
  case OEM_CMD_DEVICE_TIME_GET:
    OemCmdDeviceTimeGet(vCMD);
    break;
  case OEM_CMD_DEVICE_TIME_SET:
    OemCmdDeviceTimeSet(vCMD);
    break;
  case OEM_CMD_LOG_GET:
    OemCmdLogGet(vCMD);
    break;
  case OEM_CMD_POLLING_MODE:
    OemCmdPollingMode(vCMD);
    break;
  case OEM_CMD_ENROLL_CARD:
    OemCmdEnrollCard(vCMD);
    break;
  case OEM_CMD_READ_CARD_NUM:
    OemCmdReadCardNum(vCMD);
  }

  if (bBank1Saved)
    _Db_Bank1Save();

  return true;
}

BOOL Com_SimpleAppHostCommandProc(void)
{
  return TRUE;
}