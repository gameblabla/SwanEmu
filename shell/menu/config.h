#ifndef CONFIG_H__
#define CONFIG_H__

#define HORIZONTAL_CONTROLS 0
#define VERTICAL_CONTROLS 1

typedef struct {
	int32_t fullscreen;
	/* For input remapping */
	uint32_t config_buttons[2][19];
	int32_t orientation_settings;
} t_config;
extern t_config option;

#endif
