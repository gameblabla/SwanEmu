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
#include <SDL/SDL.h>
#include <sys/time.h>
#include <sys/types.h>
#include "mednafen/mednafen.h"
#include "mednafen/mempatcher.h"
#include "mednafen/wswan/gfx.h"
#include "mednafen/wswan/wswan-memory.h"
#include "mednafen/wswan/start.inc"
#include "mednafen/wswan/sound.h"
#include "mednafen/wswan/interrupt.h"
#include "mednafen/wswan/v30mz.h"
#include "mednafen/wswan/rtc.h"
#include "mednafen/wswan/gfx.h"
#include "mednafen/wswan/eeprom.h"

static uint8_t rotate_tall;
static uint8_t select_pressed_last_frame;

static unsigned rotate_joymap;
static uint32_t done = 0;

static SDL_Surface *screen;

/* Color/Mono */
uint32_t wsc = 1;			
uint16_t WSButtonStatus;
uint32_t rom_size;

#ifdef OSS_SOUND
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
static int32_t oss_audio_fd = -1;
#else
#include <portaudio.h>
PaStream *apu_stream;
#endif

static void Reset(void)
{
	int u0;
	v30mz_reset();				/* Reset CPU */
	WSwan_MemoryReset();
	WSwan_GfxReset();
	WSwan_SoundReset();
	WSwan_InterruptReset();
	WSwan_RTCReset();
	WSwan_EEPROMReset();

	for(u0=0;u0<0xc9;u0++)
	{
		if(u0 != 0xC4 && u0 != 0xC5 && u0 != 0xBA && u0 != 0xBB)
		{
			WSwan_writeport(u0,startio[u0]);
		}
	}

	v30mz_set_reg(NEC_SS,0);
	v30mz_set_reg(NEC_SP,0x2000);
}


static int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	
	SDL_PollEvent(&event);
	uint8_t* keys = SDL_GetKeyState(NULL);
	
	// UP -> Y1
	if (keys[ SDLK_UP ] == SDL_PRESSED)
	{
		button |= (1<<0);
	}
	
	// RIGHT -> Y2
	if (keys[ SDLK_RIGHT ] == SDL_PRESSED)
	{
		button |= (1<<1);
	}
	
	// DOWN -> Y3
	if (keys[ SDLK_DOWN ] == SDL_PRESSED)
	{
		button |= (1<<2);
	}
	
	// LEFT -> Y4
	if (keys[ SDLK_LEFT ] == SDL_PRESSED)
	{
		button |= (1<<3);
	}
	
	// UP -> X1
	if (keys[ SDLK_UP ] == SDL_PRESSED)
	{
		button |= (1<<4);
	}
	
	// RIGHT -> X2
	if (keys[ SDLK_RIGHT ] == SDL_PRESSED)
	{
		button |= (1<<5);
	}
	
	// DOWN -> X3
	if (keys[ SDLK_DOWN ] == SDL_PRESSED)
	{
		button |= (1<<6);
	}
	
	// LEFT -> X4
	if (keys[ SDLK_LEFT ] == SDL_PRESSED)
	{
		button |= (1<<7);
	}

	// SELECT/OTHER -> OPTION (Wonderswan button)
	if (keys[ SDLK_ESCAPE ] == SDL_PRESSED)
	{
		button |= (1<<8);
	}

	// START -> START (Wonderswan button)
	if (keys[ SDLK_RETURN ] == SDL_PRESSED)
	{
		button |= (1<<9);
	}
	
	// A -> A (Wonderswan button)
	if (keys[ SDLK_LCTRL ] == SDL_PRESSED)
	{
		button |= (1<<10);
	}
	
	// B -> B (Wonderswan button)
	if (keys[ SDLK_LALT ] == SDL_PRESSED)
	{
		button |= (1<<11);
	}
	
	
	if ( keys[SDLK_ESCAPE] )
	{
		done = 1;
	}
	
	return button;
}



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
static const uint32_t TblSkip[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1},
};
#endif


static void Emulate(EmulateSpecStruct *espec)
{
	WSButtonStatus = update_input();

#ifdef FRAMESKIP
	SkipCnt++;
	if (SkipCnt > 7) SkipCnt = 0;
	while(!wsExecuteLine(screen->pixels, screen->w, TblSkip[FrameSkip][SkipCnt] ));
#else
	while(!wsExecuteLine(screen->pixels, screen->w, 0 ));
#endif
	
	espec->SoundBufSize = WSwan_SoundFlush(espec->SoundBuf, espec->SoundBufMaxSize);

	espec->MasterCycles = v30mz_timestamp;
	v30mz_timestamp = 0;
}


static uint32_t SRAMSize;


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
    fseek (fp, 0, SEEK_END);
    size = ftell (fp);
    
    /* Invalid Wonderswan ROM, too small */
	if (size < 65536)
	{
		return(0);
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
			SRAMSize =   8 * 1024;
		break;
		case 0x02:
			SRAMSize =  32 * 1024;
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
	for(unsigned int i = 0; i < rom_size - 2; i++)
		real_crc += wsCartROM[i];
	printf("Real Checksum:      0x%04x\n", real_crc);

	/* Detective Conan */
	if ((header[8] | (header[9] << 8)) == 0x8de1 && (header[0]==0x01)&&(header[2]==0x27))
	{
		//puts("HAX");
		/* WS cpu is using cache/pipeline or there's protected ROM bank where pointing CS */
		wsCartROM[0xfffe8]=0xea;
		wsCartROM[0xfffe9]=0x00;
		wsCartROM[0xfffea]=0x00;
		wsCartROM[0xfffeb]=0x00;
		wsCartROM[0xfffec]=0x20;
	}

	if(header[6] & 0x1)
	{
		//MDFNGameInfo->rotated = MDFN_ROTATE90;
	}

	MDFNMP_Init(16384, (1 << 20) / 1024);

	v30mz_init(WSwan_readmem20, WSwan_writemem20, WSwan_readport, WSwan_writeport);
	WSwan_MemoryInit(MDFN_GetSettingB("wswan.language"), wsc, SRAMSize, 0); // EEPROM and SRAM are loaded in this func.
	WSwan_GfxInit();
	//MDFNGameInfo->fps = (uint32)((uint64)3072000 * 65536 * 256 / (159*256));

	WSwan_SoundInit();

	wsMakeTiles();

	Reset();

	return(1);
}

static void CloseGame(void)
{
	WSwan_MemoryKill();

	WSwan_SoundKill();

	if(wsCartROM)
	{
		free(wsCartROM);
		wsCartROM = NULL;
	}
}

static void MDFNI_CloseGame(void)
{
	CloseGame();
	MDFNMP_Kill();
}


void WS_reset(void)
{
	Reset();
}

void Unload_game()
{
	MDFNI_CloseGame();
}


static void Run_Emulator(void)
{
	static int16_t sound_buf[0x10000];

	EmulateSpecStruct spec = {0};
	spec.SoundRate = 44100;
	spec.SoundBuf = sound_buf;
	spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
	spec.SoundBufSize = 0;

	Emulate(&spec);

	int32_t SoundBufSize = spec.SoundBufSize - spec.SoundBufSizeALMS;

	spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

#ifdef OSS_SOUND
	write(oss_audio_fd, spec.SoundBuf, spec.SoundBufSize * 4 );
#else
	Pa_WriteStream( apu_stream, spec.SoundBuf, spec.SoundBufSize);
#endif

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
			if (FrameSkip > 7) FrameSkip = 7;
		}
	}
#endif

}

void *Get_memory_data(unsigned type)
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

size_t Get_memory_size(unsigned type)
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

void EEPROM_file(char* path, uint32_t state)
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

void SaveState(char* path, uint32_t state)
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
		}
	}
}

static uint32_t Sound_Init()
{
#ifdef OSS_SOUND
	uint32_t channels = 2;
	uint32_t format = AFMT_S16_LE;
	uint32_t tmp = 44100;
	int32_t err_ret;

	oss_audio_fd = open("/dev/dsp", O_WRONLY);
	if (oss_audio_fd < 0)
	{
		printf("Couldn't open /dev/dsp.\n");
		return 1;
	}
	
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SPEED,&tmp);
	if (err_ret == -1)
	{
		printf("Could not set sound frequency\n");
		return 1;
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_CHANNELS, &channels);
	if (err_ret == -1)
	{
		printf("Could not set channels\n");
		return 1;
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SETFMT, &format);
	if (err_ret == -1)
	{
		printf("Could not set sound format\n");
		return 1;
	}
	return 0;
#else
	Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		printf("No sound output\n");
		return EXIT_FAILURE;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	Pa_OpenStream( &apu_stream, NULL, &outputParameters, 44100, 1024, paNoFlag, NULL, NULL);
	Pa_StartStream( apu_stream );
#endif
	
	return 1;
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
    
	SDL_Init( SDL_INIT_VIDEO );
	SDL_ShowCursor(0);
	
    screen = SDL_SetVideoMode(224, 144, 16, SDL_HWSURFACE);
	
	Sound_Init();

	isloaded = Load_Game(argv[1]);
	if (!isloaded)
	{
		printf("Could not load ROM in memory\n");
		return 0;
	}
   
	rotate_tall = 0;
	select_pressed_last_frame = 0;
	rotate_joymap = 0;

	WSwan_SetPixelFormat();
	
	done = 0;
    
    // get the game ready
    while (!done)
    {
		Run_Emulator();

		
		SDL_Flip(screen);
    }
    
#ifdef OSS_SOUND
	if (oss_audio_fd >= 0)
	{
		close(oss_audio_fd);
		oss_audio_fd = -1;
	}
#endif
	//Deinit();

    SDL_FreeSurface(screen);
    SDL_Quit();

    return 0;
}

