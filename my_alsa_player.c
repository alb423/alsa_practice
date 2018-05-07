/*
	Usage:
		./aplayer < /dev/urandom
		
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ALSA include files
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include "my_alsa_common.h"


int main(int argc, char **argv)
{
	int vRet, vDirection, vSize, vLoops;
	unsigned int vVal;
	char *pBuffer;
	
	snd_pcm_uframes_t vFrames;	
	snd_pcm_t *pHandle;
	snd_pcm_hw_params_t *pParams;
	
	/* ================
	 * Open PCM device
	 * ================ */	
	vRet = snd_pcm_open(&pHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (vRet < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(vRet));
		exit(1);
	}
	
	
	/* ================
	 * set parameters
	 * ================ */

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&pParams);

	/* Fill it in with default vValues. */
	snd_pcm_hw_params_any(pHandle, pParams);

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(pHandle, pParams, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(pHandle, pParams, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(pHandle, pParams, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	vVal = 44100;
	snd_pcm_hw_params_set_rate_near(pHandle, pParams, &vVal, &vDirection);

	/* Set period size to 32 frames. */
	vFrames = 32;
	snd_pcm_hw_params_set_period_size_near(pHandle, pParams, &vFrames, &vDirection);
	
	/* Write the parameters to the driver */
	vRet = snd_pcm_hw_params(pHandle, pParams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	ShowAlsaParameters(pHandle, pParams);
	
	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(pParams, &vFrames, &vDirection);
	vSize = vFrames * 4; /* 2 bytes/sample, 2 channels */
	pBuffer = (char *) malloc(vSize);
	
	
	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDirection);
	/* 5 seconds in microseconds divided by
	* period time */
	vLoops = 5000000 / vVal;

	
	/* ================
	 * while loop to play audio
	 * ================ */	
	while (vLoops > 0) {
		vLoops--;
		vRet = read(0, pBuffer, vSize);
		if (vRet == 0) {
		  fprintf(stderr, "end of file on input\n");
		  break;
		} else if (vRet != vSize) {
		  fprintf(stderr, "short read: read %d bytes\n", vRet);
		}
		vRet = snd_pcm_writei(pHandle, pBuffer, vFrames);
		if (vRet == -EPIPE) {
		  /* EPIPE means underrun */
		  fprintf(stderr, "underrun occurred\n");
		  snd_pcm_prepare(pHandle);
		} else if (vRet < 0) {
		  fprintf(stderr, "error from writei: %s\n", snd_strerror(vRet));
		}  else if (vRet != (int)vFrames) {
		  fprintf(stderr, "short write, write %d frames\n", vRet);
		}
	}

	snd_pcm_drain(pHandle);
	snd_pcm_close(pHandle);
	free(pBuffer);
	
	/* ================
	 * Close PCM device
	 * ================ */		
	snd_pcm_close(pHandle);

	return 0;
}

	
