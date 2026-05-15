#include "hc_jsonl_fields.h"

#include "hc_datetime.h"
#include "hc_jsonl_parse.h"
#include "hc_jsonl_rsp.h"
#include "adc_sense_drv.h"
#include "fw_app.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN (512U)
#define HC_JSONL_SET_ARGS_JSON_MAX_LEN    (1024U)

static bool hc_jsonl_validate_adc_channel(uint32_t channel)
{
    return (channel < (uint32_t)ADC_SENSE_CHANNEL_COUNT);
}

static const char *hc_jsonl_get_adc_channel_name(uint32_t channel)
{
    static const char * const s_adc_channel_names[] = {
        "vupstream",
        "ltc3901_vcc",
        "lt8316_vout",
        "ltc3901_me",
        "ltc3901_mf",
        "lt8316_gate",
        "temp",
        "vrefint",
    };

    if (channel >= (uint32_t)(sizeof(s_adc_channel_names) / sizeof(s_adc_channel_names[0])))
    {
        return NULL;
    }

    return s_adc_channel_names[channel];
}

static bool hc_jsonl_apply_ltc3901_cmd(const char *command)
{
    if (strcmp(command, "RUN") == 0)
    {
        return fw_app_set_ltc3901_command(FW_APP_LTC3901_COMMAND_RUN);
    }

    if (strcmp(command, "HALT") == 0)
    {
        return fw_app_set_ltc3901_command(FW_APP_LTC3901_COMMAND_HALT);
    }

    if (strcmp(command, "RESET") == 0)
    {
        return fw_app_set_ltc3901_command(FW_APP_LTC3901_COMMAND_RESET);
    }

    return false;
}

static bool hc_jsonl_apply_lt8316_cmd(const char *command)
{
    if (strcmp(command, "RUN") == 0)
    {
        return fw_app_set_lt8316_command(FW_APP_LT8316_COMMAND_RUN);
    }

    if (strcmp(command, "RESET") == 0)
    {
        return fw_app_set_lt8316_command(FW_APP_LT8316_COMMAND_RESET);
    }

    return false;
}

static bool hc_jsonl_apply_hc_cmd(const char *command)
{
    if (strcmp(command, "RESET") == 0)
    {
        return fw_app_request_system_reset();
    }

    return false;
}

static bool hc_jsonl_build_dbg_signals_json(const hc_debug_telemetry_config_t *config,
                                            char *buffer,
                                            size_t buffer_size)
{
    size_t offset;
    uint8_t i;

    if ((config == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    offset = 0U;
    if (snprintf(buffer, buffer_size, "[") >= (int)buffer_size)
    {
        return false;
    }
    offset = 1U;

    for (i = 0U; i < config->SignalCount; ++i)
    {
        const char *signal_name;
        int written;

        signal_name = fw_app_debug_get_signal_name(config->SignalIds[i]);
        if (signal_name == NULL)
        {
            return false;
        }

        written = snprintf(&buffer[offset],
                           buffer_size - offset,
                           "%s\"%s\"",
                           (i > 0U) ? "," : "",
                           signal_name);
        if ((written < 0) || ((size_t)written >= (buffer_size - offset)))
        {
            return false;
        }

        offset += (size_t)written;
    }

    if (snprintf(&buffer[offset], buffer_size - offset, "]") >= (int)(buffer_size - offset))
    {
        return false;
    }

    return true;
}

static bool hc_jsonl_build_ltc3901_cfg_json(const ltc3901_manager_config_t *config,
                                            char *buffer,
                                            size_t buffer_size)
{
    int written;

    if ((config == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    written = snprintf(buffer,
                       buffer_size,
                       "{\"isupply_ma_max\":%ld,\"vupstream_mv_min\":%ld,\"ltc3901_vcc_mv_min\":%ld,\"power_up_timeout_ms\":%lu,\"power_retry_delay_ms\":%lu,\"power_fault_max\":%lu,\"sync_on_delay_ms\":%lu,\"sync_hold_on_time_ms\":%lu,\"sync_hold_off_time_ms\":%lu,\"sync_stabilization_time_ms\":%lu,\"sync_fault_delay_ms\":%lu}",
                       (long)config->isupply_ma_max,
                       (long)config->vupstream_mv_min,
                       (long)config->ltc3901_vcc_mv_min,
                       (unsigned long)config->power_up_timeout_ms,
                       (unsigned long)config->power_retry_delay_ms,
                       (unsigned long)config->power_fault_max,
                       (unsigned long)config->sync_on_delay_ms,
                       (unsigned long)config->sync_hold_on_time_ms,
                       (unsigned long)config->sync_hold_off_time_ms,
                       (unsigned long)config->sync_stabilization_time_ms,
                       (unsigned long)config->sync_fault_delay_ms);

    return ((written > 0) && ((size_t)written < buffer_size));
}

static bool hc_jsonl_build_lt8316_cfg_json(const lt8316_manager_config_t *config,
                                           char *buffer,
                                           size_t buffer_size)
{
    int written;

    if ((config == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return false;
    }

    written = snprintf(buffer,
                       buffer_size,
                       "{\"power_retry_delay_ms\":%lu,\"power_fault_max\":%lu,\"power_on_stabilization_time_ms\":%lu}",
                       (unsigned long)config->power_retry_delay_ms,
                       (unsigned long)config->power_fault_max,
                       (unsigned long)config->power_on_stabilization_time_ms);

    return ((written > 0) && ((size_t)written < buffer_size));
}

static bool hc_jsonl_appendf(char *buffer,
                             size_t buffer_size,
                             size_t *offset,
                             const char *fmt,
                             ...)
{
    int written;
    va_list args;

    if ((buffer == NULL) || (offset == NULL) || (fmt == NULL) || (*offset >= buffer_size))
    {
        return false;
    }

    va_start(args, fmt);
    written = vsnprintf(&buffer[*offset], buffer_size - *offset, fmt, args);
    va_end(args);

    if ((written < 0) || ((size_t)written >= (buffer_size - *offset)))
    {
        return false;
    }

    *offset += (size_t)written;
    return true;
}

static bool hc_jsonl_append_set_arg_separator(char *buffer,
                                              size_t buffer_size,
                                              size_t *offset,
                                              bool *first)
{
    if ((first == NULL) || *first)
    {
        if (first != NULL)
        {
            *first = false;
        }
        return true;
    }

    return hc_jsonl_appendf(buffer, buffer_size, offset, ",");
}

static bool hc_jsonl_append_object_members(char *buffer,
                                           size_t buffer_size,
                                           size_t *offset,
                                           const char *object_json)
{
    size_t object_len;

    if ((buffer == NULL) || (offset == NULL) || (object_json == NULL))
    {
        return false;
    }

    object_len = strlen(object_json);
    if ((object_len < 2U) ||
        (object_json[0] != '{') ||
        (object_json[object_len - 1U] != '}') ||
        (*offset + object_len - 2U >= buffer_size))
    {
        return false;
    }

    memcpy(&buffer[*offset], &object_json[1], object_len - 2U);
    *offset += object_len - 2U;
    buffer[*offset] = '\0';

    return true;
}

hc_cmd_status_t hc_jsonl_handle_set(const char *line,
                                    const jsmntok_t *tokens,
                                    const hc_cmd_request_t *request,
                                    char *rsp_buf,
                                    size_t rsp_buf_size)
{
    char date_time[HC_CMD_MAX_DATE_TIME_LEN] = {0};
    char args_json[HC_JSONL_SET_ARGS_JSON_MAX_LEN];
    char dbg_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    char digital_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    hc_jsonl_set_sts_period_request_t sts_period_request;
    hc_jsonl_set_debug_request_t debug_request;
    hc_jsonl_set_adc_cal_request_t adc_cal_request;
    hc_jsonl_set_ltc3901_cmd_request_t ltc3901_cmd_request;
    hc_jsonl_set_lt8316_cmd_request_t lt8316_cmd_request;
    hc_jsonl_set_hc_cmd_request_t hc_cmd_request;
    hc_jsonl_set_ltc3901_cfg_request_t ltc3901_cfg_request;
    hc_jsonl_set_lt8316_cfg_request_t lt8316_cfg_request;
    hc_jsonl_set_digital_signals_request_t digital_signals_request;
    adc_sense_calibration_t calibration;
    ltc3901_manager_config_t ltc3901_config;
    lt8316_manager_config_t lt8316_config;
    hc_debug_telemetry_config_t debug_config;
    size_t args_offset = 0U;
    bool first_arg = true;
    bool any_set_field = false;
    const char *current_date_time;
    hc_cmd_status_t date_time_parse_status;
    hc_cmd_status_t sts_period_parse_status;
    hc_cmd_status_t debug_parse_status;
    hc_cmd_status_t adc_cal_parse_status;
    hc_cmd_status_t ltc3901_cmd_parse_status;
    hc_cmd_status_t lt8316_cmd_parse_status;
    hc_cmd_status_t hc_cmd_parse_status;
    hc_cmd_status_t ltc3901_cfg_parse_status;
    hc_cmd_status_t lt8316_cfg_parse_status;
    hc_cmd_status_t digital_signals_parse_status;
    uint8_t set_field_count = 0U;

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    date_time_parse_status = hc_jsonl_parse_set_date_time(line, tokens, request, date_time, sizeof(date_time));
    sts_period_parse_status = hc_jsonl_parse_set_sts_period_ms(line, tokens, request, &sts_period_request);
    debug_parse_status = hc_jsonl_parse_set_debug_config(line, tokens, request, &debug_request);
    adc_cal_parse_status = hc_jsonl_parse_set_adc_calibration(line, tokens, request, &adc_cal_request);
    ltc3901_cmd_parse_status = hc_jsonl_parse_set_ltc3901_cmd(line, tokens, request, &ltc3901_cmd_request);
    lt8316_cmd_parse_status = hc_jsonl_parse_set_lt8316_cmd(line, tokens, request, &lt8316_cmd_request);
    hc_cmd_parse_status = hc_jsonl_parse_set_hc_cmd(line, tokens, request, &hc_cmd_request);
    ltc3901_cfg_parse_status = hc_jsonl_parse_set_ltc3901_cfg(line, tokens, request, &ltc3901_cfg_request);
    lt8316_cfg_parse_status = hc_jsonl_parse_set_lt8316_cfg(line, tokens, request, &lt8316_cfg_request);
    digital_signals_parse_status = hc_jsonl_parse_set_digital_signals(line, tokens, request, &digital_signals_request);

    if ((date_time_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (sts_period_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (debug_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (adc_cal_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (ltc3901_cmd_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (lt8316_cmd_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (hc_cmd_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (ltc3901_cfg_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (lt8316_cfg_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (digital_signals_parse_status == HC_CMD_ERR_BAD_ARGS))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_ARGS",
                                      "SET requires an args object"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (date_time_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.date_time must be a valid timestamp string"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (sts_period_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.sts_period_ms must be a non-negative integer"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (debug_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.dbg_period_ms must be 0 or 100..60000, and args.dbg_signals must be a valid signal array"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (adc_cal_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "ADC calibration SET requires one or more adc.<channel> calibration fields for one channel"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if ((adc_cal_parse_status == HC_CMD_OK) && !hc_jsonl_validate_adc_channel(adc_cal_request.Channel))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "ADC calibration channel is out of range"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (ltc3901_cmd_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.ltc3901_cmd must be one of RUN, HALT, or RESET"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (lt8316_cmd_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.lt8316_cmd must be one of RUN or RESET"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (hc_cmd_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.hc_cmd must be RESET"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if ((digital_signals_parse_status == HC_CMD_ERR_BAD_VALUE) && !debug_request.HasSignals)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "settable digital signal args must use direct signal names with boolean values"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (ltc3901_cfg_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "LTC3901 config SET requires valid direct ltc3901.<field> numeric values"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (lt8316_cfg_parse_status == HC_CMD_ERR_BAD_VALUE)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "LT8316 config SET requires valid direct lt8316.<field> numeric values"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if ((date_time_parse_status != HC_CMD_OK) &&
        (sts_period_parse_status != HC_CMD_OK) &&
        (debug_parse_status != HC_CMD_OK) &&
        (adc_cal_parse_status != HC_CMD_OK) &&
        (ltc3901_cmd_parse_status != HC_CMD_OK) &&
        (lt8316_cmd_parse_status != HC_CMD_OK) &&
        (hc_cmd_parse_status != HC_CMD_OK) &&
        (ltc3901_cfg_parse_status != HC_CMD_OK) &&
        (lt8316_cfg_parse_status != HC_CMD_OK) &&
        (digital_signals_parse_status != HC_CMD_OK))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_FIELD",
                                      "SET currently supports args.date_time, args.sts_period_ms, args.dbg_period_ms, args.dbg_signals, args.hc_cmd, direct adc.<channel> calibration fields, args.ltc3901_cmd, args.lt8316_cmd, direct ltc3901.<field> config fields, direct lt8316.<field> config fields, or direct settable digital signal fields"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (date_time_parse_status == HC_CMD_OK) { set_field_count++; }
    if (sts_period_parse_status == HC_CMD_OK) { set_field_count++; }
    if (debug_parse_status == HC_CMD_OK) { set_field_count++; }
    if (adc_cal_parse_status == HC_CMD_OK) { set_field_count++; }
    if (ltc3901_cmd_parse_status == HC_CMD_OK) { set_field_count++; }
    if (lt8316_cmd_parse_status == HC_CMD_OK) { set_field_count++; }
    if (hc_cmd_parse_status == HC_CMD_OK) { set_field_count++; }
    if (ltc3901_cfg_parse_status == HC_CMD_OK) { set_field_count++; }
    if (lt8316_cfg_parse_status == HC_CMD_OK) { set_field_count++; }
    if (digital_signals_parse_status == HC_CMD_OK) { set_field_count++; }

    if ((hc_cmd_parse_status == HC_CMD_OK) && (set_field_count > 1U))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_STATE",
                                      "hc_cmd RESET must be sent as a standalone SET command"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_STATE;
    }

    args_json[0] = '\0';
    if (!hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "{"))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (date_time_parse_status == HC_CMD_OK)
    {
        switch (hc_datetime_set(date_time))
        {
            case HC_DATETIME_OK:
                break;

            case HC_DATETIME_ERR_BAD_FORMAT:
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "BAD_VALUE",
                                              "args.date_time must match YYYYMMDD HH:MM:SS"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_BAD_VALUE;

            case HC_DATETIME_ERR_BAD_VALUE:
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "BAD_VALUE",
                                              "args.date_time contains an invalid calendar/time value"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_BAD_VALUE;

            case HC_DATETIME_ERR_RTC_WRITE:
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "INTERNAL",
                                              "RTC write failed while applying args.date_time"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_INTERNAL;

            case HC_DATETIME_ERR_RTC_READ:
            default:
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "INTERNAL",
                                              "RTC state unavailable"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_INTERNAL;
        }

        current_date_time = hc_datetime_get();
        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"date_time\":\"%s\"", current_date_time))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (sts_period_parse_status == HC_CMD_OK)
    {
        if (!fw_app_set_sts_period_ms(sts_period_request.PeriodMs))
        {
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "Failed to apply args.sts_period_ms"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json,
                              sizeof(args_json),
                              &args_offset,
                              "\"sts_period_ms\":%lu",
                              (unsigned long)fw_app_get_sts_period_ms()))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (debug_parse_status == HC_CMD_OK)
    {
        if (!fw_app_get_debug_config(&debug_config))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (debug_request.HasPeriodMs)
        {
            debug_config.PeriodMs = debug_request.PeriodMs;
        }

        if (debug_request.HasSignals)
        {
            debug_config.SignalCount = debug_request.SignalCount;
            memcpy(debug_config.SignalIds,
                   debug_request.SignalIds,
                   sizeof(debug_request.SignalIds));
        }

        if (!fw_app_set_debug_config(&debug_config) ||
            !fw_app_get_debug_config(&debug_config))
        {
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "args.dbg_period_ms or args.dbg_signals is invalid"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (debug_request.HasPeriodMs)
        {
            if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
                !hc_jsonl_appendf(args_json,
                                  sizeof(args_json),
                                  &args_offset,
                                  "\"dbg_period_ms\":%lu",
                                  (unsigned long)debug_config.PeriodMs))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }

        if (debug_request.HasSignals)
        {
            if (!hc_jsonl_build_dbg_signals_json(&debug_config, dbg_signals_json, sizeof(dbg_signals_json)) ||
                !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
                !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"dbg_signals\":%s", dbg_signals_json))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }
        any_set_field = true;
    }

    if (adc_cal_parse_status == HC_CMD_OK)
    {
        const char *channel_name;
        bool first_cal_arg = true;

        if (!adc_sense_drv_get_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (adc_cal_request.HasSlopeScaled)
        {
            calibration.SlopeScaled = adc_cal_request.SlopeScaled;
        }
        if (adc_cal_request.HasOffset)
        {
            calibration.Offset = adc_cal_request.Offset;
        }
        if (adc_cal_request.HasValid)
        {
            calibration.Valid = adc_cal_request.Valid;
        }

        if (!adc_sense_drv_set_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        channel_name = hc_jsonl_get_adc_channel_name(adc_cal_request.Channel);
        if (channel_name == NULL)
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (adc_cal_request.HasSlopeScaled)
        {
            if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_cal_arg) ||
                !hc_jsonl_appendf(args_json,
                                  sizeof(args_json),
                                  &args_offset,
                                  "\"adc.%s.slope_scaled\":%ld",
                                  channel_name,
                                  (long)calibration.SlopeScaled))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }

        if (adc_cal_request.HasOffset)
        {
            if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_cal_arg) ||
                !hc_jsonl_appendf(args_json,
                                  sizeof(args_json),
                                  &args_offset,
                                  "\"adc.%s.offset\":%ld",
                                  channel_name,
                                  (long)calibration.Offset))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }

        if (adc_cal_request.HasValid)
        {
            if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_cal_arg) ||
                !hc_jsonl_appendf(args_json,
                                  sizeof(args_json),
                                  &args_offset,
                                  "\"adc.%s.valid\":%s",
                                  channel_name,
                                  calibration.Valid ? "true" : "false"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }
        any_set_field = true;
    }

    if (ltc3901_cfg_parse_status == HC_CMD_OK)
    {
        bool first_config_arg = true;

        if (!fw_app_get_ltc3901_config(&ltc3901_config))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (ltc3901_cfg_request.HasIsupplyMaMax)
        {
            ltc3901_config.isupply_ma_max = ltc3901_cfg_request.Config.isupply_ma_max;
        }
        if (ltc3901_cfg_request.HasVupstreamMvMin)
        {
            ltc3901_config.vupstream_mv_min = ltc3901_cfg_request.Config.vupstream_mv_min;
        }
        if (ltc3901_cfg_request.HasLtc3901VccMvMin)
        {
            ltc3901_config.ltc3901_vcc_mv_min = ltc3901_cfg_request.Config.ltc3901_vcc_mv_min;
        }
        if (ltc3901_cfg_request.HasPowerUpTimeoutMs)
        {
            ltc3901_config.power_up_timeout_ms = ltc3901_cfg_request.Config.power_up_timeout_ms;
        }
        if (ltc3901_cfg_request.HasPowerRetryDelayMs)
        {
            ltc3901_config.power_retry_delay_ms = ltc3901_cfg_request.Config.power_retry_delay_ms;
        }
        if (ltc3901_cfg_request.HasPowerFaultMax)
        {
            ltc3901_config.power_fault_max = ltc3901_cfg_request.Config.power_fault_max;
        }
        if (ltc3901_cfg_request.HasSyncOnDelayMs)
        {
            ltc3901_config.sync_on_delay_ms = ltc3901_cfg_request.Config.sync_on_delay_ms;
        }
        if (ltc3901_cfg_request.HasSyncHoldOnTimeMs)
        {
            ltc3901_config.sync_hold_on_time_ms = ltc3901_cfg_request.Config.sync_hold_on_time_ms;
        }
        if (ltc3901_cfg_request.HasSyncHoldOffTimeMs)
        {
            ltc3901_config.sync_hold_off_time_ms = ltc3901_cfg_request.Config.sync_hold_off_time_ms;
        }
        if (ltc3901_cfg_request.HasSyncStabilizationTimeMs)
        {
            ltc3901_config.sync_stabilization_time_ms = ltc3901_cfg_request.Config.sync_stabilization_time_ms;
        }
        if (ltc3901_cfg_request.HasSyncFaultDelayMs)
        {
            ltc3901_config.sync_fault_delay_ms = ltc3901_cfg_request.Config.sync_fault_delay_ms;
        }

        if (!fw_app_set_ltc3901_config(&ltc3901_config) ||
            !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (ltc3901_cfg_request.HasIsupplyMaMax &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.isupply_ma_max\":%ld", (long)ltc3901_config.isupply_ma_max)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasVupstreamMvMin &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.vupstream_mv_min\":%ld", (long)ltc3901_config.vupstream_mv_min)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasLtc3901VccMvMin &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.ltc3901_vcc_mv_min\":%ld", (long)ltc3901_config.ltc3901_vcc_mv_min)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasPowerUpTimeoutMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.power_up_timeout_ms\":%lu", (unsigned long)ltc3901_config.power_up_timeout_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasPowerRetryDelayMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.power_retry_delay_ms\":%lu", (unsigned long)ltc3901_config.power_retry_delay_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasPowerFaultMax &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.power_fault_max\":%lu", (unsigned long)ltc3901_config.power_fault_max)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasSyncOnDelayMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.sync_on_delay_ms\":%lu", (unsigned long)ltc3901_config.sync_on_delay_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasSyncHoldOnTimeMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.sync_hold_on_time_ms\":%lu", (unsigned long)ltc3901_config.sync_hold_on_time_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasSyncHoldOffTimeMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.sync_hold_off_time_ms\":%lu", (unsigned long)ltc3901_config.sync_hold_off_time_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasSyncStabilizationTimeMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.sync_stabilization_time_ms\":%lu", (unsigned long)ltc3901_config.sync_stabilization_time_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (ltc3901_cfg_request.HasSyncFaultDelayMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901.sync_fault_delay_ms\":%lu", (unsigned long)ltc3901_config.sync_fault_delay_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (lt8316_cfg_parse_status == HC_CMD_OK)
    {
        bool first_config_arg = true;

        if (!fw_app_get_lt8316_config(&lt8316_config))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (lt8316_cfg_request.HasPowerRetryDelayMs)
        {
            lt8316_config.power_retry_delay_ms = lt8316_cfg_request.Config.power_retry_delay_ms;
        }
        if (lt8316_cfg_request.HasPowerFaultMax)
        {
            lt8316_config.power_fault_max = lt8316_cfg_request.Config.power_fault_max;
        }
        if (lt8316_cfg_request.HasPowerOnStabilizationTimeMs)
        {
            lt8316_config.power_on_stabilization_time_ms =
                lt8316_cfg_request.Config.power_on_stabilization_time_ms;
        }

        if (!fw_app_set_lt8316_config(&lt8316_config) ||
            !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (lt8316_cfg_request.HasPowerRetryDelayMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"lt8316.power_retry_delay_ms\":%lu", (unsigned long)lt8316_config.power_retry_delay_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (lt8316_cfg_request.HasPowerFaultMax &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"lt8316.power_fault_max\":%lu", (unsigned long)lt8316_config.power_fault_max)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        if (lt8316_cfg_request.HasPowerOnStabilizationTimeMs &&
            (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_config_arg) ||
             !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"lt8316.power_on_stabilization_time_ms\":%lu", (unsigned long)lt8316_config.power_on_stabilization_time_ms)))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (ltc3901_cmd_parse_status == HC_CMD_OK)
    {
        if (!hc_jsonl_apply_ltc3901_cmd(ltc3901_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json,
                              sizeof(args_json),
                              &args_offset,
                              "\"ltc3901_cmd\":\"%s\"",
                              ltc3901_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (lt8316_cmd_parse_status == HC_CMD_OK)
    {
        if (!hc_jsonl_apply_lt8316_cmd(lt8316_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json,
                              sizeof(args_json),
                              &args_offset,
                              "\"lt8316_cmd\":\"%s\"",
                              lt8316_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (hc_cmd_parse_status == HC_CMD_OK)
    {
        if (!hc_jsonl_apply_hc_cmd(hc_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json,
                              sizeof(args_json),
                              &args_offset,
                              "\"hc_cmd\":\"%s\"",
                              hc_cmd_request.Command))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (digital_signals_parse_status == HC_CMD_OK)
    {
        for (uint8_t i = 0; i < digital_signals_request.SignalCount; ++i)
        {
            uint8_t signal_id = digital_signals_request.SignalIds[i];
            bool value = digital_signals_request.Values[i];

            if (!fw_app_debug_signal_is_digital(signal_id))
            {
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "BAD_VALUE",
                                              "Signal is not a settable digital signal"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_BAD_VALUE;
            }

            if (!fw_app_set_digital_signal(signal_id, value))
            {
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "BAD_VALUE",
                                              "Failed to set digital signal"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_BAD_VALUE;
            }
        }

        if (!fw_app_debug_format_digital_signals_json(digital_signals_request.SignalIds,
                                                      digital_signals_request.Values,
                                                      digital_signals_request.SignalCount,
                                                      digital_signals_json,
                                                      sizeof(digital_signals_json)) ||
            !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_append_object_members(args_json, sizeof(args_json), &args_offset, digital_signals_json))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (!any_set_field ||
        !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "}") ||
        !hc_jsonl_rsp_build_set_args_ok(rsp_buf,
                                        rsp_buf_size,
                                        HC_CMD_HOST_CONTROLLER_ID,
                                        request->msg,
                                        hc_datetime_get(),
                                        args_json))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_handle_get(const char *line,
                                    const jsmntok_t *tokens,
                                    const hc_cmd_request_t *request,
                                    char *rsp_buf,
                                    size_t rsp_buf_size)
{
    const char *current_date_time;
    hc_jsonl_get_debug_request_t debug_request;
    hc_jsonl_get_adc_cal_request_t adc_cal_request;
    hc_jsonl_get_ltc3901_cfg_request_t ltc3901_cfg_request;
    hc_jsonl_get_lt8316_cfg_request_t lt8316_cfg_request;
    hc_cmd_status_t sw_version_parse_status;
    hc_cmd_status_t ltc3901_cfg_parse_status;
    hc_cmd_status_t lt8316_cfg_parse_status;
    hc_cmd_status_t adc_cal_parse_status;
    hc_cmd_status_t debug_parse_status;
    hc_cmd_status_t date_time_parse_status;
    hc_debug_telemetry_config_t debug_config;
    char dbg_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    char get_args_json[HC_JSONL_SET_ARGS_JSON_MAX_LEN];
    char ltc3901_cfg_json[384];
    char lt8316_cfg_json[160];
    adc_sense_calibration_t calibration;
    ltc3901_manager_config_t ltc3901_config;
    lt8316_manager_config_t lt8316_config;
    size_t args_offset = 0U;
    bool first_arg = true;
    bool any_get_field = false;

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    sw_version_parse_status = hc_jsonl_parse_get_sw_version(line, tokens, request);
    ltc3901_cfg_parse_status = hc_jsonl_parse_get_ltc3901_cfg(line, tokens, request, &ltc3901_cfg_request);
    lt8316_cfg_parse_status = hc_jsonl_parse_get_lt8316_cfg(line, tokens, request, &lt8316_cfg_request);
    adc_cal_parse_status = hc_jsonl_parse_get_adc_calibration(line, tokens, request, &adc_cal_request);
    debug_parse_status = hc_jsonl_parse_get_debug_config(line, tokens, request, &debug_request);
    date_time_parse_status = hc_jsonl_parse_get_date_time(line, tokens, request);

    if ((sw_version_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (ltc3901_cfg_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (lt8316_cfg_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (adc_cal_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (debug_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (date_time_parse_status == HC_CMD_ERR_BAD_ARGS))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_ARGS",
                                      "GET requires args to be an array of field names"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_ARGS;
    }

    if ((sw_version_parse_status == HC_CMD_ERR_BAD_VALUE) ||
        (ltc3901_cfg_parse_status == HC_CMD_ERR_BAD_VALUE) ||
        (lt8316_cfg_parse_status == HC_CMD_ERR_BAD_VALUE) ||
        (adc_cal_parse_status == HC_CMD_ERR_BAD_VALUE) ||
        (debug_parse_status == HC_CMD_ERR_BAD_VALUE) ||
        (date_time_parse_status == HC_CMD_ERR_BAD_VALUE))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "GET args contains an invalid selector value"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if ((sw_version_parse_status != HC_CMD_OK) &&
        (ltc3901_cfg_parse_status != HC_CMD_OK) &&
        (lt8316_cfg_parse_status != HC_CMD_OK) &&
        (adc_cal_parse_status != HC_CMD_OK) &&
        (debug_parse_status != HC_CMD_OK) &&
        (date_time_parse_status != HC_CMD_OK))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_FIELD",
                                      "GET currently supports date_time, sw_version, dbg_period_ms, dbg_signals, direct debug signal names, direct adc.<channel> calibration fields, ltc3901, or lt8316"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "{"))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (date_time_parse_status == HC_CMD_OK)
    {
        current_date_time = hc_datetime_get();
        if (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "\"date_time\":\"%s\"", current_date_time))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_get_field = true;
    }

    if (sw_version_parse_status == HC_CMD_OK)
    {
        if (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "\"sw_version\":\"%s\"", fw_app_get_sw_version()))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_get_field = true;
    }

    if (debug_parse_status == HC_CMD_OK)
    {
        if (debug_request.RequestSample)
        {
            if (!fw_app_debug_format_signals_json(debug_request.SignalIds,
                                                  debug_request.SignalCount,
                                                  dbg_signals_json,
                                                  sizeof(dbg_signals_json)))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            if (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
                !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "\"dbg_signals\":%s", dbg_signals_json))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }
        else if (debug_request.RequestConfig)
        {
            if (!fw_app_get_debug_config(&debug_config) ||
                !hc_jsonl_build_dbg_signals_json(&debug_config, dbg_signals_json, sizeof(dbg_signals_json)))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            if (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
                !hc_jsonl_appendf(get_args_json,
                                  sizeof(get_args_json),
                                  &args_offset,
                                  "\"dbg_period_ms\":%lu,\"dbg_signals\":%s",
                                  (unsigned long)debug_config.PeriodMs,
                                  dbg_signals_json))
            {
                return HC_CMD_ERR_INTERNAL;
            }
        }
        any_get_field = true;
    }

    if (adc_cal_parse_status == HC_CMD_OK)
    {
        const char *channel_name;

        if (!adc_cal_request.Requested || !hc_jsonl_validate_adc_channel(adc_cal_request.Channel))
        {
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "ADC calibration channel is out of range"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;
        }

        channel_name = hc_jsonl_get_adc_channel_name(adc_cal_request.Channel);
        if ((channel_name == NULL) ||
            !adc_sense_drv_get_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (adc_cal_request.HasSlopeScaled &&
            (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
             !hc_jsonl_appendf(get_args_json,
                               sizeof(get_args_json),
                               &args_offset,
                               "\"adc.%s.slope_scaled\":%ld",
                               channel_name,
                               (long)calibration.SlopeScaled)))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (adc_cal_request.HasOffset &&
            (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
             !hc_jsonl_appendf(get_args_json,
                               sizeof(get_args_json),
                               &args_offset,
                               "\"adc.%s.offset\":%ld",
                               channel_name,
                               (long)calibration.Offset)))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (adc_cal_request.HasValid &&
            (!hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
             !hc_jsonl_appendf(get_args_json,
                               sizeof(get_args_json),
                               &args_offset,
                               "\"adc.%s.valid\":%s",
                               channel_name,
                               calibration.Valid ? "true" : "false")))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        any_get_field = true;
    }

    if (ltc3901_cfg_parse_status == HC_CMD_OK)
    {
        if (!ltc3901_cfg_request.Requested ||
            !fw_app_get_ltc3901_config(&ltc3901_config) ||
            !hc_jsonl_build_ltc3901_cfg_json(&ltc3901_config, ltc3901_cfg_json, sizeof(ltc3901_cfg_json)) ||
            !hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "\"ltc3901\":%s", ltc3901_cfg_json))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_get_field = true;
    }

    if (lt8316_cfg_parse_status == HC_CMD_OK)
    {
        if (!lt8316_cfg_request.Requested ||
            !fw_app_get_lt8316_config(&lt8316_config) ||
            !hc_jsonl_build_lt8316_cfg_json(&lt8316_config, lt8316_cfg_json, sizeof(lt8316_cfg_json)) ||
            !hc_jsonl_append_set_arg_separator(get_args_json, sizeof(get_args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "\"lt8316\":%s", lt8316_cfg_json))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_get_field = true;
    }

    if (!any_get_field ||
        !hc_jsonl_appendf(get_args_json, sizeof(get_args_json), &args_offset, "}") ||
        !hc_jsonl_rsp_build_set_args_ok(rsp_buf,
                                        rsp_buf_size,
                                        HC_CMD_HOST_CONTROLLER_ID,
                                        request->msg,
                                        hc_datetime_get(),
                                        get_args_json))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return HC_CMD_OK;
}
