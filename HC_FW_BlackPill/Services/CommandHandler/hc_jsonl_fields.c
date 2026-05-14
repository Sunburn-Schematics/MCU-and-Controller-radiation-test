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
    char ltc3901_cfg_json[384];
    char lt8316_cfg_json[160];
    hc_jsonl_set_sts_period_request_t sts_period_request;
    hc_jsonl_set_debug_request_t debug_request;
    hc_jsonl_set_adc_cal_request_t adc_cal_request;
    hc_jsonl_set_ltc3901_cmd_request_t ltc3901_cmd_request;
    hc_jsonl_set_lt8316_cmd_request_t lt8316_cmd_request;
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
    hc_cmd_status_t ltc3901_cfg_parse_status;
    hc_cmd_status_t lt8316_cfg_parse_status;
    hc_cmd_status_t digital_signals_parse_status;

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
    ltc3901_cfg_parse_status = hc_jsonl_parse_set_ltc3901_cfg(line, tokens, request, &ltc3901_cfg_request);
    lt8316_cfg_parse_status = hc_jsonl_parse_set_lt8316_cfg(line, tokens, request, &lt8316_cfg_request);
    digital_signals_parse_status = hc_jsonl_parse_set_digital_signals(line, tokens, request, &digital_signals_request);

    if ((date_time_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (sts_period_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (debug_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (adc_cal_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (ltc3901_cmd_parse_status == HC_CMD_ERR_BAD_ARGS) ||
        (lt8316_cmd_parse_status == HC_CMD_ERR_BAD_ARGS) ||
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
                                      "args.adc_cal must include channel, slope_scaled, offset, and valid; channel may be an index or ADC signal name"))
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
                                      "args.adc_cal.channel is out of range"))
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

    if ((digital_signals_parse_status == HC_CMD_ERR_BAD_VALUE) && !debug_request.HasSignals)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "args.dbg_signals must be an object with boolean values for settable digital signals"))
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
                                      "args.ltc3901_cfg must be an object containing one or more valid numeric LTC3901 manager config fields"))
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
                                      "args.lt8316_cfg must be an object containing one or more valid numeric LT8316 manager config fields"))
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
                                      "SET currently supports args.date_time, args.sts_period_ms, dbg_period_ms, dbg_signals, adc_cal, ltc3901_cmd, lt8316_cmd, ltc3901_cfg, or lt8316_cfg"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_FIELD;
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
        calibration.SlopeScaled = adc_cal_request.SlopeScaled;
        calibration.Offset = adc_cal_request.Offset;
        calibration.Valid = adc_cal_request.Valid;

        if (!adc_sense_drv_set_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration))
        {
            return HC_CMD_ERR_INTERNAL;
        }

        if (!hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json,
                              sizeof(args_json),
                              &args_offset,
                              "\"adc_cal\":{\"channel\":%lu,\"slope_scaled\":%ld,\"offset\":%ld,\"valid\":%s}",
                              (unsigned long)adc_cal_request.Channel,
                              (long)calibration.SlopeScaled,
                              (long)calibration.Offset,
                              calibration.Valid ? "true" : "false"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (ltc3901_cfg_parse_status == HC_CMD_OK)
    {
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
            !hc_jsonl_build_ltc3901_cfg_json(&ltc3901_config, ltc3901_cfg_json, sizeof(ltc3901_cfg_json)) ||
            !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"ltc3901_cfg\":%s", ltc3901_cfg_json))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        any_set_field = true;
    }

    if (lt8316_cfg_parse_status == HC_CMD_OK)
    {
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
            !hc_jsonl_build_lt8316_cfg_json(&lt8316_config, lt8316_cfg_json, sizeof(lt8316_cfg_json)) ||
            !hc_jsonl_append_set_arg_separator(args_json, sizeof(args_json), &args_offset, &first_arg) ||
            !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"lt8316_cfg\":%s", lt8316_cfg_json))
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
            !hc_jsonl_appendf(args_json, sizeof(args_json), &args_offset, "\"dbg_signals\":%s", digital_signals_json))
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
    hc_jsonl_get_raw_adc_request_t raw_adc_request;
    hc_jsonl_get_debug_request_t debug_request;
    hc_jsonl_get_adc_cal_request_t adc_cal_request;
    hc_jsonl_get_ltc3901_cfg_request_t ltc3901_cfg_request;
    hc_jsonl_get_lt8316_cfg_request_t lt8316_cfg_request;
    hc_cmd_status_t sw_version_parse_status;
    hc_debug_telemetry_config_t debug_config;
    char dbg_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    char get_args_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    char ltc3901_cfg_json[384];
    char lt8316_cfg_json[160];
    adc_sense_calibration_t calibration;
    ltc3901_manager_config_t ltc3901_config;
    lt8316_manager_config_t lt8316_config;
    uint16_t raw_adc_value;
    int written;

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    sw_version_parse_status = hc_jsonl_parse_get_sw_version(line, tokens, request);
    switch (sw_version_parse_status)
    {
        case HC_CMD_OK:
            if (!hc_jsonl_rsp_build_get_sw_version_ok(rsp_buf,
                                                      rsp_buf_size,
                                                      HC_CMD_HOST_CONTROLLER_ID,
                                                      request->msg,
                                                      hc_datetime_get(),
                                                      fw_app_get_sw_version()))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_OK;

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.sw_version must be true"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_ltc3901_cfg(line, tokens, request, &ltc3901_cfg_request))
    {
        case HC_CMD_OK:
            if (!ltc3901_cfg_request.Requested ||
                !fw_app_get_ltc3901_config(&ltc3901_config) ||
                !hc_jsonl_build_ltc3901_cfg_json(&ltc3901_config, ltc3901_cfg_json, sizeof(ltc3901_cfg_json)))
            {
                return HC_CMD_ERR_INTERNAL;
            }

            written = snprintf(get_args_json, sizeof(get_args_json), "{\"ltc3901_cfg\":%s}", ltc3901_cfg_json);
            if ((written < 0) ||
                ((size_t)written >= sizeof(get_args_json)) ||
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

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.ltc3901_cfg must be true"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_lt8316_cfg(line, tokens, request, &lt8316_cfg_request))
    {
        case HC_CMD_OK:
            if (!lt8316_cfg_request.Requested ||
                !fw_app_get_lt8316_config(&lt8316_config) ||
                !hc_jsonl_build_lt8316_cfg_json(&lt8316_config, lt8316_cfg_json, sizeof(lt8316_cfg_json)))
            {
                return HC_CMD_ERR_INTERNAL;
            }

            written = snprintf(get_args_json, sizeof(get_args_json), "{\"lt8316_cfg\":%s}", lt8316_cfg_json);
            if ((written < 0) ||
                ((size_t)written >= sizeof(get_args_json)) ||
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

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.lt8316_cfg must be true"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_raw_adc(line, tokens, request, &raw_adc_request))
    {
        case HC_CMD_OK:
            if (!raw_adc_request.Requested)
            {
                break;
            }

            if (raw_adc_request.HasChannel)
            {
                if (raw_adc_request.Channel >= (uint32_t)ADC_SENSE_CHANNEL_COUNT)
                {
                    if (!hc_jsonl_rsp_build_error(rsp_buf,
                                                  rsp_buf_size,
                                                  HC_CMD_HOST_CONTROLLER_ID,
                                                  request->has_msg,
                                                  request->msg,
                                                  hc_datetime_get(),
                                                  "BAD_VALUE",
                                                  "args.raw_adc channel is out of range"))
                    {
                        return HC_CMD_ERR_INTERNAL;
                    }
                    return HC_CMD_ERR_BAD_VALUE;
                }

                raw_adc_value = adc_sense_drv_get_raw((adc_sense_channel_t)raw_adc_request.Channel);
            }
            else
            {
                raw_adc_value = adc_sense_drv_get_raw(ADC_SENSE_CHANNEL_VUPSTREAM);
            }

            current_date_time = hc_datetime_get();
            if (!hc_jsonl_rsp_build_get_raw_adc_ok(rsp_buf,
                                                   rsp_buf_size,
                                                   HC_CMD_HOST_CONTROLLER_ID,
                                                   request->msg,
                                                   current_date_time,
                                                   raw_adc_request.HasChannel,
                                                   raw_adc_request.Channel,
                                                   raw_adc_value))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_OK;

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.raw_adc must be true or a valid channel index"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_adc_calibration(line, tokens, request, &adc_cal_request))
    {
        case HC_CMD_OK:
            if (!adc_cal_request.Requested || !hc_jsonl_validate_adc_channel(adc_cal_request.Channel))
            {
                if (!hc_jsonl_rsp_build_error(rsp_buf,
                                              rsp_buf_size,
                                              HC_CMD_HOST_CONTROLLER_ID,
                                              request->has_msg,
                                              request->msg,
                                              hc_datetime_get(),
                                              "BAD_VALUE",
                                              "args.adc_cal channel is out of range"))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_ERR_BAD_VALUE;
            }

            if (!adc_sense_drv_get_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration) ||
                !hc_jsonl_rsp_build_get_adc_cal_ok(rsp_buf,
                                                   rsp_buf_size,
                                                   HC_CMD_HOST_CONTROLLER_ID,
                                                   request->msg,
                                                   hc_datetime_get(),
                                                   adc_cal_request.Channel,
                                                   calibration.SlopeScaled,
                                                   calibration.Offset,
                                                   calibration.Valid))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_OK;

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.adc_cal must be a valid channel index or ADC signal name"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_debug_config(line, tokens, request, &debug_request))
    {
        case HC_CMD_OK:
            if (debug_request.RequestSample)
            {
                if (!fw_app_debug_format_signals_json(debug_request.SignalIds,
                                                      debug_request.SignalCount,
                                                      dbg_signals_json,
                                                      sizeof(dbg_signals_json)) ||
                    !hc_jsonl_rsp_build_get_debug_signals_ok(rsp_buf,
                                                             rsp_buf_size,
                                                             HC_CMD_HOST_CONTROLLER_ID,
                                                             request->msg,
                                                             hc_datetime_get(),
                                                             dbg_signals_json))
                {
                    return HC_CMD_ERR_INTERNAL;
                }
                return HC_CMD_OK;
            }

            if (debug_request.RequestConfig &&
                fw_app_get_debug_config(&debug_config) &&
                hc_jsonl_build_dbg_signals_json(&debug_config, dbg_signals_json, sizeof(dbg_signals_json)) &&
                hc_jsonl_rsp_build_get_debug_config_ok(rsp_buf,
                                                       rsp_buf_size,
                                                       HC_CMD_HOST_CONTROLLER_ID,
                                                       request->msg,
                                                       hc_datetime_get(),
                                                       debug_config.PeriodMs,
                                                       dbg_signals_json))
            {
                return HC_CMD_OK;
            }
            return HC_CMD_ERR_INTERNAL;

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            break;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.dbg_period_ms must be true, and args.dbg_signals must be true or a valid signal array"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_get_date_time(line, tokens, request))
    {
        case HC_CMD_OK:
            break;

        case HC_CMD_ERR_BAD_ARGS:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_ARGS",
                                          "GET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_FIELD:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_FIELD",
                                          "GET currently supports args.date_time, args.sw_version, args.raw_adc, dbg_period_ms, dbg_signals, adc_cal, ltc3901_cfg, or lt8316_cfg"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_FIELD;

        case HC_CMD_ERR_BAD_VALUE:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_VALUE",
                                          "GET args.date_time must be true"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_VALUE;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "GET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    current_date_time = hc_datetime_get();
    if (!hc_jsonl_rsp_build_get_datetime_ok(rsp_buf,
                                            rsp_buf_size,
                                            HC_CMD_HOST_CONTROLLER_ID,
                                            request->msg,
                                            current_date_time,
                                            current_date_time))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return HC_CMD_OK;
}
