#ifndef _GIT_H
#define _GIT_H

#include "video.h"

typedef struct
{
 const char *extension; // Example ".nes"
 const char *description; // Example "iNES Format ROM Image"
} FileExtensionSpecStruct;

enum
{
 MDFN_ROTATE0 = 0,
 MDFN_ROTATE90,
 MDFN_ROTATE180,
 MDFN_ROTATE270
};

typedef enum
{
 VIDSYS_NONE, // Can be used internally in system emulation code, but it is an error condition to let it continue to be
	      // after the Load() or LoadCD() function returns!
 VIDSYS_PAL,
 VIDSYS_PAL_M, // Same timing as NTSC, but uses PAL-style colour encoding
 VIDSYS_NTSC,
 VIDSYS_SECAM
} VideoSystems;

typedef enum
{
 GMT_CART,	// Self-explanatory!
 GMT_ARCADE,	// VS Unisystem, PC-10...
 GMT_DISK,	// Famicom Disk System, mostly
 GMT_CDROM,	// PC Engine CD, PC-FX
 GMT_PLAYER	// Music player(NSF, HES, GSF)
} GameMediumTypes;

#include "settings-common.h"

typedef struct
{
	// Pitch(32-bit) must be equal to width and >= the "fb_width" specified in the MDFNGI struct for the emulated system.
	// Height must be >= to the "fb_height" specified in the MDFNGI struct for the emulated system.
	// The framebuffer pointed to by surface->pixels is written to by the system emulation code.
	MDFN_Surface *surface;

	// Will be set to TRUE if the video pixel format has changed since the last call to Emulate(), FALSE otherwise.
	// Will be set to TRUE on the first call to the Emulate() function/method
	uint32_t VideoFormatChanged;

	// Set by the system emulation code every frame, to denote the horizontal and vertical offsets of the image, and the size
	// of the image.  If the emulated system sets the elements of LineWidths, then the horizontal offset(x) and width(w) of this structure
	// are ignored while drawing the image.
	MDFN_Rect DisplayRect;

	// Pointer to an array of MDFN_Rect, number of elements = fb_height, set by the driver code.  Individual MDFN_Rect structs written
	// to by system emulation code.  If the emulated system doesn't support multiple screen widths per frame, or if you handle
	// such a situation by outputting at a constant width-per-frame that is the least-common-multiple of the screen widths, then
	// you can ignore this.  If you do wish to use this, you must set all elements every frame.
	MDFN_Rect *LineWidths;

	// TODO
	uint32_t *IsFMV;

	// Set(optionally) by emulation code.  If InterlaceOn is true, then assume field height is 1/2 DisplayRect.h, and
	// only every other line in surface (with the start line defined by InterlacedField) has valid data
	// (it's up to internal Mednafen code to deinterlace it).
	uint32_t InterlaceOn;
	uint32_t InterlaceField;

	// Skip rendering this frame if true.  Set by the driver code.
	int skip;

	//
	// If sound is disabled, the driver code must set SoundRate to false, SoundBuf to NULL, SoundBufMaxSize to 0.

        // Will be set to TRUE if the sound format(only rate for now, at least) has changed since the last call to Emulate(), FALSE otherwise.
        // Will be set to TRUE on the first call to the Emulate() function/method
	uint32_t SoundFormatChanged;

	// Sound rate.  Set by driver side.
	double SoundRate;

	// Pointer to sound buffer, set by the driver code, that the emulation code should render sound to.
	// Guaranteed to be at least 500ms in length, but emulation code really shouldn't exceed 40ms or so.  Additionally, if emulation code
	// generates >= 100ms, 
	// DEPRECATED: Emulation code may set this pointer to a sound buffer internal to the emulation module.
	int16 *SoundBuf;

	// Maximum size of the sound buffer, in frames.  Set by the driver code.
	int32 SoundBufMaxSize;

	// Number of frames currently in internal sound buffer.  Set by the system emulation code, to be read by the driver code.
	int32 SoundBufSize;
	int32 SoundBufSizeALMS;	// SoundBufSize value at last MidSync(), 0
				// if mid sync isn't implemented for the emulation module in use.

	// Number of cycles that this frame consumed, using MDFNGI::MasterClock as a time base.
	// Set by emulation code.
	int64 MasterCycles;
	int64 MasterCyclesALMS;	// MasterCycles value at last MidSync(), 0
				// if mid sync isn't implemented for the emulation module in use.

	// Current sound volume(0.000...<=volume<=1.000...).  If, after calling Emulate(), it is still != 1, Mednafen will handle it internally.
	// Emulation modules can handle volume themselves if they like, for speed reasons.  If they do, afterwards, they should set its value to 1.
	double SoundVolume;

	// Current sound speed multiplier.  Set by the driver code.  If, after calling Emulate(), it is still != 1, Mednafen will handle it internally
	// by resampling the audio.  This means that emulation modules can handle(and set the value to 1 after handling it) it if they want to get the most
	// performance possible.  HOWEVER, emulation modules must make sure the value is in a range(with minimum and maximum) that their code can handle
	// before they try to handle it.
	double soundmultiplier;

	// True if we want to rewind one frame.  Set by the driver code.
	uint32_t NeedRewind;

	// Sound reversal during state rewinding is normally done in mednafen.cpp, but
        // individual system emulation code can also do it if this is set, and clear it after it's done.
        // (Also, the driver code shouldn't touch this variable)
	uint32_t NeedSoundReverse;

} EmulateSpecStruct;

typedef enum
{
 MODPRIO_INTERNAL_EXTRA_LOW = 0,	// For "cdplay" module, mostly.

 MODPRIO_INTERNAL_LOW = 10,
 MODPRIO_EXTERNAL_LOW = 20,
 MODPRIO_INTERNAL_HIGH = 30,
 MODPRIO_EXTERNAL_HIGH = 40
} ModPrio;

//class CDIF;

#define MDFN_MASTERCLOCK_FIXED(n)	((int64)((double)(n) * (1LL << 32)))

typedef struct
{
 const MDFNSetting *Settings;

 // Time base for EmulateSpecStruct::MasterCycles
 int64 MasterClock;

 uint32 fps; // frames per second * 65536 * 256, truncated

 // multires is a hint that, if set, indicates that the system has fairly programmable video modes(particularly, the ability
 // to display multiple horizontal resolutions, such as the PCE, PC-FX, or Genesis).  In practice, it will cause the driver
 // code to set the linear interpolation on by default.
 //
 // lcm_width and lcm_height are the least common multiples of all possible
 // resolutions in the frame buffer as specified by DisplayRect/LineWidths(Ex for PCE: widths of 256, 341.333333, 512,
 // lcm = 1024)
 //
 // nominal_width and nominal_height specify the resolution that Mednafen should display
 // the framebuffer image in at 1x scaling, scaled from the dimensions of DisplayRect, and optionally the LineWidths array
 // passed through espec to the Emulate() function.
 //
 uint32_t multires;

 int lcm_width;
 int lcm_height;

 void *dummy_separator;	//

 int nominal_width;
 int nominal_height;

 int fb_width;		// Width of the framebuffer(not necessarily width of the image).  MDFN_Surface width should be >= this.
 int fb_height;		// Height of the framebuffer passed to the Emulate() function(not necessarily height of the image)

 int soundchan; 	// Number of output sound channels.


 int rotated;

 int soundrate;  /* For Ogg Vorbis expansion sound wacky support.  0 for default. */

 VideoSystems VideoSystem;
 GameMediumTypes GameType;

 //int DiskLogicalCount;	// A single double-sided disk would be 2 here.
 //const char *DiskNames;	// Null-terminated.

 const char *cspecial;  /* Special cart expansion: DIP switches, barcode reader, etc. */

 double mouse_sensitivity;
} MDFNGI;

#endif
