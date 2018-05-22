/*
	Usage:
		./aplayer < /dev/urandom

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "my_alsa_common.h"
int format = SND_PCM_FORMAT_S16_LE;

int main(int argc, char **argv)
{
	int vRet, vDirection, vSize, vLoops;
	unsigned int vVal;
	unsigned char *pBuffer;

	snd_pcm_uframes_t vFrames;
	snd_pcm_t *pHandle;
	snd_pcm_hw_params_t *pParams;

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
	pBuffer = (unsigned char *) malloc(vSize);


	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDirection);
	/* 5 seconds in microseconds divided by
	* period time */
	vLoops = 5000000 / vVal;

	if(MI_ACCESS_TYPE == SND_PCM_ACCESS_RW_INTERLEAVED) {
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
	}
	else {
		// reference direct_loop() in alsa-lib/test/pcm.c
		const snd_pcm_channel_area_t *my_areas;
		snd_pcm_uframes_t offset, frames, size;
		snd_pcm_sframes_t avail, commitres;
		snd_pcm_state_t state;
		int first = 1;

		state = snd_pcm_state(pHandle);
		if (state == SND_PCM_STATE_XRUN) {
			vRet = xrun_recovery(pHandle, -EPIPE);
			if (vRet < 0) {
				printf("XRUN recovery failed: %s\n", snd_strerror(vRet));
				return vRet;
			}
			first = 1;
		} else if (state == SND_PCM_STATE_SUSPENDED) {
			vRet = xrun_recovery(pHandle, -ESTRPIPE);
			if (vRet < 0) {
				printf("SUSPEND recovery failed: %s\n", snd_strerror(vRet));
				return vRet;
			}
		}

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
			vRet = read(0, pBuffer, vSize);
			if (vRet == 0) {
				fprintf(stderr, "end of file on input\n");
				break;
			} else if (vRet != vSize) {
				fprintf(stderr, "short read: read %d bytes\n", vRet);
			}

			size = vSize/4;  /* 2 bytes/sample, 2 channels */
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
					samples[0][2*i]     = pBuffer[4 * i]  & 0xff;
					samples[0][2*i + 1] = pBuffer[4 * i + 1] & 0xff;
					samples[1][2*i]     = pBuffer[4 * i + 2] & 0xff;
					samples[1][2*i + 1] = pBuffer[4 * i + 3] & 0xff;
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
		}
	}

	snd_pcm_drain(pHandle);
	snd_pcm_close(pHandle);
	free(pBuffer);
	return 0;
}


