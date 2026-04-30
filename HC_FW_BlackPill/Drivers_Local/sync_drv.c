#include "sync_drv.h"

#include <assert.h>
#include <limits.h>

#include "tim.h"

#define SYNC_DRV_NS_PER_SECOND (1000000000ULL)
#define SYNC_DRV_TIMER_MAX_ARR (0xFFFFUL)

/***
 * The Sync Output is implemented as a quadrature output with controllable delay between SDRA and SDRB.
    Both SDRA and SDRB should be in OC mode with 'Toggle On Match' enabled. This will cause their output state to toggle 
    whenever the timer counter matches the value in their respective Capture/Compare Register (CCR).
    The Auto-Reload Register (ARR) should be set to half the period of the desired square wave.
    CCR1 (SDRA) should be set to 0, causing it to toggle at the start of each period.
    CCR2 (SDRB) should be set to a value corresponding to the desired delay, causing it to toggle at that point in the timer count.
***/


typedef struct
{
    uint32_t period_ticks;
    uint32_t half_period_ticks;
    uint32_t delay_ticks;
} sync_drv_ticks_t;

static TIM_HandleTypeDef *hSyncTimer = &htim3;   /* Timer handle for the synchronization timer */
//static sync_drv_config_t s_config;
static bool s_config_valid;
static bool s_enabled;

#if 0
static uint32_t prv_get_tim3_input_clock_hz(void)
{
    uint32_t pclk1_hz = HAL_RCC_GetPCLK1Freq() * 2U;
//    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1);
//    if (ppre1 != 0) {
//        pclk1_hz *= 2U;
//    }
    return pclk1_hz;
}

static bool prv_ns_to_ticks(uint32_t time_ns, uint32_t tick_hz, uint32_t *ticks)
{
    uint64_t scaled;

    assert_param(ticks != NULL);
    assert_param(tick_hz > 0U);

    scaled = ((uint64_t)time_ns * (uint64_t)tick_hz) + (SYNC_DRV_NS_PER_SECOND / 2ULL);
    scaled /= SYNC_DRV_NS_PER_SECOND;

    if (scaled > UINT32_MAX)
    {
        return false;
    }

    *ticks = (uint32_t)scaled;
    return true;
}

static bool prv_validate_config(const sync_drv_config_t *cfg)
{
    assert_param(cfg != NULL);
    assert_param(cfg->frequency_hz > 0U);

    return true;
}

static bool prv_compute_ticks(const sync_drv_config_t *cfg, sync_drv_ticks_t *ticks)
{
    uint32_t timer_clk_hz;
    uint32_t timer_tick_hz;
    uint64_t period_ns;
    uint64_t period_ticks_rounded;

    assert_param(cfg != NULL);
    assert_param(ticks != NULL);
    (void)prv_validate_config(cfg);

    timer_clk_hz = prv_get_tim3_input_clock_hz();
    assert_param(timer_clk_hz > 0U);

    timer_tick_hz = timer_clk_hz / (hSyncTimer->Init.Prescaler + 1U);
    assert_param(timer_tick_hz > 0U);

    period_ns = SYNC_DRV_NS_PER_SECOND / (uint64_t)cfg->frequency_hz;
    if (period_ns == 0ULL)
    {
        return false;
    }

    period_ticks_rounded = (((uint64_t)timer_tick_hz * period_ns) + (SYNC_DRV_NS_PER_SECOND / 2ULL)) /
                           SYNC_DRV_NS_PER_SECOND;

    if ((period_ticks_rounded < 2ULL) || ((period_ticks_rounded - 1ULL) > SYNC_DRV_TIMER_MAX_ARR))
    {
        return false;
    }

    if ((period_ticks_rounded & 1ULL) != 0ULL)
    {
        return false;
    }

    ticks->period_ticks = (uint32_t)period_ticks_rounded;
    ticks->half_period_ticks = ticks->period_ticks / 2U;

    if (!prv_ns_to_ticks(cfg->sdrb_delay_ns, timer_tick_hz, &ticks->delay_ticks))
    {
        return false;
    }

    if (ticks->delay_ticks >= ticks->period_ticks)
    {
        return false;
    }

    return true;
}

static bool prv_apply_channel_config(uint32_t channel, uint32_t pulse_ticks)
{
    TIM_OC_InitTypeDef oc_cfg = {0};

    oc_cfg.OCMode = TIM_OCMODE_TOGGLE;
    oc_cfg.Pulse = pulse_ticks;
    oc_cfg.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc_cfg.OCFastMode = TIM_OCFAST_DISABLE;

    return (HAL_TIM_OC_ConfigChannel(hSyncTimer, &oc_cfg, channel) == HAL_OK);
}

static bool prv_apply_config(const sync_drv_config_t *cfg)
{
    sync_drv_ticks_t ticks;
    bool restart_required;

    assert_param(cfg != NULL);

    if (!prv_compute_ticks(cfg, &ticks))
    {
        return false;
    }

    restart_required = s_enabled;
    if (restart_required)
    {
        sync_drv_disable();
    }

    hSyncTimer->Instance->CR1 &= ~TIM_CR1_CEN;
    hSyncTimer->Instance->CNT = 0U;
    hSyncTimer->Instance->ARR = ticks.half_period_ticks - 1U;
    hSyncTimer->Init.Period = ticks.half_period_ticks - 1U;

    if (!prv_apply_channel_config(TIM_CHANNEL_1, 0U))
    {
        return false;
    }

    if (!prv_apply_channel_config(TIM_CHANNEL_2, ticks.delay_ticks % ticks.half_period_ticks))
    {
        return false;
    }

    s_config = *cfg;
    s_config_valid = true;

    if (restart_required)
    {
        return sync_drv_enable();
    }

    return true;
}
#endif

#include "stm32f4xx_hal_tim.h"
#include "tim.h"

/* Init assumes the timer resource has already been initialized by MX_TIM3_Init()
   so all we need do is take a copy of the timer handle 
   NOTE: hSyncTimer is statically initialized to point to htim3, so this function doesn't 
   actually need to do anything right now. 
*/
void sync_drv_init(void)
{
    assert_param(hSyncTimer != NULL);
    assert_param(hSyncTimer->Instance == TIM3);
    s_config_valid = true;
}

void sync_drv_disable(void)
{
    (void)HAL_TIM_OC_Stop(hSyncTimer, TIM_CHANNEL_1);
    (void)HAL_TIM_OC_Stop(hSyncTimer, TIM_CHANNEL_2);
    s_enabled = false;
}

bool sync_drv_enable(void)
{
    if (!s_config_valid)
    {
        return false;
    }

    if (HAL_TIM_OC_Start(hSyncTimer, TIM_CHANNEL_1) != HAL_OK)
    {
        return false;
    }

    if (HAL_TIM_OC_Start(hSyncTimer, TIM_CHANNEL_2) != HAL_OK)
    {
        (void)HAL_TIM_OC_Stop(hSyncTimer, TIM_CHANNEL_1);
        return false;
    }

    s_enabled = true;
    return true;
}

bool sync_drv_configure(const sync_drv_raw_config_t *raw_cfg)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    assert_param(raw_cfg != NULL);
    assert_param(raw_cfg->ARR > raw_cfg->CCR2); // Delay must be less than the period

    /* Switch to a safe mode while we update the config */
    sync_drv_disable();

    /* Update Square Wave Period */
    __HAL_TIM_SET_AUTORELOAD(hSyncTimer, raw_cfg->ARR);

    /* This 'works' but no guarantee what state the outputs will toggle to relative to each other 
    __HAL_TIM_SET_COMPARE(hSyncTimer, TIM_CHANNEL_1, 0U);
    __HAL_TIM_SET_COMPARE(hSyncTimer, TIM_CHANNEL_2, raw_cfg->CCR2);
    HAL_TIM_GenerateEvent(hSyncTimer, TIM_EVENTSOURCE_UPDATE);  // */

    /* Force both SDRA / SDRB Outputs to a known state*/
    sConfigOC.OCMode = TIM_OCMODE_FORCED_INACTIVE; // Set to a safe mode while we update the config to avoid glitches on the output pins
    sConfigOC.Pulse = 0;    // 0 for CCR1 (Ch1)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_OC_ConfigChannel(hSyncTimer, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_OC_ConfigChannel(hSyncTimer, &sConfigOC, TIM_CHANNEL_2);
    /* Not sure if required but included incase we need the event in order to force the pin output state */
    HAL_TIM_GenerateEvent(hSyncTimer, TIM_EVENTSOURCE_UPDATE);  // */

    /* Now put back into Toggle Mode */
    sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
    HAL_TIM_OC_ConfigChannel(hSyncTimer, &sConfigOC, TIM_CHANNEL_1);
    sConfigOC.Pulse = raw_cfg->CCR2;    // Use raw CCR2 value for Ch2
    HAL_TIM_OC_ConfigChannel(hSyncTimer, &sConfigOC, TIM_CHANNEL_2);

//    HAL_TIM_OC_Start(hSyncTimer, TIM_CHANNEL_1);
//    HAL_TIM_OC_Start(hSyncTimer, TIM_CHANNEL_2);
    s_config_valid = true;
    return true;
}

bool sync_drv_configure_and_enable(const sync_drv_raw_config_t *raw_cfg)
{
    if (!sync_drv_configure(raw_cfg))
    {
        return false;
    }
    return sync_drv_enable();
}


bool sync_drv_is_enabled(void)
{
    return s_enabled;
}
