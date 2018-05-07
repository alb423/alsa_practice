#ifndef __MY_ALSA_COMMON_H__
#define __MY_ALSA_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int ShowAlsaParameters(snd_pcm_t *pHandle, snd_pcm_hw_params_t *pParams);

#ifdef __cplusplus
}
#endif


#endif