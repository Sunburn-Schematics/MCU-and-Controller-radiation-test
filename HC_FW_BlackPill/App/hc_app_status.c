#include "hc_app_status.h"

#include <stdio.h>

#include "bsp_board.h"
#include "adc_sense_drv.h"
#include "hc_datetime.h"
#include "pwm_capture_drv.h"

#define HC_APP_ISUPPLY_SHUNT_OHMS (10L)

static hc_app_status_t s_hc_app_status;
static bool s_ltc3901_manager_state_valid;
static const char *s_ltc3901_manager_state_name;
static bool s_ltc3901_manager_sync_enabled;
static bool s_lt8316_manager_state_valid;
static const char *s_lt8316_manager_state_name;

static void hc_app_format_json_int_or_null(char *buffer, size_t buffer_size, int32_t value)
{
    if ((buffer == 0) || (buffer_size == 0U))
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

static int32_t hc_app_ratio_x100_to_pct(uint16_t ratio_x100)
{
    return (int32_t)((ratio_x100 + 50U) / 100U);
}

static int32_t hc_app_calculate_isupply_ma(int32_t vsupply_mv, int32_t vshunt_mv)
{
    int32_t shunt_mv;

    if ((vsupply_mv < 0) || (vshunt_mv < 0) || (vsupply_mv < vshunt_mv))
    {
        return -1;
    }

    shunt_mv = vsupply_mv - vshunt_mv;
    return (int32_t)(((int64_t)shunt_mv +
                      (HC_APP_ISUPPLY_SHUNT_OHMS / 2L)) /
                     HC_APP_ISUPPLY_SHUNT_OHMS);
}

static int32_t hc_app_get_adc_engineering_or_pin_mv(adc_sense_channel_t channel)
{
    int32_t engineering_value;

    if (adc_sense_drv_get_channel_engineering_units(channel, &engineering_value))
    {
        return engineering_value;
    }

    return adc_sense_drv_get_channel_millivolts(channel);
}

void hc_app_status_init(void)
{
    s_hc_app_status.HcId = 1U;
    s_hc_app_status.BeamOn = false;

    s_hc_app_status.Ltc3901.ManagerState = "RESET";
    s_hc_app_status.Ltc3901.PowerEnabled = false;
    s_hc_app_status.Ltc3901.SyncEnabled = true;
    s_hc_app_status.Ltc3901.VSupply_mV = -1;
    s_hc_app_status.Ltc3901.VShunt_mV = -1;
    s_hc_app_status.Ltc3901.ISupply_mA = -1;
    s_hc_app_status.Ltc3901.MeFreq_Hz = -1;
    s_hc_app_status.Ltc3901.MeRatio_Pct = -1;
    s_hc_app_status.Ltc3901.MeAnlg_mV = -1;
    s_hc_app_status.Ltc3901.MfFreq_Hz = -1;
    s_hc_app_status.Ltc3901.MfRatio_Pct = -1;
    s_hc_app_status.Ltc3901.MfAnlg_mV = -1;

    s_hc_app_status.Lt8316.ManagerState = "RESET";
    s_hc_app_status.Lt8316.PowerEnabled = false;
    s_hc_app_status.Lt8316.GateFreq_Hz = -1;
    s_hc_app_status.Lt8316.GateAnlg_mV = -1;
    s_hc_app_status.Lt8316.VOut_mV = -1;

    s_ltc3901_manager_state_valid = false;
    s_ltc3901_manager_state_name = "RESET";
    s_ltc3901_manager_sync_enabled = false;
    s_lt8316_manager_state_valid = false;
    s_lt8316_manager_state_name = "RESET";

    hc_app_status_refresh_from_bsp();
}

void hc_app_status_refresh_from_bsp(void)
{
    bsp_status_t bsp_status;
    pwm_capture_result_t capture_result;

    bsp_get_status(&bsp_status);

    s_hc_app_status.HcId = (uint32_t)bsp_status.id_raw;
    s_hc_app_status.BeamOn = bsp_status.beam_on;

    s_hc_app_status.Ltc3901.PowerEnabled = bsp_power_is_enabled(BSP_POWER_LTC3901);
    if (s_ltc3901_manager_state_valid)
    {
        s_hc_app_status.Ltc3901.ManagerState = s_ltc3901_manager_state_name;
        s_hc_app_status.Ltc3901.SyncEnabled = s_ltc3901_manager_sync_enabled;
    }
    else
    {
        s_hc_app_status.Ltc3901.ManagerState = "UNMANAGED";
        s_hc_app_status.Ltc3901.SyncEnabled = false;
    }
    s_hc_app_status.Ltc3901.VSupply_mV = hc_app_get_adc_engineering_or_pin_mv(ADC_SENSE_CHANNEL_VUPSTREAM);
    s_hc_app_status.Ltc3901.VShunt_mV = hc_app_get_adc_engineering_or_pin_mv(ADC_SENSE_CHANNEL_LTC3901_VCC);
    s_hc_app_status.Ltc3901.ISupply_mA = hc_app_calculate_isupply_ma(s_hc_app_status.Ltc3901.VSupply_mV,
                                                                     s_hc_app_status.Ltc3901.VShunt_mV);
    s_hc_app_status.Ltc3901.MeAnlg_mV = adc_sense_drv_get_channel_millivolts(ADC_SENSE_CHANNEL_LTC3901_ME_ANLG);
    s_hc_app_status.Ltc3901.MfAnlg_mV = adc_sense_drv_get_channel_millivolts(ADC_SENSE_CHANNEL_LTC3901_MF_ANLG);
    if (pwm_capture_drv_get_result(PWM_CAPTURE_SIGNAL_LTC3901_ME, &capture_result))
    {
        s_hc_app_status.Ltc3901.MeFreq_Hz = (int32_t)capture_result.frequency_hz;
        s_hc_app_status.Ltc3901.MeRatio_Pct = capture_result.has_duty_cycle ?
                                              hc_app_ratio_x100_to_pct(capture_result.duty_pct_x100) : -1;
    }
    else
    {
        s_hc_app_status.Ltc3901.MeFreq_Hz = -1;
        s_hc_app_status.Ltc3901.MeRatio_Pct = -1;
    }

    if (pwm_capture_drv_get_result(PWM_CAPTURE_SIGNAL_LTC3901_MF, &capture_result))
    {
        s_hc_app_status.Ltc3901.MfFreq_Hz = (int32_t)capture_result.frequency_hz;
        s_hc_app_status.Ltc3901.MfRatio_Pct = capture_result.has_duty_cycle ?
                                              hc_app_ratio_x100_to_pct(capture_result.duty_pct_x100) : -1;
    }
    else
    {
        s_hc_app_status.Ltc3901.MfFreq_Hz = -1;
        s_hc_app_status.Ltc3901.MfRatio_Pct = -1;
    }

    s_hc_app_status.Lt8316.PowerEnabled = bsp_power_is_enabled(BSP_POWER_LT8316);
    if (s_lt8316_manager_state_valid)
    {
        s_hc_app_status.Lt8316.ManagerState = s_lt8316_manager_state_name;
    }
    else
    {
        s_hc_app_status.Lt8316.ManagerState = "UNMANAGED";
    }
    s_hc_app_status.Lt8316.GateAnlg_mV = adc_sense_drv_get_channel_millivolts(ADC_SENSE_CHANNEL_LT8316_GATE_ANLG);
    s_hc_app_status.Lt8316.VOut_mV = adc_sense_drv_get_channel_millivolts(ADC_SENSE_CHANNEL_LT8316_VOUT);
    if (pwm_capture_drv_get_result(PWM_CAPTURE_SIGNAL_LT8316_GATE, &capture_result))
    {
        s_hc_app_status.Lt8316.GateFreq_Hz = (int32_t)capture_result.frequency_hz;
    }
    else
    {
        s_hc_app_status.Lt8316.GateFreq_Hz = -1;
    }
}

void hc_app_status_set_ltc3901_manager_state(const char *manager_state,
                                             bool sync_enabled)
{
    if (manager_state == 0)
    {
        manager_state = "RESET";
    }

    s_ltc3901_manager_state_valid = true;
    s_ltc3901_manager_state_name = manager_state;
    s_ltc3901_manager_sync_enabled = sync_enabled;

    s_hc_app_status.Ltc3901.ManagerState = manager_state;
    s_hc_app_status.Ltc3901.PowerEnabled = bsp_power_is_enabled(BSP_POWER_LTC3901);
    s_hc_app_status.Ltc3901.SyncEnabled = sync_enabled;
}

void hc_app_status_set_lt8316_manager_state(const char *manager_state)
{
    if (manager_state == 0)
    {
        manager_state = "RESET";
    }

    s_lt8316_manager_state_valid = true;
    s_lt8316_manager_state_name = manager_state;

    s_hc_app_status.Lt8316.ManagerState = manager_state;
    s_hc_app_status.Lt8316.PowerEnabled = bsp_power_is_enabled(BSP_POWER_LT8316);
}

hc_app_status_t *hc_app_status_get(void)
{
    return &s_hc_app_status;
}

const hc_app_status_t *hc_app_status_get_const(void)
{
    return &s_hc_app_status;
}

bool hc_app_status_format_sts_json(char *buffer, size_t buffer_len)
{
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
    char lt8316_gate_anlg[16];
    char lt8316_vout[16];
    int written;
    const hc_app_status_t *status = hc_app_status_get_const();

    if ((buffer == 0) || (buffer_len == 0U))
    {
        return false;
    }

    if (!hc_datetime_get_now(ts, sizeof(ts)))
    {
        (void)snprintf(ts, sizeof(ts), "19700101 00:00:00");
    }

    hc_app_format_json_int_or_null(ltc3901_vsupply, sizeof(ltc3901_vsupply), status->Ltc3901.VSupply_mV);
    hc_app_format_json_int_or_null(ltc3901_vshunt, sizeof(ltc3901_vshunt), status->Ltc3901.VShunt_mV);
    hc_app_format_json_int_or_null(ltc3901_isupply, sizeof(ltc3901_isupply), status->Ltc3901.ISupply_mA);
    hc_app_format_json_int_or_null(ltc3901_me_freq, sizeof(ltc3901_me_freq), status->Ltc3901.MeFreq_Hz);
    hc_app_format_json_int_or_null(ltc3901_me_ratio, sizeof(ltc3901_me_ratio), status->Ltc3901.MeRatio_Pct);
    hc_app_format_json_int_or_null(ltc3901_me_anlg, sizeof(ltc3901_me_anlg), status->Ltc3901.MeAnlg_mV);
    hc_app_format_json_int_or_null(ltc3901_mf_freq, sizeof(ltc3901_mf_freq), status->Ltc3901.MfFreq_Hz);
    hc_app_format_json_int_or_null(ltc3901_mf_ratio, sizeof(ltc3901_mf_ratio), status->Ltc3901.MfRatio_Pct);
    hc_app_format_json_int_or_null(ltc3901_mf_anlg, sizeof(ltc3901_mf_anlg), status->Ltc3901.MfAnlg_mV);
    hc_app_format_json_int_or_null(lt8316_gate_freq, sizeof(lt8316_gate_freq), status->Lt8316.GateFreq_Hz);
    hc_app_format_json_int_or_null(lt8316_gate_anlg, sizeof(lt8316_gate_anlg), status->Lt8316.GateAnlg_mV);
    hc_app_format_json_int_or_null(lt8316_vout, sizeof(lt8316_vout), status->Lt8316.VOut_mV);

    written = snprintf(
        buffer,
        buffer_len,
        "{\"type\":\"STS\",\"hc_id\":%u,\"ts\":\"%s\",\"beam_on\":%s,\"duts\":{\"LTC3901\":{\"state\":\"%s\",\"pwr_en\":%s,\"sync\":%s,\"vsupply\":%s,\"vshunt\":%s,\"isupply\":%s,\"me_freq\":%s,\"me_ratio\":%s,\"me_anlg\":%s,\"mf_freq\":%s,\"mf_ratio\":%s,\"mf_anlg\":%s},\"LT8316\":{\"state\":\"%s\",\"pwr_en\":%s,\"gate_freq\":%s,\"gate_anlg\":%s,\"vout\":%s}}}",
        (unsigned int)status->HcId,
        ts,
        status->BeamOn ? "true" : "false",
        status->Ltc3901.ManagerState,
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
        status->Lt8316.ManagerState,
        status->Lt8316.PowerEnabled ? "true" : "false",
        lt8316_gate_freq,
        lt8316_gate_anlg,
        lt8316_vout);

    return (written >= 0) && ((size_t)written < buffer_len);
}
