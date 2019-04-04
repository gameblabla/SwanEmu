#ifndef MEDNAFEN_H
#define MEDNAFEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

#ifdef __GNUC__
#define SWANEMU_COLD __attribute__((cold))
#else
#define SWANEMU_COLD 
#endif

enum
{
	MDFN_ROTATE0 = 0,
	MDFN_ROTATE90,
	MDFN_ROTATE180,
	MDFN_ROTATE270
};

typedef struct
{
	// Skip rendering this frame if true.  Set by the driver code.
	uint32_t skip;

	// Sound rate.  Set by driver side.
	uint32_t SoundRate;

	// Pointer to sound buffer, set by the driver code, that the emulation code should render sound to.
	// Guaranteed to be at least 500ms in length, but emulation code really shouldn't exceed 40ms or so.  Additionally, if emulation code
	// generates >= 100ms, 
	// DEPRECATED: Emulation code may set this pointer to a sound buffer internal to the emulation module.
	int16_t *SoundBuf;

	// Maximum size of the sound buffer, in frames.  Set by the driver code.
	int32_t SoundBufMaxSize;

	// Number of frames currently in internal sound buffer.  Set by the system emulation code, to be read by the driver code.
	int32_t SoundBufSize;
	int32_t SoundBufSizeALMS;	// SoundBufSize value at last MidSync(), 0
				// if mid sync isn't implemented for the emulation module in use.

	// Number of cycles that this frame consumed, using MDFNGI::MasterClock as a time base.
	// Set by emulation code.
	int64_t MasterCycles;

	// True if we want to rewind one frame.  Set by the driver code.
	uint32_t NeedRewind;

	// Sound reversal during state rewinding is normally done in mednafen.cpp, but
        // individual system emulation code can also do it if this is set, and clear it after it's done.
        // (Also, the driver code shouldn't touch this variable)
	uint32_t NeedSoundReverse;

} EmulateSpecStruct;

#endif
