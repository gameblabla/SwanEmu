PRGNAME     = swanemu.elf
CC          = clang

# change compilation / linking flag options
F_OPTS		= -DLSB_FIRST -Ilibretro-common/include -Imednafen -Imednafen/sound -DINLINE="inline" -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565
F_OPTS		+= -DMEDNAFEN_VERSION_NUMERIC=931 
CC_OPTS		= -O0 -g3 $(F_OPTS) -Weverything
CFLAGS		= $(CC_OPTS) -std=gnu99
LDFLAGS     = -lSDL -lportaudio -lm

# Files to be compiled
SRCDIR    = ./mednafen ./mednafen/sound ./mednafen/wswan .
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS     = $(OBJ_C)

# Rules to make executable
$(PRGNAME)$(EXESUFFIX): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME)$(EXESUFFIX) $^ $(LDFLAGS)
	
$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
