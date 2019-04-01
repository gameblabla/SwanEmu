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
#include <time.h>

static uint64_t CurrentTime;
static uint32_t ClockCycleCounter;
static uint8_t wsCA15;
static uint8_t Command, Data;

void WSwan_RTCWrite(uint32 A, uint8 V)
{
   switch(A)
   {
      case 0xca: 
         if(V==0x15)
            wsCA15=0; 
         Command = V;
         break;
      case 0xcb:
         Data = V;
         break;
   }
}

uint8 WSwan_RTCRead(uint32 A)
{
   time_t long_time;
   struct tm *newtime;

   switch(A)
   {
      case 0xca:
         return (Command|0x80);
      case 0xcb:
         if(Command != 0x15)
            return Data | 0x80;

         long_time = CurrentTime;
         newtime = gmtime( &long_time );

         switch(wsCA15)
         {
            case 0:
               wsCA15++;
               return mBCD(newtime->tm_year-100);
            case 1:
               wsCA15++;
               return mBCD(newtime->tm_mon);
            case 2:
               wsCA15++;
               return mBCD(newtime->tm_mday);
            case 3:
               wsCA15++;
               return mBCD(newtime->tm_wday);
            case 4:
               wsCA15++;
               return mBCD(newtime->tm_hour);
            case 5:
               wsCA15++;
               return mBCD(newtime->tm_min);
            case 6:
               wsCA15=0;
               return mBCD(newtime->tm_sec);
         }
         break;
   }

   return 0;
}

void WSwan_RTCReset(void)
{
   time_t happy_time = time(NULL);

   CurrentTime = mktime(localtime(&happy_time));
   ClockCycleCounter = 0;
   wsCA15 = 0;
}

void WSwan_RTCClock(uint32 cycles)
{
   ClockCycleCounter += cycles;
   while(ClockCycleCounter >= 3072000)
   {
      ClockCycleCounter -= 3072000;
      CurrentTime++;
   }
}

void WSwan_RTCSaveState(uint32_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&CurrentTime, sizeof(uint8_t), sizeof(CurrentTime), fp);
		fread(&ClockCycleCounter, sizeof(uint8_t), sizeof(ClockCycleCounter), fp);
		fread(&wsCA15, sizeof(uint8_t), sizeof(wsCA15), fp);
		fread(&Command, sizeof(uint8_t), sizeof(Command), fp);
		fread(&Data, sizeof(uint8_t), sizeof(Data), fp);
	}
	else
	{
		fwrite(&CurrentTime, sizeof(uint8_t), sizeof(CurrentTime), fp);
		fwrite(&ClockCycleCounter, sizeof(uint8_t), sizeof(ClockCycleCounter), fp);
		fwrite(&wsCA15, sizeof(uint8_t), sizeof(wsCA15), fp);
		fwrite(&Command, sizeof(uint8_t), sizeof(Command), fp);
		fwrite(&Data, sizeof(uint8_t), sizeof(Data), fp);
	}

}
