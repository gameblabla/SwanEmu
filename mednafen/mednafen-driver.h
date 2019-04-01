#ifndef __MDFN_MEDNAFEN_DRIVER_H
#define __MDFN_MEDNAFEN_DRIVER_H

#include <stdio.h>

#include "settings-common.h"

/* Indent stdout newlines +- "indent" amount */
void MDFN_indent(int indent);

uint32 MDFND_GetTime(void);
void MDFND_Sleep(uint32 ms);

// Call this function as early as possible, even before MDFNI_Initialize()
uint32_t MDFNI_InitializeModule(void);

/* allocates memory.  0 on failure, 1 on success. */
/* Also pass it the base directory to load the configuration file. */
int MDFNI_Initialize(const char *basedir);

/* Sets the base directory(save states, snapshots, etc. are saved in directories
   below this directory. */
void MDFNI_SetBaseDirectory(const char *dir);

void MDFN_DispMessage(const char *format, ...);
#define MDFNI_DispMessage MDFN_DispMessage

uint32 MDFNI_CRC32(uint32 crc, uint8 *buf, uint32 len);

// NES hackish function.  Should abstract in the future.
int MDFNI_DatachSet(const uint8 *rcode);

void MDFNI_DumpModulesDef(const char *fn);

#endif
