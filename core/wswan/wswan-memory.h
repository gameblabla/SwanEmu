#ifndef WSWAN_MEMORY_H
#define WSWAN_MEMORY_H

#include <stdint.h>

enum
{
   MEMORY_GSREG_ROMBBSLCT = 0,
   MEMORY_GSREG_BNK1SLCT,
   MEMORY_GSREG_BNK2SLCT,
   MEMORY_GSREG_BNK3SLCT
};

extern uint8_t wsRAM[65536];
extern uint8_t *wsCartROM;
extern uint32_t eeprom_size;
extern uint8_t wsEEPROM[2048];
extern uint8_t *wsSRAM;
extern uint32_t wsRAMSize;

uint8_t WSwan_readmem20(uint32_t A);
void WSwan_writemem20(uint32_t A, uint8_t V);

void WSwan_MemoryInit(uint32_t lang, uint32_t IsWSC, uint32_t ssize, uint32_t SkipSaveLoad) SWANEMU_COLD;
void WSwan_MemoryKill(void) SWANEMU_COLD;

void WSwan_CheckSoundDMA(void);
void WSwan_MemoryReset(void);
void WSwan_writeport(uint32_t IOPort, uint8_t V);
uint8_t WSwan_readport(uint32_t number);

uint32_t WSwan_MemoryGetRegister(const unsigned int id, char *special, const uint32_t special_len);
void WSwan_MemorySetRegister(const unsigned int id, uint32_t value);

void WSwan_MemorySaveState(uint32_t load, FILE* fp);

#endif
