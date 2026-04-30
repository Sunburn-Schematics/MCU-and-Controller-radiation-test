#include "fw_app.h"

#include "bsp_board.h"
#include "stm32f4xx_hal.h"

#define FW_APP_HEARTBEAT_PERIOD_MS    (250U)

static uint32_t s_last_toggle_ms;

void fw_app_init(void)
{
    bsp_init();
    s_last_toggle_ms = HAL_GetTick();
}

void fw_app_run(void)
{
    uint32_t now_ms = HAL_GetTick();

    if ((now_ms - s_last_toggle_ms) >= FW_APP_HEARTBEAT_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        bsp_led_toggle(BSP_LED_BLUE);
    }
}
