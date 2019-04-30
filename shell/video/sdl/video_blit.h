#ifndef VIDEO_BLIT_H
#define VIDEO_BLIT_H

#include <SDL/SDL.h>

#define HOST_WIDTH_RESOLUTION 320
#define HOST_HEIGHT_RESOLUTION 240

#define INTERNAL_WSWAN_WIDTH 224
#define INTERNAL_WSWAN_HEIGHT 144

extern SDL_Surface *screen, *wswan_vs, *backbuffer;

extern uint32_t width_of_surface;
extern uint32_t* Draw_to_Virtual_Screen;

void Init_Video();
void Set_Video_Menu();
void Set_Video_InGame();
void Close_Video();
void Update_Video_Menu();
void Update_Video_Ingame();

#endif
