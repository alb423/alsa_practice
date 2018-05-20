#ifndef __MY_ALSA_COMMON_H__
#define __MY_ALSA_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MI_BUFFER_TIME 	500000              /* ring buffer length in us */
#define MI_PERIOD_TIME 	15000               /* ring buffer length in us */

#define _TRACE_ALSA_ printf("%s:%d\n", __func__, __LINE__);

#include <alsa/asoundlib.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

extern void ShowAlsaParameters(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams, snd_pcm_sw_params_t *pSwParams);
extern int SetParametersByAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams);

#if SUPPORT_TINYALSA == 1
#include <tinyalsa/pcm.h>
extern int SetParametersByTinyAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *hwparams, struct pcm_config *pConfigs);
#endif


#ifdef __cplusplus
}
#endif


#endif