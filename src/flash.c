#include "stm32f10x_flash.h"
#include <string.h>
#include "core_cm3.h"
#include "typedef.h"
#include "flash.h"
#include "database.h"
#include "buzz.h"
#include "wdg.h"

static bool flashWritePage(u32 dest, void* src)
{
  __IO FLASH_Status FLASHStatus = FLASH_COMPLETE;
  
  FLASH_ErasePage(dest);
  
  if ((int)src == -1)
    return true;

  u32* p = (u32*)src;
  while (FLASHStatus == FLASH_COMPLETE && (u8*)p < (u8*)src + FLASH_PAGE_SIZE) {
#ifdef USE_WDG
    wdgRefresh();
#endif
    if (*p != -1)
      FLASHStatus = FLASH_ProgramWord(dest, *p);
    p++, dest += sizeof(u32);
  }

  return ((u8*)p >= (u8*)src + FLASH_PAGE_SIZE);
}

static bool isEmptyBlock(const void* p, const int len)
{
  u32* p_;
  for (p_ = (u32*)p; p_ < (u32*)p + len / sizeof(u32); p_++)
    if (*p_ != -1)
      break;
  return (p_ >= (u32*)p + len / sizeof(u32));
}

static bool flashWrite2(const void* dest, const void* src, int len)
{
  bool ret = true;

  FLASH_UnlockBank1();
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

  u8* p = (u8*)src;
  u32 addr = (uint)dest / FLASH_PAGE_SIZE * FLASH_PAGE_SIZE;
  u32 remainder = (uint)dest % FLASH_PAGE_SIZE, remainder_ = FLASH_PAGE_SIZE - remainder;
  u32 len_ = MIN(remainder_, len);
  
  /* Write first page */
  bool to_write = false;
  if ((int)src == -1) {
    if (!isEmptyBlock((u8*)addr + remainder, len_)) {
      memcpy(flash_page_buf, (void*)addr, FLASH_PAGE_SIZE);
      memset(flash_page_buf + remainder, -1, len_);
      to_write = true;
    }
  } else if (memcmp((void*)(addr + remainder), p, len_)) {
    memcpy(flash_page_buf, (void*)addr, FLASH_PAGE_SIZE);
    memcpy(flash_page_buf + remainder, p, len_);
    to_write = true;
  }
  if (to_write && !flashWritePage(addr, flash_page_buf)) {
    ret = false;
    goto l_exit;
  }

  addr += FLASH_PAGE_SIZE;
  p += len_;
  len -= len_;

  /* Write the pages */
  while (len >= FLASH_PAGE_SIZE) {
    to_write = false;
    if ((int)src == -1) {
      if (!isEmptyBlock((u8*)addr, FLASH_PAGE_SIZE))
        to_write = true;
    } else if (memcmp((void*)addr, p, FLASH_PAGE_SIZE)) {
      to_write = true;
    }
    if (to_write && !flashWritePage(addr, ((int)src != -1) ? p : (void*)-1)) {
      ret = false;
      goto l_exit;
    }
    addr += FLASH_PAGE_SIZE;
    p += FLASH_PAGE_SIZE;
    len -= FLASH_PAGE_SIZE;
  }

  /* Write last page */
  to_write = false;
  if ((int)src == -1) {
    if (!isEmptyBlock((u8*)addr, len)) {
      memcpy(flash_page_buf, (void*)addr, FLASH_PAGE_SIZE);
      memset(flash_page_buf, -1, len);
      to_write = true;
    }
  } else if (memcmp((void*)addr, p, len)) {
    memcpy(flash_page_buf, (void*)addr, FLASH_PAGE_SIZE);
    memcpy(flash_page_buf, p, len);
    to_write = true;
  }
  if (to_write)
    ret = flashWritePage(addr, flash_page_buf);
  
l_exit:
  FLASH_LockBank1();
  return ret;
}

bool flashWrite(const void *dest, const void* src, const int len)
{
  /* Wait during buzzer arming */
  buzzWait();

  /* Write to flash */
  return flashWrite2(dest, src, len);
}