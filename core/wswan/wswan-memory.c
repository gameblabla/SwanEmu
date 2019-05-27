/* Cygne
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Dox dox@space.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wswan.h"
#include "gfx.h"
#include "wswan-memory.h"
#include "sound.h"
#include "eeprom.h"
#include "rtc.h"
#include "v30mz.h"
#include "../mempatcher.h"
#include <time.h>
#include <math.h>

static uint32_t SkipSL; // Skip save and load

uint32_t wsRAMSize;
uint8_t wsRAM[65536];
uint8_t *wsSRAM = NULL;

uint8_t *wsCartROM;
static uint32_t sram_size;
uint32_t eeprom_size;

static uint8_t ButtonWhich, ButtonReadLatch;

static uint32_t DMASource;
static uint16_t DMADest;
static uint16_t DMALength;
static uint8_t DMAControl;

static uint32_t SoundDMASource, SoundDMASourceSaved;
static uint32_t SoundDMALength, SoundDMALengthSaved;
static uint8_t SoundDMAControl;
static uint8_t SoundDMATimer;

static uint8_t BankSelector[4];

static uint8_t CommControl, CommData;

static uint32_t language;

extern uint16_t WSButtonStatus;

void WSwan_writemem20(uint32_t A, uint8_t V)
{
	uint32_t offset = A & 0xffff;
	uint32_t bank = (A>>16) & 0xF;

	/* RAM */
	if (!bank)
	{
		WSwan_SoundCheckRAMWrite(offset);
		wsRAM[offset] = V;

		WSWan_TCacheInvalidByAddr(offset);

		/* WSC palettes */
		if (offset>=0xfe00) 
		WSwan_GfxWSCPaletteRAMWrite(offset, V);
   }
   /* SRAM */
   else if (bank == 1)
   {	 
      if(sram_size)
         wsSRAM[(offset | (BankSelector[1] << 16)) & (sram_size - 1)] = V;
   }
}

uint8_t WSwan_readmem20(uint32_t A)
{
   uint8_t bank_num;
   uint32_t offset = A & 0xFFFF;
   uint32_t bank = (A >> 16) & 0xF;

   switch(bank)
   {
		case 0: 
			return wsRAM[offset];
		case 1:
			if(sram_size)
				return wsSRAM[(offset | (BankSelector[1] << 16)) & (sram_size - 1)];
		return(0);
		case 2:
		case 3:
			return wsCartROM[offset+((BankSelector[bank]&((rom_size>>16)-1))<<16)];
		default: 
		{
			uint8_t bank_num = ((BankSelector[0] & 0xF) << 4) | (bank & 0xf);
			bank_num &= (rom_size >> 16) - 1;
			return(wsCartROM[(bank_num << 16) | offset]);
			uint32_t rom_addr;

			if(bank == 2 || bank == 3)
			{
				rom_addr = offset + ((BankSelector[bank] & ((rom_size >> 16) - 1)) << 16);
			}
			else
			{
				uint8_t bank_num = (((BankSelector[0] & 0xF) << 4) | (bank & 0xf)) & ((rom_size >> 16) - 1);
				rom_addr = (bank_num << 16) | offset; 
			}

			return wsCartROM[rom_addr];
		}
		break;
   }

   bank_num = ((BankSelector[0] & 0xF) << 4) | (bank & 0xf);
   bank_num &= (rom_size >> 16) - 1;

   return(wsCartROM[(bank_num << 16) | offset]);
}

static void ws_CheckDMA(void)
{
	if(DMAControl & 0x80)
	{
		while(DMALength)
		{
			WSwan_writemem20(DMADest, WSwan_readmem20(DMASource));
			WSwan_writemem20(DMADest+1, WSwan_readmem20(DMASource+1));

			if(DMAControl & 0x40)
			{
				DMASource -= 2;
				DMADest -= 2;
			}
			else
			{
				DMASource += 2;
				DMADest += 2;
			}
			DMASource &= 0x000FFFFE;
			DMALength -= 2;
		}
	}
	DMAControl &= ~0x80;
}

void WSwan_CheckSoundDMA(void)
{
	if(!(SoundDMAControl & 0x80))
		return;

	if(!SoundDMATimer)
	{
		uint8_t zebyte = WSwan_readmem20(SoundDMASource);

		if(SoundDMAControl & 0x10)
			WSwan_SoundWrite(0x95, zebyte); // Pick a port, any port?!
		else
			WSwan_SoundWrite(0x89, zebyte);

		if(SoundDMAControl & 0x40)
			SoundDMASource--;
		else
			SoundDMASource++;
		SoundDMASource &= 0x000FFFFF;

		SoundDMALength--;
		SoundDMALength &= 0x000FFFFF;
		if(!SoundDMALength)
		{
			if(SoundDMAControl & 8)
			{
				SoundDMALength = SoundDMALengthSaved;
				SoundDMASource = SoundDMASourceSaved;
			}
			else
			{
				SoundDMAControl &= ~0x80;
			}
		}

		switch(SoundDMAControl & 3)
		{
			case 0: SoundDMATimer = 5; break;
			case 1: SoundDMATimer = 3; break;
			case 2: SoundDMATimer = 1; break;
			case 3: SoundDMATimer = 0; break;
		}
	}
	else
	{
		SoundDMATimer--;
	}
}

uint8_t WSwan_readport(uint32_t number)
{
	number &= 0xFF;

	switch(number)
	{
		//default: printf("Read: %04x\n", number); break;
		case 0x6A:
		case 0x6B:
		case 0x80 ... 0x9F:
			return(WSwan_SoundRead(number));
			
		case 0xA0 ... 0xAF:
		case 0x60:
			return(WSwan_GfxRead(number));
      
		case 0xBA ... 0xBE:
		case 0xC4 ... 0xC8:
			return(WSwan_EEPROMRead(number));

		case 0xCA:
		case 0xCB:
			return(WSwan_RTCRead(number));
		case 0x40: return(DMASource >> 0);
		case 0x41: return(DMASource >> 8);
		case 0x42: return(DMASource >> 16);

		case 0x44: return(DMADest >> 0);
		case 0x45: return(DMADest >> 8);

		case 0x46: return(DMALength >> 0);
		case 0x47: return(DMALength >> 8);

		case 0x48: return(DMAControl);

		case 0xB0:
		case 0xB2:
		case 0xB6: return(WSwan_InterruptRead(number));

		case 0xC0: return(BankSelector[0] | 0x20);
		case 0xC1: return(BankSelector[1]);
		case 0xC2: return(BankSelector[2]);
		case 0xC3: return(BankSelector[3]);

		case 0x4a: return(SoundDMASource >> 0);
		case 0x4b: return(SoundDMASource >> 8);
		case 0x4c: return(SoundDMASource >> 16);
		case 0x4e: return(SoundDMALength >> 0);
		case 0x4f: return(SoundDMALength >> 8);
		case 0x50: return(SoundDMALength >> 16);
		case 0x52: return(SoundDMAControl);

		case 0xB1: return(CommData);

		case 0xb3: 
		{
			uint8_t ret = CommControl & 0xf0;
			if(CommControl & 0x80)
			{
				ret |= 0x4; // Send complete
			}
			return(ret);
		}
		case 0xb5: 
		{
			uint8_t ret = (ButtonWhich << 4) | ButtonReadLatch;
			return(ret);
		}
		default:
		break;
	}
	
	/* This doesn't work well when using Case ranges inside of switch statement so do it after switch */
	if (number <= 0x3F)
	{
		return(WSwan_GfxRead(number));
	}
	else if (number >= 0xC8)
	{
		return(0xD0 | language);
	}

	return(0);
}

void WSwan_writeport(uint32_t IOPort, uint8_t V)
{
	IOPort &= 0xFF;

	switch(IOPort)
	{
      //default: printf("%04x %02x\n", IOPort, V); break;
		case 0x80 ... 0x9F:
		case 0x6A:
		case 0x6B:
			WSwan_SoundWrite(IOPort, V);
		break;
		
		case 0x00 ... 0x3F:
		case 0xA0 ... 0xAF:
		case 0x60:
			WSwan_GfxWrite(IOPort, V);
		break;
		
		case 0xBA ... 0xBE:
		case 0xC4 ... 0xC8:
			WSwan_EEPROMWrite(IOPort, V);
		break;
		
		case 0xCA:
		case 0xCB:
			WSwan_RTCWrite(IOPort, V);
		break;
		case 0x40: DMASource &= 0xFFFF00; DMASource |= (V << 0) & ~1; break;
		case 0x41: DMASource &= 0xFF00FF; DMASource |= (V << 8); break;
		case 0x42: DMASource &= 0x00FFFF; DMASource |= ((V & 0x0F) << 16); break;

		case 0x44: DMADest &= 0xFF00; DMADest |= (V << 0) & ~1; break;
		case 0x45: DMADest &= 0x00FF; DMADest |= (V << 8); break;

		case 0x46: DMALength &= 0xFF00; DMALength |= (V << 0) & ~1; break;
		case 0x47: DMALength &= 0x00FF; DMALength |= (V << 8); break;

		case 0x48:
			DMAControl = V & ~0x3F;
			ws_CheckDMA(); 
		break;

		case 0x4a: SoundDMASource &= 0xFFFF00; SoundDMASource |= (V << 0); SoundDMASourceSaved = SoundDMASource; break;
		case 0x4b: SoundDMASource &= 0xFF00FF; SoundDMASource |= (V << 8); SoundDMASourceSaved = SoundDMASource; break;
		case 0x4c: SoundDMASource &= 0x00FFFF; SoundDMASource |= ((V & 0xF) << 16); SoundDMASourceSaved = SoundDMASource; break;

		case 0x4e: SoundDMALength &= 0xFFFF00; SoundDMALength |= (V << 0); SoundDMALengthSaved = SoundDMALength; break;
		case 0x4f: SoundDMALength &= 0xFF00FF; SoundDMALength |= (V << 8); SoundDMALengthSaved = SoundDMALength; break;
		case 0x50: SoundDMALength &= 0x00FFFF; SoundDMALength |= ((V & 0xF) << 16); SoundDMALengthSaved = SoundDMALength; break;
		case 0x52: SoundDMAControl = V & ~0x20; break;

		case 0xB0:
		case 0xB2:
		case 0xB6: WSwan_InterruptWrite(IOPort, V); break;

		case 0xB1: CommData = V; break;
		case 0xB3: CommControl = V & 0xF0; break;

		case 0xb5:
				ButtonWhich = V >> 4;
				ButtonReadLatch = 0;

				/* Buttons */
                 /*if(ButtonWhich & 0x4) 
                    ButtonReadLatch |= ((WSButtonStatus >> 8) << 1) & 0xF;*/
                    
				/* Removed the << 1 because i couldn't make input with it and this is how Oswan does it. */
				if(ButtonWhich & 0x4) 
                    ButtonReadLatch |= ((WSButtonStatus >> 8)) & 0xF;
                    
				if(ButtonWhich & 0x2) /* H/X cursors */
					ButtonReadLatch |= WSButtonStatus & 0xF;

				if(ButtonWhich & 0x1) /* V/Y cursors */
					ButtonReadLatch |= (WSButtonStatus >> 4) & 0xF;
		break;

		case 0xC0: BankSelector[0] = V & 0xF; break;
		case 0xC1: BankSelector[1] = V; break;
		case 0xC2: BankSelector[2] = V; break;
		case 0xC3: BankSelector[3] = V; break;
	}
}

void WSwan_MemoryKill(void)
{
   if(wsSRAM)
      free(wsSRAM);
   wsSRAM = NULL;
}

void WSwan_MemoryInit(uint32_t lang, uint32_t IsWSC, uint32_t ssize, uint32_t SkipSaveLoad)
{
   const uint16_t byear = MDFN_GetSettingUI("wswan.byear");
   const uint8_t bmonth = MDFN_GetSettingUI("wswan.bmonth");
   const uint8_t bday = MDFN_GetSettingUI("wswan.bday");
   const uint8_t sex = MDFN_GetSettingI("wswan.sex");
   const uint8_t blood = MDFN_GetSettingI("wswan.blood");

   language = lang;
   SkipSL = SkipSaveLoad;

   wsRAMSize = 65536;
   sram_size = ssize;

   // WSwan_EEPROMInit() will also clear wsEEPROM
   WSwan_EEPROMInit(MDFN_GetSettingS("wswan.name"), byear, bmonth, bday, sex, blood);

   if(sram_size)
   {
      wsSRAM = (uint8_t*)malloc(sram_size);
      memset(wsSRAM, 0, sram_size);
   }

   MDFNMP_AddRAM(wsRAMSize, 0x00000, wsRAM);

   if(sram_size)
      MDFNMP_AddRAM(sram_size, 0x10000, wsSRAM);
}

void WSwan_MemoryReset(void)
{
	memset(wsRAM, 0, 65536);

	wsRAM[0x75AC] = 0x41;
	wsRAM[0x75AD] = 0x5F;
	wsRAM[0x75AE] = 0x43;
	wsRAM[0x75AF] = 0x31;
	wsRAM[0x75B0] = 0x6E;
	wsRAM[0x75B1] = 0x5F;
	wsRAM[0x75B2] = 0x63;
	wsRAM[0x75B3] = 0x31;

	memset(BankSelector, 0, sizeof(BankSelector));
	ButtonWhich = 0;
	ButtonReadLatch = 0;
	DMASource = 0;
	DMADest = 0;
	DMALength = 0;
	DMAControl = 0;

	SoundDMASource = SoundDMASourceSaved = 0;
	SoundDMALength = SoundDMALengthSaved = 0;
	SoundDMALength = 0;
	SoundDMAControl = 0;

	CommControl = 0;
	CommData = 0;
}

void WSwan_MemorySaveState(uint32_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&wsRAM, sizeof(uint8_t), sizeof(wsRAM), fp);
		fread(&ButtonWhich, sizeof(uint8_t), sizeof(ButtonWhich), fp);
		fread(&ButtonReadLatch, sizeof(uint8_t), sizeof(ButtonReadLatch), fp);
		fread(&WSButtonStatus, sizeof(uint8_t), sizeof(WSButtonStatus), fp);
		fread(&DMASource, sizeof(uint8_t), sizeof(DMASource), fp);
		fread(&DMADest, sizeof(uint8_t), sizeof(DMADest), fp);
		fread(&DMALength, sizeof(uint8_t), sizeof(DMALength), fp);
		fread(&DMAControl, sizeof(uint8_t), sizeof(DMAControl), fp);
		fread(&SoundDMASource, sizeof(uint8_t), sizeof(SoundDMASource), fp);
		fread(&SoundDMASourceSaved, sizeof(uint8_t), sizeof(SoundDMASourceSaved), fp);
		fread(&SoundDMALength, sizeof(uint8_t), sizeof(SoundDMALength), fp);
		fread(&SoundDMALengthSaved, sizeof(uint8_t), sizeof(SoundDMALengthSaved), fp);
		fread(&SoundDMAControl, sizeof(uint8_t), sizeof(SoundDMAControl), fp);
		fread(&SoundDMATimer, sizeof(uint8_t), sizeof(SoundDMATimer), fp);
		fread(&CommControl, sizeof(uint8_t), sizeof(CommControl), fp);
		fread(&CommData, sizeof(uint8_t), sizeof(CommData), fp);
		fread(&BankSelector, sizeof(uint8_t), sizeof(BankSelector), fp);
		fread(sram_size ? wsSRAM : NULL, sizeof(uint8_t), sram_size, fp);
		
		DMADest &= 0xFFFE;
		DMALength &= 0xFFFE;
		DMASource &= 0x000FFFFE;
		SoundDMASource &= 0x000FFFFF;
		SoundDMASourceSaved &= 0x000FFFFF;
		SoundDMALength &= 0x000FFFFF;
		SoundDMALengthSaved &= 0x000FFFFF;
		
		for(uint32_t A = 0xFE00; A <= 0xFFFF; A++)
		{
			WSwan_GfxWSCPaletteRAMWrite(A, wsRAM[A]);
		}
	}
	else
	{
		fwrite(&wsRAM, sizeof(uint8_t), sizeof(wsRAM), fp);
		fwrite(&ButtonWhich, sizeof(uint8_t), sizeof(ButtonWhich), fp);
		fwrite(&ButtonReadLatch, sizeof(uint8_t), sizeof(ButtonReadLatch), fp);
		fwrite(&WSButtonStatus, sizeof(uint8_t), sizeof(WSButtonStatus), fp);
		fwrite(&DMASource, sizeof(uint8_t), sizeof(DMASource), fp);
		fwrite(&DMADest, sizeof(uint8_t), sizeof(DMADest), fp);
		fwrite(&DMALength, sizeof(uint8_t), sizeof(DMALength), fp);
		fwrite(&DMAControl, sizeof(uint8_t), sizeof(DMAControl), fp);
		fwrite(&SoundDMASource, sizeof(uint8_t), sizeof(SoundDMASource), fp);
		fwrite(&SoundDMASourceSaved, sizeof(uint8_t), sizeof(SoundDMASourceSaved), fp);
		fwrite(&SoundDMALength, sizeof(uint8_t), sizeof(SoundDMALength), fp);
		fwrite(&SoundDMALengthSaved, sizeof(uint8_t), sizeof(SoundDMALengthSaved), fp);
		fwrite(&SoundDMAControl, sizeof(uint8_t), sizeof(SoundDMAControl), fp);
		fwrite(&SoundDMATimer, sizeof(uint8_t), sizeof(SoundDMATimer), fp);
		fwrite(&CommControl, sizeof(uint8_t), sizeof(CommControl), fp);
		fwrite(&CommData, sizeof(uint8_t), sizeof(CommData), fp);
		fwrite(&BankSelector, sizeof(uint8_t), sizeof(BankSelector), fp);
		fwrite(sram_size ? wsSRAM : NULL, sizeof(uint8_t), sram_size, fp);
	}
}
