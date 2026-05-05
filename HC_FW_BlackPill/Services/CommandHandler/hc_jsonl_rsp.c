#include "hc_jsonl_rsp.h"

#include <stdio.h>

bool hc_jsonl_rsp_build_set_datetime_ok(char *out,
                                        size_t out_size,
                                        uint32_t hc_id,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (date_time == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       date_time);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_sts_period_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          uint32_t sts_period_ms)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"sts_period_ms\":%lu}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       (unsigned long)sts_period_ms);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_debug_config_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            uint32_t dbg_period_ms,
                                            const char *dbg_signals_json)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (dbg_signals_json == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"dbg_period_ms\":%lu,\"dbg_signals\":%s}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       (unsigned long)dbg_period_ms,
                       dbg_signals_json);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_adc_cal_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       uint32_t channel,
                                       int32_t slope_scaled,
                                       int32_t offset,
                                       bool valid)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"adc_cal\":{\"channel\":%lu,\"slope_scaled\":%ld,\"offset\":%ld,\"valid\":%s}}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       (unsigned long)channel,
                       (long)slope_scaled,
                       (long)offset,
                       valid ? "true" : "false");

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_get_datetime_ok(char *out,
                                        size_t out_size,
                                        uint32_t hc_id,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (date_time == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       date_time);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_get_debug_config_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            uint32_t dbg_period_ms,
                                            const char *dbg_signals_json)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (dbg_signals_json == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"dbg_period_ms\":%lu,\"dbg_signals\":%s}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       (unsigned long)dbg_period_ms,
                       dbg_signals_json);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_get_debug_signals_ok(char *out,
                                             size_t out_size,
                                             uint32_t hc_id,
                                             uint32_t msg,
                                             const char *ts,
                                             const char *signals_json)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (signals_json == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"dbg_signals\":%s}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       signals_json);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_get_adc_cal_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       uint32_t channel,
                                       int32_t slope_scaled,
                                       int32_t offset,
                                       bool valid)
{
    return hc_jsonl_rsp_build_set_adc_cal_ok(out,
                                             out_size,
                                             hc_id,
                                             msg,
                                             ts,
                                             channel,
                                             slope_scaled,
                                             offset,
                                             valid);
}

bool hc_jsonl_rsp_build_get_raw_adc_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       bool include_channel,
                                       uint32_t channel,
                                       uint16_t raw_adc)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL))
    {
        return false;
    }

    if (include_channel)
    {
        written = snprintf(out,
                           out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"raw_adc\":%u,\"channel\":%lu}}",
                           (unsigned long)hc_id,
                           (unsigned long)msg,
                           ts,
                           (unsigned int)raw_adc,
                           (unsigned long)channel);
    }
    else
    {
        written = snprintf(out,
                           out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"raw_adc\":%u}}",
                           (unsigned long)hc_id,
                           (unsigned long)msg,
                           ts,
                           (unsigned int)raw_adc);
    }

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_error(char *out,
                              size_t out_size,
                              uint32_t hc_id,
                              bool include_msg,
                              uint32_t msg,
                              const char *ts,
                              const char *code,
                              const char *message)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (code == NULL) || (message == NULL))
    {
        return false;
    }

    if (include_msg)
    {
        written = snprintf(out,
                           out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
                           (unsigned long)hc_id,
                           (unsigned long)msg,
                           ts,
                           code,
                           message);
    }
    else
    {
        written = snprintf(out,
                           out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
                           (unsigned long)hc_id,
                           ts,
                           code,
                           message);
    }

    return ((written > 0) && ((size_t)written < out_size));
}
