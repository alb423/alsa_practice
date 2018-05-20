/*
	Usage:
		./arecorder > 1.raw

	Render recorded file
		$ ffplay -f s16le -ar 16000 -ac 2 1.raw
		$ ffplay -f s16le -ar 44.1k -ac 2 1.raw
*/

#include <stdlib.h>
#include <string.h>
#include "my_alsa_common.h"

int main()
{

	long vLoops;
	int rc;
	int vSize;
	unsigned int vVal;
	int vDirection;
	snd_pcm_uframes_t vxFrames;
	char *pBuffer;

	snd_pcm_t *pHandle;
	snd_pcm_hw_params_t *pParams;

	/* Open PCM device for playback. */
	rc = snd_pcm_open(&pHandle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&pParams);

#if SUPPORT_TINYALSA == 1
	struct pcm_config vxConfigs;
	memset(&vxConfigs, 0, sizeof(struct pcm_config));
	vxConfigs.channels = 2;
	vxConfigs.rate = 16000;
	vxConfigs.period_size = 256;
	vxConfigs.period_count = 4;
	vxConfigs.format = PCM_FORMAT_S16_LE;
	SetParametersByTinyAlsaConfigs(pHandle, pParams, &vxConfigs);
#else
	SetParametersByAlsaConfigs(pHandle, pParams);
#endif

	//ShowAlsaParameters(pHandle, pParams);

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