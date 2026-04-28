#ifndef __STIMULUS_H__
#define __STIMULUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  STIMULUS_MODE_DISABLED = 0,
  STIMULUS_MODE_IN_PHASE,
  STIMULUS_MODE_OPPOSED
} StimulusMode_t;

HAL_StatusTypeDef Stimulus_Init(void);
HAL_StatusTypeDef Stimulus_Start(void);
HAL_StatusTypeDef Stimulus_Stop(void);
HAL_StatusTypeDef Stimulus_SetFrequencyHz(uint32_t hz);
HAL_StatusTypeDef Stimulus_SetMode(StimulusMode_t mode);
uint32_t Stimulus_GetFrequencyHz(void);
StimulusMode_t Stimulus_GetMode(void);

#ifdef __cplusplus
}
#endif

#endif /* __STIMULUS_H__ */
