#ifndef __MDFN_MEMPATCHER_H
#define __MDFN_MEMPATCHER_H

uint32_t MDFNMP_Init(uint32_t ps, uint32_t numpages);
void MDFNMP_AddRAM(uint32_t size, uint32_t address, uint8_t *RAM);
void MDFNMP_Kill(void);

#endif
