PRGNAME     = swanemu.elf
CC          = gcc

# change compilation / linking flag options

INCLUDES	= -Ishell/input/sdl -Icore -Icore/sound -Ishell/menu -Ishell/video/sdl -Ishell/audio -Ishell/scalers -Ishell/headers
DEFINES		= -DLSB_FIRST -DINLINE="inline" -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565 -DIPU_SCALING_NONATIVE
DEFINES		+= -DOSS_SOUND -DFRAMESKIP -DIPU_SCALE

CFLAGS		= -O0 -g3 -std=gnu99 $(INCLUDES) $(DEFINES)
LDFLAGS     = -lSDL -lm -lasound

# Files to be compiled
SRCDIR 		= ./core ./core/sound ./core/wswan
SRCDIR 		+= ./shell/video/gcw0 ./shell/audio/alsa ./shell/input/sdl ./shell/menu ./shell/emu ./shell/scalers
VPATH		= $(SRCDIR)
SRC_C		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C		= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS		= $(OBJ_C)

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)
	
$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
