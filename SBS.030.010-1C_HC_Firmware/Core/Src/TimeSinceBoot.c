#include "TimeSinceBoot.h"
#include "main.h"

static uint32_t s_boot_tick_ms = 0U;

void TSB_Init(void)
{
    s_boot_tick_ms = HAL_GetTick();
}

void TSB_Reset(void)
{
    s_boot_tick_ms = HAL_GetTick();
}

uint32_t TSB_GetMs(void)
{
    return HAL_GetTick() - s_boot_tick_ms;
}

uint32_t TSB_GetSeconds(void)
{
    return TSB_GetMs() / 1000U;
}
