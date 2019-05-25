/* Cygne
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Dox dox@space.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wswan.h"
#include "gfx.h"
#include "wswan-memory.h"
#include "v30mz.h"
#include "rtc.h"
#include "../video.h"

static uint32_t wsMonoPal[16][4];
static uint32_t wsColors[8];
static uint32_t wsCols[16][16];

static uint16_t ColorMapG[16];
static uint16_t ColorMap[4096];
static uint32_t LayerEnabled;

/* Current scanline */
static uint8_t wsLine;

static uint8_t SpriteTable[2][0x80][4];
static uint32_t SpriteCountCache[2];
static uint_fast8_t FrameWhichActive;
static uint8_t DispControl;
static uint8_t BGColor;
static uint8_t LineCompare;
static uint8_t SPRBase;
static uint8_t SpriteStart, SpriteCount;
static uint8_t FGBGLoc;
static uint8_t FGx0, FGy0, FGx1, FGy1;
static uint8_t SPRx0, SPRy0, SPRx1, SPRy1;

static uint8_t BGXScroll, BGYScroll;
static uint8_t FGXScroll, FGYScroll;
static uint8_t LCDControl, LCDIcons;
static uint8_t LCDVtotal;

static uint8_t BTimerControl;
static uint16_t HBTimerPeriod;
static uint16_t VBTimerPeriod;

static uint16_t HBCounter, VBCounter;
static uint8_t VideoMode;

void WSwan_GfxInit(void)
{
	LayerEnabled = 7; // BG, FG, sprites
}

void WSwan_GfxWSCPaletteRAMWrite(uint32_t ws_offset, uint8_t data)
{
	ws_offset = (ws_offset&0xfffe)-0xfe00;
	wsCols[(ws_offset>>1)>>4][(ws_offset>>1)&15] = wsRAM[ws_offset+0xfe00] | ((wsRAM[ws_offset+0xfe01]&0x0f) << 8);
}

void WSwan_GfxWrite(uint32_t A, uint8_t V)
{
   switch(A)
   {
		case 0x00:
			DispControl = V;
		break;
		case 0x01:
			BGColor = V;
		break;
		case 0x03:
			LineCompare = V;
		break;
		case 0x04:
			SPRBase = V & 0x3F;
		break;
		case 0x05:
			SpriteStart = V;
		break;
		case 0x06:
			SpriteCount = V;
		break;
		case 0x07:
			FGBGLoc = V;
		break;
		case 0x08:
			FGx0 = V;
		break;
		case 0x09:
			FGy0 = V;
		break;
		case 0x0A:
			FGx1 = V;
		break;
		case 0x0B:
			FGy1 = V;
		break;
		case 0x0C:
			SPRx0 = V;
		break;
		case 0x0D:
			SPRy0 = V;
		break;
		case 0x0E:
			SPRx1 = V;
		break;
		case 0x0F:
			SPRy1 = V;
		break;
		case 0x10:
			BGXScroll = V;
		break;
		case 0x11:
			BGYScroll = V;
		break;
		case 0x12:
			FGXScroll = V;
		break;
		case 0x13:
			FGYScroll = V;
		break;
		case 0x14:
			LCDControl = V;
		break;
		case 0x15:
			LCDIcons = V;
		break;
		case 0x16:
			LCDVtotal = V;
		break;
		case 0x1C ... 0x1F:
			wsColors[(A - 0x1C) * 2 + 0] = 0xF - (V & 0xf);
			wsColors[(A - 0x1C) * 2 + 1] = 0xF - (V >> 4);
		break;
		case 0x20 ... 0x3F:
			wsMonoPal[(A - 0x20) >> 1][((A & 0x1) << 1) + 0] = V&7;
			wsMonoPal[(A - 0x20) >> 1][((A & 0x1) << 1) | 1] = (V>>4)&7;
		break;
		case 0x60:
			VideoMode = V; 
			wsSetVideo(V>>5, false); 
		break;
		case 0xa2:
			BTimerControl = V; 
		break;
		case 0xa4:
			HBTimerPeriod &= 0xFF00;
			HBTimerPeriod |= (V << 0);
		break;
		case 0xa5:
			HBTimerPeriod &= 0x00FF; HBTimerPeriod |= (V << 8);
			HBCounter = HBTimerPeriod;
		break;
		case 0xa6:
			VBTimerPeriod &= 0xFF00; VBTimerPeriod |= (V << 0);
		break;
		case 0xa7:
			VBTimerPeriod &= 0x00FF; VBTimerPeriod |= (V << 8);
			VBCounter = VBTimerPeriod;
		break;
	}
}

uint8_t WSwan_GfxRead(uint32_t A)
{
	uint8_t ret;
	switch(A)
	{
		case 0x1C ... 0x1F:
			ret = 0;
			ret |= 0xF - wsColors[(A - 0x1C) * 2 + 0];
			ret |= (0xF - wsColors[(A - 0x1C) * 2 + 1]) << 4;
			return(ret);
		break;
		case 0x20 ... 0x3F:
			return wsMonoPal[(A - 0x20) >> 1][((A & 0x1) << 1) + 0] | (wsMonoPal[(A - 0x20) >> 1][((A & 0x1) << 1) | 1] << 4);
		break;
		case 0x00:
			return(DispControl);
		case 0x01:
			return(BGColor);
		case 0x02:
			return(wsLine);
		case 0x03:
			return(LineCompare);
		case 0x04:
			return(SPRBase);
		case 0x05:
			return(SpriteStart);
		case 0x06:
			return(SpriteCount);
		case 0x07:
			return(FGBGLoc);
		case 0x08:
			return(FGx0);
		case 0x09:
			return(FGy0);
		case 0x0A:
			return(FGx1);
		case 0x0B:
			return(FGy1);
		case 0x0C:
			return(SPRx0);
		case 0x0D:
			return(SPRy0);
		case 0x0E:
			return(SPRx1);
		case 0x0F:
			return(SPRy1);
		case 0x10:
			return(BGXScroll);
		case 0x11:
			return(BGYScroll);
		case 0x12:
			return(FGXScroll);
		case 0x13:
			return(FGYScroll);
		case 0x14:
			return(LCDControl);
		case 0x15:
			return(LCDIcons);
		case 0x16:
			return(LCDVtotal);
		case 0x60:
			return(VideoMode);
		case 0xa0:
			return(wsc ? 0x87 : 0x86);
		case 0xa2:
			return(BTimerControl);
		case 0xa4:
			return((HBTimerPeriod >> 0) & 0xFF);
		case 0xa5:
			return((HBTimerPeriod >> 8) & 0xFF);
		case 0xa6:
			return((VBTimerPeriod >> 0) & 0xFF);
		case 0xa7:
			return((VBTimerPeriod >> 8) & 0xFF);
		case 0xa8:
			return((HBCounter >> 0) & 0xFF);
		case 0xa9:
			return((HBCounter >> 8) & 0xFF);
		case 0xaa:
			return((VBCounter >> 0) & 0xFF);
		case 0xab:
			return((VBCounter >> 8) & 0xFF);
		default:
			break;
	}

	return 0;
}

uint32_t wsExecuteLine(uint16_t* restrict pixels, uint8_t pitch, const uint32_t skip)
{
	uint32_t ret = false;
	
	if (!skip)
	{
		if (wsLine < 144)
		{
			wsScanline(pixels + wsLine * pitch);
		}

		WSwan_CheckSoundDMA();

		// Update sprite data table
		if (wsLine == 142)
		{
			SpriteCountCache[!FrameWhichActive] = min(0x80, SpriteCount);
			memcpy(SpriteTable[!FrameWhichActive], &wsRAM[(SPRBase << 9) + (SpriteStart << 2)], SpriteCountCache[!FrameWhichActive] << 2);
		}
	}
	
	if (wsLine == 144)
	{
		FrameWhichActive = !FrameWhichActive;
		ret = true;
		WSwan_Interrupt(WSINT_VBLANK);
		if(VBCounter && (BTimerControl & 0x04))
		{
			VBCounter--;
			if(!VBCounter)
			{
				if(BTimerControl & 0x08) // loop
					VBCounter = VBTimerPeriod;
				WSwan_Interrupt(WSINT_VBLANK_TIMER);
			}
		}
	}

	if (HBCounter && (BTimerControl & 0x01))
	{
		HBCounter--;
		if (!HBCounter)
		{
			if(BTimerControl & 0x02) // loop
				HBCounter = HBTimerPeriod;
			WSwan_Interrupt(WSINT_HBLANK_TIMER);
		}
	}

	v30mz_execute(128);
	WSwan_CheckSoundDMA();
	v30mz_execute(96);
	wsLine = (wsLine + 1) % (max(144, LCDVtotal) + 1);
	if(wsLine == LineCompare)
	{
		WSwan_Interrupt(WSINT_LINE_HIT);
		//printf("Line hit: %d\n", wsLine);
	}
	v30mz_execute(32);
	WSwan_RTCClock(256);

	return(ret);
}

void WSwan_SetLayerEnableMask(uint64_t mask)
{
	LayerEnabled = mask;
}

void WSwan_SetPixelFormat(void)
{
	uint_fast8_t r, g, b, i;
	for (r = 0; r < 16; r++)
	{
		for (g = 0; g < 16; g++)
		{
			for (b = 0; b < 16; b++)
			{
				ColorMap[(r << 8) | (g << 4) | (b << 0)] = MAKECOLOR((r * 17), (g * 17), (b * 17), 0); //(neo_r << rs) | (neo_g << gs) | (neo_b << bs);
			}
		}
	}

	for (i = 0; i < 16; i++)
	{
		ColorMapG[i] = MAKECOLOR((i * 17), (i * 17), (i * 17), 0); //(neo_r << rs) | (neo_g << gs) | (neo_b << bs);
	}
}

void wsScanline(uint16_t* restrict target)
{
	uint32_t start_tile_n,map_a,startindex,adrbuf,b1,b2,j,t,l;
	uint8_t b_bg[256];
	uint8_t b_bg_pal[256];

	if (!wsVMode)
	{
		memset(b_bg, wsColors[BGColor&0xF]&0xF, 256);
	}
	else
	{
		memset(&b_bg[0], BGColor & 0xF, 256);
		memset(&b_bg_pal[0], (BGColor>>4)  & 0xF, 256);
	}
   
	/* First line */
	start_tile_n=(wsLine+BGYScroll)&0xff;
	map_a=(((uint32_t)(FGBGLoc&0xF))<<11)+((start_tile_n&0xfff8)<<3);
   
	/* First tile in row */
	startindex = BGXScroll >> 3;
   
	/* Pixel in tile */
	adrbuf = 7-(BGXScroll&7);

	/* BG layer */
	if ((DispControl & 0x01) && (LayerEnabled & 0x01))
	{
		for(t=0;t<29;t++)
		{
			b1=wsRAM[map_a+(startindex<<1)];
			b2=wsRAM[map_a+(startindex<<1)+1];
			uint32_t palette=(b2>>1)&15;
			b2=(b2<<8)|b1;
			wsGetTile(b2&0x1ff,start_tile_n&7,b2&0x8000,b2&0x4000,b2&0x2000);

			if (wsVMode)
			{
				if (wsVMode & 0x2)
				{
					for(uint32_t x = 0; x < 8; x++)
					{
						if(wsTileRow[x])
						{
							b_bg[adrbuf + x] = wsTileRow[x];
							b_bg_pal[adrbuf + x] = palette;
						}
					}
				}
				else
				{
					for (uint32_t x = 0; x < 8; x++)
					{
						if (wsTileRow[x] || !(palette & 0x4))
						{
							b_bg[adrbuf + x] = wsTileRow[x];
							b_bg_pal[adrbuf + x] = palette;
						}
					}
				}
			}
			else
			{
				for(uint32_t x = 0; x < 8; x++)
				{
					if(wsTileRow[x] || !(palette & 4))
					{
						b_bg[adrbuf + x] = wsColors[wsMonoPal[palette][wsTileRow[x]]];
					}
				}
			}
			adrbuf += 8;
			startindex=(startindex + 1)&31;
		} // end for(t = 0 ...
	} // End BG layer drawing

	/* FG layer */
	if ((DispControl & 0x02) && (LayerEnabled & 0x02))
	{
		uint8_t windowtype = DispControl&0x30;
		uint32_t in_window[256 + 8*2];

		if (windowtype)
		{
			memset(in_window, 0, sizeof(in_window));

			if(windowtype == 0x20) // Display FG only inside window
			{
				if((wsLine >= FGy0) && (wsLine <= FGy1))
					for(j = FGx0; j <= FGx1 && j < 224; j++)
						in_window[7 + j] = 1;
			}
			else if(windowtype == 0x30) // Display FG only outside window
			{
				for(j = 0; j < 224; j++)
				{
					if(!(j >= FGx0 && j < FGx1) || !((wsLine >= FGy0) && (wsLine <= FGy1)))
						in_window[7 + j] = 1;
				}
			}
			/*else
			{
				puts("Who knows!");
			}*/
		}
		else
		{
			memset(in_window, 1, sizeof(in_window));
		}

		start_tile_n=(wsLine+FGYScroll)&0xff;
		map_a=(((uint32_t)((FGBGLoc>>4)&0xF))<<11)+((start_tile_n>>3)<<6);
		startindex = FGXScroll >> 3;
		adrbuf = 7-(FGXScroll&7);

		for (t=0; t<29; t++)
		{
			b1=wsRAM[map_a+(startindex<<1)];
			b2=wsRAM[map_a+(startindex<<1)+1];
			uint32_t palette=(b2>>1)&15;
			b2=(b2<<8)|b1;
			wsGetTile(b2&0x1ff,start_tile_n&7,b2&0x8000,b2&0x4000,b2&0x2000);

			if(wsVMode)
			{
				if(wsVMode & 0x2)
				{
					for(uint32_t x = 0; x < 8; x++)
					{
						if(wsTileRow[x] && in_window[adrbuf + x])
						{
							b_bg[adrbuf + x] = wsTileRow[x] | 0x10;
							b_bg_pal[adrbuf + x] = palette;
						}
					}
				}
				else
				{
					for(uint32_t x = 0; x < 8; x++)
					{
						if((wsTileRow[x] || !(palette & 0x4)) && in_window[adrbuf + x])
						{
							b_bg[adrbuf + x] = wsTileRow[x] | 0x10;
							b_bg_pal[adrbuf + x] = palette;
						}
					}
				}
         }
         else
         {
            for(uint32_t x = 0; x < 8; x++)
            {
				if ((wsTileRow[x] || !(palette & 4)) && in_window[adrbuf + x])
				{
					b_bg[adrbuf + x] = wsColors[wsMonoPal[palette][wsTileRow[x]]] | 0x10;
				}
			}
		}
		adrbuf += 8;
		startindex=(startindex + 1)&31;
		} // end for(t = 0 ...

	} // end FG drawing

	/* Sprites */
	if ((DispControl & 0x04) && SpriteCountCache[FrameWhichActive] && (LayerEnabled & 0x04))
	{
		uint32_t in_window[256 + 8*2];

		if(DispControl & 0x08)
		{
			memset(in_window, 0, sizeof(in_window));
			if((wsLine >= SPRy0) && (wsLine <= SPRy1))
			{
				for(j = SPRx0; j <= SPRx1 && j < 256; j++)
				{
					in_window[7 + j] = 1;
				}
			}
		}
		else
		{
			memset(in_window, 1, sizeof(in_window));
		}

		for (int_fast32_t h = SpriteCountCache[FrameWhichActive] - 1; h >= 0; h--)
		{
			int_fast32_t ts = SpriteTable[FrameWhichActive][h][0];
			int_fast32_t as = SpriteTable[FrameWhichActive][h][1];
			int_fast32_t ysx = SpriteTable[FrameWhichActive][h][2];
			int_fast32_t xs = SpriteTable[FrameWhichActive][h][3];
			int_fast32_t ys;

			 if(xs >= 249) xs -= 256;

			 if(ysx > 150) 
				ys = (int8_t)ysx;
			 else 
				ys = ysx;

			 ys = wsLine - ys;

			 if (ys >= 0 && ys < 8 && xs < 224)
			 {
				uint32_t palette = ((as >> 1) & 0x7);

				ts |= (as&1) << 8;
				wsGetTile(ts, ys, as & 0x80, as & 0x40, 0);

				if(wsVMode)
				{
				   if(wsVMode & 0x2)
				   {
						for (int32_t x = 0; x < 8; x++)
						{
							if (wsTileRow[x])
							{
								if ((as & 0x20) || !(b_bg[xs + x + 7] & 0x10))
								{
									uint32_t drawthis = 0;

									if (!(DispControl & 0x08)) 
										drawthis = true;
									else if ((as & 0x10) && !in_window[7 + xs + x])
										drawthis = true;
									else if (!(as & 0x10) && in_window[7 + xs + x])
										drawthis = true;

									if (drawthis)
									{
										b_bg[xs + x + 7] = wsTileRow[x] | (b_bg[xs + x + 7] & 0x10);
										b_bg_pal[xs + x + 7] = 8 + palette;
									}
								}
							}
						}
				   }
				   else
				   {
						for(int32_t x = 0; x < 8; x++)
						{
							if(wsTileRow[x] || !(palette & 0x4))
							{
								if((as & 0x20) || !(b_bg[xs + x + 7] & 0x10))
								{
									uint32_t drawthis = 0;
									if(!(DispControl & 0x08))
										drawthis = true;
									else if((as & 0x10) && !in_window[7 + xs + x])
										drawthis = true;
									else if(!(as & 0x10) && in_window[7 + xs + x])
										drawthis = true;

									if (drawthis)
									{
										b_bg[xs + x + 7] = wsTileRow[x] | (b_bg[xs + x + 7] & 0x10);
										b_bg_pal[xs + x + 7] = 8 + palette;
									}
								}
							}
						}
					}
				}
				else
				{
					for(int32_t x = 0; x < 8; x++)
					{
						if(wsTileRow[x] || !(palette & 4))
						{
							if ((as & 0x20) || !(b_bg[xs + x + 7] & 0x10))
							{
								uint32_t drawthis = 0;

								if(!(DispControl & 0x08))
								   drawthis = true;
								else if((as & 0x10) && !in_window[7 + xs + x])
								   drawthis = true;
								else if(!(as & 0x10) && in_window[7 + xs + x])
								   drawthis = true;
								   
								if (drawthis)
								//if((as & 0x10) || in_window[7 + xs + x])
								{
									b_bg[xs + x + 7] = wsColors[wsMonoPal[8 + palette][wsTileRow[x]]] | (b_bg[xs + x + 7] & 0x10);
								}
							}
						}
					}
				}
			 }
		  }
	}	// End sprite drawing

	if(wsVMode)
	{
		for (l=0;l<224;l++)
		{
			target[l] = ColorMap[wsCols[b_bg_pal[l+7]][b_bg[(l+7)]&0xf]];
		}
	}
	else
	{
		for (l=0;l<224;l++)
		{
			target[l] = ColorMapG[(b_bg[l+7])&15];
		}
	}

}


void WSwan_GfxReset(void)
{
	wsLine = 0;
	wsSetVideo(0,true);

	memset(SpriteTable, 0, sizeof(SpriteTable));
	SpriteCountCache[0] = SpriteCountCache[1] = 0;
	FrameWhichActive = false;
	DispControl = 0;
	BGColor = 0;
	LineCompare = 0xBB;
	SPRBase = 0;

	SpriteStart = 0;
	SpriteCount = 0;
	FGBGLoc = 0;

	FGx0 = 0;
	FGy0 = 0;
	FGx1 = 0;
	FGy1 = 0;
	SPRx0 = 0;
	SPRy0 = 0;
	SPRx1 = 0;
	SPRy1 = 0;

	BGXScroll = BGYScroll = 0;
	FGXScroll = FGYScroll = 0;
	LCDControl = 0;
	LCDIcons = 0;
	LCDVtotal = 158;

	BTimerControl = 0;
	HBTimerPeriod = 0;
	VBTimerPeriod = 0;

	HBCounter = 0;
	VBCounter = 0;


	for(uint_fast8_t u0=0;u0<16;u0++)
	{
		for(uint_fast8_t u1=0;u1<16;u1++)
		{
			wsCols[u0][u1]=0;
		}
	}
}

void WSwan_GfxSaveState(uint32_t load, FILE* fp)
{
	/* Load state */
	if (load == 1)
	{
		fread(&wsMonoPal, sizeof(uint8_t), sizeof(wsMonoPal), fp);
		fread(&wsColors, sizeof(uint8_t), sizeof(wsColors), fp);
		fread(&wsCols, sizeof(uint8_t), sizeof(wsCols), fp);

		fread(&ColorMapG, sizeof(uint8_t), sizeof(ColorMapG), fp);
		fread(&ColorMap, sizeof(uint8_t), sizeof(ColorMap), fp);
		fread(&LayerEnabled, sizeof(uint8_t), sizeof(LayerEnabled), fp);

		fread(&wsLine, sizeof(uint8_t), sizeof(wsLine), fp);

		fread(&SpriteTable, sizeof(uint8_t), sizeof(SpriteTable), fp);
		fread(&SpriteCountCache, sizeof(uint8_t), sizeof(SpriteCountCache), fp);
		fread(&FrameWhichActive, sizeof(uint8_t), sizeof(FrameWhichActive), fp);
		fread(&DispControl, sizeof(uint8_t), sizeof(DispControl), fp);
		fread(&BGColor, sizeof(uint8_t), sizeof(BGColor), fp);
		fread(&LineCompare, sizeof(uint8_t), sizeof(LineCompare), fp);
		fread(&SPRBase, sizeof(uint8_t), sizeof(SPRBase), fp);
		fread(&SpriteStart, sizeof(uint8_t), sizeof(SpriteStart), fp);
		fread(&SpriteCount, sizeof(uint8_t), sizeof(SpriteCount), fp);
		fread(&FGBGLoc, sizeof(uint8_t), sizeof(FGBGLoc), fp);
		fread(&FGx0, sizeof(uint8_t), sizeof(FGx0), fp);
		fread(&FGy0, sizeof(uint8_t), sizeof(FGy0), fp);
		fread(&FGx1, sizeof(uint8_t), sizeof(FGx1), fp);
		fread(&FGy1, sizeof(uint8_t), sizeof(FGy1), fp);
		fread(&SPRx0, sizeof(uint8_t), sizeof(SPRx0), fp);
		fread(&SPRy0, sizeof(uint8_t), sizeof(SPRy0), fp);
		fread(&SPRx1, sizeof(uint8_t), sizeof(SPRx1), fp);
		fread(&SPRy1, sizeof(uint8_t), sizeof(SPRy1), fp);

		fread(&BGXScroll, sizeof(uint8_t), sizeof(BGXScroll), fp);
		fread(&BGYScroll, sizeof(uint8_t), sizeof(BGYScroll), fp);
		fread(&FGXScroll, sizeof(uint8_t), sizeof(FGXScroll), fp);
		fread(&FGYScroll, sizeof(uint8_t), sizeof(FGYScroll), fp);
		fread(&LCDControl, sizeof(uint8_t), sizeof(LCDControl), fp);
		fread(&LCDIcons, sizeof(uint8_t), sizeof(LCDIcons), fp);
		fread(&LCDVtotal, sizeof(uint8_t), sizeof(LCDVtotal), fp);

		fread(&BTimerControl, sizeof(uint8_t), sizeof(BTimerControl), fp);
		fread(&HBTimerPeriod, sizeof(uint8_t), sizeof(HBTimerPeriod), fp);
		fread(&VBTimerPeriod, sizeof(uint8_t), sizeof(VBTimerPeriod), fp);

		fread(&HBCounter, sizeof(uint8_t), sizeof(HBCounter), fp);
		fread(&VBCounter, sizeof(uint8_t), sizeof(VBCounter), fp);
		fread(&VideoMode, sizeof(uint8_t), sizeof(VideoMode), fp);
		
		for(uint_fast8_t i = 0; i < 2; i++)
		{
			if(SpriteCountCache[i] > 0x80)
				SpriteCountCache[i] = 0x80;
		}

		for(uint_fast8_t i = 0; i < 16; i++)
		{
			for(uint_fast8_t j = 0; j < 4; j++)
			{
				wsMonoPal[i][j] &= 0x7;
			}
		}
		wsSetVideo(VideoMode >> 5, true);
	}
	/* Save State */
	else
	{
		fwrite(&wsMonoPal, sizeof(uint8_t), sizeof(wsMonoPal), fp);
		fwrite(&wsColors, sizeof(uint8_t), sizeof(wsColors), fp);
		fwrite(&wsCols, sizeof(uint8_t), sizeof(wsCols), fp);

		fwrite(&ColorMapG, sizeof(uint8_t), sizeof(ColorMapG), fp);
		fwrite(&ColorMap, sizeof(uint8_t), sizeof(ColorMap), fp);
		fwrite(&LayerEnabled, sizeof(uint8_t), sizeof(LayerEnabled), fp);

		fwrite(&wsLine, sizeof(uint8_t), sizeof(wsLine), fp);

		fwrite(&SpriteTable, sizeof(uint8_t), sizeof(SpriteTable), fp);
		fwrite(&SpriteCountCache, sizeof(uint8_t), sizeof(SpriteCountCache), fp);
		fwrite(&FrameWhichActive, sizeof(uint8_t), sizeof(FrameWhichActive), fp);
		fwrite(&DispControl, sizeof(uint8_t), sizeof(DispControl), fp);
		fwrite(&BGColor, sizeof(uint8_t), sizeof(BGColor), fp);
		fwrite(&LineCompare, sizeof(uint8_t), sizeof(LineCompare), fp);
		fwrite(&SPRBase, sizeof(uint8_t), sizeof(SPRBase), fp);
		fwrite(&SpriteStart, sizeof(uint8_t), sizeof(SpriteStart), fp);
		fwrite(&SpriteCount, sizeof(uint8_t), sizeof(SpriteCount), fp);
		fwrite(&FGBGLoc, sizeof(uint8_t), sizeof(FGBGLoc), fp);
		fwrite(&FGx0, sizeof(uint8_t), sizeof(FGx0), fp);
		fwrite(&FGy0, sizeof(uint8_t), sizeof(FGy0), fp);
		fwrite(&FGx1, sizeof(uint8_t), sizeof(FGx1), fp);
		fwrite(&FGy1, sizeof(uint8_t), sizeof(FGy1), fp);
		fwrite(&SPRx0, sizeof(uint8_t), sizeof(SPRx0), fp);
		fwrite(&SPRy0, sizeof(uint8_t), sizeof(SPRy0), fp);
		fwrite(&SPRx1, sizeof(uint8_t), sizeof(SPRx1), fp);
		fwrite(&SPRy1, sizeof(uint8_t), sizeof(SPRy1), fp);

		fwrite(&BGXScroll, sizeof(uint8_t), sizeof(BGXScroll), fp);
		fwrite(&BGYScroll, sizeof(uint8_t), sizeof(BGYScroll), fp);
		fwrite(&FGXScroll, sizeof(uint8_t), sizeof(FGXScroll), fp);
		fwrite(&FGYScroll, sizeof(uint8_t), sizeof(FGYScroll), fp);
		fwrite(&LCDControl, sizeof(uint8_t), sizeof(LCDControl), fp);
		fwrite(&LCDIcons, sizeof(uint8_t), sizeof(LCDIcons), fp);
		fwrite(&LCDVtotal, sizeof(uint8_t), sizeof(LCDVtotal), fp);

		fwrite(&BTimerControl, sizeof(uint8_t), sizeof(BTimerControl), fp);
		fwrite(&HBTimerPeriod, sizeof(uint8_t), sizeof(HBTimerPeriod), fp);
		fwrite(&VBTimerPeriod, sizeof(uint8_t), sizeof(VBTimerPeriod), fp);

		fwrite(&HBCounter, sizeof(uint8_t), sizeof(HBCounter), fp);
		fwrite(&VBCounter, sizeof(uint8_t), sizeof(VBCounter), fp);
		fwrite(&VideoMode, sizeof(uint8_t), sizeof(VideoMode), fp);
	}
}
