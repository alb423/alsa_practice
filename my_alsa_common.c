#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alsa/asoundlib.h>


int ShowAlsaParameters(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams)
{
	snd_pcm_uframes_t vxFrames;
	unsigned int vVal, vVal2;
	int vDirection;
	
	/* Display information about the PCM interface */

	printf("PCM pHandle name = '%s'\n", snd_pcm_name(pHandle));
	printf("PCM state = %s\n", snd_pcm_state_name(snd_pcm_state(pHandle)));
	
	snd_pcm_hw_params_get_access(pParams, (snd_pcm_access_t *) &vVal);
	printf("access type = %s\n", snd_pcm_access_name((snd_pcm_access_t)vVal));
	
	snd_pcm_hw_params_get_format(pParams, (snd_pcm_format_t *)&vVal);
	printf("format = '%s' (%s)\n", 
		snd_pcm_format_name((snd_pcm_format_t)vVal), 
		snd_pcm_format_description((snd_pcm_format_t)vVal));
		
	snd_pcm_hw_params_get_subformat(pParams,(snd_pcm_subformat_t *)&vVal);
	printf("subformat = '%s' (%s)\n",
		snd_pcm_subformat_name((snd_pcm_subformat_t)vVal),
		snd_pcm_subformat_description((snd_pcm_subformat_t)vVal));

	snd_pcm_hw_params_get_channels(pParams, &vVal);
	printf("channels = %d\n", vVal);

	snd_pcm_hw_params_get_rate(pParams, &vVal, &vDirection);
	printf("rate = %d bps\n", vVal);

	snd_pcm_hw_params_get_period_time(pParams, &vVal, &vDirection);
	printf("period time = %d us\n", vVal);

	snd_pcm_hw_params_get_period_size(pParams, &vxFrames, &vDirection);
	printf("period size = %d vxFrames\n", (int)vxFrames);

	snd_pcm_hw_params_get_buffer_time(pParams, &vVal, &vDirection);
	printf("buffer time = %d us\n", vVal);

	snd_pcm_hw_params_get_buffer_size(pParams, (snd_pcm_uframes_t *) &vVal);
	printf("buffer size = %d vxFrames\n", vVal);

	snd_pcm_hw_params_get_periods(pParams, &vVal, &vDirection);
	printf("periods per buffer = %d vxFrames\n", vVal);

	snd_pcm_hw_params_get_rate_numden(pParams, &vVal, &vVal2);
	printf("exact rate = %d/%d bps\n", vVal, vVal2);

	vVal = snd_pcm_hw_params_get_sbits(pParams);
	printf("significant bits = %d\n", vVal);

	//snd_pcm_hw_params_get_tick_time(pParams, &vVal, &vDirection);
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