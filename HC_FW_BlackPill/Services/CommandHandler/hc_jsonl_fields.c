#include "hc_jsonl_fields.h"

#include "hc_datetime.h"
#include "hc_jsonl_parse.h"
#include "hc_jsonl_rsp.h"
#include "adc_sense_drv.h"
#include "fw_app.h"

#include <stdio.h>
#include <string.h>

#define HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN (512U)

static bool hc_jsonl_validate_adc_channel(uint32_t channel)
{
    return (channel < (uint32_t)ADC_SENSE_CHANNEL_COUNT);
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

hc_cmd_status_t hc_jsonl_handle_set(const char *line,
                                    const jsmntok_t *tokens,
                                    const hc_cmd_request_t *request,
                                    char *rsp_buf,
                                    size_t rsp_buf_size)
{
    char date_time[HC_CMD_MAX_DATE_TIME_LEN] = {0};
    char dbg_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    hc_jsonl_set_sts_period_request_t sts_period_request;
    hc_jsonl_set_debug_request_t debug_request;
    hc_jsonl_set_adc_cal_request_t adc_cal_request;
    adc_sense_calibration_t calibration;
    hc_debug_telemetry_config_t debug_config;
    const char *current_date_time;
    hc_datetime_status_t set_status;
    hc_cmd_status_t parse_status;

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    parse_status = hc_jsonl_parse_set_date_time(line, tokens, request, date_time, sizeof(date_time));
    switch (parse_status)
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
                                          "SET requires an args object"))
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
                                          "args.date_time must be a valid timestamp string"))
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
                                          "SET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    if (parse_status == HC_CMD_OK)
    {
        set_status = hc_datetime_set(date_time);
        switch (set_status)
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
        if (!hc_jsonl_rsp_build_set_datetime_ok(rsp_buf,
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

    switch (hc_jsonl_parse_set_sts_period_ms(line, tokens, request, &sts_period_request))
    {
        case HC_CMD_OK:
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

            if (!hc_jsonl_rsp_build_set_sts_period_ok(rsp_buf,
                                                      rsp_buf_size,
                                                      HC_CMD_HOST_CONTROLLER_ID,
                                                      request->msg,
                                                      hc_datetime_get(),
                                                      fw_app_get_sts_period_ms()))
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
                                          "SET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_VALUE:
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

        case HC_CMD_ERR_BAD_FIELD:
            break;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "SET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_set_debug_config(line, tokens, request, &debug_request))
    {
        case HC_CMD_OK:
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

            if (!fw_app_set_debug_config(&debug_config))
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

            if (!fw_app_get_debug_config(&debug_config) ||
                !hc_jsonl_build_dbg_signals_json(&debug_config, dbg_signals_json, sizeof(dbg_signals_json)) ||
                !hc_jsonl_rsp_build_set_debug_config_ok(rsp_buf,
                                                        rsp_buf_size,
                                                        HC_CMD_HOST_CONTROLLER_ID,
                                                        request->msg,
                                                        hc_datetime_get(),
                                                        debug_config.PeriodMs,
                                                        dbg_signals_json))
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
                                          "SET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_VALUE:
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

        case HC_CMD_ERR_BAD_FIELD:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_FIELD",
                                          "SET currently supports args.date_time, args.sts_period_ms, dbg_period_ms, dbg_signals, or adc_cal"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_FIELD;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "SET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_set_adc_calibration(line, tokens, request, &adc_cal_request))
    {
        case HC_CMD_OK:
            if (!hc_jsonl_validate_adc_channel(adc_cal_request.Channel))
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

            calibration.SlopeScaled = adc_cal_request.SlopeScaled;
            calibration.Offset = adc_cal_request.Offset;
            calibration.Valid = adc_cal_request.Valid;
            if (!adc_sense_drv_set_calibration((adc_sense_channel_t)adc_cal_request.Channel, &calibration) ||
                !hc_jsonl_rsp_build_set_adc_cal_ok(rsp_buf,
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
                                          "SET requires an args object"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_ARGS;

        case HC_CMD_ERR_BAD_VALUE:
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

        case HC_CMD_ERR_BAD_FIELD:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_FIELD",
                                          "SET currently supports args.date_time, args.sts_period_ms, dbg_period_ms, dbg_signals, or adc_cal"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_BAD_FIELD;

        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "INTERNAL",
                                          "SET parsing failed"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_INTERNAL;
    }
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
    hc_debug_telemetry_config_t debug_config;
    char dbg_signals_json[HC_JSONL_DBG_SIGNALS_JSON_MAX_LEN];
    adc_sense_calibration_t calibration;
    uint16_t raw_adc_value;

    if ((request == NULL) || (rsp_buf == NULL))
    {
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
                                          "GET currently supports args.date_time, args.raw_adc, dbg_period_ms, dbg_signals, or adc_cal"))
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
