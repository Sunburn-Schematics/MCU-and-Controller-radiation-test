#include "fw_app.h"

#include "bsp_board.h"
#include "stm32f4xx_hal.h"

#include "usbd_cdc_if.h"
#include <stdint.h>

#include <stdio.h>

#include "sync_drv.h"
#include "adc_sense_drv.h"
#include "hc_debug_telemetry.h"
#include "pwm_capture_drv.h"
#include "usb_vcp_drv.h"
#include "command_processor.h"
#include "hc_comms_tx.h"
#include "hc_app_status.h"

#define FW_APP_HEARTBEAT_PERIOD_MS    (500U)
#define FW_APP_STS_PERIOD_DEFAULT_MS  (1000U)
#define FW_APP_STS_BUFFER_SIZE        (1024U)
static uint32_t s_last_toggle_ms;
static uint32_t s_last_sts_ms;
static uint32_t s_sts_period_ms;

static void fw_app_send_periodic_status(void)
{
    char sts_line[FW_APP_STS_BUFFER_SIZE];

    hc_app_status_refresh_from_bsp();

    if (!hc_app_status_format_sts_json(sts_line, sizeof(sts_line)))
    {
        return;
    }

    (void)hc_comms_tx_send_line(sts_line);
}

void fw_app_init(void)
{
    bsp_init();
    adc_sense_drv_init();
    pwm_capture_drv_init();
    hc_debug_telemetry_init();
    (void)pwm_capture_drv_start_burst();
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
    s_sts_period_ms = FW_APP_STS_PERIOD_DEFAULT_MS;
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

    if ((s_sts_period_ms != 0U) && ((now_ms - s_last_sts_ms) >= s_sts_period_ms))
    {
        s_last_sts_ms = now_ms;
        fw_app_send_periodic_status();
    }

    adc_sense_drv_task();
    usb_vcp_drv_task();
    pwm_capture_drv_task();
    hc_debug_telemetry_task();
    command_processor_task();
    HAL_Delay(10);

}

bool fw_app_set_sts_period_ms(uint32_t period_ms)
{
    s_sts_period_ms = period_ms;
    s_last_sts_ms = HAL_GetTick();
    return true;
}

uint32_t fw_app_get_sts_period_ms(void)
{
    return s_sts_period_ms;
}

bool fw_app_set_debug_config(const hc_debug_telemetry_config_t *config)
{
    return hc_debug_telemetry_set_config(config);
}

bool fw_app_get_debug_config(hc_debug_telemetry_config_t *config_out)
{
    return hc_debug_telemetry_get_config(config_out);
}

bool fw_app_debug_lookup_signal_id(const char *name, size_t name_len, uint8_t *signal_id_out)
{
    return hc_debug_telemetry_lookup_signal_id(name, name_len, signal_id_out);
}

const char *fw_app_debug_get_signal_name(uint8_t signal_id)
{
    return hc_debug_telemetry_get_signal_name(signal_id);
}

bool fw_app_debug_format_signals_json(const uint8_t *signal_ids,
                                      uint8_t signal_count,
                                      char *buffer,
                                      size_t buffer_size)
{
    return hc_debug_telemetry_format_signals_json(signal_ids, signal_count, buffer, buffer_size);
}

bool fw_app_debug_signal_is_digital(uint8_t signal_id)
{
    const char *name = hc_debug_telemetry_get_signal_name(signal_id);
    if (name == NULL)
    {
        return false;
    }

    // Check if it's one of the settable digital signals
    return (strcmp(name, "ltc3901.pwr_en") == 0) ||
           (strcmp(name, "lt8316.pwr_en") == 0) ||
           (strcmp(name, "led.blue") == 0) ||
           (strcmp(name, "led.red") == 0) ||
           (strcmp(name, "led.green") == 0) ||
           (strcmp(name, "sync.enable") == 0);
}

bool fw_app_set_digital_signal(uint8_t signal_id, bool value)
{
    const char *name = hc_debug_telemetry_get_signal_name(signal_id);
    if (name == NULL)
    {
        return false;
    }

    if (strcmp(name, "ltc3901.pwr_en") == 0)
    {
        if (value)
        {
            bsp_power_enable(BSP_POWER_LTC3901);
        }
        else
        {
            bsp_power_disable(BSP_POWER_LTC3901);
        }
        return true;
    }
    else if (strcmp(name, "lt8316.pwr_en") == 0)
    {
        if (value)
        {
            bsp_power_enable(BSP_POWER_LT8316);
        }
        else
        {
            bsp_power_disable(BSP_POWER_LT8316);
        }
        return true;
    }
    else if (strcmp(name, "led.blue") == 0)
    {
        bsp_led_write(BSP_LED_BLUE, value);
        return true;
    }
    else if (strcmp(name, "led.red") == 0)
    {
        bsp_led_write(BSP_LED_RED, value);
        return true;
    }
    else if (strcmp(name, "led.green") == 0)
    {
        bsp_led_write(BSP_LED_GREEN, value);
        return true;
    }
    else if (strcmp(name, "sync.enable") == 0)
    {
        return fw_app_set_sync_enable(value);
    }

    return false;
}

bool fw_app_set_sync_enable(bool enable)
{
    if (enable)
    {
        return sync_drv_enable();
    }
    else
    {
        sync_drv_disable();
        return true;
    }
}

bool fw_app_get_sync_enable(void)
{
    return sync_drv_is_enabled();
}

bool fw_app_debug_format_digital_signals_json(const uint8_t *signal_ids,
                                              const bool *values,
                                              uint8_t signal_count,
                                              char *buffer,
                                              size_t buffer_size)
{
    size_t written = 0;
    bool first = true;

    if (buffer == NULL || buffer_size == 0)
    {
        return false;
    }

    written += snprintf(buffer + written, buffer_size - written, "{");
    if (written >= buffer_size)
    {
        return false;
    }

    for (uint8_t i = 0; i < signal_count; ++i)
    {
        const char *name = hc_debug_telemetry_get_signal_name(signal_ids[i]);
        if (name == NULL)
        {
            continue;
        }

        if (!first)
        {
            written += snprintf(buffer + written, buffer_size - written, ",");
            if (written >= buffer_size)
            {
                return false;
            }
        }
        first = false;

        written += snprintf(buffer + written, buffer_size - written, "\"%s\":%s",
                           name, values[i] ? "true" : "false");
        if (written >= buffer_size)
        {
            return false;
        }
    }

    written += snprintf(buffer + written, buffer_size - written, "}");
    if (written >= buffer_size)
    {
        return false;
    }

    return true;
}
