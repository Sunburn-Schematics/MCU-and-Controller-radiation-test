#include "hc_debug_telemetry.h"

#include "stm32f4xx_hal.h"
#include "adc_sense_drv.h"
#include "hc_app_status.h"
#include "hc_comms_tx.h"
#include "hc_cmd_types.h"
#include "hc_datetime.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define HC_DEBUG_TELEMETRY_BUFFER_SIZE   (1024U)
#define HC_DEBUG_TELEMETRY_MIN_PERIOD_MS (100U)
#define HC_DEBUG_TELEMETRY_MAX_PERIOD_MS (60000U)

typedef enum
{
    HC_DEBUG_VALUE_INT = 0,
    HC_DEBUG_VALUE_BOOL,
    HC_DEBUG_VALUE_STRING
} hc_debug_value_type_t;

typedef struct
{
    hc_debug_value_type_t Type;
    union
    {
        int32_t IntValue;
        bool BoolValue;
        const char *StringValue;
    } Data;
} hc_debug_value_t;

typedef bool (*hc_debug_value_getter_t)(hc_debug_value_t *value_out);

typedef struct
{
    const char *Name;
    hc_debug_value_getter_t Getter;
} hc_debug_signal_def_t;

static hc_debug_telemetry_config_t s_debug_config;
static uint32_t s_last_tx_ms;
static const hc_app_status_t *s_active_status;

static bool hc_debug_telemetry_append(char *buffer,
                                      size_t buffer_size,
                                      size_t *offset,
                                      const char *format,
                                      ...)
{
    va_list args;
    int written;

    if ((buffer == NULL) || (offset == NULL) || (format == NULL) || (*offset >= buffer_size))
    {
        return false;
    }

    va_start(args, format);
    written = vsnprintf(&buffer[*offset], buffer_size - *offset, format, args);
    va_end(args);

    if ((written < 0) || ((size_t)written >= (buffer_size - *offset)))
    {
        return false;
    }

    *offset += (size_t)written;
    return true;
}

static bool hc_debug_telemetry_get_status_int(int32_t value, hc_debug_value_t *value_out)
{
    if ((value_out == NULL) || (value < 0))
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_INT;
    value_out->Data.IntValue = value;
    return true;
}

static bool hc_debug_telemetry_get_status_bool(bool value, hc_debug_value_t *value_out)
{
    if (value_out == NULL)
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_BOOL;
    value_out->Data.BoolValue = value;
    return true;
}

static bool hc_debug_telemetry_get_status_string(const char *value, hc_debug_value_t *value_out)
{
    if ((value_out == NULL) || (value == NULL))
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_STRING;
    value_out->Data.StringValue = value;
    return true;
}

static bool hc_debug_telemetry_get_adc_raw(adc_sense_channel_t channel, hc_debug_value_t *value_out)
{
    if (value_out == NULL)
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_INT;
    value_out->Data.IntValue = (int32_t)adc_sense_drv_get_raw(channel);
    return true;
}

static bool hc_debug_telemetry_get_adc_mv(adc_sense_channel_t channel, hc_debug_value_t *value_out)
{
    int32_t value_mv;

    if (value_out == NULL)
    {
        return false;
    }

    value_mv = adc_sense_drv_get_channel_millivolts(channel);
    if (value_mv < 0)
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_INT;
    value_out->Data.IntValue = value_mv;
    return true;
}

static bool hc_debug_telemetry_get_adc_eng(adc_sense_channel_t channel, hc_debug_value_t *value_out)
{
    int32_t value_eng;

    if (value_out == NULL)
    {
        return false;
    }

    if (!adc_sense_drv_get_channel_engineering_units(channel, &value_eng))
    {
        return false;
    }

    value_out->Type = HC_DEBUG_VALUE_INT;
    value_out->Data.IntValue = value_eng;
    return true;
}

static bool hc_debug_telemetry_get_adc_vupstream_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_VUPSTREAM, value_out);
}

static bool hc_debug_telemetry_get_adc_vupstream_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_VUPSTREAM, value_out);
}

static bool hc_debug_telemetry_get_adc_vupstream_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_VUPSTREAM, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_vcc_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_LTC3901_VCC, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_vcc_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_LTC3901_VCC, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_vcc_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_LTC3901_VCC, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_me_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_LTC3901_ME_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_me_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_LTC3901_ME_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_me_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_LTC3901_ME_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_mf_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_LTC3901_MF_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_mf_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_LTC3901_MF_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_ltc3901_mf_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_LTC3901_MF_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_gate_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_LT8316_GATE_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_gate_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_LT8316_GATE_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_gate_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_LT8316_GATE_ANLG, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_vout_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_LT8316_VOUT, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_vout_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_LT8316_VOUT, value_out);
}

static bool hc_debug_telemetry_get_adc_lt8316_vout_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_LT8316_VOUT, value_out);
}

static bool hc_debug_telemetry_get_adc_temp_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_TEMP, value_out);
}

static bool hc_debug_telemetry_get_adc_temp_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_TEMP, value_out);
}

static bool hc_debug_telemetry_get_adc_temp_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_TEMP, value_out);
}

static bool hc_debug_telemetry_get_adc_vrefint_raw(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_raw(ADC_SENSE_CHANNEL_VREFINT, value_out);
}

static bool hc_debug_telemetry_get_adc_vrefint_mv(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_mv(ADC_SENSE_CHANNEL_VREFINT, value_out);
}

static bool hc_debug_telemetry_get_adc_vrefint_eng(hc_debug_value_t *value_out)
{
    return hc_debug_telemetry_get_adc_eng(ADC_SENSE_CHANNEL_VREFINT, value_out);
}

static bool hc_debug_telemetry_get_pwm_me_freq(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_int(s_active_status->Ltc3901.MeFreq_Hz, value_out);
}

static bool hc_debug_telemetry_get_pwm_me_duty(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_int(s_active_status->Ltc3901.MeRatio_Pct, value_out);
}

static bool hc_debug_telemetry_get_pwm_mf_freq(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_int(s_active_status->Ltc3901.MfFreq_Hz, value_out);
}

static bool hc_debug_telemetry_get_pwm_mf_duty(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_int(s_active_status->Ltc3901.MfRatio_Pct, value_out);
}

static bool hc_debug_telemetry_get_pwm_gate_freq(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_int(s_active_status->Lt8316.GateFreq_Hz, value_out);
}

static bool hc_debug_telemetry_get_beam_on(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_bool(s_active_status->BeamOn, value_out);
}

static bool hc_debug_telemetry_get_ltc3901_pwr_en(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_bool(s_active_status->Ltc3901.PowerEnabled, value_out);
}

static bool hc_debug_telemetry_get_lt8316_pwr_en(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_bool(s_active_status->Lt8316.PowerEnabled, value_out);
}

static bool hc_debug_telemetry_get_hc_state(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_string(hc_app_state_to_string(s_active_status->State), value_out);
}

static bool hc_debug_telemetry_get_ltc3901_state(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_string(hc_dut_state_to_string(s_active_status->Ltc3901.State), value_out);
}

static bool hc_debug_telemetry_get_lt8316_state(hc_debug_value_t *value_out)
{
    if (s_active_status == NULL)
    {
        return false;
    }

    return hc_debug_telemetry_get_status_string(hc_dut_state_to_string(s_active_status->Lt8316.State), value_out);
}

static const hc_debug_signal_def_t s_signal_catalog[] = {
    { "adc.vupstream.raw",     hc_debug_telemetry_get_adc_vupstream_raw },
    { "adc.vupstream.mv",      hc_debug_telemetry_get_adc_vupstream_mv },
    { "adc.vupstream.eng",     hc_debug_telemetry_get_adc_vupstream_eng },
    { "adc.ltc3901_vcc.raw",   hc_debug_telemetry_get_adc_ltc3901_vcc_raw },
    { "adc.ltc3901_vcc.mv",    hc_debug_telemetry_get_adc_ltc3901_vcc_mv },
    { "adc.ltc3901_vcc.eng",   hc_debug_telemetry_get_adc_ltc3901_vcc_eng },
    { "adc.ltc3901_me.raw",    hc_debug_telemetry_get_adc_ltc3901_me_raw },
    { "adc.ltc3901_me.mv",     hc_debug_telemetry_get_adc_ltc3901_me_mv },
    { "adc.ltc3901_me.eng",    hc_debug_telemetry_get_adc_ltc3901_me_eng },
    { "adc.ltc3901_mf.raw",    hc_debug_telemetry_get_adc_ltc3901_mf_raw },
    { "adc.ltc3901_mf.mv",     hc_debug_telemetry_get_adc_ltc3901_mf_mv },
    { "adc.ltc3901_mf.eng",    hc_debug_telemetry_get_adc_ltc3901_mf_eng },
    { "adc.lt8316_gate.raw",   hc_debug_telemetry_get_adc_lt8316_gate_raw },
    { "adc.lt8316_gate.mv",    hc_debug_telemetry_get_adc_lt8316_gate_mv },
    { "adc.lt8316_gate.eng",   hc_debug_telemetry_get_adc_lt8316_gate_eng },
    { "adc.lt8316_vout.raw",   hc_debug_telemetry_get_adc_lt8316_vout_raw },
    { "adc.lt8316_vout.mv",    hc_debug_telemetry_get_adc_lt8316_vout_mv },
    { "adc.lt8316_vout.eng",   hc_debug_telemetry_get_adc_lt8316_vout_eng },
    { "adc.temp.raw",          hc_debug_telemetry_get_adc_temp_raw },
    { "adc.temp.mv",           hc_debug_telemetry_get_adc_temp_mv },
    { "adc.temp.eng",          hc_debug_telemetry_get_adc_temp_eng },
    { "adc.vrefint.raw",       hc_debug_telemetry_get_adc_vrefint_raw },
    { "adc.vrefint.mv",        hc_debug_telemetry_get_adc_vrefint_mv },
    { "adc.vrefint.eng",       hc_debug_telemetry_get_adc_vrefint_eng },
    { "pwm.me.freq_hz",        hc_debug_telemetry_get_pwm_me_freq },
    { "pwm.me.duty_pct",       hc_debug_telemetry_get_pwm_me_duty },
    { "pwm.mf.freq_hz",        hc_debug_telemetry_get_pwm_mf_freq },
    { "pwm.mf.duty_pct",       hc_debug_telemetry_get_pwm_mf_duty },
    { "pwm.gate.freq_hz",      hc_debug_telemetry_get_pwm_gate_freq },
    { "beam_on",               hc_debug_telemetry_get_beam_on },
    { "ltc3901.pwr_en",        hc_debug_telemetry_get_ltc3901_pwr_en },
    { "lt8316.pwr_en",         hc_debug_telemetry_get_lt8316_pwr_en },
    { "hc.state",              hc_debug_telemetry_get_hc_state },
    { "ltc3901.state",         hc_debug_telemetry_get_ltc3901_state },
    { "lt8316.state",          hc_debug_telemetry_get_lt8316_state },
};

static bool hc_debug_telemetry_format_signal_object(const uint8_t *signal_ids,
                                                    uint8_t signal_count,
                                                    char *buffer,
                                                    size_t buffer_size)
{
    size_t offset;
    uint8_t i;

    if ((signal_ids == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    offset = 0U;
    if (!hc_debug_telemetry_append(buffer, buffer_size, &offset, "{"))
    {
        return false;
    }

    for (i = 0U; i < signal_count; ++i)
    {
        const hc_debug_signal_def_t *signal_def;
        hc_debug_value_t value;
        bool value_valid;

        if (signal_ids[i] >= (uint8_t)(sizeof(s_signal_catalog) / sizeof(s_signal_catalog[0])))
        {
            continue;
        }

        signal_def = &s_signal_catalog[signal_ids[i]];
        value_valid = signal_def->Getter(&value);

        if (i > 0U)
        {
            if (!hc_debug_telemetry_append(buffer, buffer_size, &offset, ","))
            {
                return false;
            }
        }

        if (!hc_debug_telemetry_append(buffer, buffer_size, &offset, "\"%s\":", signal_def->Name))
        {
            return false;
        }

        if (!value_valid)
        {
            if (!hc_debug_telemetry_append(buffer, buffer_size, &offset, "null"))
            {
                return false;
            }
            continue;
        }

        switch (value.Type)
        {
        case HC_DEBUG_VALUE_INT:
            if (!hc_debug_telemetry_append(buffer,
                                           buffer_size,
                                           &offset,
                                           "%ld",
                                           (long)value.Data.IntValue))
            {
                return false;
            }
            break;

        case HC_DEBUG_VALUE_BOOL:
            if (!hc_debug_telemetry_append(buffer,
                                           buffer_size,
                                           &offset,
                                           "%s",
                                           value.Data.BoolValue ? "true" : "false"))
            {
                return false;
            }
            break;

        case HC_DEBUG_VALUE_STRING:
            if (!hc_debug_telemetry_append(buffer,
                                           buffer_size,
                                           &offset,
                                           "\"%s\"",
                                           value.Data.StringValue))
            {
                return false;
            }
            break;

        default:
            return false;
        }
    }

    return hc_debug_telemetry_append(buffer, buffer_size, &offset, "}");
}

static bool hc_debug_telemetry_format_dbg_json(char *buffer, size_t buffer_size)
{
    const hc_app_status_t *status;
    char ts[HC_CMD_MAX_DATE_TIME_LEN];
    size_t offset;
    char signals_json[HC_DEBUG_TELEMETRY_BUFFER_SIZE];

    if ((buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    hc_app_status_refresh_from_bsp();
    status = hc_app_status_get_const();
    s_active_status = status;
    if (status == NULL)
    {
        s_active_status = NULL;
        return false;
    }

    if (!hc_datetime_get_now(ts, sizeof(ts)))
    {
        (void)snprintf(ts, sizeof(ts), "19700101 00:00:00");
    }

    if (!hc_debug_telemetry_format_signal_object(s_debug_config.SignalIds,
                                                 s_debug_config.SignalCount,
                                                 signals_json,
                                                 sizeof(signals_json)))
    {
        s_active_status = NULL;
        return false;
    }

    offset = 0U;
    if (!hc_debug_telemetry_append(buffer,
                                   buffer_size,
                                   &offset,
                                   "{\"type\":\"DBG\",\"hc_id\":%u,\"ts\":\"%s\",\"signals\":%s}",
                                   (unsigned int)status->HcId,
                                   ts,
                                   signals_json))
    {
        s_active_status = NULL;
        return false;
    }

    s_active_status = NULL;
    return true;
}

void hc_debug_telemetry_init(void)
{
    memset(&s_debug_config, 0, sizeof(s_debug_config));
    s_last_tx_ms = HAL_GetTick();
}

void hc_debug_telemetry_task(void)
{
    char dbg_line[HC_DEBUG_TELEMETRY_BUFFER_SIZE];
    uint32_t now_ms;

    if ((s_debug_config.PeriodMs == 0U) || (s_debug_config.SignalCount == 0U))
    {
        return;
    }

    now_ms = HAL_GetTick();
    if ((now_ms - s_last_tx_ms) < s_debug_config.PeriodMs)
    {
        return;
    }

    s_last_tx_ms = now_ms;

    if (!hc_debug_telemetry_format_dbg_json(dbg_line, sizeof(dbg_line)))
    {
        return;
    }

    (void)hc_comms_tx_send_line(dbg_line);
}

bool hc_debug_telemetry_set_config(const hc_debug_telemetry_config_t *config)
{
    uint8_t i;

    if (config == NULL)
    {
        return false;
    }

    if (config->SignalCount > HC_DEBUG_TELEMETRY_MAX_SIGNALS)
    {
        return false;
    }

    if ((config->PeriodMs != 0U) &&
        ((config->PeriodMs < HC_DEBUG_TELEMETRY_MIN_PERIOD_MS) ||
         (config->PeriodMs > HC_DEBUG_TELEMETRY_MAX_PERIOD_MS)))
    {
        return false;
    }

    for (i = 0U; i < config->SignalCount; ++i)
    {
        uint8_t j;

        if (config->SignalIds[i] >= (uint8_t)(sizeof(s_signal_catalog) / sizeof(s_signal_catalog[0])))
        {
            return false;
        }

        for (j = (uint8_t)(i + 1U); j < config->SignalCount; ++j)
        {
            if (config->SignalIds[i] == config->SignalIds[j])
            {
                return false;
            }
        }
    }

    s_debug_config = *config;
    s_last_tx_ms = HAL_GetTick();
    return true;
}

bool hc_debug_telemetry_get_config(hc_debug_telemetry_config_t *config_out)
{
    if (config_out == NULL)
    {
        return false;
    }

    *config_out = s_debug_config;
    return true;
}

bool hc_debug_telemetry_lookup_signal_id(const char *name, size_t name_len, uint8_t *signal_id_out)
{
    uint8_t i;

    if ((name == NULL) || (signal_id_out == NULL))
    {
        return false;
    }

    for (i = 0U; i < (uint8_t)(sizeof(s_signal_catalog) / sizeof(s_signal_catalog[0])); ++i)
    {
        size_t catalog_name_len;

        catalog_name_len = strlen(s_signal_catalog[i].Name);
        if ((catalog_name_len == name_len) && (strncmp(s_signal_catalog[i].Name, name, name_len) == 0))
        {
            *signal_id_out = i;
            return true;
        }
    }

    return false;
}

const char *hc_debug_telemetry_get_signal_name(uint8_t signal_id)
{
    if (signal_id >= (uint8_t)(sizeof(s_signal_catalog) / sizeof(s_signal_catalog[0])))
    {
        return NULL;
    }

    return s_signal_catalog[signal_id].Name;
}

bool hc_debug_telemetry_format_signals_json(const uint8_t *signal_ids,
                                            uint8_t signal_count,
                                            char *buffer,
                                            size_t buffer_size)
{
    if ((signal_ids == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    hc_app_status_refresh_from_bsp();
    s_active_status = hc_app_status_get_const();
    if (s_active_status == NULL)
    {
        return false;
    }

    if (!hc_debug_telemetry_format_signal_object(signal_ids, signal_count, buffer, buffer_size))
    {
        s_active_status = NULL;
        return false;
    }

    s_active_status = NULL;
    return true;
}
