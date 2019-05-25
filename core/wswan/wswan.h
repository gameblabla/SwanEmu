#ifndef WSWAN_H
#define WSWAN_H

#include "../mednafen.h"

#include "interrupt.h"

#define true 1
#define false 0

#define  mBCD(value) (((value)/10)<<4)|((value)%10)

#define min(a,b) \
	({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a < _b ? _a : _b; })

#define max(a,b) \
	({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a > _b ? _a : _b; })

extern uint32_t rom_size;
extern int wsc;

enum
{
   WSWAN_SEX_MALE = 1,
   WSWAN_SEX_FEMALE = 2
};

enum
{
   WSWAN_BLOOD_A = 1,
   WSWAN_BLOOD_B = 2,
   WSWAN_BLOOD_O = 3,
   WSWAN_BLOOD_AB = 4
};

#endif
