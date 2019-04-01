#ifndef __WSWAN_MEMORY_H
#define __WSWAN_MEMORY_H

#include <stdint.h>

enum
{
   MEMORY_GSREG_ROMBBSLCT = 0,
   MEMORY_GSREG_BNK1SLCT,
   MEMORY_GSREG_BNK2SLCT,
   MEMORY_GSREG_BNK3SLCT
};

extern uint8 wsRAM[65536];
extern uint8 *wsCartROM;
extern uint32 eeprom_size;
extern uint8 wsEEPROM[2048];
extern uint8 *wsSRAM;
extern uint32 wsRAMSize;

uint8 WSwan_readmem20(uint32);
void WSwan_writemem20(uint32 address,uint8 data);

void WSwan_MemoryInit(uint32_t lang, uint32_t IsWSC, uint32 ssize, uint32_t SkipSaveLoad);
void WSwan_MemoryKill(void);

void WSwan_CheckSoundDMA(void);
void WSwan_MemoryReset(void);
void WSwan_writeport(uint32 IOPort, uint8 V);
uint8 WSwan_readport(uint32 number);

uint32 WSwan_MemoryGetRegister(const unsigned int id, char *special, const uint32 special_len);
void WSwan_MemorySetRegister(const unsigned int id, uint32 value);

void WSwan_MemorySaveState(uint32_t load, FILE* fp);

#endif
