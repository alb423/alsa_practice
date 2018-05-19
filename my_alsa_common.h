#ifndef __MY_ALSA_COMMON_H__
#define __MY_ALSA_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <alsa/asoundlib.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

extern void ShowAlsaParameters(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams);
extern int SetParametersByAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams);

#if SUPPORT_TINYALSA == 1
#include <tinyalsa/pcm.h>
extern int SetParametersByTinyAlsaConfigs(snd_pcm_t *pHandle, snd_pcm_hw_params_t *hwparams, struct pcm_config *pConfigs);
#endif


#ifdef __cplusplus
}
#endif


#endif