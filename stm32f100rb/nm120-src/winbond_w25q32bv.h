#ifndef _WINBOND_FLASH_H_
#define _WINBOND_FLASH_H_
void winbond_init();
void winbond_get_ids(unsigned char *man_id, unsigned char *dev_id);
void winbond_dump_memory_start();
unsigned char winbond_dump_memory();
#endif

