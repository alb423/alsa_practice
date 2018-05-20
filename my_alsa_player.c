/*
	Usage:
		./aplayer < /dev/urandom

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	//vRet = snd_pcm_open(&pHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	vRet = snd_pcm_open(&pHandle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
	if (vRet < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(vRet));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&pParams);

#if SUPPORT_TINYALSA == 1
	struct pcm_config vxConfigs;
	memset(&vxConfigs, 0, sizeof(struct pcm_config));
	vxConfigs.channels = 2;
	vxConfigs.rate = 16000;
	vxConfigs.period_size = 512;//256;
	vxConfigs.period_count = 8;
	vxConfigs.format = PCM_FORMAT_S16_LE;
	SetParametersByTinyAlsaConfigs(pHandle, pParams, (struct pcm_config *)&vxConfigs);
#else
	SetParametersByAlsaConfigs(pHandle, pParams);
#endif

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
			// EPIPE means underrun
			fprintf(stderr, "underrun occurred\n");
			snd_pcm_prepare(pHandle);
		} else if (vRet < 0) {
			fprintf(stderr, "error from writei: %s\n", snd_strerror(vRet));
		}  else if (vRet != (int)vFrames) {
			fprintf(stderr, "short write, write %d frames\n", vRet);
		}
	}

	/* ================
	 * Close PCM device
	 * ================ */
	snd_pcm_drain(pHandle);
	snd_pcm_close(pHandle);
	free(pBuffer);
	return 0;
}


