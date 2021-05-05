#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

typedef unsigned char bool, BOOL, BYTE;
typedef unsigned int uint, DWORD;
typedef unsigned short WORD;

#define false		0
#define true 		1
#define FALSE		0
#define TRUE		1

#define BIT(nr)		  (1<<(nr))
#define MIN(x1,x2)  (((x1)<(x2))? (x1):(x2))
#define LOBYTE(w)   ((u8)(w))
#define HIBYTE(w)   ((u8)(((u16)(w) >> 8) & 0xFF))
#define LOWORD(l)   ((u16)(l))
#define HIWORD(l)   ((u16)(((u32)(l) >> 16) & 0xFFFF))

#endif //__TYPEDEF_H__