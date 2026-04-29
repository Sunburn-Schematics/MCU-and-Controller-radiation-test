#include "fw_app.h"

#include "main.h"
#include "stm32f4xx_hal.h"

#define FW_APP_HEARTBEAT_PERIOD_MS    (250U)

static uint32_t s_last_toggle_ms;

void fw_app_init(void)
{
    s_last_toggle_ms = HAL_GetTick();
    HAL_GPIO_WritePin(LED_Blue_GPIO_Port, LED_Blue_Pin, GPIO_PIN_RESET);
}

void fw_app_run(void)
{
    uint32_t now_ms = HAL_GetTick();

    if ((now_ms - s_last_toggle_ms) >= FW_APP_HEARTBEAT_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        HAL_GPIO_TogglePin(LED_Blue_GPIO_Port, LED_Blue_Pin);
    }
}
