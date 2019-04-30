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
#include "mednafen.h"
#include "mempatcher.h"
#include "wswan/gfx.h"
#include "wswan/wswan-memory.h"
#include "wswan/sound.h"
#include "wswan/interrupt.h"
#include "wswan/v30mz.h"
#include "wswan/rtc.h"
#include "wswan/gfx.h"
#include "wswan/eeprom.h"

#include "video_blit.h"
#include "scaler.h"
#include "config.h"

SDL_Surface *sdl_screen, *backbuffer, *wswan_vs;

uint32_t width_of_surface;
uint32_t* Draw_to_Virtual_Screen;

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(0, 0, 16, SDL_HWSURFACE);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	wswan_vs = SDL_CreateRGBSurface(SDL_SWSURFACE, INTERNAL_WSWAN_WIDTH, INTERNAL_WSWAN_HEIGHT, 16, 0,0,0,0);

	Set_Video_InGame();
}

void Set_Video_Menu()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		memcpy(wswan_vs->pixels, sdl_screen->pixels, (INTERNAL_WSWAN_WIDTH * INTERNAL_WSWAN_HEIGHT)*2);
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	}
}

void Set_Video_InGame()
{
	switch(option.fullscreen) 
	{
		// Native
		#ifdef SUPPORT_NATIVE_RESOLUTION
        case 0:
			/* For drawing to Wonderswan screen */
			if (sdl_screen->w != INTERNAL_WSWAN_WIDTH) sdl_screen = SDL_SetVideoMode(224, 144, 16, SDL_HWSURFACE);
			Draw_to_Virtual_Screen = sdl_screen->pixels;
			width_of_surface = sdl_screen->w;
        break;
        #endif
        default:
			if (sdl_screen->w != HOST_WIDTH_RESOLUTION) sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
			Draw_to_Virtual_Screen = wswan_vs->pixels;
			width_of_surface = INTERNAL_WSWAN_WIDTH;
        break;
    }
}

void Close_Video()
{
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	if (wswan_vs) SDL_FreeSurface(wswan_vs);
	SDL_Quit();
}

void Update_Video_Menu()
{
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame()
{
#ifndef SUPPORT_NATIVE_RESOLUTION
	uint32_t y, pitch;
	uint16_t *src, *dst;
#endif
	SDL_Rect pos;
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen) 
	{
		#ifndef SUPPORT_NATIVE_RESOLUTION
		case 0:
		pitch = HOST_WIDTH_RESOLUTION;
		src = (uint16_t* restrict)wswan_vs->pixels;
		dst = (uint16_t* restrict)sdl_screen->pixels
			+ ((HOST_WIDTH_RESOLUTION - INTERNAL_WSWAN_WIDTH) / 4) * sizeof(uint16_t)
			+ ((HOST_HEIGHT_RESOLUTION - INTERNAL_WSWAN_HEIGHT) / 2) * pitch;
		for (y = 0; y < INTERNAL_WSWAN_HEIGHT; y++)
		{
			memmove(dst, src, INTERNAL_WSWAN_WIDTH * sizeof(uint16_t));
			src += INTERNAL_WSWAN_WIDTH;
			dst += pitch;
		}
		break;
		#endif
		// Fullscreen
		case 1:
			bitmap_scale(0, 0, INTERNAL_WSWAN_WIDTH, INTERNAL_WSWAN_HEIGHT, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, INTERNAL_WSWAN_WIDTH, 0, (uint16_t* restrict)wswan_vs->pixels, (uint16_t* restrict)sdl_screen->pixels);
		break;
		case 2:
			bitmap_scale(0,0,INTERNAL_WSWAN_WIDTH,INTERNAL_WSWAN_HEIGHT,320,206,INTERNAL_WSWAN_WIDTH,0,(uint16_t* restrict)wswan_vs->pixels,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-320)/2+(HOST_HEIGHT_RESOLUTION-206)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Hqx
		case 3:
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}
