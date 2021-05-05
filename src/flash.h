#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_PAGE_SIZE 1024

bool flashWrite(const void *dest, const void* src, const int len);

#endif //__FLASH_H__