#ifndef __MDFN_MEMPATCHER_H
#define __MDFN_MEMPATCHER_H

uint32_t MDFNMP_Init(uint32 ps, uint32 numpages);
void MDFNMP_AddRAM(uint32 size, uint32 address, uint8 *RAM);
void MDFNMP_Kill(void);



#endif
