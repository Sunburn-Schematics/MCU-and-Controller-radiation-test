#include "pwm_capture_drv.h"

#include "stm32f4xx_hal.h"
#include "tim.h"

#include <stddef.h>
#include <string.h>

#define PWM_CAPTURE_BURST_SAMPLES      (16U)
#define PWM_CAPTURE_MIN_PERIOD_SAMPLES (1U)
#define PWM_CAPTURE_BURST_TIMEOUT_MS   (50U)

#define PWM_CAPTURE_JOB_DONE_RISE      (1UL << 0)
#define PWM_CAPTURE_JOB_DONE_FALL      (1UL << 1)

typedef struct
{
    pwm_capture_signal_t signal;
    uint32_t required_mask;
    volatile uint32_t done_mask;
    volatile bool dma_error;
    bool armed;
    bool completed_once;
    uint32_t start_ms;
    pwm_capture_result_t result;
} pwm_capture_job_t;

static uint16_t s_me_rising_ticks[PWM_CAPTURE_BURST_SAMPLES];
static uint16_t s_me_falling_ticks[PWM_CAPTURE_BURST_SAMPLES];
static uint32_t s_mf_rising_ticks[PWM_CAPTURE_BURST_SAMPLES];
static uint32_t s_mf_falling_ticks[PWM_CAPTURE_BURST_SAMPLES];
static uint16_t s_gate_rising_ticks[PWM_CAPTURE_BURST_SAMPLES];

static pwm_capture_job_t s_jobs[PWM_CAPTURE_SIGNAL_COUNT] = {
    {
        .signal = PWM_CAPTURE_SIGNAL_LTC3901_ME,
        .required_mask = PWM_CAPTURE_JOB_DONE_RISE | PWM_CAPTURE_JOB_DONE_FALL,
    },
    {
        .signal = PWM_CAPTURE_SIGNAL_LTC3901_MF,
        .required_mask = PWM_CAPTURE_JOB_DONE_RISE | PWM_CAPTURE_JOB_DONE_FALL,
    },
    {
        .signal = PWM_CAPTURE_SIGNAL_LT8316_GATE,
        .required_mask = PWM_CAPTURE_JOB_DONE_RISE,
    },
};

static bool s_initialized;
static bool s_repeat_enabled;

static uint32_t pwm_capture_drv_get_tim_apb1_clock_hz(void)
{
    uint32_t pclk1_hz;

    pclk1_hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
    {
        pclk1_hz *= 2U;
    }

    return pclk1_hz;
}

static uint32_t pwm_capture_drv_get_tick_hz(const TIM_HandleTypeDef *htim)
{
    uint32_t timer_clock_hz;

    if (htim == NULL)
    {
        return 0U;
    }

    timer_clock_hz = pwm_capture_drv_get_tim_apb1_clock_hz();
    return timer_clock_hz / (htim->Init.Prescaler + 1U);
}

static void pwm_capture_drv_reset_result(pwm_capture_result_t *result)
{
    if (result == NULL)
    {
        return;
    }

    result->frequency_hz = 0U;
    result->period_ticks_avg = 0U;
    result->high_ticks_avg = 0U;
    result->duty_pct_x100 = 0U;
    result->valid_samples = 0U;
    result->has_duty_cycle = false;
    result->data_valid = false;
}

static void pwm_capture_drv_invalidate_job(pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return;
    }

    pwm_capture_drv_reset_result(&job->result);
}

static void pwm_capture_drv_reset_jobs(void)
{
    uint32_t job_index;

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        s_jobs[job_index].done_mask = 0U;
        s_jobs[job_index].dma_error = false;
        s_jobs[job_index].armed = false;
        s_jobs[job_index].completed_once = false;
        s_jobs[job_index].start_ms = 0U;
        pwm_capture_drv_reset_result(&s_jobs[job_index].result);
    }
}

static void pwm_capture_drv_clear_job_buffers(const pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return;
    }

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        memset(s_me_rising_ticks, 0, sizeof(s_me_rising_ticks));
        memset(s_me_falling_ticks, 0, sizeof(s_me_falling_ticks));
        break;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        memset(s_mf_rising_ticks, 0, sizeof(s_mf_rising_ticks));
        memset(s_mf_falling_ticks, 0, sizeof(s_mf_falling_ticks));
        break;

    case PWM_CAPTURE_SIGNAL_LT8316_GATE:
        memset(s_gate_rising_ticks, 0, sizeof(s_gate_rising_ticks));
        break;

    default:
        break;
    }
}

static void pwm_capture_drv_stop_job(const pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return;
    }

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        (void)HAL_TIM_IC_Stop_DMA(&htim4, TIM_CHANNEL_1);
        (void)HAL_TIM_IC_Stop_DMA(&htim4, TIM_CHANNEL_2);
        break;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        (void)HAL_TIM_IC_Stop_DMA(&htim2, TIM_CHANNEL_1);
        (void)HAL_TIM_IC_Stop_DMA(&htim2, TIM_CHANNEL_2);
        break;

    case PWM_CAPTURE_SIGNAL_LT8316_GATE:
        (void)HAL_TIM_IC_Stop_DMA(&htim4, TIM_CHANNEL_3);
        break;

    default:
        break;
    }
}

static DMA_HandleTypeDef *pwm_capture_drv_get_rise_dma(const pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return NULL;
    }

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        return &hdma_tim4_ch1;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        return &hdma_tim2_ch1;

    case PWM_CAPTURE_SIGNAL_LT8316_GATE:
        return &hdma_tim4_ch3;

    default:
        return NULL;
    }
}

static DMA_HandleTypeDef *pwm_capture_drv_get_fall_dma(const pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return NULL;
    }

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        return &hdma_tim4_ch2;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        return &hdma_tim2_ch2;

    default:
        return NULL;
    }
}

static uint32_t pwm_capture_drv_get_captured_sample_count(DMA_HandleTypeDef *hdma)
{
    uint32_t remaining;

    if ((hdma == NULL) || (hdma->Instance == NULL))
    {
        return 0U;
    }

    remaining = __HAL_DMA_GET_COUNTER(hdma);
    if (remaining > PWM_CAPTURE_BURST_SAMPLES)
    {
        remaining = PWM_CAPTURE_BURST_SAMPLES;
    }

    return PWM_CAPTURE_BURST_SAMPLES - remaining;
}

static uint8_t pwm_capture_drv_compute_period_stats_u16(const uint16_t *rising_ticks,
                                                        uint32_t rising_count,
                                                        uint64_t *period_sum_out,
                                                        uint32_t *avg_period_ticks_out)
{
    uint64_t period_sum;
    uint32_t i;
    uint8_t valid_samples;

    if ((rising_ticks == NULL) || (period_sum_out == NULL) || (avg_period_ticks_out == NULL) || (rising_count < 2U))
    {
        return 0U;
    }

    period_sum = 0ULL;
    valid_samples = 0U;

    for (i = 0U; (i + 1U) < rising_count; ++i)
    {
        uint16_t period_ticks;

        period_ticks = (uint16_t)(rising_ticks[i + 1U] - rising_ticks[i]);
        if (period_ticks == 0U)
        {
            continue;
        }

        period_sum += (uint64_t)period_ticks;
        valid_samples++;
    }

    if (valid_samples == 0U)
    {
        return 0U;
    }

    *period_sum_out = period_sum;
    *avg_period_ticks_out = (uint32_t)((period_sum + ((uint64_t)valid_samples / 2ULL)) / (uint64_t)valid_samples);
    return valid_samples;
}

static uint8_t pwm_capture_drv_compute_period_stats_u32(const uint32_t *rising_ticks,
                                                        uint32_t rising_count,
                                                        uint64_t *period_sum_out,
                                                        uint32_t *avg_period_ticks_out)
{
    uint64_t period_sum;
    uint32_t i;
    uint8_t valid_samples;

    if ((rising_ticks == NULL) || (period_sum_out == NULL) || (avg_period_ticks_out == NULL) || (rising_count < 2U))
    {
        return 0U;
    }

    period_sum = 0ULL;
    valid_samples = 0U;

    for (i = 0U; (i + 1U) < rising_count; ++i)
    {
        uint32_t period_ticks;

        period_ticks = rising_ticks[i + 1U] - rising_ticks[i];
        if (period_ticks == 0U)
        {
            continue;
        }

        period_sum += (uint64_t)period_ticks;
        valid_samples++;
    }

    if (valid_samples == 0U)
    {
        return 0U;
    }

    *period_sum_out = period_sum;
    *avg_period_ticks_out = (uint32_t)((period_sum + ((uint64_t)valid_samples / 2ULL)) / (uint64_t)valid_samples);
    return valid_samples;
}

static bool pwm_capture_drv_process_pwm_u16(const uint16_t *rising_ticks,
                                            uint32_t rising_count,
                                            const uint16_t *falling_ticks,
                                            uint32_t falling_count,
                                            uint32_t tick_hz,
                                            pwm_capture_result_t *result_out)
{
    uint64_t period_sum;
    uint64_t high_sum;
    uint32_t avg_period_ticks;
    uint32_t avg_high_ticks;
    uint32_t i;
    uint8_t valid_period_samples;
    uint8_t best_duty_samples;
    uint8_t best_offset;

    if ((rising_ticks == NULL) || (falling_ticks == NULL) || (result_out == NULL) || (tick_hz == 0U))
    {
        return false;
    }

    valid_period_samples = pwm_capture_drv_compute_period_stats_u16(rising_ticks,
                                                                    rising_count,
                                                                    &period_sum,
                                                                    &avg_period_ticks);
    if (valid_period_samples < PWM_CAPTURE_MIN_PERIOD_SAMPLES)
    {
        return false;
    }

    if (avg_period_ticks == 0U)
    {
        return false;
    }

    pwm_capture_drv_reset_result(result_out);
    result_out->frequency_hz = (uint32_t)(((uint64_t)tick_hz + ((uint64_t)avg_period_ticks / 2ULL)) /
                                          (uint64_t)avg_period_ticks);
    result_out->period_ticks_avg = avg_period_ticks;
    result_out->valid_samples = valid_period_samples;
    result_out->data_valid = true;

    best_duty_samples = 0U;
    best_offset = 0U;

    for (best_offset = 0U; best_offset <= 1U; ++best_offset)
    {
        uint8_t duty_samples;
        uint32_t sample_limit;

        if (falling_count <= best_offset)
        {
            continue;
        }

        sample_limit = rising_count - 1U;
        if ((falling_count - best_offset) < sample_limit)
        {
            sample_limit = falling_count - best_offset;
        }

        duty_samples = 0U;
        for (i = 0U; i < sample_limit; ++i)
        {
            uint16_t period_ticks;
            uint16_t high_ticks;

            period_ticks = (uint16_t)(rising_ticks[i + 1U] - rising_ticks[i]);
            high_ticks = (uint16_t)(falling_ticks[i + best_offset] - rising_ticks[i]);

            if ((period_ticks == 0U) || (high_ticks == 0U) || (high_ticks >= period_ticks))
            {
                continue;
            }

            duty_samples++;
        }

        if (duty_samples > best_duty_samples)
        {
            best_duty_samples = duty_samples;
        }
    }

    if (best_duty_samples == 0U)
    {
        return true;
    }

    {
        uint8_t offset0_samples;
        uint8_t offset1_samples;
        uint32_t offset0_limit;
        uint32_t offset1_limit;

        offset0_samples = 0U;
        offset1_samples = 0U;
        offset0_limit = (falling_count < (rising_count - 1U)) ? falling_count : (rising_count - 1U);
        offset1_limit = (falling_count > 0U) ? (falling_count - 1U) : 0U;
        if (offset1_limit > (rising_count - 1U))
        {
            offset1_limit = rising_count - 1U;
        }

        for (i = 0U; i < offset0_limit; ++i)
        {
            uint16_t period_ticks;
            uint16_t high_ticks;

            period_ticks = (uint16_t)(rising_ticks[i + 1U] - rising_ticks[i]);
            high_ticks = (uint16_t)(falling_ticks[i] - rising_ticks[i]);
            if ((period_ticks != 0U) && (high_ticks != 0U) && (high_ticks < period_ticks))
            {
                offset0_samples++;
            }
        }

        for (i = 0U; i < offset1_limit; ++i)
        {
            uint16_t period_ticks;
            uint16_t high_ticks;

            period_ticks = (uint16_t)(rising_ticks[i + 1U] - rising_ticks[i]);
            high_ticks = (uint16_t)(falling_ticks[i + 1U] - rising_ticks[i]);
            if ((period_ticks != 0U) && (high_ticks != 0U) && (high_ticks < period_ticks))
            {
                offset1_samples++;
            }
        }

        best_offset = (offset1_samples > offset0_samples) ? 1U : 0U;
    }

    high_sum = 0ULL;
    best_duty_samples = 0U;
    {
        uint32_t sample_limit;

        sample_limit = rising_count - 1U;
        if ((falling_count - best_offset) < sample_limit)
        {
            sample_limit = falling_count - best_offset;
        }

        for (i = 0U; i < sample_limit; ++i)
        {
            uint16_t period_ticks;
            uint16_t high_ticks;

            period_ticks = (uint16_t)(rising_ticks[i + 1U] - rising_ticks[i]);
            high_ticks = (uint16_t)(falling_ticks[i + best_offset] - rising_ticks[i]);

            if ((period_ticks == 0U) || (high_ticks == 0U) || (high_ticks >= period_ticks))
            {
                continue;
            }

            high_sum += (uint64_t)high_ticks;
            best_duty_samples++;
        }
    }

    if (best_duty_samples > 0U)
    {
        avg_high_ticks = (uint32_t)((high_sum + ((uint64_t)best_duty_samples / 2ULL)) / (uint64_t)best_duty_samples);
        result_out->high_ticks_avg = avg_high_ticks;
        result_out->duty_pct_x100 = (uint16_t)(((10000ULL * high_sum) + (period_sum / 2ULL)) / period_sum);
        result_out->valid_samples = best_duty_samples;
        result_out->has_duty_cycle = true;
    }

    return true;
}

static bool pwm_capture_drv_process_pwm_u32(const uint32_t *rising_ticks,
                                            uint32_t rising_count,
                                            const uint32_t *falling_ticks,
                                            uint32_t falling_count,
                                            uint32_t tick_hz,
                                            pwm_capture_result_t *result_out)
{
    uint64_t period_sum;
    uint64_t high_sum;
    uint32_t avg_period_ticks;
    uint32_t avg_high_ticks;
    uint32_t i;
    uint8_t valid_period_samples;
    uint8_t best_duty_samples;
    uint8_t best_offset;

    if ((rising_ticks == NULL) || (falling_ticks == NULL) || (result_out == NULL) || (tick_hz == 0U))
    {
        return false;
    }

    valid_period_samples = pwm_capture_drv_compute_period_stats_u32(rising_ticks,
                                                                    rising_count,
                                                                    &period_sum,
                                                                    &avg_period_ticks);
    if (valid_period_samples < PWM_CAPTURE_MIN_PERIOD_SAMPLES)
    {
        return false;
    }

    if (avg_period_ticks == 0U)
    {
        return false;
    }

    pwm_capture_drv_reset_result(result_out);
    result_out->frequency_hz = (uint32_t)(((uint64_t)tick_hz + ((uint64_t)avg_period_ticks / 2ULL)) /
                                          (uint64_t)avg_period_ticks);
    result_out->period_ticks_avg = avg_period_ticks;
    result_out->valid_samples = valid_period_samples;
    result_out->data_valid = true;

    best_duty_samples = 0U;
    best_offset = 0U;

    for (best_offset = 0U; best_offset <= 1U; ++best_offset)
    {
        uint8_t duty_samples;
        uint32_t sample_limit;

        if (falling_count <= best_offset)
        {
            continue;
        }

        sample_limit = rising_count - 1U;
        if ((falling_count - best_offset) < sample_limit)
        {
            sample_limit = falling_count - best_offset;
        }

        duty_samples = 0U;
        for (i = 0U; i < sample_limit; ++i)
        {
            uint32_t period_ticks;
            uint32_t high_ticks;

            period_ticks = rising_ticks[i + 1U] - rising_ticks[i];
            high_ticks = falling_ticks[i + best_offset] - rising_ticks[i];

            if ((period_ticks == 0U) || (high_ticks == 0U) || (high_ticks >= period_ticks))
            {
                continue;
            }

            duty_samples++;
        }

        if (duty_samples > best_duty_samples)
        {
            best_duty_samples = duty_samples;
        }
    }

    if (best_duty_samples == 0U)
    {
        return true;
    }

    {
        uint8_t offset0_samples;
        uint8_t offset1_samples;
        uint32_t offset0_limit;
        uint32_t offset1_limit;

        offset0_samples = 0U;
        offset1_samples = 0U;
        offset0_limit = (falling_count < (rising_count - 1U)) ? falling_count : (rising_count - 1U);
        offset1_limit = (falling_count > 0U) ? (falling_count - 1U) : 0U;
        if (offset1_limit > (rising_count - 1U))
        {
            offset1_limit = rising_count - 1U;
        }

        for (i = 0U; i < offset0_limit; ++i)
        {
            uint32_t period_ticks;
            uint32_t high_ticks;

            period_ticks = rising_ticks[i + 1U] - rising_ticks[i];
            high_ticks = falling_ticks[i] - rising_ticks[i];
            if ((period_ticks != 0U) && (high_ticks != 0U) && (high_ticks < period_ticks))
            {
                offset0_samples++;
            }
        }

        for (i = 0U; i < offset1_limit; ++i)
        {
            uint32_t period_ticks;
            uint32_t high_ticks;

            period_ticks = rising_ticks[i + 1U] - rising_ticks[i];
            high_ticks = falling_ticks[i + 1U] - rising_ticks[i];
            if ((period_ticks != 0U) && (high_ticks != 0U) && (high_ticks < period_ticks))
            {
                offset1_samples++;
            }
        }

        best_offset = (offset1_samples > offset0_samples) ? 1U : 0U;
    }

    high_sum = 0ULL;
    best_duty_samples = 0U;
    {
        uint32_t sample_limit;

        sample_limit = rising_count - 1U;
        if ((falling_count - best_offset) < sample_limit)
        {
            sample_limit = falling_count - best_offset;
        }

        for (i = 0U; i < sample_limit; ++i)
        {
            uint32_t period_ticks;
            uint32_t high_ticks;

            period_ticks = rising_ticks[i + 1U] - rising_ticks[i];
            high_ticks = falling_ticks[i + best_offset] - rising_ticks[i];

            if ((period_ticks == 0U) || (high_ticks == 0U) || (high_ticks >= period_ticks))
            {
                continue;
            }

            high_sum += (uint64_t)high_ticks;
            best_duty_samples++;
        }
    }

    if (best_duty_samples > 0U)
    {
        avg_high_ticks = (uint32_t)((high_sum + ((uint64_t)best_duty_samples / 2ULL)) / (uint64_t)best_duty_samples);
        result_out->high_ticks_avg = avg_high_ticks;
        result_out->duty_pct_x100 = (uint16_t)(((10000ULL * high_sum) + (period_sum / 2ULL)) / period_sum);
        result_out->valid_samples = best_duty_samples;
        result_out->has_duty_cycle = true;
    }

    return true;
}

static bool pwm_capture_drv_process_frequency_u16(const uint16_t *rising_ticks,
                                                  uint32_t rising_count,
                                                  uint32_t tick_hz,
                                                  pwm_capture_result_t *result_out)
{
    uint64_t period_sum;
    uint32_t avg_period_ticks;
    uint8_t valid_samples;

    if ((rising_ticks == NULL) || (result_out == NULL) || (tick_hz == 0U))
    {
        return false;
    }

    valid_samples = pwm_capture_drv_compute_period_stats_u16(rising_ticks,
                                                             rising_count,
                                                             &period_sum,
                                                             &avg_period_ticks);
    if (valid_samples < PWM_CAPTURE_MIN_PERIOD_SAMPLES)
    {
        return false;
    }

    if (avg_period_ticks == 0U)
    {
        return false;
    }

    pwm_capture_drv_reset_result(result_out);
    result_out->frequency_hz = (uint32_t)(((uint64_t)tick_hz + ((uint64_t)avg_period_ticks / 2ULL)) /
                                          (uint64_t)avg_period_ticks);
    result_out->period_ticks_avg = avg_period_ticks;
    result_out->valid_samples = valid_samples;
    result_out->data_valid = true;

    return true;
}

static bool pwm_capture_drv_arm_job(pwm_capture_job_t *job)
{
    bool success;

    if (job == NULL)
    {
        return false;
    }

    pwm_capture_drv_clear_job_buffers(job);
    job->done_mask = 0U;
    job->dma_error = false;
    job->start_ms = HAL_GetTick();
    job->armed = true;
    success = true;

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        if (HAL_TIM_IC_Start_DMA(&htim4, TIM_CHANNEL_1, (uint32_t *)s_me_rising_ticks, PWM_CAPTURE_BURST_SAMPLES) != HAL_OK)
        {
            success = false;
        }
        else if (HAL_TIM_IC_Start_DMA(&htim4, TIM_CHANNEL_2, (uint32_t *)s_me_falling_ticks, PWM_CAPTURE_BURST_SAMPLES) != HAL_OK)
        {
            success = false;
        }
        break;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        if (HAL_TIM_IC_Start_DMA(&htim2, TIM_CHANNEL_1, s_mf_rising_ticks, PWM_CAPTURE_BURST_SAMPLES) != HAL_OK)
        {
            success = false;
        }
        else if (HAL_TIM_IC_Start_DMA(&htim2, TIM_CHANNEL_2, s_mf_falling_ticks, PWM_CAPTURE_BURST_SAMPLES) != HAL_OK)
        {
            success = false;
        }
        break;

    case PWM_CAPTURE_SIGNAL_LT8316_GATE:
        if (HAL_TIM_IC_Start_DMA(&htim4, TIM_CHANNEL_3, (uint32_t *)s_gate_rising_ticks, PWM_CAPTURE_BURST_SAMPLES) != HAL_OK)
        {
            success = false;
        }
        break;

    default:
        success = false;
        break;
    }

    if (!success)
    {
        pwm_capture_drv_stop_job(job);
        job->done_mask = 0U;
        job->dma_error = true;
        job->armed = false;
        pwm_capture_drv_invalidate_job(job);
    }

    return success;
}

static bool pwm_capture_drv_job_is_complete(const pwm_capture_job_t *job)
{
    if (job == NULL)
    {
        return false;
    }

    return ((job->done_mask & job->required_mask) == job->required_mask);
}

static bool pwm_capture_drv_finalize_job(pwm_capture_job_t *job)
{
    uint32_t rise_count;
    uint32_t fall_count;
    uint32_t tick_hz;
    bool processed_ok;

    if ((job == NULL) || !job->armed)
    {
        return false;
    }

    rise_count = pwm_capture_drv_get_captured_sample_count(pwm_capture_drv_get_rise_dma(job));
    fall_count = pwm_capture_drv_get_captured_sample_count(pwm_capture_drv_get_fall_dma(job));
    pwm_capture_drv_stop_job(job);
    job->armed = false;
    job->completed_once = true;

    if (job->dma_error)
    {
        pwm_capture_drv_invalidate_job(job);
        return false;
    }

    tick_hz = 0U;
    processed_ok = false;

    switch (job->signal)
    {
    case PWM_CAPTURE_SIGNAL_LTC3901_ME:
        tick_hz = pwm_capture_drv_get_tick_hz(&htim4);
        processed_ok = pwm_capture_drv_process_pwm_u16(s_me_rising_ticks,
                                                       rise_count,
                                                       s_me_falling_ticks,
                                                       fall_count,
                                                       tick_hz,
                                                       &job->result);
        break;

    case PWM_CAPTURE_SIGNAL_LTC3901_MF:
        tick_hz = pwm_capture_drv_get_tick_hz(&htim2);
        processed_ok = pwm_capture_drv_process_pwm_u32(s_mf_rising_ticks,
                                                       rise_count,
                                                       s_mf_falling_ticks,
                                                       fall_count,
                                                       tick_hz,
                                                       &job->result);
        break;

    case PWM_CAPTURE_SIGNAL_LT8316_GATE:
        tick_hz = pwm_capture_drv_get_tick_hz(&htim4);
        processed_ok = pwm_capture_drv_process_frequency_u16(s_gate_rising_ticks,
                                                             rise_count,
                                                             tick_hz,
                                                             &job->result);
        break;

    default:
        break;
    }

    if (!processed_ok)
    {
        pwm_capture_drv_invalidate_job(job);
    }

    return processed_ok;
}

static bool pwm_capture_drv_any_job_armed(void)
{
    uint32_t job_index;

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        if (s_jobs[job_index].armed)
        {
            return true;
        }
    }

    return false;
}

static bool pwm_capture_drv_all_jobs_completed_once(void)
{
    uint32_t job_index;

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        if (!s_jobs[job_index].completed_once)
        {
            return false;
        }
    }

    return true;
}

void pwm_capture_drv_init(void)
{
    pwm_capture_drv_abort();
    s_initialized = true;
}

void pwm_capture_drv_task(void)
{
    uint32_t job_index;
    uint32_t now_ms;

    now_ms = HAL_GetTick();

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        pwm_capture_job_t *job;

        job = &s_jobs[job_index];
        if (!job->armed)
        {
            continue;
        }

        if (job->dma_error || pwm_capture_drv_job_is_complete(job) ||
            ((now_ms - job->start_ms) >= PWM_CAPTURE_BURST_TIMEOUT_MS))
        {
            (void)pwm_capture_drv_finalize_job(job);

            if (s_repeat_enabled)
            {
                (void)pwm_capture_drv_arm_job(job);
            }
        }
    }
}

bool pwm_capture_drv_is_initialized(void)
{
    return s_initialized;
}

bool pwm_capture_drv_is_busy(void)
{
    return pwm_capture_drv_any_job_armed();
}

bool pwm_capture_drv_is_complete(void)
{
    return pwm_capture_drv_all_jobs_completed_once();
}

bool pwm_capture_drv_start_burst(void)
{
    uint32_t job_index;
    bool all_started;

    if (!s_initialized || pwm_capture_drv_any_job_armed())
    {
        return false;
    }

    pwm_capture_drv_reset_jobs();
    s_repeat_enabled = true;
    all_started = true;

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        if (!pwm_capture_drv_arm_job(&s_jobs[job_index]))
        {
            all_started = false;
        }
    }

    return all_started;
}

void pwm_capture_drv_abort(void)
{
    uint32_t job_index;

    for (job_index = 0U; job_index < (uint32_t)PWM_CAPTURE_SIGNAL_COUNT; ++job_index)
    {
        pwm_capture_drv_stop_job(&s_jobs[job_index]);
    }

    s_repeat_enabled = false;
    pwm_capture_drv_reset_jobs();
}

bool pwm_capture_drv_get_result(pwm_capture_signal_t signal, pwm_capture_result_t *result_out)
{
    if ((result_out == NULL) || ((unsigned int)signal >= (unsigned int)PWM_CAPTURE_SIGNAL_COUNT))
    {
        return false;
    }

    *result_out = s_jobs[(unsigned int)signal].result;
    return s_jobs[(unsigned int)signal].result.data_valid;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim4)
    {
        if ((htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) && s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].done_mask |= PWM_CAPTURE_JOB_DONE_RISE;
        }
        else if ((htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) && s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].done_mask |= PWM_CAPTURE_JOB_DONE_FALL;
        }
        else if ((htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) && s_jobs[PWM_CAPTURE_SIGNAL_LT8316_GATE].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LT8316_GATE].done_mask |= PWM_CAPTURE_JOB_DONE_RISE;
        }
    }
    else if (htim == &htim2)
    {
        if ((htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) && s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].done_mask |= PWM_CAPTURE_JOB_DONE_RISE;
        }
        else if ((htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) && s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].done_mask |= PWM_CAPTURE_JOB_DONE_FALL;
        }
    }
}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim4)
    {
        if (s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_ME].dma_error = true;
        }

        if (s_jobs[PWM_CAPTURE_SIGNAL_LT8316_GATE].armed)
        {
            s_jobs[PWM_CAPTURE_SIGNAL_LT8316_GATE].dma_error = true;
        }
    }
    else if ((htim == &htim2) && s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].armed)
    {
        s_jobs[PWM_CAPTURE_SIGNAL_LTC3901_MF].dma_error = true;
    }
}
