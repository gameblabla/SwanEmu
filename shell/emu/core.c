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
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/types.h>

#include <SDL/SDL.h>

#include "mednafen.h"
#include "mempatcher.h"
#include "wswan/gfx.h"
#include "wswan/wswan-memory.h"
#include "wswan/start.inc"
#include "wswan/sound.h"
#include "wswan/interrupt.h"
#include "wswan/v30mz.h"
#include "wswan/rtc.h"
#include "wswan/gfx.h"
#include "wswan/eeprom.h"

#include "sound_output.h"
#include "video_blit.h"
#include "input.h"
#include "menu.h"
#include "config.h"
#include "shared.h"

static uint8_t rotate_tall, select_pressed_last_frame;
static uint32_t rotate_joymap, SRAMSize;

char GameName_emu[512];

/* Color/Mono */		
uint16_t WSButtonStatus;
uint32_t rom_size, done = 0, wsc = 1;

#ifdef FRAMESKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t SkipCnt = 0, video_frames = 0, FPS = 75, FrameSkip;
static const uint32_t TblSkip[5][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 1, 1},
    {0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1},
};
#endif

static void Reset(void)
{
	uint_fast8_t u0;
	/* Reset CPU */
	v30mz_reset();
	WSwan_MemoryReset();
	WSwan_GfxReset();
	WSwan_SoundReset();
	WSwan_InterruptReset();
	WSwan_RTCReset();
	WSwan_EEPROMReset();

	for (u0=0;u0<0xc9;u0++)
	{
		if (u0 != 0xC4 && u0 != 0xC5 && u0 != 0xBA && u0 != 0xBB)
		{
			WSwan_writeport(u0,startio[u0]);
		}
	}

	v30mz_set_reg(NEC_SS,0);
	v30mz_set_reg(NEC_SP,0x2000);
}

static void Emulate(EmulateSpecStruct *espec)
{
	WSButtonStatus = update_input();
	
	LOCK_VIDEO

#ifdef FRAMESKIP
	SkipCnt++;
	if (SkipCnt > 4) SkipCnt = 0;
	while(!wsExecuteLine((uint16_t* restrict)Draw_to_Virtual_Screen, width_of_surface, TblSkip[FrameSkip][SkipCnt] ));
#else
	while(!wsExecuteLine((uint16_t* restrict)Draw_to_Virtual_Screen, width_of_surface, 0 ));
#endif

	UNLOCK_VIDEO
	
	espec->SoundBufSize = WSwan_SoundFlush(espec->SoundBuf, espec->SoundBufMaxSize);

	espec->MasterCycles = v30mz_timestamp;
	v30mz_timestamp = 0;
}

static INLINE uint32_t next_pow2(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

static uint32_t Load_Game(char* path)
{
	FILE* fp;
	uint32_t pow_size      = 0;
	uint32_t real_rom_size = 0;
	uint8_t header[10];
	
	uint32_t size;
	
	fp = fopen(path, "rb");
	if (!fp) return 0;
	
    fseek (fp, 0, SEEK_END);
    size = ftell (fp);
    
    /* Invalid Wonderswan ROM, too small */
	if (size < 65536)
	{
		return 0;
	}
	
	fseek (fp, 0, SEEK_SET);

	real_rom_size = (size + 0xFFFF) & ~0xFFFF;
	pow_size      = next_pow2(real_rom_size);
	rom_size      = pow_size + (pow_size == 0);

	wsCartROM     = (uint8_t *)calloc(1, size);
	
	/* This real_rom_size vs rom_size funny business is intended primarily for handling WSR files. */
	if(real_rom_size < rom_size)
		memset(wsCartROM, 0xFF, rom_size - real_rom_size);

	fread (wsCartROM, sizeof(uint8_t), size, fp);
	
	fclose (fp);
	
	memcpy(header, wsCartROM + rom_size - 10, 10);

	SRAMSize = 0;
	eeprom_size = 0;

	switch(header[5])
	{
		case 0x01:
			SRAMSize = 8 * 1024;
		break;
		case 0x02:
			SRAMSize = 32 * 1024;
		break;
		case 0x03:
			SRAMSize = 128 * 1024;
		break;
		// Dicing Knight!, Judgement Silver
		case 0x04:
			SRAMSize = 256 * 1024;
		break;
		// Wonder Gate
		case 0x05:
			SRAMSize = 512 * 1024;
		break; 
		case 0x10:
			eeprom_size = 128;
		break;
		case 0x20:
			eeprom_size = 2 *1024; 
		break;
		case 0x50:
			eeprom_size = 1024;
		break;
	}

	uint16_t real_crc = 0;
	for (uint_fast32_t i = 0; i < rom_size - 2; i++)
	{
		real_crc += wsCartROM[i];
	}
	printf("Real Checksum:      0x%04x\n", real_crc);

	/* Detective Conan (Hack due to lack of prefetch) */
	if ((header[8] | (header[9] << 8)) == 0x8de1 && (header[0]==0x01)&&(header[2]==0x27))
	{
		/* WS cpu is using cache/pipeline or there's protected ROM bank where pointing CS */
		wsCartROM[0xfffe8] = 0xea;
		wsCartROM[0xfffe9] = 0x00;
		wsCartROM[0xfffea] = 0x00;
		wsCartROM[0xfffeb] = 0x00;
		wsCartROM[0xfffec] = 0x20;
	}
	/* Digimon Battle Spirits 1.5 has a hidden english translation that is disabled.
	 * This enables it back. */
	else if (real_crc == 0x443e)
	{
		const uint32_t table_topatch[29] =
		{
			0x75077B, 0x755956, 0x75A716, 0x75A734,
			0x75A74E, 0x76DAEC, 0x7700E7, 0x770A57,
			0x77120A, 0x772150, 0x77567B, 0x775818,
			0x77708F, 0x77732A, 0x7773DA, 0x777467,
			0x7788A9, 0x779F27, 0x77E489, 0x77FF59,
			0x780048, 0x780075, 0x78008F, 0x78014A,
			0x780DFB, 0x78280E, 0x786204, 0x786292,
			0x786339
		};
		for(uint32_t i=0;i<sizeof(table_topatch)/sizeof(uint32_t);i++)
		{
			wsCartROM[table_topatch[i]] = 0xB0;
			wsCartROM[table_topatch[i]+0x000001] = 0x01;
		}
	}

	if (header[6] & 0x1)
	{
		/* Force rotate for Game's config file if header says game is portrait-only */
		option.orientation_settings = 1;
	}

	MDFNMP_Init(16384, (1 << 20) / 1024);

	v30mz_init(WSwan_readmem20, WSwan_writemem20, WSwan_readport, WSwan_writeport);
	WSwan_MemoryInit(MDFN_GetSettingB("wswan.language"), wsc, SRAMSize, 0); // EEPROM and SRAM are loaded in this func.
	WSwan_GfxInit();
	//MDFNGameInfo->fps = (uint32)((uint64)3072000 * 65536 * 256 / (159*256));

	WSwan_SoundInit();

	wsMakeTiles();

	Reset();

	return 1;
}

void Unload_game()
{
	WSwan_MemoryKill();
	WSwan_SoundKill();

	if (wsCartROM)
	{
		free(wsCartROM);
		wsCartROM = NULL;
	}
	MDFNMP_Kill();
}

void WS_reset(void)
{
	Reset();
}

static void Run_Emulator(void)
{
	static int16_t sound_buf[0x10000];

	EmulateSpecStruct spec = {0};
	spec.SoundRate = SOUND_OUTPUT_FREQUENCY;
	spec.SoundBuf = sound_buf;
	spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
	spec.SoundBufSize = 0;

	Emulate(&spec);

	int32_t SoundBufSize = spec.SoundBufSize - spec.SoundBufSizeALMS;

	spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

	Audio_Write(spec.SoundBuf, spec.SoundBufSize);

#ifdef FRAMESKIP
	video_frames++;
	newTick = Timer_Read();
	if ( (newTick) - (lastTick) > 1000000) 
	{
		FPS = video_frames;
		video_frames = 0;
		lastTick = newTick;
		if (FPS >= 75)
		{
			FrameSkip = 0;
		}
		else
		{
			FrameSkip = 75 / FPS;
			if (FrameSkip > 4) FrameSkip = 4;
		}
	}
#endif

}

void *Get_memory_data(size_t type)
{
   switch (type)
   {
      case 0:
         if (eeprom_size)
            return (uint8_t*)wsEEPROM;
         else if (SRAMSize)
            return wsSRAM;
         else
            return NULL;
      case 1:
         return (uint8_t*)wsRAM;
      default:
         break;
   }

   return NULL;
}

size_t Get_memory_size(size_t type)
{
   switch (type)
   {
      case 0:
         if (eeprom_size)
            return eeprom_size;
         else if (SRAMSize)
            return SRAMSize;
         else
            return 0;
      case 1:
         return wsRAMSize;
      default:
         break;
   }

   return 0;
}

void EEPROM_file(char* path, uint_fast8_t state)
{
	FILE* fp;
	if (state == 1)
	{
		fp = fopen(path, "rb");
		if (fp)
		{
			fread(Get_memory_data(0), sizeof(uint8_t), Get_memory_size(0), fp);
			fclose(fp);
		}
	}
	else
	{
		fp = fopen(path, "wb");
		if (fp)
		{
			fwrite(Get_memory_data(0), sizeof(uint8_t), Get_memory_size(0), fp);
			fclose(fp);
		}
	}
}

void SaveState(char* path, uint_fast8_t state)
{
	FILE* fp;
	if (state == 1)
	{
		fp = fopen(path, "rb");
		if (fp)
		{
			WSwan_v30mzSaveState(1, fp);
			WSwan_MemorySaveState(1, fp);
			WSwan_GfxSaveState(1, fp);
			WSwan_RTCSaveState(1, fp);
			WSwan_InterruptSaveState(1, fp);
			WSwan_SoundSaveState(1, fp);
			WSwan_EEPROMSaveState(1, fp);
			fclose(fp);
		}
	}
	else
	{
		fp = fopen(path, "wb");
		if (fp)
		{
			WSwan_v30mzSaveState(0, fp);
			WSwan_MemorySaveState(0, fp);
			WSwan_GfxSaveState(0, fp);
			WSwan_RTCSaveState(0, fp);
			WSwan_InterruptSaveState(0, fp);
			WSwan_SoundSaveState(0, fp);
			WSwan_EEPROMSaveState(0, fp);
			fclose(fp);
		}
	}
}

int main(int argc, char* argv[])
{
	uint32_t isloaded;
	
    printf("Starting Oswan\n");
    
    if (argc < 2)
	{
		printf("Specify a ROM to load in memory\n");
		return 0;
	}

	snprintf(GameName_emu, sizeof(GameName_emu), "%s", basename(argv[1]));
	
	Init_Video();
	Audio_Init();

	isloaded = Load_Game(argv[1]);
	if (!isloaded)
	{
		printf("Could not load ROM in memory\n");
		return 0;
	}
	
	/* Init_Configuration also takes care of EEPROM saves so execute it after the game has been loaded in memory. */
	Init_Configuration();
   
	rotate_tall = 0;
	select_pressed_last_frame = 0;
	rotate_joymap = 0;

	WSwan_SetPixelFormat();
	
	done = 0;
    
    // get the game ready
    while (!done)
    {
		switch(emulator_state)
		{
			case 0:
				Run_Emulator();
				Update_Video_Ingame();
			break;
			case 1:
				Menu();
			break;
		}
    }
    
    Audio_Close();
	Close_Video();

    return 0;
}

