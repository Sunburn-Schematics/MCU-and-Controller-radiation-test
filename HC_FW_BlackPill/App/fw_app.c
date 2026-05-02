#include "fw_app.h"

#include "bsp_board.h"
#include "stm32f4xx_hal.h"

#include "usbd_cdc_if.h"
#include <stdint.h>

#include <stdio.h>

#include "sync_drv.h"
#include "usb_vcp_drv.h"
#include "command_processor.h"
#include "hc_comms_tx.h"
#include "hc_app_status.h"

#define FW_APP_HEARTBEAT_PERIOD_MS    (500U)
#define FW_APP_STS_PERIOD_MS          (1000U)
#define FW_APP_STS_BUFFER_SIZE        (1024U)
static uint32_t s_last_toggle_ms;
static uint32_t s_last_sts_ms;

static void fw_app_send_periodic_status(void)
{
    char sts_line[FW_APP_STS_BUFFER_SIZE];

    if (!hc_app_status_format_sts_json(sts_line, sizeof(sts_line)))
    {
        return;
    }

    (void)hc_comms_tx_send_line(sts_line);
}

void fw_app_init(void)
{
    bsp_init();
    hc_app_status_init();
    usb_vcp_drv_init();
    command_processor_init();

    sync_drv_init();
    sync_drv_raw_config_t raw_cfg = {
    /*    .ARR = 839U, // 84MHz / (2 * (839 + 1)) = 100kHz square wave
        .CCR2 = 419U, // */
        .ARR = 167U, // 84MHz / (2 * (167 + 1)) = 250kHz square wave
        .CCR2 = 83U, // */
    };
    sync_drv_configure_and_enable(&raw_cfg);

    s_last_toggle_ms = HAL_GetTick();
    s_last_sts_ms = HAL_GetTick();
    printf("App Initialized\r\n");
}

char buf[] = "Hello World! with a string that is a lot longer than 64 bytes because I've padded it with fluff.\r\n";
  
void fw_app_run(void)
{
    uint32_t now_ms = HAL_GetTick();

    if ((now_ms - s_last_toggle_ms) >= FW_APP_HEARTBEAT_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        bsp_led_toggle(BSP_LED_BLUE);
//        printf("App Initialized\r\n");
//        printf("Test printf: %s", buf);
    }

    if ((now_ms - s_last_sts_ms) >= FW_APP_STS_PERIOD_MS)
    {
        s_last_sts_ms = now_ms;
        fw_app_send_periodic_status();
    }

    usb_vcp_drv_task();
    command_processor_task();
    HAL_Delay(10);

}
