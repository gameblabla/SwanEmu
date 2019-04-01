#ifndef __WSWAN_RTC_H
#define __WSWAN_RTC_H

void WSwan_RTCWrite(uint32 A, uint8 V);
uint8 WSwan_RTCRead(uint32 A);
void WSwan_RTCReset(void);
void WSwan_RTCClock(uint32 cycles);
void WSwan_RTCSaveState(uint32_t load, FILE* fp);

#endif
