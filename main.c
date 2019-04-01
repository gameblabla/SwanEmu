#include "mednafen/mednafen.h"
#include "mednafen/mempatcher.h"
#include "mednafen/git.h"

#include "mednafen/wswan/gfx.h"

#include "libretro.h"
//#include <retro_math.h>
#include <SDL/SDL.h>
#include <portaudio.h>

PaStream *apu_stream;

static MDFNGI *game;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static bool overscan;
static double last_sound_rate;

static bool rotate_tall;
static bool select_pressed_last_frame;

static unsigned rotate_joymap;

static MDFN_Surface *surf;

uint32_t done = 0;

char* buf_rom;

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

#include "mednafen/wswan/wswan.h"
#include "mednafen/mempatcher.h"

#include <sys/types.h>
#include <SDL/SDL.h>

#include "mednafen/wswan/gfx.h"
#include "mednafen/wswan/wswan-memory.h"
#include "mednafen/wswan/start.inc"
#include "mednafen/wswan/sound.h"
#include "mednafen/wswan/v30mz.h"
#include "mednafen/wswan/rtc.h"
#include "mednafen/wswan/eeprom.h"

static SDL_Surface *screen;
static uint16_t input_buf;

/* Color/Mono */
int wsc = 1;			
uint32_t rom_size;
uint16_t WSButtonStatus;


static uint8 WSRCurrentSong;

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
         WSwan_writeport(u0,startio[u0]);
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


static void Emulate(EmulateSpecStruct *espec)
{
	if (espec->SoundFormatChanged)
	WSwan_SetSoundRate(espec->SoundRate);

	WSButtonStatus = update_input();
 
	while(!wsExecuteLine(espec->surface, espec->skip))
	{

	}
	
	espec->SoundBufSize = WSwan_SoundFlush(espec->SoundBuf, espec->SoundBufMaxSize);

	espec->MasterCycles = v30mz_timestamp;
	v30mz_timestamp = 0;
}

typedef struct
{
 const uint8_t id;
 const char *name;
} DLEntry;

static const DLEntry Developers[] =
{
	{ 0x01, "Bandai" },
	{ 0x02, "Taito" },
	{ 0x03, "Tomy" },
	{ 0x04, "Koei" },
	{ 0x05, "Data East" },
	{ 0x06, "Asmik" }, // Asmik Ace?
	{ 0x07, "Media Entertainment" },
	{ 0x08, "Nichibutsu" },
	{ 0x0A, "Coconuts Japan" },
	{ 0x0B, "Sammy" },
	{ 0x0C, "Sunsoft" },
	{ 0x0D, "Mebius" },
	{ 0x0E, "Banpresto" },
	{ 0x10, "Jaleco" },
	{ 0x11, "Imagineer" },
	{ 0x12, "Konami" },
	{ 0x16, "Kobunsha" },
	{ 0x17, "Bottom Up" },
	{ 0x18, "Kaga Tech" },
	{ 0x19, "Sunrise" },
	{ 0x1A, "Cyber Front" },
	{ 0x1B, "Mega House" },
	{ 0x1D, "Interbec" },
	{ 0x1E, "Nihon Application" },
	{ 0x1F, "Bandai Visual" },
	{ 0x20, "Athena" },
	{ 0x21, "KID" },
	{ 0x22, "HAL Corporation" },
	{ 0x23, "Yuki Enterprise" },
	{ 0x24, "Omega Micott" },
	{ 0x25, "Layup" },
	{ 0x26, "Kadokawa Shoten" },
	{ 0x27, "Shall Luck" },
	{ 0x28, "Squaresoft" },
	{ 0x2B, "Tom Create" },
	{ 0x2D, "Namco" },
	{ 0x2E, "Movic" }, // ????
	{ 0x2F, "E3 Staff" }, // ????
	{ 0x31, "Vanguard" },
	{ 0x32, "Megatron" },
	{ 0x33, "Wiz" },
	{ 0x36, "Capcom" }
};

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

static int Load(const uint8_t *data, size_t size)
{
	uint32_t pow_size      = 0;
	uint32_t real_rom_size = 0;
	uint8_t header[10];

	if(size < 65536)
		return(0);

	real_rom_size = (size + 0xFFFF) & ~0xFFFF;
	pow_size      = next_pow2(real_rom_size);
	rom_size      = pow_size + (pow_size == 0);

	wsCartROM     = (uint8 *)calloc(1, rom_size);

	/* This real_rom_size vs rom_size funny business is intended primarily for handling WSR files. */
	if(real_rom_size < rom_size)
		memset(wsCartROM, 0xFF, rom_size - real_rom_size);

	memcpy(wsCartROM + (rom_size - real_rom_size), data, size);

	memcpy(header, wsCartROM + rom_size - 10, 10);

	{
		const char *developer_name = "???";
		for(unsigned int x = 0; x < sizeof(Developers) / sizeof(DLEntry); x++)
		{
			if(Developers[x].id == header[0])
			{
				developer_name = Developers[x].name;
				break;
			}
		}
		printf("Developer: %s (0x%02x)\n", developer_name, header[0]);
	}

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
	WSwan_MemoryInit(MDFN_GetSettingB("wswan.language"), wsc, SRAMSize, false); // EEPROM and SRAM are loaded in this func.
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

static const MDFNSetting_EnumList SexList[] =
{
	{ "m", WSWAN_SEX_MALE },
	{ "male", WSWAN_SEX_MALE, "Male" },

	{ "f", WSWAN_SEX_FEMALE },
	{ "female", WSWAN_SEX_FEMALE, "Female" },

	{ "3", 3 },

	{ NULL, 0 },
};

static const MDFNSetting_EnumList BloodList[] =
{
	{ "a", WSWAN_BLOOD_A, "A" },
	{ "b", WSWAN_BLOOD_B, "B" },
	{ "o", WSWAN_BLOOD_O, "O" },
	{ "ab", WSWAN_BLOOD_AB, "AB" },

	{ "5", 5 },

	{ NULL, 0 },
};

static const MDFNSetting_EnumList LanguageList[] =
{
	{ "japanese", 0, "Japanese" },
	{ "0", 0 },

	{ "english", 1, "English" },
	{ "1", 1 },

	{ NULL, 0 },
};

static const MDFNSetting WSwanSettings[] =
{
	{ "wswan.rotateinput", MDFNSF_NOFLAGS, "Virtually rotate D-pads along with screen.", NULL, MDFNST_BOOL, "0" },
	{ "wswan.language", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Language games should display text in.", "The only game this setting is known to affect is \"Digimon Tamers - Battle Spirit\".", MDFNST_ENUM, "english", NULL, NULL, NULL, NULL, LanguageList },
	{ "wswan.name", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Name", NULL, MDFNST_STRING, "Mednafen" },
	{ "wswan.byear", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Birth Year", NULL, MDFNST_UINT, "1989", "0", "9999" },
	{ "wswan.bmonth", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Birth Month", NULL, MDFNST_UINT, "6", "1", "12" },
	{ "wswan.bday", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Birth Day", NULL, MDFNST_UINT, "23", "1", "31" },
	{ "wswan.sex", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Sex", NULL, MDFNST_ENUM, "F", NULL, NULL, NULL, NULL, SexList },
	{ "wswan.blood", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Blood Type", NULL, MDFNST_ENUM, "O", NULL, NULL, NULL, NULL, BloodList },
	{ NULL }
};

static const FileExtensionSpecStruct KnownExtensions[] =
{
	{ ".ws", "WonderSwan ROM Image" },
	{ ".wsc", "WonderSwan Color ROM Image" },
	{ ".wsr", "WonderSwan Music Rip" },
	{ ".pc2", "Benesse Pocket Challenge 2" },
	{ NULL, NULL }
};

MDFNGI EmulatedWSwan =
{
	WSwanSettings,
	MDFN_MASTERCLOCK_FIXED(3072000),
	0,
	false, // Multires possible?

	224,   // lcm_width
	144,   // lcm_height
	NULL,  // Dummy

	224,	// Nominal width
	144,	// Nominal height

	224,	// Framebuffer width
	144,	// Framebuffer height

	2,     // Number of output sound channels
};


#define MEDNAFEN_CORE_NAME_MODULE "wswan"
#define MEDNAFEN_CORE_NAME "Mednafen WonderSwan"
#define MEDNAFEN_CORE_VERSION "v0.9.35.1"
#define MEDNAFEN_CORE_EXTENSIONS "ws|wsc|pc2"
#define MEDNAFEN_CORE_TIMING_FPS 75.47
#define MEDNAFEN_CORE_GEOMETRY_BASE_W (game->nominal_width)
#define MEDNAFEN_CORE_GEOMETRY_BASE_H (game->nominal_height)
#define MEDNAFEN_CORE_GEOMETRY_MAX_W 224
#define MEDNAFEN_CORE_GEOMETRY_MAX_H 144
#define MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO (14.0 / 9.0)
#define FB_WIDTH 224
#define FB_HEIGHT 144

#define FB_MAX_HEIGHT FB_HEIGHT


bool retro_video_refresh_callback(const void *data, unsigned width, unsigned height, size_t pitch)
{
	return true;
}


static uint32_t MDFNI_LoadGame(const char *force_module, const uint8_t *data, size_t size)
{
	if(Load(data, size) <= 0)
		return 0;
	return 1;
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

static void set_volume (uint32_t *ptr, unsigned number)
{
   switch(number)
   {
      default:
         *ptr = number;
         break;
   }
}

static void check_variables(void)
{
   struct retro_variable var = {0};

   var.key = "wswan_rotate_keymap",
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
      if (!strcmp(var.value, "disabled"))
         rotate_joymap = 0;
      else if (!strcmp(var.value, "enabled"))
         rotate_joymap = 1;
      else if (!strcmp(var.value, "auto"))
         rotate_joymap = 2;
   }

}

#define MAX_PLAYERS 1
#define MAX_BUTTONS 11


bool retro_load_game(const struct retro_game_info *info)
{
   if (!info)
      return false;

   overscan = false;
   environ_cb(RETRO_ENVIRONMENT_GET_OVERSCAN, &overscan);

   game = MDFNI_LoadGame(MEDNAFEN_CORE_NAME_MODULE, (const uint8_t*)info->data, info->size);
   if (!game)
   {
	 
		printf("Failed to load game ROM\n");
		return false;
   }

   surf = (MDFN_Surface*)calloc(1, sizeof(*surf));
   
   if (!surf)
      return false;
   
   surf->width  = FB_WIDTH;
   surf->height = FB_HEIGHT;
   surf->pitch  = FB_WIDTH;

   surf->pixels = (uint16_t*)calloc(1, FB_WIDTH * FB_HEIGHT * 2);

   if (!surf->pixels)
   {
      free(surf);
      return false;
   }
   
   rotate_tall = false;
   select_pressed_last_frame = false;
   rotate_joymap = 0;

   check_variables();

   WSwan_SetPixelFormat();

   return true;
}

void retro_unload_game()
{
   if (!game)
      return;

   MDFNI_CloseGame();
}


static uint64_t video_frames, audio_frames;


void retro_run(void)
{
   static int16_t sound_buf[0x10000];
   static MDFN_Rect rects[FB_MAX_HEIGHT];
   rects[0].w = ~0;

   EmulateSpecStruct spec = {0};
   spec.surface = surf;
   spec.SoundRate = 44100;
   spec.SoundBuf = sound_buf;
   spec.LineWidths = rects;
   spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
   spec.SoundVolume = 1.0;
   spec.soundmultiplier = 1.0;
   spec.SoundBufSize = 0;
   spec.VideoFormatChanged = false;
   spec.SoundFormatChanged = false;

   if (spec.SoundRate != last_sound_rate)
   {
      spec.SoundFormatChanged = true;
      last_sound_rate = spec.SoundRate;
   }

   Emulate(&spec);

   int16_t *const SoundBuf = spec.SoundBuf + spec.SoundBufSizeALMS * 2;
   int32_t SoundBufSize = spec.SoundBufSize - spec.SoundBufSizeALMS;
   const int32_t SoundBufMaxSize = spec.SoundBufMaxSize - spec.SoundBufSizeALMS;

   spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

   unsigned width  = spec.DisplayRect.w;
   unsigned height = spec.DisplayRect.h;
   
   video_cb(surf->pixels, width, height, FB_WIDTH << 1);

   video_frames++;
   audio_frames += spec.SoundBufSize;

   audio_batch_cb(spec.SoundBuf, spec.SoundBufSize);
}

void retro_deinit(void)
{
   if (surf)
      free(surf);
   surf = NULL;
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
	struct retro_variable variables[] = 
	{
		{ "wswan_rotate_keymap", "Rotate button mappings; auto|disabled|enabled" },
		{ NULL, NULL },
	};
	cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void *retro_get_memory_data(unsigned type)
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

size_t retro_get_memory_size(unsigned type)
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


int16_t retro_input_state_callback(unsigned port, unsigned device, unsigned index, unsigned id)
{
	if (port != 0)
	{
		return 0;
	}
}

void retro_input_poll_callback()
{

}

size_t retro_audio_sample_batch_callback(const int16_t *data, size_t frames)
{
	PaError err = Pa_WriteStream( apu_stream, data, frames);
	return frames;
}




/***
 * Callback for updating the libretro environment.
 */
int retro_environment_callback(unsigned cmd, void *data)
{
    // TODO: do something with this data...
    // Also, retro_init() doesn't work if I just
    // put a return here, hence the stupid printf.

    return 0;
}

/***
 * Sets up all of our appropriate callback functions with libretro.
 */
void setup_callbacks()
{
    retro_set_environment(&retro_environment_callback);
    retro_set_video_refresh(&retro_video_refresh_callback);
    retro_set_input_poll(&retro_input_poll_callback);
    retro_set_input_state(&retro_input_state_callback);
    retro_set_audio_sample_batch(&retro_audio_sample_batch_callback);
}


static uint32_t Sound_Init()
{
	int32_t err;
	err = Pa_Initialize();
	
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
	
	err = Pa_OpenStream( &apu_stream, NULL, &outputParameters, 44100, 1024, paNoFlag, NULL, NULL);
	err = Pa_StartStream( apu_stream );
	
	return 1;
}

extern void WSwan_GfxSaveState(uint32_t load, FILE* fp);

int main(int argc, char* argv[])
{
	struct retro_game_info game;
	uint32_t start;
	int isloaded;
	FILE* fp;
	uint32_t size_rom;
	
    printf("Starting Oswan\n");
    
    if (argc < 2)
	{
		printf("Specify a ROM to load in memory\n");
		return 0;
	}
    
	SDL_Init( SDL_INIT_VIDEO );
	SDL_ShowCursor(0);
	
    screen = SDL_SetVideoMode(224, 144, 16, SDL_HWSURFACE);

    setup_callbacks();
	
	Sound_Init();

	/* Retroarch core expects the game to be already loaded in memory.
	 * This isn't done by the core itself hence why the following lines 
	 * of codes are needed.
	 * */

    fp = fopen(argv[1], "rb");
    
    fseek (fp, 0, SEEK_END);
    size_rom = ftell (fp);
    
    buf_rom = malloc(size_rom);
    
	fseek (fp, 0, SEEK_SET);
	fread (buf_rom,1,size_rom,fp);
	
    fclose (fp);
    
    game.path = argv[1];
    game.meta = NULL;
    game.data = buf_rom;
    game.size = size_rom;

	isloaded = retro_load_game(&game);
	if (!isloaded)
	{
		printf("Could not load ROM in memory\n");
		return 0;
	}
	
    fp = fopen("klonoa.epm", "rb");
    if (fp)
    {
		fread(retro_get_memory_data(0), sizeof(uint8_t), retro_get_memory_size(0), fp);
		fclose(fp);
	}
    
    fp = fopen("klonoa.sts", "rb");
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
	
	done = 0;
    
    // get the game ready
    while (!done)
    {
		retro_run();

		memcpy(screen->pixels, surf->pixels, (224*144)*2);
		
		/* This needs to be switched to something better on the RS-97 */
		SDL_Flip(screen);
    }
    
    fp = fopen("klonoa.epm", "wb");
	if (fp)
	{
		fwrite(retro_get_memory_data(0), sizeof(uint8_t), retro_get_memory_size(0), fp);
		fclose(fp);
	}
	
    fp = fopen("klonoa.sts", "wb");
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
	//Deinit();

    SDL_FreeSurface(screen);
    SDL_Quit();

    return 0;
}

