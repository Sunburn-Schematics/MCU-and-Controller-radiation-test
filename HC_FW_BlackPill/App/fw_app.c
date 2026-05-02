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
#include "hc_datetime.h"
#include "hc_app_status.h"

#define FW_APP_HEARTBEAT_PERIOD_MS    (500U)
#define FW_APP_STS_PERIOD_MS          (1000U)
#define FW_APP_STS_BUFFER_SIZE        (1024U)
static uint32_t s_last_toggle_ms;
static uint32_t s_last_sts_ms;

static void fw_app_format_json_int_or_null(char *buffer, size_t buffer_size, int32_t value)
{
    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if (value < 0)
    {
        (void)snprintf(buffer, buffer_size, "null");
        return;
    }

    (void)snprintf(buffer, buffer_size, "%ld", (long)value);
}

static void fw_app_send_periodic_status(void)
{
    char sts_line[FW_APP_STS_BUFFER_SIZE];
    char ts[HC_DATETIME_BUFFER_LEN];
    char ltc3901_vsupply[16];
    char ltc3901_vshunt[16];
    char ltc3901_isupply[16];
    char ltc3901_me_freq[16];
    char ltc3901_me_ratio[16];
    char ltc3901_me_anlg[16];
    char ltc3901_mf_freq[16];
    char ltc3901_mf_ratio[16];
    char ltc3901_mf_anlg[16];
    char lt8316_gate_freq[16];
    char lt8316_gate_ratio[16];
    char lt8316_gate_anlg[16];
    char lt8316_vout[16];
    const hc_app_status_t *status = hc_app_status_get_const();

    if (!hc_datetime_get_now(ts, sizeof(ts)))
    {
        (void)snprintf(ts, sizeof(ts), "19700101 00:00:00");
    }

    fw_app_format_json_int_or_null(ltc3901_vsupply, sizeof(ltc3901_vsupply), status->Ltc3901.VSupply_mV);
    fw_app_format_json_int_or_null(ltc3901_vshunt, sizeof(ltc3901_vshunt), status->Ltc3901.VShunt_mV);
    fw_app_format_json_int_or_null(ltc3901_isupply, sizeof(ltc3901_isupply), status->Ltc3901.ISupply_mA);
    fw_app_format_json_int_or_null(ltc3901_me_freq, sizeof(ltc3901_me_freq), status->Ltc3901.MeFreq_Hz);
    fw_app_format_json_int_or_null(ltc3901_me_ratio, sizeof(ltc3901_me_ratio), status->Ltc3901.MeRatio_Pct);
    fw_app_format_json_int_or_null(ltc3901_me_anlg, sizeof(ltc3901_me_anlg), status->Ltc3901.MeAnlg_mV);
    fw_app_format_json_int_or_null(ltc3901_mf_freq, sizeof(ltc3901_mf_freq), status->Ltc3901.MfFreq_Hz);
    fw_app_format_json_int_or_null(ltc3901_mf_ratio, sizeof(ltc3901_mf_ratio), status->Ltc3901.MfRatio_Pct);
    fw_app_format_json_int_or_null(ltc3901_mf_anlg, sizeof(ltc3901_mf_anlg), status->Ltc3901.MfAnlg_mV);
    fw_app_format_json_int_or_null(lt8316_gate_freq, sizeof(lt8316_gate_freq), status->Lt8316.GateFreq_Hz);
    fw_app_format_json_int_or_null(lt8316_gate_ratio, sizeof(lt8316_gate_ratio), status->Lt8316.GateRatio_Pct);
    fw_app_format_json_int_or_null(lt8316_gate_anlg, sizeof(lt8316_gate_anlg), status->Lt8316.GateAnlg_mV);
    fw_app_format_json_int_or_null(lt8316_vout, sizeof(lt8316_vout), status->Lt8316.VOut_mV);

    (void)snprintf(
        sts_line,
        sizeof(sts_line),
        "{\"type\":\"STS\",\"hc_id\":%u,\"ts\":\"%s\",\"state\":\"%s\",\"beam_on\":%s,\"duts\":{\"LTC3901\":{\"state\":\"%s\",\"pwr_en\":%s,\"sync\":%s,\"vsupply\":%s,\"vshunt\":%s,\"isupply\":%s,\"me_freq\":%s,\"me_ratio\":%s,\"me_anlg\":%s,\"mf_freq\":%s,\"mf_ratio\":%s,\"mf_anlg\":%s,\"faults\":{\"count\":%lu,\"summary\":\"%s\",\"ids\":%s}},\"LT8316\":{\"state\":\"%s\",\"pwr_en\":%s,\"gate_freq\":%s,\"gate_ratio\":%s,\"gate_anlg\":%s,\"vout\":%s,\"faults\":{\"count\":%lu,\"summary\":\"%s\",\"ids\":%s}}}}",
        (unsigned int)status->HcId,
        ts,
        hc_app_state_to_string(status->State),
        status->BeamOn ? "true" : "false",
        hc_dut_state_to_string(status->Ltc3901.State),
        status->Ltc3901.PowerEnabled ? "true" : "false",
        status->Ltc3901.SyncEnabled ? "true" : "false",
        ltc3901_vsupply,
        ltc3901_vshunt,
        ltc3901_isupply,
        ltc3901_me_freq,
        ltc3901_me_ratio,
        ltc3901_me_anlg,
        ltc3901_mf_freq,
        ltc3901_mf_ratio,
        ltc3901_mf_anlg,
        (unsigned long)status->Ltc3901.Faults.Count,
        status->Ltc3901.Faults.Summary,
        status->Ltc3901.Faults.IdsJson,
        hc_dut_state_to_string(status->Lt8316.State),
        status->Lt8316.PowerEnabled ? "true" : "false",
        lt8316_gate_freq,
        lt8316_gate_ratio,
        lt8316_gate_anlg,
        lt8316_vout,
        (unsigned long)status->Lt8316.Faults.Count,
        status->Lt8316.Faults.Summary,
        status->Lt8316.Faults.IdsJson);

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
