#ifndef _GIT_H
#define _GIT_H

#include "video.h"

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


	// Current sound volume(0.000...<=volume<=1.000...).  If, after calling Emulate(), it is still != 1, Mednafen will handle it internally.
	// Emulation modules can handle volume themselves if they like, for speed reasons.  If they do, afterwards, they should set its value to 1.
	float SoundVolume;

	// Current sound speed multiplier.  Set by the driver code.  If, after calling Emulate(), it is still != 1, Mednafen will handle it internally
	// by resampling the audio.  This means that emulation modules can handle(and set the value to 1 after handling it) it if they want to get the most
	// performance possible.  HOWEVER, emulation modules must make sure the value is in a range(with minimum and maximum) that their code can handle
	// before they try to handle it.
	float soundmultiplier;

	// True if we want to rewind one frame.  Set by the driver code.
	uint32_t NeedRewind;

	// Sound reversal during state rewinding is normally done in mednafen.cpp, but
        // individual system emulation code can also do it if this is set, and clear it after it's done.
        // (Also, the driver code shouldn't touch this variable)
	uint32_t NeedSoundReverse;

} EmulateSpecStruct;


#endif
