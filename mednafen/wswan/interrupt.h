#ifndef __WSWAN_INTERRUPT_H
#define __WSWAN_INTERRUPT_H

enum
{
 WSINT_SERIAL_SEND = 0,
 WSINT_KEY_PRESS,
 WSINT_RTC_ALARM,
 WSINT_SERIAL_RECV,
 WSINT_LINE_HIT,
 WSINT_VBLANK_TIMER,
 WSINT_VBLANK,
 WSINT_HBLANK_TIMER
};

void WSwan_Interrupt(int);
void WSwan_InterruptWrite(uint32 A, uint8 V);
uint8 WSwan_InterruptRead(uint32 A);
void WSwan_InterruptCheck(void);
void WSwan_InterruptReset(void);
void WSwan_InterruptDebugForce(unsigned int level);
void WSwan_InterruptSaveState(uint32_t load, FILE* fp);


#endif
