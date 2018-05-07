/*
	Usage:
		./arecorder > 1.raw
		
	Render recorded file
		$ ffplay -f s16le -ar 44.1k -ac 2 1.raw
*/

#include <stdlib.h>
#include <string.h>
// ALSA include files
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include "my_alsa_common.h"

int main() {
	
	long vLoops;
	int rc;
	int vSize;
	snd_pcm_t *pHandle;
	snd_pcm_hw_params_t *pParams;
	unsigned int vVal;
	int vDirection;
	snd_pcm_uframes_t vxFrames;
	char *pBuffer;

	/* Open PCM device for playback. */
	rc = snd_pcm_open(&pHandle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&pParams);

	/* Fill it in with default values. */
	snd_pcm_hw_params_any(pHandle, pParams);

	/* Set the desired hardware parameters. */

	/* Interleaved mode */
	snd_pcm_hw_params_set_access(pHandle, pParams, SND_PCM_ACCESS_RW_INTERLEAVED);

	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(pHandle, pParams, SND_PCM_FORMAT_S16_LE);

	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(pHandle, pParams, 2);

	/* 44100 bits/second sampling rate (CD quality) */
	vVal = 44100;
	snd_pcm_hw_params_set_rate_near(pHandle, pParams, &vVal, &vDirection);

	/* Set period vSize to 32 vxFrames. */
	vxFrames = 32;
	snd_pcm_hw_params_set_period_size_near(pHandle, pParams, &vxFrames, &vDirection);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(pHandle, pParams);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* Use a pBuffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(pParams, &vxFrames, &vDirection);
	vSize = vxFrames * 4; /* 2 bytes/sample, 2 channels */
	pBuffer = (char *) malloc(vSize);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDirection);
	/* 5 seconds in microseconds divided by
	* period time */
	vLoops = 5000000 / vVal;

	while (vLoops > 0) {
		vLoops--;
		rc = snd_pcm_readi(pHandle, pBuffer, vxFrames);
		if (rc == -EPIPE) {
		  /* EPIPE means overrun */
		  fprintf(stderr, "overrun occurred\n");
		  snd_pcm_prepare(pHandle);
		} else if (rc < 0) {
		  fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)vxFrames) {
		  fprintf(stderr, "short read, read %d frames\n", rc);
		}
		rc = write(1, pBuffer, vSize);
		if (rc != vSize)
		  fprintf(stderr, "short write: wrote %d bytes\n", rc);
	}

	snd_pcm_drain(pHandle);
	snd_pcm_close(pHandle);
	free(pBuffer);

	return 0;
}