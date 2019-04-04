#ifndef WSWAN_SOUND_H
#define WSWAN_SOUND_H

#include <stdint.h>

int32_t WSwan_SoundFlush(int16_t *SoundBuf, const int32_t MaxSoundFrames);

void WSwan_SoundInit(void);
void WSwan_SoundKill(void);
void WSwan_SetSoundMultiplier(float multiplier);
uint32_t WSwan_SetSoundRate(uint32_t rate);

void WSwan_SoundWrite(uint32_t, uint8_t);
uint8_t WSwan_SoundRead(uint32_t);
void WSwan_SoundReset(void);
void WSwan_SoundCheckRAMWrite(uint32_t A);

void WSwan_SoundSaveState(uint32_t load, FILE* fp);

#endif
