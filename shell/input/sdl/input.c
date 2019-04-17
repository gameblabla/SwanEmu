#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

extern uint32_t done;

int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	
	SDL_PollEvent(&event);
	uint8_t* keys = SDL_GetKeyState(NULL);
	
	// UP -> Y1
	if (keys[ keys_config[0].buttons[0] ] == SDL_PRESSED)
	{
		button |= (1<<0);
	}
	
	// RIGHT -> Y2
	if (keys[ keys_config[0].buttons[1] ] == SDL_PRESSED)
	{
		button |= (1<<1);
	}
	
	// DOWN -> Y3
	if (keys[ keys_config[0].buttons[2] ] == SDL_PRESSED)
	{
		button |= (1<<2);
	}
	
	// LEFT -> Y4
	if (keys[ keys_config[0].buttons[3] ] == SDL_PRESSED)
	{
		button |= (1<<3);
	}
	
	// UP -> X1
	if (keys[ keys_config[0].buttons[4] ] == SDL_PRESSED)
	{
		button |= (1<<4);
	}
	
	// RIGHT -> X2
	if (keys[ keys_config[0].buttons[5] ] == SDL_PRESSED)
	{
		button |= (1<<5);
	}
	
	// DOWN -> X3
	if (keys[ keys_config[0].buttons[6] ] == SDL_PRESSED)
	{
		button |= (1<<6);
	}
	
	// LEFT -> X4
	if (keys[ keys_config[0].buttons[7] ] == SDL_PRESSED)
	{
		button |= (1<<7);
	}

	// SELECT/OTHER -> OPTION (Wonderswan button)
	if (keys[ keys_config[0].buttons[8] ] == SDL_PRESSED)
	{
		button |= (1<<8);
	}

	// START -> START (Wonderswan button)
	if (keys[ keys_config[0].buttons[9] ] == SDL_PRESSED)
	{
		button |= (1<<9);
	}
	
	// A -> A (Wonderswan button)
	if (keys[ keys_config[0].buttons[10] ] == SDL_PRESSED)
	{
		button |= (1<<10);
	}
	
	// B -> B (Wonderswan button)
	if (keys[ keys_config[0].buttons[11] ] == SDL_PRESSED)
	{
		button |= (1<<11);
	}
	
	
	if ( keys[SDLK_ESCAPE] || keys[SDLK_RCTRL] || keys[SDLK_END] )
	{
		done = 1;
	}
	
	return button;
}

