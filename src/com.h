#ifndef __COM_H__
#define __COM_H__

typedef enum {
	COMM_MODE_NONE = -1,
	COMM_MODE_SERIAL,
} T_COMM_MODE;

typedef enum {
	DISABLE_MODE_NONE,
	DISABLE_MODE_UART0,
	DISABLE_MODE_UART1,
	DISABLE_MODE_UART2,
	DISABLE_MODE_USB,
	DISABLE_MODE_TCP,
	DISABLE_MODE_UDP,
} T_DISABLE_MODE;

#define STX1				0x55
#define STX2				0xAA
#define STX3				0x5A
#define STX4				0xA5

#define CMD_ANS_ACK			1
#define CMD_ANS_NACK		0

#define EXE_RES_TRUE		1
#define EXE_RES_FALSE		0

#define BLOCK_SIZE			1020
#define COMM_BLOCK_SIZE		BLOCK_SIZE
#define CMDSIZE	        16
#define ACKSIZE	        8
#define EXERETSIZE	    14

typedef struct {
	BYTE 	Head1;		  // 55=STX1
	BYTE 	Head2;		  // AA=STX2
	WORD  MachineID;
	WORD  Reserved;
	WORD 	Command;
	DWORD	Length;		  // SettingValue
  WORD	InParam;
	WORD 	ChkSum;
} CMDPKT; // 14BYTE  

typedef struct {
	BYTE 	Head1;		  // 5A=STX1
	BYTE 	Head2;		  // A5=STX2
	WORD  MachineID;
  WORD	Response;
	WORD 	ChkSum;
} ACKPKT; // 8Byte  

//Response = 0x02  (NAK)
//           0x03  (ACK)

typedef struct {
	BYTE  Head1;		// AA=STX1
	BYTE  Head2;		// 55=STX2
	WORD  MachineID;
	WORD  Reserved;
  WORD	Ret;		  // 1: OK 0: Error
	DWORD	OutParam;	// Result or ErrorCode
	WORD 	ChkSum;
} RESULTPKT; // 12Byte

extern int comm_mode;

void comm_crypt(void* pbuf, int nsize, DWORD dwCommPassword);
int  Com_CheckCMD(CMDPKT* pvCMD);
void Com_SendCmd(WORD aCmd, WORD aInParam, DWORD aTransSize);
BOOL Com_SendCmdAck(WORD wResult);
BOOL Com_HostCommandProc( void );
BOOL Com_CommanProc( CMDPKT* vCMD );
BOOL Com_SimpleAppHostCommandProc( void );

int communication_check_cmd(void* cmd, int size, int channel, int sub_channel);
void communication_proc_cmd(void);

enum {
	POLLING_MODE_NONE,
	POLLING_MODE_GETLOG,
	POLLING_MODE_EVENT
};

extern int gbPollingMode;

#endif