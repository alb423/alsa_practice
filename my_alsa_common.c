#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_alsa_common.h"
/*
https://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
*/

#if SUPPORT_TINYALSA == 1
int pcm_format_to_alsa(enum pcm_format format)
{
	switch (format) {

	case PCM_FORMAT_S8:
		return SND_PCM_FORMAT_S8;

	default:
	case PCM_FORMAT_S16_LE:
		return SND_PCM_FORMAT_S16_LE;
	case PCM_FORMAT_S16_BE:
		return SND_PCM_FORMAT_S16_BE;

	case PCM_FORMAT_S24_LE:
		return SND_PCM_FORMAT_S24_LE;
	case PCM_FORMAT_S24_BE:
		return SND_PCM_FORMAT_S24_BE;

	case PCM_FORMAT_S24_3LE:
		return SND_PCM_FORMAT_S24_3LE;
	case PCM_FORMAT_S24_3BE:
		return SND_PCM_FORMAT_S24_3BE;

	case PCM_FORMAT_S32_LE:
		return SND_PCM_FORMAT_S32_LE;
	case PCM_FORMAT_S32_BE:
		return SND_PCM_FORMAT_S32_BE;
	};
}

int SetParametersByTinyAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *hwparams, struct pcm_config *pConfigsIn)
{
	int vRet = 0;
	int vDir = 0;
	snd_pcm_format_t    vFormat;
	snd_pcm_uframes_t   vFrames;
	snd_pcm_sw_params_t *swparams = NULL;
	int period_event = 0;
	int buffer_size = 1024;
	int period_size = 4;
	unsigned int buffer_time = MI_BUFFER_TIME;
	unsigned int period_time = MI_PERIOD_TIME;

	// tinyalsa configuration
	struct pcm_config *pConfigs = (struct pcm_config *)pConfigsIn;

	if (!pConfigs)
		return -1;

	buffer_size = pConfigs->period_size;
	period_size = pConfigs->period_size;

	/* Fill it in with default vValues. */
	snd_pcm_hw_params_any(pHandle, hwparams);

	/* Interleaved mode */
	vRet = snd_pcm_hw_params_set_access(pHandle, hwparams, MI_ACCESS_TYPE);
	if (vRet < 0) {
		fprintf(stderr, "Error setting access : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* Signed 16-bit little-endian format */
	vFormat = pcm_format_to_alsa(pConfigs->format);
	vRet = snd_pcm_hw_params_set_format(pHandle, hwparams, vFormat); // SND_PCM_FORMAT_S16_LE
	if (vRet < 0) {
		fprintf(stderr, "Error setting format : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* Two channels (stereo) */
	vRet = snd_pcm_hw_params_set_channels(pHandle, hwparams, pConfigs->channels); // 2
	if (vRet < 0) {
		fprintf(stderr, "Error setting channels : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* 16000 bits/second sampling rate (CD quality) */
	vRet = snd_pcm_hw_params_set_rate_near(pHandle, hwparams, &pConfigs->rate, &vDir); // 16000
	if (vRet < 0) {
		fprintf(stderr, "Error setting rate : %s\n", snd_strerror(vRet));
		return (-1);
	}

	if(MI_BUFFER_SET_METHOD == MI_BUFFER_SET_BY_PERIOD_TIME3)
	{
		vRet = snd_pcm_hw_params_set_buffer_time_near(pHandle, hwparams, &buffer_time, &vDir);
		if (vRet < 0) {
			printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(vRet));
			return vRet;
		}
		vRet = snd_pcm_hw_params_get_buffer_size(hwparams, &vFrames);
		if (vRet < 0) {
			printf("Unable to get buffer size for playback: %s\n", snd_strerror(vRet));
			return vRet;
		}
		buffer_size = vFrames;

		vRet = snd_pcm_hw_params_set_period_time_near(pHandle, hwparams, &period_time, &vDir);
		if (vRet < 0) {
			printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(vRet));
			return vRet;
		}
		vRet = snd_pcm_hw_params_get_period_size(hwparams, &vFrames, &vDir);
		if (vRet < 0) {
			printf("Unable to get period size for playback: %s\n", snd_strerror(vRet));
			return vRet;
		}
		period_size = vFrames;
	}
	else {
		/* Set period size to 256 frames. */
		vFrames = 256;
		vRet = snd_pcm_hw_params_set_period_size_near(pHandle, hwparams, &vFrames, &vDir);
		//snd_pcm_hw_params_set_period_size(pHandle, hwparams, pConfigs->period_size, vDir);	// 1024
		//snd_pcm_hw_params_set_periods(pHandle, hwparams, pConfigs->period_count, vDir);  // 4
		if (vRet < 0) {
			fprintf(stderr, "Error setting period_size : %s\n", snd_strerror(vRet));
			return (-1);
		}
		else {
			/* Set period size to 256 frames. */
			vFrames = 256;
			vRet = snd_pcm_hw_params_set_period_size_near(pHandle, hwparams, &vFrames, &vDir);
			//snd_pcm_hw_params_set_period_size(pHandle, hwparams, pConfigs->period_size, vDir);	// 1024
			//snd_pcm_hw_params_set_periods(pHandle, hwparams, pConfigs->period_count, vDir);  // 4
			if (vRet < 0) {
				fprintf(stderr, "Error setting period_size : %s\n", snd_strerror(vRet));
				return (-1);
			}

			/* NOTE:  here may cause underrun*/
			/* Set buffer size (in frames). The resulting latency is given by */
			/* latency = periodsize * periods / (rate * bytes_per_frame)     */
			vRet = snd_pcm_hw_params_set_buffer_size(pHandle, hwparams, (pConfigs->period_size * pConfigs->period_count) >> 2);
			if (vRet < 0) {
				fprintf(stderr, "Error setting buffer_size : %s\n", snd_strerror(vRet));
				return (-1);
			}
		}
	}

	/* Write the parameters to the driver */
	vRet = snd_pcm_hw_params(pHandle, hwparams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	// Below setting may cause segmentation fault
	snd_pcm_sw_params_alloca(&swparams);

	/* get the current swparams */
	vRet = snd_pcm_sw_params_current(pHandle, swparams);
	if (vRet < 0) {
		printf("Unable to determine current swparams for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}


	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */

	vRet = snd_pcm_sw_params_set_start_threshold(pHandle, swparams, (buffer_size / period_size) * period_size);
	//vRet = snd_pcm_sw_params_set_start_threshold(pHandle, swparams, 1);
	if (vRet < 0) {
		printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	vRet = snd_pcm_sw_params_set_avail_min(pHandle, swparams, period_event ? buffer_size : period_size);
	if (vRet < 0) {
		printf("Unable to set avail min for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}

	/* enable period events when requested */
	if (period_event) {
		vRet = snd_pcm_sw_params_set_period_event(pHandle, swparams, 1);
		if (vRet < 0) {
			printf("Unable to set period event: %s\n", snd_strerror(vRet));
			return vRet;
		}
	}

	/* Write the parameters to the driver */
	vRet = snd_pcm_sw_params(pHandle, swparams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set sw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	ShowAlsaParameters(pHandle, hwparams, swparams);
	return 0;
}


#else

int SetParametersByAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams)
{
	int vRet, vDir;
	unsigned int vVal;

	int period_event = 0;
	int buffer_size = 512;
	int period_size = 512;
	unsigned int buffer_time = MI_BUFFER_TIME;
	unsigned int period_time = MI_PERIOD_TIME;

	snd_pcm_uframes_t vFrames;
	snd_pcm_hw_params_t *hwparams = pParams;
	snd_pcm_sw_params_t *swparams = NULL;

	_TRACE_ALSA_;
	/* Fill it in with default vValues. */
	snd_pcm_hw_params_any(pHandle, hwparams);

	/* Interleaved mode */
	vRet = snd_pcm_hw_params_set_access(pHandle, hwparams, MI_ACCESS_TYPE);
	if (vRet < 0) {
		fprintf(stderr, "Error setting access : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* Signed 16-bit little-endian format */
	vRet = snd_pcm_hw_params_set_format(pHandle, hwparams, SND_PCM_FORMAT_S16_LE);
	if (vRet < 0) {
		fprintf(stderr, "Error setting format : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* Two channels (stereo) */
	vRet = snd_pcm_hw_params_set_channels(pHandle, hwparams, 2);
	if (vRet < 0) {
		fprintf(stderr, "Error setting channels : %s\n", snd_strerror(vRet));
		return (-1);
	}

	/* 16000 bits/second sampling rate (CD quality) */
	vVal = 16000;
	vRet = snd_pcm_hw_params_set_rate_near(pHandle, hwparams, &vVal, &vDir);
	if (vRet < 0) {
		fprintf(stderr, "Error setting rate : %s\n", snd_strerror(vRet));
		return (-1);
	}

	if(MI_BUFFER_SET_METHOD == MI_BUFFER_SET_BY_PERIOD_TIME)
	{
		vRet = snd_pcm_hw_params_set_buffer_time_near(pHandle, hwparams, &buffer_time, &vDir);
		if (vRet < 0) {
			printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(vRet));
			return vRet;
		}
		vRet = snd_pcm_hw_params_get_buffer_size(hwparams, &vFrames);
		if (vRet < 0) {
			printf("Unable to get buffer size for playback: %s\n", snd_strerror(vRet));
			return vRet;
		}
		buffer_size = vFrames;

		vRet = snd_pcm_hw_params_set_period_time_near(pHandle, hwparams, &period_time, &vDir);
		if (vRet < 0) {
			printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(vRet));
			return vRet;
		}
		vRet = snd_pcm_hw_params_get_period_size(hwparams, &vFrames, &vDir);
		if (vRet < 0) {
			printf("Unable to get period size for playback: %s\n", snd_strerror(vRet));
			return vRet;
		}
		period_size = vFrames;
	}
	else {
		/* Set buffer size (in frames). The resulting latency is given by */
		/* latency = periodsize * periods / (rate * bytes_per_frame)     */
		vRet = snd_pcm_hw_params_set_buffer_size(pHandle, hwparams, period_size * 8);
		if (vRet < 0) {
			fprintf(stderr, "Error setting buffer_size : %s\n", snd_strerror(vRet));
			return (-1);
		}

		/* Set period size to 256 frames. */
		vFrames = 256;
		vRet = snd_pcm_hw_params_set_period_size_near(pHandle, hwparams, &vFrames, &vDir);
		if (vRet < 0) {
			fprintf(stderr, "Error setting period_size : %s\n", snd_strerror(vRet));
			return (-1);
		}
	}

	/* Write the parameters to the driver */
	vRet = snd_pcm_hw_params(pHandle, hwparams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	/* Write the parameters to the driver */
	vRet = snd_pcm_hw_params(pHandle, hwparams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	// Below setting may cause segmentation fault
	snd_pcm_sw_params_alloca(&swparams);

	/* get the current swparams */
	vRet = snd_pcm_sw_params_current(pHandle, swparams);
	if (vRet < 0) {
		printf("Unable to determine current swparams for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}


	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */

	vRet = snd_pcm_sw_params_set_start_threshold(pHandle, swparams, (buffer_size / period_size) * period_size);
	//vRet = snd_pcm_sw_params_set_start_threshold(pHandle, swparams, 1);
	if (vRet < 0) {
		printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	vRet = snd_pcm_sw_params_set_avail_min(pHandle, swparams, period_event ? buffer_size : period_size);
	if (vRet < 0) {
		printf("Unable to set avail min for playback: %s\n", snd_strerror(vRet));
		return vRet;
	}

	/* enable period events when requested */
	if (period_event) {
		vRet = snd_pcm_sw_params_set_period_event(pHandle, swparams, 1);
		if (vRet < 0) {
			printf("Unable to set period event: %s\n", snd_strerror(vRet));
			return vRet;
		}
	}

	/* Write the parameters to the driver */
	vRet = snd_pcm_sw_params(pHandle, swparams);
	if (vRet < 0) {
		fprintf(stderr, "unable to set sw parameters: %s\n", snd_strerror(vRet));
		exit(1);
	}

	ShowAlsaParameters(pHandle, pParams, NULL);
	return 0;
}
#endif


int xrun_recovery(snd_pcm_t *handle, int err)
{
	printf("stream recovery\n");
	if (err == -EPIPE) {    /* under-run */
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
		return 0;
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);       /* wait until the suspend flag is released */
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return 0;
	}
	return err;
}

void ShowAlsaParameters(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams, snd_pcm_sw_params_t *pSwParams)
{
	snd_pcm_uframes_t vxFrames;
	snd_pcm_tstamp_t vxTimeStamp;
	//snd_pcm_tstamp_type_t vxTimeStampType;
	unsigned int vVal, vVal2;
	int vDir;

	/* Display information about the PCM interface */
	if (pParams) {
		printf("PCM pHandle name = '%s'\n", snd_pcm_name(pHandle));
		printf("PCM state = %s\n", snd_pcm_state_name(snd_pcm_state(pHandle)));

		snd_pcm_hw_params_get_access(pParams, (snd_pcm_access_t *) &vVal);
		printf("access type = %s\n", snd_pcm_access_name((snd_pcm_access_t)vVal));

		snd_pcm_hw_params_get_format(pParams, (snd_pcm_format_t *)&vVal);
		printf("format = '%s' (%s)\n",
		       snd_pcm_format_name((snd_pcm_format_t)vVal),
		       snd_pcm_format_description((snd_pcm_format_t)vVal));

		snd_pcm_hw_params_get_subformat(pParams, (snd_pcm_subformat_t *)&vVal);
		printf("subformat = '%s' (%s)\n",
		       snd_pcm_subformat_name((snd_pcm_subformat_t)vVal),
		       snd_pcm_subformat_description((snd_pcm_subformat_t)vVal));

		snd_pcm_hw_params_get_channels(pParams, &vVal);
		printf("channels = %d\n", vVal);

		snd_pcm_hw_params_get_rate(pParams, &vVal, &vDir);
		printf("rate = %d bps\n", vVal);

		snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDir);
		printf("period time = %d us\n", vVal);

		snd_pcm_hw_params_get_period_size(pParams, &vxFrames, &vDir);
		printf("period size = %d vxFrames\n", (int)vxFrames);

		snd_pcm_hw_params_get_buffer_time(pParams, &vVal, &vDir);
		printf("buffer time = %d us\n", vVal);

		snd_pcm_hw_params_get_buffer_size(pParams, (snd_pcm_uframes_t *) &vVal);
		printf("buffer size = %d vxFrames\n", vVal);

		snd_pcm_hw_params_get_periods(pParams, &vVal, &vDir);
		printf("periods per buffer = %d vxFrames\n", vVal);

		snd_pcm_hw_params_get_rate_numden(pParams, &vVal, &vVal2);
		printf("exact rate = %d/%d bps\n", vVal, vVal2);

		vVal = snd_pcm_hw_params_get_sbits(pParams);
		printf("significant bits = %d\n", vVal);

		//snd_pcm_hw_params_get_tick_time(pParams, &vVal, &vDir);
		//printf("tick time = %d us\n", vVal);

		vVal = snd_pcm_hw_params_is_batch(pParams);
		printf("is batch = %d\n", vVal);

		vVal = snd_pcm_hw_params_is_block_transfer(pParams);
		printf("is block transfer = %d\n", vVal);

		vVal = snd_pcm_hw_params_is_double(pParams);
		printf("is double = %d\n", vVal);

		vVal = snd_pcm_hw_params_is_half_duplex(pParams);
		printf("is half duplex = %d\n", vVal);

		vVal = snd_pcm_hw_params_is_joint_duplex(pParams);
		printf("is joint duplex = %d\n", vVal);

		vVal = snd_pcm_hw_params_can_overrange(pParams);
		printf("can overrange = %d\n", vVal);

		vVal = snd_pcm_hw_params_can_mmap_sample_resolution(pParams);
		printf("can mmap = %d\n", vVal);

		vVal = snd_pcm_hw_params_can_pause(pParams);
		printf("can pause = %d\n", vVal);

		vVal = snd_pcm_hw_params_can_resume(pParams);
		printf("can resume = %d\n", vVal);

		vVal = snd_pcm_hw_params_can_sync_start(pParams);
		printf("can sync start = %d\n", vVal);
	}

	// software params
	if (pSwParams) {
		snd_pcm_sw_params_get_avail_min(pSwParams, &vxFrames);
		printf("minimum available frames = %d vxFrames\n", (int)vxFrames);

		snd_pcm_sw_params_get_boundary(pSwParams, &vxFrames);
		printf("boundary in frames = %d vxFrames\n", (int)vxFrames);

		snd_pcm_sw_params_get_period_event(pSwParams, (int *)&vVal);
		printf("period event state = %d\n", vVal);

		snd_pcm_sw_params_get_silence_size(pSwParams, &vxFrames);
		printf("silence size in frames = %d\n", (int)vxFrames);

		snd_pcm_sw_params_get_silence_threshold(pSwParams, &vxFrames);
		printf("silence threshold in frames = %d\n", (int)vxFrames);

		snd_pcm_sw_params_get_start_threshold(pSwParams, &vxFrames);
		printf("start threshold in frames = %d\n", (int)vxFrames);

		snd_pcm_sw_params_get_stop_threshold(pSwParams, &vxFrames);
		printf("stop threshold in frames = %d\n", (int)vxFrames);

		snd_pcm_sw_params_get_tstamp_mode(pSwParams, &vxTimeStamp);
		printf("timestamp val = %d\n", (int)vxTimeStamp);
		//snd_pcm_sw_params_get_tstamp_type(pSwParams, &vxTimeStampType);
		//printf("timestamp type = %d\n", (int)vxTimeStampType);
	}
}
