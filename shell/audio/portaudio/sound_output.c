#include <portaudio.h>
#include <stdint.h>
#include <stdio.h>

static PaStream *apu_stream;

#ifdef CALLBACK_AUDIO
extern EmulateSpecStruct spec;
static int patestCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData )
{
    /* Cast data passed through stream to our structure. */
    uint16_t *out = (uint16_t*)spec.SoundBuf;
    int32_t i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    
	/*for (i = 0; i < spec.SoundBufSize; i++) 
	{
		out[i * 2] = spec.SoundBuf[i];
		out[i * 2 + 1] = spec.SoundBuf[i];
	}*/
	
    return 0;
}
#endif

uint32_t Audio_Init()
{
	Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		printf("No sound output\n");
		return EXIT_FAILURE;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
#ifdef CALLBACK_AUDIO
	Pa_OpenStream( &apu_stream, NULL, &outputParameters, SOUND_OUTPUT_FREQUENCY, 1024, paNoFlag, patestCallback, NULL);
#else
	Pa_OpenStream( &apu_stream, NULL, &outputParameters, SOUND_OUTPUT_FREQUENCY, 1024, paNoFlag, NULL, NULL);
#endif
	Pa_StartStream( apu_stream );
	
	return 0;
}

#ifndef CALLBACK_AUDIO
void Audio_Write(int16_t* restrict buffer, uint32_t buffer_size)
{
	Pa_WriteStream( apu_stream, buffer, buffer_size);
}
#endif
