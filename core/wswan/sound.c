/* Mednafen - Multi-system Emulator
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

/*
 Noise emulation is almost certainly wrong wrong wrong.  Testing on a real system is needed to determine LFSR(assuming it uses an LFSR) taps.
*/

#include "wswan.h"
#include "sound.h"
#include "v30mz.h"
#include "wswan-memory.h"

#include "Blip_Buffer.h"
#include "shared.h"

static Blip_Synth WaveSynth;

static Blip_Buffer sbuf[2];

static uint16_t period[4];
static uint8_t volume[4]; // left volume in upper 4 bits, right in lower 4 bits
static uint8_t voice_volume;

static uint8_t sweep_step, sweep_value;
static uint8_t noise_control;
static uint8_t control;
static uint8_t output_control;

static int32_t sweep_8192_divider;
static uint8_t sweep_counter;
static uint8_t SampleRAMPos;

static int32_t sample_cache[4][2];

static int32_t last_v_val;

static uint8_t HyperVoice;
static int32_t last_hv_val[2];
static uint8_t HVoiceCtrl, HVoiceChanCtrl;

static int32_t period_counter[4];
static int32_t last_val[4][2]; // Last outputted value, l&r
static uint8_t sample_pos[4];
static uint16_t nreg;
static uint32_t last_ts;


#define MK_SAMPLE_CACHE	\
   {    \
    int sample; \
    sample = (((wsRAM[((SampleRAMPos << 6) + (sample_pos[ch] >> 1) + (ch << 4)) ] >> ((sample_pos[ch] & 1) ? 4 : 0)) & 0x0F));    \
    sample_cache[ch][0] = sample * ((volume[ch] >> 4) & 0x0F); \
    sample_cache[ch][1] = sample * ((volume[ch] >> 0) & 0x0F); \
   }

#define MK_SAMPLE_CACHE_NOISE \
   {    \
    int sample; \
    sample = ((nreg & 1) ? 0xF : 0x0);	\
    sample_cache[ch][0] = sample * ((volume[ch] >> 4) & 0x0F);        \
    sample_cache[ch][1] = sample * ((volume[ch] >> 0) & 0x0F);        \
   }

#define MK_SAMPLE_CACHE_VOICE \
   {    \
    int sample, half; \
    sample = volume[ch]; \
    half = sample >> 1; \
    sample_cache[ch][0] = (voice_volume & 4) ? sample : (voice_volume & 8) ? half : 0; \
    sample_cache[ch][1] = (voice_volume & 1) ? sample : (voice_volume & 2) ? half : 0; \
   }


#define SYNCSAMPLE(wt)	\
   {	\
    int32_t left = sample_cache[ch][0], right = sample_cache[ch][1];	\
    Blip_Synth_offset(&WaveSynth, wt, left - last_val[ch][0], &sbuf[0]);	\
    Blip_Synth_offset(&WaveSynth, wt, right - last_val[ch][1], &sbuf[1]);	\
    last_val[ch][0] = left;	\
    last_val[ch][1] = right;	\
   }

#define SYNCSAMPLE_NOISE(wt) SYNCSAMPLE(wt)


void WSwan_SoundUpdate(void)
{
   int32_t run_time;

   //printf("%d\n", v30mz_timestamp);
   //printf("%02x %02x\n", control, noise_control);
   run_time = v30mz_timestamp - last_ts;

   for(uint_fast8_t ch = 0; ch < 4; ch++)
   {
      // Channel is disabled?
      if(!(control & (1 << ch)))
         continue;

      if(ch == 1 && (control & 0x20)) // Direct D/A mode?
      {
		MK_SAMPLE_CACHE_VOICE;
		SYNCSAMPLE(v30mz_timestamp);
      }
      else if(ch == 2 && (control & 0x40) && sweep_value) // Sweep
      {
         uint32_t tmp_pt = 2048 - period[ch];
         uint32_t meow_timestamp = v30mz_timestamp - run_time;
         uint32_t tmp_run_time = run_time;

         while(tmp_run_time)
         {
            int32_t sub_run_time = tmp_run_time;

            if(sub_run_time > sweep_8192_divider)
               sub_run_time = sweep_8192_divider;

            sweep_8192_divider -= sub_run_time;
            if(sweep_8192_divider <= 0)
            {
               sweep_8192_divider += 8192;
               sweep_counter--;
               if(sweep_counter == 0)
               {
                  sweep_counter = sweep_step + 1;
                  period[ch] = (period[ch] + (int8_t)sweep_value) & 0x7FF;
               }
            }

            meow_timestamp += sub_run_time;
            if(tmp_pt > 4)
            {
               period_counter[ch] -= sub_run_time;
               while(period_counter[ch] <= 0)
               {
                  sample_pos[ch] = (sample_pos[ch] + 1) & 0x1F;

                  MK_SAMPLE_CACHE;
                  SYNCSAMPLE(meow_timestamp + period_counter[ch]);
                  period_counter[ch] += tmp_pt;
               }
            }
            tmp_run_time -= sub_run_time;
         }
      }
      else if(ch == 3 && (control & 0x80) && (noise_control & 0x10)) // Noise
      {
         uint32_t tmp_pt = 2048 - period[ch];

         period_counter[ch] -= run_time;
         while(period_counter[ch] <= 0)
         {
            static const uint8_t stab[8] = { 14, 10, 13, 4, 8, 6, 9, 11 };
            nreg = ((nreg << 1) | ((1 ^ (nreg >> 7) ^ (nreg >> stab[noise_control & 0x7])) & 1)) & 0x7FFF;
            if(control & 0x80)
            {
               MK_SAMPLE_CACHE_NOISE;
               SYNCSAMPLE_NOISE(v30mz_timestamp + period_counter[ch]);
            }
            else if(tmp_pt > 4)
            {
               sample_pos[ch] = (sample_pos[ch] + 1) & 0x1F;
               MK_SAMPLE_CACHE;
               SYNCSAMPLE(v30mz_timestamp + period_counter[ch]);
            }
            period_counter[ch] += tmp_pt;
         }
      }
      else
      {
         uint32_t tmp_pt = 2048 - period[ch];

         if(tmp_pt > 4)
         {
            period_counter[ch] -= run_time;
            while(period_counter[ch] <= 0)
            {
               sample_pos[ch] = (sample_pos[ch] + 1) & 0x1F;

               MK_SAMPLE_CACHE;
               SYNCSAMPLE(v30mz_timestamp + period_counter[ch]); // - period_counter[ch]);
               period_counter[ch] += tmp_pt;
            }
         }
      }
   }
	
	if(HVoiceCtrl & 0x80)
	{
		int16_t sample = (uint8_t)HyperVoice;

		switch(HVoiceCtrl & 0xC)
		{
			case 0x0: sample = (uint16_t)sample << (8 - (HVoiceCtrl & 3)); break;
			case 0x4: sample = (uint16_t)(sample | -0x100) << (8 - (HVoiceCtrl & 3)); break;
			case 0x8: sample = (uint16_t)((int8_t)sample) << (8 - (HVoiceCtrl & 3)); break;
			case 0xC: sample = (uint16_t)sample << 8; break;
		}

		// bring back to 11bit, keeping signedness
		sample >>= 5;

		int32_t left, right;
		left  = (HVoiceChanCtrl & 0x40) ? sample : 0;
		right = (HVoiceChanCtrl & 0x20) ? sample : 0;

		Blip_Synth_offset(&WaveSynth, v30mz_timestamp, left - last_hv_val[0], &sbuf[0]);
		Blip_Synth_offset(&WaveSynth, v30mz_timestamp, right - last_hv_val[1], &sbuf[1]);
		last_hv_val[0] = left;
		last_hv_val[1] = right;
	}
   last_ts = v30mz_timestamp;
}

void WSwan_SoundWrite(uint32_t A, uint8_t V)
{
	uint32_t ch;
	WSwan_SoundUpdate();

	switch(A)
	{
		case 0x80 ... 0x87:
			ch = (A - 0x80) >> 1;
			if(A & 1)
			{
				period[ch] = (period[ch] & 0x00FF) | ((V & 0x07) << 8);
			}
			else
			{
				period[ch] = (period[ch] & 0x0700) | ((V & 0xFF) << 0);
			}
		break;
		case 0x88 ... 0x8B:
			volume[A - 0x88] = V;
		break;
		case 0x8C:
			sweep_value = V;
		break;
		case 0x8D:
			sweep_step = V;
			sweep_counter = sweep_step + 1;
			sweep_8192_divider = 8192;
		break;
		case 0x8E:
			if(V & 0x8)
			{
				nreg = 0;
			}
			noise_control = V & 0x17;
		break;
		case 0x90:
			for(uint32_t n = 0; n < 4; n++)
			{
				if(!(control & (1 << n)) && (V & (1 << n)))
				{
					period_counter[n] = 1;
					sample_pos[n] = 0x1F;
				}
			}
			control = V;
		break;
		case 0x91:
			output_control = V & 0xF;
		break;
		case 0x92:
			nreg = (nreg & 0xFF00) | (V << 0);
		break;
		case 0x93:
			nreg = (nreg & 0x00FF) | ((V & 0x7F) << 8);
		break;
		case 0x94:
			voice_volume = V & 0xF;
		break;
		case 0x6A: HVoiceCtrl = V; break;
		case 0x6B: HVoiceChanCtrl = V & 0x6F; break;
		case 0x8F: SampleRAMPos = V; break;
		case 0x95: HyperVoice = V; break; // Pick a port, any port?!
                 //default: printf("%04x:%02x\n", A, V); break;
   }
   WSwan_SoundUpdate();
}

uint8_t WSwan_SoundRead(uint32_t A)
{
	uint32_t ch;
	WSwan_SoundUpdate();
	switch(A)
	{
		default:
			//printf("SoundRead: %04x\n", A);
		break;
		case 0x6A: return(HVoiceCtrl);
		case 0x6B: return(HVoiceChanCtrl);
		case 0x80 ... 0x87:
			ch = (A - 0x80) >> 1;
			if(A & 1)
				return(period[ch] >> 8);
			else
				return(period[ch]);
		break;
		case 0x88 ... 0x8B: return(volume[A - 0x88]);
		case 0x8C: return(sweep_value);
		case 0x8D: return(sweep_step);
		case 0x8E: return(noise_control);
		case 0x8F: return(SampleRAMPos);
		case 0x90: return(control);
		case 0x91: return(output_control | 0x80);
		case 0x92: return((nreg >> 0) & 0xFF);
		case 0x93: return((nreg >> 8) & 0xFF);
		case 0x94: return(voice_volume);
	}

	return(0);
}


int32_t WSwan_SoundFlush(int16_t *SoundBuf, const int32_t MaxSoundFrames)
{
   int32_t FrameCount = 0;

   WSwan_SoundUpdate();

   if(SoundBuf)
   {
      for(int16_t y = 0; y < 2; y++)
      {
			Blip_Buffer_end_frame(&sbuf[y], v30mz_timestamp);
			FrameCount = Blip_Buffer_read_samples(&sbuf[y], SoundBuf + y, MaxSoundFrames);
      }
   }

   last_ts = 0;

   return(FrameCount);
}

// Call before wsRAM is updated
void WSwan_SoundCheckRAMWrite(uint32_t A)
{
   if((A >> 6) == SampleRAMPos)
      WSwan_SoundUpdate();
}

static void RedoVolume(void)
{
	Blip_Synth_set_volume(&WaveSynth, 2.5, 4096);
}

void WSwan_SoundInit(void)
{
   uint_fast8_t i;
   for(i = 0; i < 2; i++)
   {
		Blip_Buffer_init(&sbuf[i]);
		Blip_Buffer_set_sample_rate(&sbuf[i], SOUND_OUTPUT_FREQUENCY, 60);
		Blip_Buffer_set_clock_rate(&sbuf[i], (long)(3072000));
		Blip_Buffer_bass_freq(&sbuf[i], 20);
   }

   RedoVolume();
}

void WSwan_SoundKill(void)
{
   for(uint_fast8_t i = 0; i < 2; i++)
   {
	   Blip_Buffer_deinit(&sbuf[i]);
   }

}

uint32_t WSwan_SetSoundRate(uint32_t rate)
{
	uint_fast8_t i;
	for(i = 0; i < 2; i++)
	{
		Blip_Buffer_set_sample_rate(&sbuf[i], rate ? rate : SOUND_OUTPUT_FREQUENCY, 60);
	}

	return(true);
}

void WSwan_SoundSaveState(uint32_t load, FILE* fp)
{
	if (load == 1)
	{
		fread(&period, sizeof(uint8_t), sizeof(period), fp);
		fread(&volume, sizeof(uint8_t), sizeof(volume), fp);
		fread(&voice_volume, sizeof(uint8_t), sizeof(voice_volume), fp);
		
		fread(&sweep_step, sizeof(uint8_t), sizeof(sweep_step), fp);
		fread(&sweep_value, sizeof(uint8_t), sizeof(sweep_value), fp);
		fread(&noise_control, sizeof(uint8_t), sizeof(noise_control), fp);
		
		fread(&control, sizeof(uint8_t), sizeof(control), fp);
		fread(&output_control, sizeof(uint8_t), sizeof(output_control), fp);
		
		fread(&HVoiceCtrl, sizeof(uint8_t), sizeof(HVoiceCtrl), fp);
		fread(&HVoiceChanCtrl, sizeof(uint8_t), sizeof(HVoiceChanCtrl), fp);
		
		fread(&noise_control, sizeof(uint8_t), sizeof(noise_control), fp);
		
		fread(&sweep_8192_divider, sizeof(uint8_t), sizeof(sweep_8192_divider), fp);
		fread(&sweep_counter, sizeof(uint8_t), sizeof(sweep_counter), fp);
		fread(&SampleRAMPos, sizeof(uint8_t), sizeof(SampleRAMPos), fp);
		fread(&period_counter, sizeof(uint8_t), sizeof(period_counter), fp);
		fread(&sample_pos, sizeof(uint8_t), sizeof(sample_pos), fp);
		fread(&nreg, sizeof(uint8_t), sizeof(nreg), fp);
		
		if(sweep_8192_divider < 1) sweep_8192_divider = 1;
		for(uint_fast8_t ch = 0; ch < 4; ch++)
		{
			period[ch] &= 0x7FF;
			if(period_counter[ch] < 1) period_counter[ch] = 1;
			sample_pos[ch] &= 0x1F;
		}
	}
	else
	{
		fwrite(&period, sizeof(uint8_t), sizeof(period), fp);
		fwrite(&volume, sizeof(uint8_t), sizeof(volume), fp);
		fwrite(&voice_volume, sizeof(uint8_t), sizeof(voice_volume), fp);
		
		fwrite(&sweep_step, sizeof(uint8_t), sizeof(sweep_step), fp);
		fwrite(&sweep_value, sizeof(uint8_t), sizeof(sweep_value), fp);
		fwrite(&noise_control, sizeof(uint8_t), sizeof(noise_control), fp);
		
		fwrite(&control, sizeof(uint8_t), sizeof(control), fp);
		fwrite(&output_control, sizeof(uint8_t), sizeof(output_control), fp);
		fwrite(&noise_control, sizeof(uint8_t), sizeof(noise_control), fp);
		
		fwrite(&HVoiceCtrl, sizeof(uint8_t), sizeof(HVoiceCtrl), fp);
		fwrite(&HVoiceChanCtrl, sizeof(uint8_t), sizeof(HVoiceChanCtrl), fp);
		
		fwrite(&sweep_8192_divider, sizeof(uint8_t), sizeof(sweep_8192_divider), fp);
		fwrite(&sweep_counter, sizeof(uint8_t), sizeof(sweep_counter), fp);
		fwrite(&SampleRAMPos, sizeof(uint8_t), sizeof(SampleRAMPos), fp);
		fwrite(&period_counter, sizeof(uint8_t), sizeof(period_counter), fp);
		fwrite(&sample_pos, sizeof(uint8_t), sizeof(sample_pos), fp);
		fwrite(&nreg, sizeof(uint8_t), sizeof(nreg), fp);
	}
}

void WSwan_SoundReset(void)
{
	uint_fast8_t y;
	memset(period, 0, sizeof(period));
	memset(volume, 0, sizeof(volume));
	voice_volume = 0;
	sweep_step = 0;
	sweep_value = 0;
	noise_control = 0;
	control = 0;
	output_control = 0;

	sweep_8192_divider = 8192;
	sweep_counter = 0;
	SampleRAMPos = 0;
	for(uint_fast8_t ch = 0; ch < 4; ch++)
	{
		period_counter[ch] = 1;
	}
	memset(sample_pos, 0, sizeof(sample_pos));
	nreg = 0;

	memset(sample_cache, 0, sizeof(sample_cache));
	memset(last_val, 0, sizeof(last_val));
	last_v_val = 0;

	HyperVoice = 0;
	last_hv_val[0] = last_hv_val[1] = 0;
	HVoiceCtrl = 0;
	HVoiceChanCtrl = 0;

	for (y = 0; y < 2; y++)
	{
		Blip_Buffer_clear(&sbuf[y], 1);
	}
}
