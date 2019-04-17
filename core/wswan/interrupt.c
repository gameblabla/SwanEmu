#include "wswan.h"
#include "interrupt.h"
#include "v30mz.h"

static uint8_t IStatus;
static uint8_t IEnable;
static uint8_t IVectorBase;

static uint32_t IOn_Cache;
static uint32_t IOn_Which;
static uint32_t IVector_Cache;

static void RecalcInterrupt(void)
{
	uint32_t i;
	IOn_Cache = false;
	IOn_Which = 0;
	IVector_Cache = 0;

	for(i = 0; i < 8; i++)
	{
		if(IStatus & IEnable & (1 << i))
		{
			IOn_Cache = true;
			IOn_Which = i;
			IVector_Cache = (IVectorBase + i) * 4;
			break;
		}
	}
}

void WSwan_InterruptDebugForce(unsigned int level)
{
	v30mz_int((IVectorBase + level) * 4, true);
}

void WSwan_Interrupt(int which)
{
	if(IEnable & (1 << which))
		IStatus |= 1 << which;

	RecalcInterrupt();
}

void WSwan_InterruptWrite(uint32_t A, uint8_t V)
{
	switch(A)
	{
		case 0xB0:
			IVectorBase = V; RecalcInterrupt();
		break;
		case 0xB2:
			IEnable = V; IStatus &= IEnable; RecalcInterrupt();
		break;
		case 0xB6:
			IStatus &= ~V;
			RecalcInterrupt();
		break;
	}
}

uint8_t WSwan_InterruptRead(uint32_t A)
{
	switch(A)
	{
		case 0xB0:
			return(IVectorBase);
		case 0xB2:
			return(IEnable);
		case 0xB6:
			return(1 << IOn_Which); //return(IStatus);
	}

	return 0;
}

void WSwan_InterruptCheck(void)
{
	if(IOn_Cache)
		v30mz_int(IVector_Cache, false);
}

void WSwan_InterruptReset(void)
{
	IEnable = 0x00;
	IStatus = 0x00;
	IVectorBase = 0x00;
	RecalcInterrupt();
}

void WSwan_InterruptSaveState(uint32_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&IStatus, sizeof(uint8_t), sizeof(IStatus), fp);
		fread(&IEnable, sizeof(uint8_t), sizeof(IEnable), fp);
		fread(&IVectorBase, sizeof(uint8_t), sizeof(IVectorBase), fp);
	}
	else
	{
		fwrite(&IStatus, sizeof(uint8_t), sizeof(IStatus), fp);
		fwrite(&IEnable, sizeof(uint8_t), sizeof(IEnable), fp);
		fwrite(&IVectorBase, sizeof(uint8_t), sizeof(IVectorBase), fp);
	}
}
