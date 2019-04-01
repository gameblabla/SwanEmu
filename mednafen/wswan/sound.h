#ifndef __WSWAN_SOUND_H
#define __WSWAN_SOUND_H

#include <stdint.h>

int32 WSwan_SoundFlush(int16 *SoundBuf, const int32 MaxSoundFrames);

void WSwan_SoundInit(void);
void WSwan_SoundKill(void);
void WSwan_SetSoundMultiplier(double multiplier);
uint32_t WSwan_SetSoundRate(uint32 rate);

void WSwan_SoundWrite(uint32, uint8);
uint8 WSwan_SoundRead(uint32);
void WSwan_SoundReset(void);
void WSwan_SoundCheckRAMWrite(uint32 A);

void WSwan_SoundSaveState(uint32_t load, FILE* fp);

#endif
