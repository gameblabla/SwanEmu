#ifndef __WSWAN_GFX_H
#define __WSWAN_GFX_H


void WSWan_TCacheInvalidByAddr(uint32_t);

extern uint8_t	wsTCache[512*64];		  //tiles cache
extern uint8_t	wsTCacheFlipped[512*64];  	  //tiles cache (H flip)
extern uint8_t	wsTileRow[8];		  //extracted 8 pixels (tile row)
extern uint8_t	wsTCacheUpdate[512];	  //tiles cache flags
extern uint8_t	wsTCache2[512*64];		  //tiles cache
extern uint8_t	wsTCacheFlipped2[512*64];  	  //tiles cache (H flip)
extern uint8_t	wsTCacheUpdate2[512];	  //tiles cache flags
extern int		wsVMode;			  //Video Mode	

void wsMakeTiles(void);
void wsGetTile(uint32_t,uint32_t,int,int,int);
void wsSetVideo(int, uint32_t);

void wsScanline(uint16_t* restrict target);

extern uint32_t	dx_r,dx_g,dx_b,dx_sr,dx_sg,dx_sb;
extern uint32_t	dx_bits,dx_pitch,cmov,dx_linewidth_blit,dx_buffer_line;


void WSwan_SetPixelFormat(void);

void WSwan_GfxInit(void);
void WSwan_GfxReset(void);
void WSwan_GfxWrite(uint32_t A, uint8_t V);
uint8_t WSwan_GfxRead(uint32_t A);
void WSwan_GfxWSCPaletteRAMWrite(uint32_t ws_offset, uint8_t data);

uint32_t wsExecuteLine(uint16_t* restrict pixels, uint8_t pitch, uint32_t skip);

void WSwan_SetLayerEnableMask(uint64_t mask);

#endif
