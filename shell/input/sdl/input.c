#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "mednafen.h"
#include "wswan/gfx.h"

#include "menu.h"
#include "config.h"

int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	uint8_t* keys;
	uint32_t isrotated = 0;

	if (Wswan_IsVertical() == 1|| option.orientation_settings > 0)
	{
		isrotated = 1;
	}
	
	keys = SDL_GetKeyState(NULL);
	
	SDL_PollEvent(&event);
	
	if (keys[SDLK_RETURN] == SDL_PRESSED && keys[SDLK_ESCAPE] == SDL_PRESSED) emulator_state = 1;

	// UP -> Y1
	if (keys[option.config_buttons[isrotated][0] ] == SDL_PRESSED)
	{
		button |= (1<<0);
	}
	
	// RIGHT -> Y2
	if (keys[option.config_buttons[isrotated][1] ] == SDL_PRESSED)
	{
		button |= (1<<1);
	}
	
	// DOWN -> Y3
	if (keys[option.config_buttons[isrotated][2] ] == SDL_PRESSED)
	{
		button |= (1<<2);
	}
	
	// LEFT -> Y4
	if (keys[option.config_buttons[isrotated][3] ] == SDL_PRESSED)
	{
		button |= (1<<3);
	}
	
	// UP -> X1
	if (keys[option.config_buttons[isrotated][4] ] == SDL_PRESSED)
	{
		button |= (1<<4);
	}
	
	// RIGHT -> X2
	if (keys[option.config_buttons[isrotated][5] ] == SDL_PRESSED)
	{
		button |= (1<<5);
	}
	
	// DOWN -> X3
	if (keys[option.config_buttons[isrotated][6] ] == SDL_PRESSED)
	{
		button |= (1<<6);
	}
	
	// LEFT -> X4
	if (keys[option.config_buttons[isrotated][7] ] == SDL_PRESSED)
	{
		button |= (1<<7);
	}

	// SELECT/OTHER -> OPTION (Wonderswan button)
	if (keys[option.config_buttons[isrotated][8] ] == SDL_PRESSED)
	{
		button |= (1<<8);
	}

	// START -> START (Wonderswan button)
	if (keys[option.config_buttons[isrotated][9] ] == SDL_PRESSED)
	{
		button |= (1<<9);
	}
	
	// A -> A (Wonderswan button)
	if (keys[option.config_buttons[isrotated][10] ] == SDL_PRESSED)
	{
		button |= (1<<10);
	}
	
	// B -> B (Wonderswan button)
	if (keys[option.config_buttons[isrotated][11] ] == SDL_PRESSED)
	{
		button |= (1<<11);
	}
	
	return button;
}

