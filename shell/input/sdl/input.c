#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "mednafen.h"
#include "wswan/gfx.h"

#include "menu.h"
#include "config.h"

#ifdef ENABLE_JOYSTICKCODE
#define joy_commit_range 8192
int32_t axis_input[4] = {0, 0, 0, 0};
uint32_t axis_rotate[4] = {0, 1, 2, 3};
#endif

int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	uint8_t* keys;
	uint32_t isrotated = 0;

	if (Wswan_IsVertical() == 1 || option.orientation_settings > 0)
	{
		isrotated = 1;
#ifdef ENABLE_JOYSTICKCODE
		axis_rotate[0] = 2;
		axis_rotate[1] = 3;
		axis_rotate[2] = 0;
		axis_rotate[3] = 1;
#endif
	}
#ifdef ENABLE_JOYSTICKCODE
	else
	{
		axis_rotate[0] = 0;
		axis_rotate[1] = 1;
		axis_rotate[2] = 2;
		axis_rotate[3] = 3;
	}
#endif
	keys = SDL_GetKeyState(NULL);
	
	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case SDLK_END:
					case SDLK_RCTRL:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			case SDL_KEYUP:
				switch(event.key.keysym.sym)
				{
					case SDLK_HOME:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			#ifdef ENABLE_JOYSTICKCODE
			case SDL_JOYAXISMOTION:
				if (event.jaxis.axis < 4)
				axis_input[event.jaxis.axis] = event.jaxis.value;
			break;
			#endif
		}
	}
	
	#ifdef GKD350H
	if (keys[SDLK_RETURN] && keys[SDLK_ESCAPE]) emulator_state = 1;
	#endif

	// UP -> Y1
	if (keys[option.config_buttons[isrotated][0] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[1]] < -joy_commit_range
#endif
	)
	{
		button |= (1<<0);
	}
	
	// RIGHT -> Y2
	if (keys[option.config_buttons[isrotated][1] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[0]] > joy_commit_range
#endif
	)
	{
		button |= (1<<1);
	}
	
	// DOWN -> Y3
	if (keys[option.config_buttons[isrotated][2] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[1]] > joy_commit_range
#endif
	)
	{
		button |= (1<<2);
	}
	
	// LEFT -> Y4
	if (keys[option.config_buttons[isrotated][3] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[0]] < -joy_commit_range
#endif
	)
	{
		button |= (1<<3);
	}
	
	// UP -> X1
	if (keys[option.config_buttons[isrotated][4] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[3]] < -joy_commit_range
#endif
	)
	{
		button |= (1<<4);
	}
	
	// RIGHT -> X2
	if (keys[option.config_buttons[isrotated][5] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[2]] > joy_commit_range
#endif
	)
	{
		button |= (1<<5);
	}
	
	// DOWN -> X3
	if (keys[option.config_buttons[isrotated][6] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[3]] > joy_commit_range
#endif
	)
	{
		button |= (1<<6);
	}
	
	// LEFT -> X4
	if (keys[option.config_buttons[isrotated][7] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[axis_rotate[2]] < -joy_commit_range
#endif
	)
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

