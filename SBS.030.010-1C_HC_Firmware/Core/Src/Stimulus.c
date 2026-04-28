#include "Stimulus.h"

#include "tim.h"

#define STIMULUS_TIM_CLK_HZ      84000000UL
#define STIMULUS_TARGET_DUTY_PC  50UL

static uint32_t g_stimulus_frequency_hz = 1000UL;
static StimulusMode_t g_stimulus_mode = STIMULUS_MODE_OPPOSED;

static HAL_StatusTypeDef Stimulus_ApplyConfig(void)
{
  uint32_t prescaler;
  uint32_t period_ticks;
  uint32_t pulse_ticks;

  if (g_stimulus_frequency_hz == 0UL)
  {
    return HAL_ERROR;
  }

  period_ticks = STIMULUS_TIM_CLK_HZ / g_stimulus_frequency_hz;
  prescaler = (STIMULUS_TIM_CLK_HZ / (g_stimulus_frequency_hz * 65536UL));
  if (prescaler > 0xFFFFUL)
  {
    return HAL_ERROR;
  }

  period_ticks = STIMULUS_TIM_CLK_HZ / ((prescaler + 1UL) * g_stimulus_frequency_hz);
  if ((period_ticks < 2UL) || (period_ticks > 65536UL))
  {
    return HAL_ERROR;
  }

  pulse_ticks = (period_ticks * STIMULUS_TARGET_DUTY_PC) / 100UL;
  if (pulse_ticks == 0UL)
  {
    pulse_ticks = 1UL;
  }
  if (pulse_ticks >= period_ticks)
  {
    pulse_ticks = period_ticks - 1UL;
  }

  __HAL_TIM_DISABLE(&htim3);
  __HAL_TIM_SET_PRESCALER(&htim3, prescaler);
  __HAL_TIM_SET_AUTORELOAD(&htim3, period_ticks - 1UL);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_ticks);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse_ticks);
  htim3.Init.Prescaler = prescaler;
  htim3.Init.Period = period_ticks - 1UL;
  htim3.Instance->CCMR1 &= ~(
      TIM_CCMR1_OC1M |
      TIM_CCMR1_OC2M |
      TIM_CCMR1_OC1PE |
      TIM_CCMR1_OC2PE);

  htim3.Instance->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;
  htim3.Instance->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);
  htim3.Instance->CCMR1 |= ((g_stimulus_mode == STIMULUS_MODE_IN_PHASE) ? 6U : 7U) << TIM_CCMR1_OC2M_Pos;

  htim3.Instance->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
  htim3.Instance->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
  htim3.Instance->CR1 |= TIM_CR1_ARPE;
  htim3.Instance->EGR = TIM_EGR_UG;

  __HAL_TIM_ENABLE(&htim3);
  return HAL_OK;
}

HAL_StatusTypeDef Stimulus_Init(void)
{
  HAL_GPIO_WritePin(SDRA_DRV_GPIO_Port, SDRA_DRV_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SDRB_DRV_GPIO_Port, SDRB_DRV_Pin, GPIO_PIN_RESET);
  return Stimulus_ApplyConfig();
}

HAL_StatusTypeDef Stimulus_Start(void)
{
  if (Stimulus_ApplyConfig() != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
  {
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef Stimulus_Stop(void)
{
  if (HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2) != HAL_OK)
  {
    return HAL_ERROR;
  }

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0U);
  HAL_GPIO_WritePin(SDRA_DRV_GPIO_Port, SDRA_DRV_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SDRB_DRV_GPIO_Port, SDRB_DRV_Pin, GPIO_PIN_RESET);

  return HAL_OK;
}

HAL_StatusTypeDef Stimulus_SetFrequencyHz(uint32_t hz)
{
  if (hz == 0UL)
  {
    return HAL_ERROR;
  }

  g_stimulus_frequency_hz = hz;
  return Stimulus_ApplyConfig();
}

HAL_StatusTypeDef Stimulus_SetMode(StimulusMode_t mode)
{
  if ((mode != STIMULUS_MODE_DISABLED) &&
      (mode != STIMULUS_MODE_IN_PHASE) &&
      (mode != STIMULUS_MODE_OPPOSED))
  {
    return HAL_ERROR;
  }

  g_stimulus_mode = mode;

  if (mode == STIMULUS_MODE_DISABLED)
  {
    return Stimulus_Stop();
  }

  return Stimulus_ApplyConfig();
}

uint32_t Stimulus_GetFrequencyHz(void)
{
  return g_stimulus_frequency_hz;
}

StimulusMode_t Stimulus_GetMode(void)
{
  return g_stimulus_mode;
}
