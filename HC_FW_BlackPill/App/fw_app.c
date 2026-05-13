#include "fw_app.h"

#include "bsp_board.h"
#include "stm32f4xx_hal.h"

#include "usbd_cdc_if.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

#include "sync_drv.h"
#include "adc_sense_drv.h"
#include "hc_debug_telemetry.h"
#include "pwm_capture_drv.h"
#include "usb_vcp_drv.h"
#include "command_processor.h"
#include "hc_comms_tx.h"
#include "hc_datetime.h"
#include "hc_jsonl_rsp.h"
#include "hc_app_status.h"
#include "ltc3901_manager.h"
#include "lt8316_manager.h"

#define HEARTBEAT_PERIOD_MS    (500U)       //The period of the Blue LED Flash.

#define STS_PERIOD_DEFAULT_MS  (1000U)    //The period of the STATUS message
//#define STS_PERIOD_DEFAULT_MS  (0U)      //The period of the STATUS message
#define STS_BUFFER_SIZE        (1024U)

#define EVT_BUFFER_SIZE        (384U)
#define EVT_MESSAGE_BUFFER_SIZE (256U)
#define EVT_SCOPED_MESSAGE_BUFFER_SIZE (288U)

//Autostart will automatically issue a RUN command to the managers after startup.
#ifndef AUTOSTART_ENABLE
#define AUTOSTART_ENABLE      (1U)
#endif
#ifndef AUTOSTART_DELAY_MS
#define AUTOSTART_DELAY_MS    (5000U)
#endif

#define LTC3901_ISUPPLY_MAX_MA (50L)
#define LTC3901_VUPSTREAM_MIN_MV (10000L)
#define LTC3901_VCC_MIN_MV (10000L)
#define LTC3901_POWER_UP_TIMEOUT_MS (2000U)
#define LTC3901_POWER_RETRY_DELAY_MS (1000U)
#define LTC3901_POWER_FAULT_MAX (3U)
#define LTC3901_SYNC_ON_DELAY_MS (1000U)
#define LTC3901_SYNC_HOLD_ON_TIME_MS (10000U)
#define LTC3901_SYNC_HOLD_OFF_TIME_MS (2000U)
#define LTC3901_SYNC_STABILIZATION_TIME_MS (100U)
#define LTC3901_SYNC_FAULT_DELAY_MS (1000U)
#define LT8316_POWER_RETRY_DELAY_MS (1000U)
#define LT8316_POWER_FAULT_MAX (3U)
#define LT8316_POWER_ON_STABILIZATION_TIME_MS (1000U)

static uint32_t s_last_toggle_ms;
static uint32_t s_last_sts_ms;
static uint32_t s_sts_period_ms;
static uint32_t s_app_started_ms;
static bool s_autostart_issued;
static ltc3901_manager_t s_ltc3901_manager;
static ltc3901_manager_outputs_t s_ltc3901_outputs;
static ltc3901_manager_request_t s_ltc3901_pending_request;
static lt8316_manager_t s_lt8316_manager;
static lt8316_manager_outputs_t s_lt8316_outputs;
static lt8316_manager_request_t s_lt8316_pending_request;

static const ltc3901_manager_config_t s_ltc3901_config = {
    .isupply_ma_max = LTC3901_ISUPPLY_MAX_MA,
    .vupstream_mv_min = LTC3901_VUPSTREAM_MIN_MV,
    .ltc3901_vcc_mv_min = LTC3901_VCC_MIN_MV,
    .power_up_timeout_ms = LTC3901_POWER_UP_TIMEOUT_MS,
    .power_retry_delay_ms = LTC3901_POWER_RETRY_DELAY_MS,
    .power_fault_max = LTC3901_POWER_FAULT_MAX,
    .sync_on_delay_ms = LTC3901_SYNC_ON_DELAY_MS,
    .sync_hold_on_time_ms = LTC3901_SYNC_HOLD_ON_TIME_MS,
    .sync_hold_off_time_ms = LTC3901_SYNC_HOLD_OFF_TIME_MS,
    .sync_stabilization_time_ms = LTC3901_SYNC_STABILIZATION_TIME_MS,
    .sync_fault_delay_ms = LTC3901_SYNC_FAULT_DELAY_MS,
};

static const lt8316_manager_config_t s_lt8316_config = {
    .power_retry_delay_ms = LT8316_POWER_RETRY_DELAY_MS,
    .power_fault_max = LT8316_POWER_FAULT_MAX,
    .power_on_stabilization_time_ms = LT8316_POWER_ON_STABILIZATION_TIME_MS,
};

static void update_ltc3901_status_from_outputs(const ltc3901_manager_outputs_t *outputs)
{
    bool sync_enabled;

    if (outputs == 0)
    {
        return;
    }

    sync_enabled = outputs->sdra_drv_enable && outputs->sdrb_drv_enable;
    hc_app_status_set_ltc3901_manager_state(
        ltc3901_manager_state_to_string(outputs->reported_state),
        sync_enabled);
}

static void update_lt8316_status_from_outputs(const lt8316_manager_outputs_t *outputs)
{
    if (outputs == 0)
    {
        return;
    }

    hc_app_status_set_lt8316_manager_state(
        lt8316_manager_state_to_string(outputs->reported_state));
}

static void apply_ltc3901_manager_outputs(const ltc3901_manager_outputs_t *outputs)
{
    bool sync_enabled;

    if (outputs == 0)
    {
        return;
    }

    if (bsp_power_is_enabled(BSP_POWER_LTC3901) != outputs->ltc3901_power_enable)
    {
        bsp_power_write(BSP_POWER_LTC3901, outputs->ltc3901_power_enable);
    }

    sync_enabled = outputs->sdra_drv_enable && outputs->sdrb_drv_enable;
    if (sync_enabled)
    {
        (void)sync_drv_enable();
    }
    else
    {
        sync_drv_disable();
    }

    update_ltc3901_status_from_outputs(outputs);
}

static void apply_lt8316_manager_outputs(const lt8316_manager_outputs_t *outputs)
{
    if (outputs == 0)
    {
        return;
    }

    if (bsp_power_is_enabled(BSP_POWER_LT8316) != outputs->hv_power_enable)
    {
        bsp_power_write(BSP_POWER_LT8316, outputs->hv_power_enable);
    }

    update_lt8316_status_from_outputs(outputs);
}

static void send_event_message(const char *message)
{
    char evt_line[EVT_BUFFER_SIZE];
    const hc_app_status_t *status;

    if ((message == 0) || (message[0] == '\0'))
    {
        return;
    }

    status = hc_app_status_get_const();
    if (!hc_jsonl_rsp_build_event_msg(evt_line,
                                      sizeof(evt_line),
                                      status->HcId,
                                      hc_datetime_get(),
                                      message))
    {
        return;
    }

    (void)hc_comms_tx_send_line(evt_line);
}

static void format_ltc3901_event_message(ltc3901_manager_event_t event,
                                          const ltc3901_manager_inputs_t *inputs,
                                          uint32_t elapsed_ms,
                                          char *buffer,
                                          size_t buffer_size)
{
    if ((inputs == 0) || (buffer == 0) || (buffer_size == 0U))
    {
        return;
    }

    switch (event)
    {
    case LTC3901_MGR_EVT_ISUPPLY_TOO_HIGH:
        if (inputs->isupply_ma >= 0)
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Isupply Current too high: measured %ld mA >= limit %ld mA",
                           (long)inputs->isupply_ma,
                           (long)s_ltc3901_config.isupply_ma_max);
        }
        else
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Isupply Current too high: measured null mA >= limit %ld mA",
                           (long)s_ltc3901_config.isupply_ma_max);
        }
        break;

    case LTC3901_MGR_EVT_POWER_UP_TIMEOUT:
        if ((inputs->vupstream_mv >= 0) && (inputs->ltc3901_vcc_mv >= 0))
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Power Up Timeout (%lums): VUpstream (mV) %ld < %ld; VCC (mV) %ld < %ld",
                           (unsigned long)s_ltc3901_config.power_up_timeout_ms,
                           (long)inputs->vupstream_mv,
                           (long)s_ltc3901_config.vupstream_mv_min,
                           (long)inputs->ltc3901_vcc_mv,
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        else if (inputs->vupstream_mv >= 0)
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Power Up Timeout (%lums): VUpstream (mV) %ld < %ld; VCC (mV) null < %ld",
                           (unsigned long)s_ltc3901_config.power_up_timeout_ms,
                           (long)inputs->vupstream_mv,
                           (long)s_ltc3901_config.vupstream_mv_min,
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        else if (inputs->ltc3901_vcc_mv >= 0)
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Power Up Timeout (%lums): VUpstream (mV) null < %ld; VCC (mV) %ld < %ld",
                           (unsigned long)s_ltc3901_config.power_up_timeout_ms,
                           (long)s_ltc3901_config.vupstream_mv_min,
                           (long)inputs->ltc3901_vcc_mv,
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        else
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "Power Up Timeout (%lums): VUpstream (mV) null < %ld; VCC (mV) null < %ld",
                           (unsigned long)s_ltc3901_config.power_up_timeout_ms,
                           (long)s_ltc3901_config.vupstream_mv_min,
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        break;

    case LTC3901_MGR_EVT_RETRYING_POWER_UP:
        (void)snprintf(buffer,
                       buffer_size,
                       "Retrying %lu/%lu Power Up",
                       (unsigned long)s_ltc3901_manager.power_fault_count,
                       (unsigned long)s_ltc3901_config.power_fault_max);
        break;

    case LTC3901_MGR_EVT_VUPSTREAM_TOO_LOW:
        if (inputs->vupstream_mv >= 0)
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "VUpstream Too Low: measured %ld mV < minimum %ld mV",
                           (long)inputs->vupstream_mv,
                           (long)s_ltc3901_config.vupstream_mv_min);
        }
        else
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "VUpstream Too Low: measured null mV < minimum %ld mV",
                           (long)s_ltc3901_config.vupstream_mv_min);
        }
        break;

    case LTC3901_MGR_EVT_LTC3901_VCC_TOO_LOW:
        if (inputs->ltc3901_vcc_mv >= 0)
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "LTC3901 VCC Too Low: measured %ld mV < minimum %ld mV",
                           (long)inputs->ltc3901_vcc_mv,
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        else
        {
            (void)snprintf(buffer,
                           buffer_size,
                           "LTC3901 VCC Too Low: measured null mV < minimum %ld mV",
                           (long)s_ltc3901_config.ltc3901_vcc_mv_min);
        }
        break;

    case LTC3901_MGR_EVT_ME_STOPPED:
        (void)snprintf(buffer,
                       buffer_size,
                       "ME Stopped: measured null Hz, required valid frequency after %lu ms; elapsed %lu ms",
                       (unsigned long)s_ltc3901_config.sync_stabilization_time_ms,
                       (unsigned long)elapsed_ms);
        break;

    case LTC3901_MGR_EVT_MF_STOPPED:
        (void)snprintf(buffer,
                       buffer_size,
                       "MF Stopped: measured null Hz, required valid frequency after %lu ms; elapsed %lu ms",
                       (unsigned long)s_ltc3901_config.sync_stabilization_time_ms,
                       (unsigned long)elapsed_ms);
        break;

    default:
        (void)snprintf(buffer,
                       buffer_size,
                       "%s",
                       ltc3901_manager_event_to_string(event));
        break;
    }
}

static void format_lt8316_event_message(lt8316_manager_event_t event,
                                        const lt8316_manager_inputs_t *inputs,
                                        uint32_t elapsed_ms,
                                        char *buffer,
                                        size_t buffer_size)
{
    (void)inputs;

    if ((buffer == 0) || (buffer_size == 0U))
    {
        return;
    }

    switch (event)
    {
    case LT8316_MGR_EVT_RETRYING_POWER_UP:
        (void)snprintf(buffer,
                       buffer_size,
                       "Retrying %lu/%lu Power Up",
                       (unsigned long)s_lt8316_manager.power_fault_count,
                       (unsigned long)s_lt8316_config.power_fault_max);
        break;

    case LT8316_MGR_EVT_GATE_STOPPED:
        (void)snprintf(buffer,
                       buffer_size,
                       "GATE Stopped: measured null Hz, required valid frequency after %lu ms; elapsed %lu ms",
                       (unsigned long)s_lt8316_config.power_on_stabilization_time_ms,
                       (unsigned long)elapsed_ms);
        break;

    default:
        (void)snprintf(buffer,
                       buffer_size,
                       "%s",
                       lt8316_manager_event_to_string(event));
        break;
    }
}

static void build_ltc3901_manager_inputs(uint32_t now_ms,
                                         ltc3901_manager_inputs_t *inputs)
{
    const hc_app_status_t *status;

    if (inputs == 0)
    {
        return;
    }

    status = hc_app_status_get_const();
    inputs->now_ms = now_ms;
    inputs->request = s_ltc3901_pending_request;
    inputs->isupply_ma = status->Ltc3901.ISupply_mA;
    inputs->vupstream_mv = status->Ltc3901.VSupply_mV;
    inputs->ltc3901_vcc_mv = status->Ltc3901.VShunt_mV;
    inputs->me_freq_valid = (status->Ltc3901.MeFreq_Hz >= 0);
    inputs->mf_freq_valid = (status->Ltc3901.MfFreq_Hz >= 0);
}

static void build_lt8316_manager_inputs(uint32_t now_ms,
                                        lt8316_manager_inputs_t *inputs)
{
    const hc_app_status_t *status;

    if (inputs == 0)
    {
        return;
    }

    status = hc_app_status_get_const();
    inputs->now_ms = now_ms;
    inputs->request = s_lt8316_pending_request;
    inputs->gate_freq_valid = (status->Lt8316.GateFreq_Hz >= 0);
}

static void service_autostart(uint32_t now_ms)
{
    if ((AUTOSTART_ENABLE == 0U) || s_autostart_issued)
    {
        return;
    }

    if ((now_ms - s_app_started_ms) < AUTOSTART_DELAY_MS)
    {
        return;
    }

    s_autostart_issued = true;

    if ((s_ltc3901_manager.state == LTC3901_MGR_STATE_RESET) &&
        (s_ltc3901_pending_request == LTC3901_MGR_REQUEST_NONE))
    {
        s_ltc3901_pending_request = LTC3901_MGR_REQUEST_RUN;
    }

    if ((s_lt8316_manager.state == LT8316_MGR_STATE_RESET) &&
        (s_lt8316_pending_request == LT8316_MGR_REQUEST_NONE))
    {
        s_lt8316_pending_request = LT8316_MGR_REQUEST_RUN;
    }
}

static void ltc3901_manager_app_task(uint32_t now_ms)
{
    ltc3901_manager_inputs_t inputs;
    uint32_t entered_ms;
    char event_message[EVT_MESSAGE_BUFFER_SIZE];
    char scoped_event_message[EVT_SCOPED_MESSAGE_BUFFER_SIZE];

    build_ltc3901_manager_inputs(now_ms, &inputs);
    entered_ms = s_ltc3901_manager.entered_ms;
    ltc3901_manager_task(&s_ltc3901_manager,
                         &inputs,
                         &s_ltc3901_config,
                         &s_ltc3901_outputs);
    s_ltc3901_pending_request = LTC3901_MGR_REQUEST_NONE;
    apply_ltc3901_manager_outputs(&s_ltc3901_outputs);
    if (s_ltc3901_outputs.event_pending)
    {
        format_ltc3901_event_message(s_ltc3901_outputs.event,
                                      &inputs,
                                      now_ms - entered_ms,
                                      event_message,
                                      sizeof(event_message));
        (void)snprintf(scoped_event_message,
                       sizeof(scoped_event_message),
                       "LTC3901: %s",
                       event_message);
        send_event_message(scoped_event_message);
    }
}

static void lt8316_manager_app_task(uint32_t now_ms)
{
    lt8316_manager_inputs_t inputs;
    uint32_t entered_ms;
    char event_message[EVT_MESSAGE_BUFFER_SIZE];
    char scoped_event_message[EVT_SCOPED_MESSAGE_BUFFER_SIZE];

    build_lt8316_manager_inputs(now_ms, &inputs);
    entered_ms = s_lt8316_manager.entered_ms;
    lt8316_manager_task(&s_lt8316_manager,
                        &inputs,
                        &s_lt8316_config,
                        &s_lt8316_outputs);
    s_lt8316_pending_request = LT8316_MGR_REQUEST_NONE;
    apply_lt8316_manager_outputs(&s_lt8316_outputs);
    if (s_lt8316_outputs.event_pending)
    {
        format_lt8316_event_message(s_lt8316_outputs.event,
                                    &inputs,
                                    now_ms - entered_ms,
                                    event_message,
                                    sizeof(event_message));
        (void)snprintf(scoped_event_message,
                       sizeof(scoped_event_message),
                       "LT8316: %s",
                       event_message);
        send_event_message(scoped_event_message);
    }
}

bool fw_app_set_ltc3901_command(fw_app_ltc3901_command_t command)
{
    switch (command)
    {
    case FW_APP_LTC3901_COMMAND_RUN:
        s_ltc3901_pending_request = LTC3901_MGR_REQUEST_RUN;
        return true;

    case FW_APP_LTC3901_COMMAND_HALT:
        s_ltc3901_pending_request = LTC3901_MGR_REQUEST_HALT;
        return true;

    case FW_APP_LTC3901_COMMAND_RESET:
        s_ltc3901_pending_request = LTC3901_MGR_REQUEST_RESET;
        return true;

    default:
        return false;
    }
}

bool fw_app_set_lt8316_command(fw_app_lt8316_command_t command)
{
    switch (command)
    {
    case FW_APP_LT8316_COMMAND_RUN:
        s_lt8316_pending_request = LT8316_MGR_REQUEST_RUN;
        return true;

    case FW_APP_LT8316_COMMAND_RESET:
        s_lt8316_pending_request = LT8316_MGR_REQUEST_RESET;
        return true;

    default:
        return false;
    }
}

const char *fw_app_get_sw_version(void)
{
    return SW_VERSION_STRING;
}

static void send_periodic_status(void)
{
    char sts_line[STS_BUFFER_SIZE];

    hc_app_status_refresh_from_bsp();

    if (!hc_app_status_format_sts_json(sts_line, sizeof(sts_line)))
    {
        return;
    }

    (void)hc_comms_tx_send_line(sts_line);
}

void fw_app_init(void)
{
    uint32_t now_ms;

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
    (void)sync_drv_configure(&raw_cfg);

    now_ms = HAL_GetTick();
    ltc3901_manager_init(&s_ltc3901_manager, now_ms);
    s_ltc3901_pending_request = LTC3901_MGR_REQUEST_NONE;
    s_ltc3901_outputs = s_ltc3901_manager.outputs;
    apply_ltc3901_manager_outputs(&s_ltc3901_outputs);
    lt8316_manager_init(&s_lt8316_manager, now_ms);
    s_lt8316_pending_request = LT8316_MGR_REQUEST_NONE;
    s_lt8316_outputs = s_lt8316_manager.outputs;
    apply_lt8316_manager_outputs(&s_lt8316_outputs);

    s_last_toggle_ms = now_ms;
    s_last_sts_ms = now_ms;
    s_app_started_ms = now_ms;
    s_autostart_issued = false;
    s_sts_period_ms = STS_PERIOD_DEFAULT_MS;
    printf("App Initialized\r\n");
}

char buf[] = "Hello World! with a string that is a lot longer than 64 bytes because I've padded it with fluff.\r\n";
  
void fw_app_run(void)
{
    uint32_t now_ms = HAL_GetTick();

    adc_sense_drv_task();
    usb_vcp_drv_task();
    pwm_capture_drv_task();
    command_processor_task();

    hc_app_status_refresh_from_bsp();
    service_autostart(now_ms);
    ltc3901_manager_app_task(now_ms);
    lt8316_manager_app_task(now_ms);
    hc_debug_telemetry_task();

    if ((now_ms - s_last_toggle_ms) >= HEARTBEAT_PERIOD_MS)
    {
        s_last_toggle_ms = now_ms;
        bsp_led_toggle(BSP_LED_BLUE);
//        printf("App Initialized\r\n");
//        printf("Test printf: %s", buf);
    }

    if ((s_sts_period_ms != 0U) && ((now_ms - s_last_sts_ms) >= s_sts_period_ms))
    {
        s_last_sts_ms = now_ms;
        send_periodic_status();
    }
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
           (strcmp(name, "led.green") == 0);
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
        return fw_app_set_ltc3901_command(value ?
                                          FW_APP_LTC3901_COMMAND_RUN :
                                          FW_APP_LTC3901_COMMAND_RESET);
    }
    else if (strcmp(name, "lt8316.pwr_en") == 0)
    {
        return fw_app_set_lt8316_command(value ?
                                         FW_APP_LT8316_COMMAND_RUN :
                                         FW_APP_LT8316_COMMAND_RESET);
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
        return false;
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
