#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include "scaler.h"
#include "font_drawing.h"
#include "sound_output.h"
#include "video_blit.h"
#include "config.h"
#include "menu.h"

t_config option;
uint32_t emulator_state = 0;

static char home_path[256], save_path[256], eeprom_path[256], conf_path[256];
static uint32_t controls_chosen = 0;

extern SDL_Surface *sdl_screen;
extern char GameName_emu[512];

extern void EEPROM_file(char* path, uint_fast8_t state);
extern void SaveState(char* path, uint_fast8_t state);

static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;
static const int8_t upscalers_available = 2
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static void SaveState_Menu(uint_fast8_t load_mode, uint_fast8_t slot)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s_%d.sts", save_path, GameName_emu, slot);
	SaveState(tmp,load_mode);
}

static void EEPROM_Menu(uint_fast8_t load_mode)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s.eps", eeprom_path, GameName_emu);
	EEPROM_file(tmp,load_mode);
}

static void config_load()
{
	uint_fast8_t i;
	char config_path[512];
	FILE* fp;
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);

	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
	else
	{
		/* Default mapping for Horizontal */
		option.config_buttons[0][0] = 273;
		option.config_buttons[0][1] = 275;
		option.config_buttons[0][2] = 274;
		option.config_buttons[0][3] = 276;
		
		option.config_buttons[0][4] = 0;
		option.config_buttons[0][5] = 0;
		option.config_buttons[0][6] = 0;
		option.config_buttons[0][7] = 0;
		
		option.config_buttons[0][8] = 1;
		option.config_buttons[0][9] = 13;
				
		option.config_buttons[0][10] = 306;
		option.config_buttons[0][11] = 308;
		
		/* Default mapping for Vertical mode */
		option.config_buttons[1][0] = 306;
		option.config_buttons[1][1] = 308;
		option.config_buttons[1][2] = 304;
		option.config_buttons[1][3] = 32;
		
		option.config_buttons[1][4] = 276;
		option.config_buttons[1][5] = 273;
		option.config_buttons[1][6] = 275;
		option.config_buttons[1][7] = 274;
		
		option.config_buttons[1][8] = 1;
		option.config_buttons[1][9] = 13;
				
		option.config_buttons[1][10] = 0;
		option.config_buttons[1][11] = 0;
	}
}

static void config_save()
{
	FILE* fp;
	char config_path[512];
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);
	
	fp = fopen(config_path, "wb");
	if (fp)
	{
		fwrite(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
}

static const char* Return_Text_Button(uint32_t button)
{
	switch(button)
	{
		/* UP button */
		case 273:
			return "DPAD UP";
		break;
		/* DOWN button */
		case 274:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case 276:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case 275:
			return "DPAD RIGHT";
		break;
		/* A button */
		case 306:
			return "A button";
		break;
		/* B button */
		case 308:
			return "B button";
		break;
		/* X button */
		case 304:
			return "X button";
		break;
		/* Y button */
		case 32:
			return "Y button";
		break;
		/* L button */
		case 9:
			return "L button";
		break;
		/* R button */
		case 8:
			return "R button";
		break;
		/* Power button */
		case 279:
			return "L2 button";
			//return "POWER";
		break;
		/* Brightness */
		case 34:
			return "R2 button";
			//return "Brightness";
		break;
		/* Volume - */
		case 38:
			return "Volume -";
		break;
		/* Volume + */
		case 233:
			return "Volume +";
		break;
		/* Start */
		case 13:
			return "Start button";
		break;
		/* Select */
		case 1:
			return "Select button";
		break;
		default:
			return "...";
		break;
	}	
}

static void Input_Remapping()
{
	SDL_Event Event;
	char text[50];
	uint32_t pressed = 0;
	int32_t currentselection = 1;
	int32_t exit_input = 0;
	uint32_t exit_map = 0;
	
	while(!exit_input)
	{
		pressed = 0;
		SDL_FillRect( backbuffer, NULL, 0 );
		
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection < 1)
                        {
							if (currentselection > 9) currentselection = 12;
							else currentselection = 9;
						}
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 10)
                        {
							currentselection = 1;
						}
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
					break;
                    case SDLK_ESCAPE:
                        option.config_buttons[controls_chosen][currentselection - 1] = 0;
					break;
                    case SDLK_LALT:
                        exit_input = 1;
					break;
                    case SDLK_LEFT:
						if (currentselection > 9) currentselection -= 9;
					break;
                    case SDLK_RIGHT:
						if (currentselection < 10) currentselection += 9;
					break;
                    case SDLK_BACKSPACE:
						controls_chosen = 1;
					break;
                    case SDLK_TAB:
						controls_chosen = 0;
					break;
					default:
					break;
                }
            }
        }

        if (pressed)
        {
            switch(currentselection)
            {
                default:
					SDL_FillRect( backbuffer, NULL, 0 );
					print_string("Please press button for mapping", TextWhite, TextBlue, 37, 108, backbuffer->pixels);
					bitmap_scale(0,0,320,240,sdl_screen->w,sdl_screen->h,320,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
					SDL_Flip(sdl_screen);
					exit_map = 0;
					while( !exit_map )
					{
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								if (Event.key.keysym.sym != SDLK_RCTRL)
								{
									option.config_buttons[controls_chosen][currentselection - 1] = Event.key.keysym.sym;
									exit_map = 1;
								}
							}
						}
					}
				break;
            }
        }
        
        if (currentselection > 12) currentselection = 12;

		if (controls_chosen == 0) print_string("Horizontal mode", TextWhite, 0, 100, 10, backbuffer->pixels);
		else print_string("Vertical mode", TextWhite, 0, 100, 10, backbuffer->pixels);
		
		print_string("Press [A] to map to a button", TextWhite, TextBlue, 50, 210, backbuffer->pixels);
		print_string("Press [B] to Exit", TextWhite, TextBlue, 85, 225, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Y1   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][0]));
		if (currentselection == 1) print_string(text, TextRed, 0, 5, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 25+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Y2   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][1]));
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Y3   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][2]));
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Y4   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][3]));
		if (currentselection == 4) print_string(text, TextRed, 0, 5, 85+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "X1   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][4]));
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 105+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 105+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "X2   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][5]));
		if (currentselection == 6) print_string(text, TextRed, 0, 5, 125+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "X3   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][6]));
		if (currentselection == 7) print_string(text, TextRed, 0, 5, 145+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 145+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "X4   : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][7]));
		if (currentselection == 8) print_string(text, TextRed, 0, 5, 165+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 165+2, backbuffer->pixels);
			
		snprintf(text, sizeof(text), "OPTN : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][8]));
		if (currentselection == 9) print_string(text, TextRed, 0, 5, 185+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 185+2, backbuffer->pixels);
			
		snprintf(text, sizeof(text), "START: %s\n", Return_Text_Button(option.config_buttons[controls_chosen][9]));
		if (currentselection == 10) print_string(text, TextRed, 0, 165, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 25+2, backbuffer->pixels);
			
		snprintf(text, sizeof(text), "A    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][10]));
		if (currentselection == 11) print_string(text, TextRed, 0, 165, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 45+2, backbuffer->pixels);
			
		snprintf(text, sizeof(text), "B    : %s\n", Return_Text_Button(option.config_buttons[controls_chosen][11]));
		if (currentselection == 12) print_string(text, TextRed, 0, 165, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 65+2, backbuffer->pixels);
		
		bitmap_scale(0,0,320,240,sdl_screen->w,sdl_screen->h,320,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
	}
	
	config_save();
}

void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Rect dstRect;
    SDL_Event Event;
    
    Set_Video_Menu();
    
	/* Save eeprom settings each time we bring up the menu */
	EEPROM_Menu(0);
    
    while (((currentselection != 1) && (currentselection != 7)) || (!pressed))
    {
        pressed = 0;
        
        SDL_FillRect( backbuffer, NULL, 0 );

		print_string("SwanEmu - Built on " __DATE__, TextWhite, 0, 5, 15, backbuffer->pixels);
		
		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 45, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 45, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", save_slot);
		
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", save_slot);
		
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);
		
        if (currentselection == 4)
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Keep scaled", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Keep scaled", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }

		
		switch(option.orientation_settings)
		{
			case 0:
				snprintf(text, sizeof(text), "Orientation : Auto");
			break;
			case 1:
				snprintf(text, sizeof(text), "Orientation : Vertical");
			break;
			case 2:
				snprintf(text, sizeof(text), "Orientation : No rotate");
			break;
		}
			
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 125, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125, backbuffer->pixels);
		
		if (currentselection == 6) print_string("Input remapping", TextRed, 0, 5, 145, backbuffer->pixels);
		else print_string("Input remapping", TextWhite, 0, 5, 145, backbuffer->pixels);
		
		if (currentselection == 7) print_string("Quit", TextRed, 0, 5, 165, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 165, backbuffer->pixels);

		print_string("Mednafen Fork by gameblabla", TextWhite, 0, 5, 205, backbuffer->pixels);
		print_string("Credits: Ryphecha, libretro", TextWhite, 0, 5, 225, backbuffer->pixels);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 7;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 8)
                            currentselection = 1;
                        break;
                    case SDLK_END:
                    case SDLK_RCTRL:
                    case SDLK_LALT:
						pressed = 1;
						currentselection = 1;
						break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (save_slot > 0) save_slot--;
							break;
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
							case 5:
								option.orientation_settings--;
								if (option.orientation_settings < 0)
									option.orientation_settings = 2;
							break;
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                save_slot++;
								if (save_slot == 10)
									save_slot = 9;
							break;
                            case 4:
                                option.fullscreen++;
                                if (option.fullscreen > upscalers_available)
                                    option.fullscreen = 0;
							break;
							case 5:
								option.orientation_settings++;
								if (option.orientation_settings > 2)
									option.orientation_settings = 0;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 7;
				pressed = 1;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 6:
					Input_Remapping();
				break;
				case 5:
					option.orientation_settings++;
					if (option.orientation_settings > 2)
						option.orientation_settings = 0;
				break;
                case 4 :
                    option.fullscreen++;
                    if (option.fullscreen > upscalers_available)
                        option.fullscreen = 0;
                    break;
                case 2 :
                    SaveState_Menu(1, save_slot);
					currentselection = 1;
                    break;
                case 3 :
					SaveState_Menu(0, save_slot);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		bitmap_scale(0,0,320,240,sdl_screen->w,sdl_screen->h,320,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
    }
    
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #ifdef SDL_TRIPLEBUF
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #endif
    
    if (currentselection == 7)
    {
        done = 1;
	}
	
	/* Switch back to emulator core */
	emulator_state = 0;
	Set_Video_InGame();
}

static void Cleanup(void)
{
#ifdef SCALE2X_UPSCALER
	if (scale2x_buf) SDL_FreeSurface(scale2x_buf);
#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	if (wswan_vs) SDL_FreeSurface(wswan_vs);

	// Deinitialize audio and video output
	Audio_Close();
	
	SDL_Quit();
}

void Init_Configuration()
{
	snprintf(home_path, sizeof(home_path), "%s/.swanemu", getenv("HOME"));
	
	snprintf(conf_path, sizeof(conf_path), "%s/conf", home_path);
	snprintf(save_path, sizeof(save_path), "%s/sstates", home_path);
	snprintf(eeprom_path, sizeof(eeprom_path), "%s/eeprom", home_path);
	
	/* We check first if folder does not exist. 
	 * Let's only try to create it if so in order to decrease boot times.
	 * */
	
	if (access( home_path, F_OK ) == -1)
	{ 
		mkdir(home_path, 0755);
	}
	
	if (access( save_path, F_OK ) == -1)
	{
		mkdir(save_path, 0755);
	}
	
	if (access( conf_path, F_OK ) == -1)
	{
		mkdir(conf_path, 0755);
	}
	
	if (access( eeprom_path, F_OK ) == -1)
	{
		mkdir(eeprom_path, 0755);
	}
	
	/* Load eeprom file if it exists */
	EEPROM_Menu(1);
	
	config_load();
}
