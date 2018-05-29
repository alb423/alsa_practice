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
	int vRet, vSize;
	unsigned int vVal;
	int vDirection;
	snd_pcm_uframes_t vFrames;
	char pCard[] = "default";
	//char pCard[] = "plughw:0,0";
	char *pBuffer;

	snd_pcm_t *pHandle;
	snd_pcm_hw_params_t *pParams;

	/* Open PCM device for playback. */
	vRet = snd_pcm_open(&pHandle, pCard, SND_PCM_STREAM_CAPTURE, 0);
	if (vRet < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(vRet));
		exit(1);
	}
	SetAlsaMasterVolume(pCard, 100);


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


	/* Use a pBuffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(pParams, &vFrames, &vDirection);
	vSize = vFrames * 4; /* 2 bytes/sample, 2 channels */
	pBuffer = (char *) malloc(vSize);

	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDirection);
	/* 5 seconds in microseconds divided by
	* period time */
	vLoops = 5000000 / vVal;

#if MI_ACCESS_TYPE == MI_ACCESS_TYPE_RW_INTERLEAVED
	while (vLoops > 0) {
		vLoops--;
		vRet = snd_pcm_readi(pHandle, pBuffer, vFrames);
		if (vRet == -EPIPE) {
			/* EPIPE means overrun */
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(pHandle);
		} else if (vRet < 0) {
			fprintf(stderr, "error from read: %s\n", snd_strerror(vRet));
		} else if (vRet != (int)vFrames) {
			fprintf(stderr, "short read, read %d frames\n", vRet);
		}
		vRet = write(1, pBuffer, vSize);
		if (vRet != vSize)
			fprintf(stderr, "short write: wrote %d bytes\n", vRet);
	}
#else
	// reference direct_loop() in alsa-lib/test/pcm.c

	snd_pcm_sframes_t avail;
	snd_pcm_uframes_t size;
	int first = 1;
#if MMAP_ZERO_COPY == 1
	const snd_pcm_channel_area_t *my_areas;
	snd_pcm_uframes_t offset, frames;
	snd_pcm_sframes_t commitres;
#endif
	while (vLoops > 0) {
		avail = snd_pcm_avail_update(pHandle);
		if (avail < 0) {
			vRet = xrun_recovery(pHandle, avail);
			if (vRet < 0) {
				printf("avail update failed: %s\n", snd_strerror(vRet));
				return vRet;
			}
			first = 1;
			continue;
		}
		if (avail < vSize) {
			if (first) {
				first = 0;
				vRet = snd_pcm_start(pHandle);
				if (vRet < 0) {
					printf("Start error: %s\n", snd_strerror(vRet));
					exit(EXIT_FAILURE);
				}
			} else {
				vRet = snd_pcm_wait(pHandle, -1);
				if (vRet < 0) {
					if ((vRet = xrun_recovery(pHandle, vRet)) < 0) {
						printf("snd_pcm_wait error: %s\n", snd_strerror(vRet));
						exit(EXIT_FAILURE);
					}
					first = 1;
				}
			}
			continue;
		}

		//printf("avail = %ld, vSize = %d\n", avail, vSize);
		vLoops--;

		size = vSize / 4; /* 2 bytes/sample, 2 channels */

#if MMAP_ZERO_COPY == 0
		snd_pcm_mmap_readi(pHandle, pBuffer, size);
#else
		while (size > 0) {
			frames = size;
			vRet = snd_pcm_mmap_begin(pHandle, &my_areas, &offset, &frames);
			if (vRet < 0) {
				if ((vRet = xrun_recovery(pHandle, vRet)) < 0) {
					printf("MMAP begin avail error: %s\n", snd_strerror(vRet));
					exit(EXIT_FAILURE);
				}
				first = 1;
			}


			int i;
			int steps[2];
			unsigned char *samples[2];
			samples[0] = /*(signed short *)*/(((unsigned char *)my_areas[0].addr) + (my_areas[0].first / 8));
			samples[1] = /*(signed short *)*/(((unsigned char *)my_areas[1].addr) + (my_areas[1].first / 8));
			steps[0] = my_areas[0].step / 8;
			steps[1] = my_areas[1].step / 8;
			samples[0] += offset * steps[0];
			samples[1] += offset * steps[1];
			for (i = 0; i < frames; i++) {
				pBuffer[4 * i]     = samples[0][2 * i];
				pBuffer[4 * i + 1] = samples[0][2 * i + 1] ;
				pBuffer[4 * i + 2] = samples[1][2 * i];
				pBuffer[4 * i + 3] = samples[1][2 * i + 1];
			}

			commitres = snd_pcm_mmap_commit(pHandle, offset, frames);
			if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
				if ((vRet = xrun_recovery(pHandle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
					printf("MMAP commit error: %s\n", snd_strerror(vRet));
					exit(EXIT_FAILURE);
				}
				first = 1;
			}
			size -= frames;
		}
#endif
		vRet = write(1, pBuffer, vSize);
		if (vRet != vSize)
			fprintf(stderr, "short write: wrote %d bytes\n", vRet);
	}
#endif

	snd_pcm_drain(pHandle);
	snd_pcm_close(pHandle);
	free(pBuffer);
	return 0;
}