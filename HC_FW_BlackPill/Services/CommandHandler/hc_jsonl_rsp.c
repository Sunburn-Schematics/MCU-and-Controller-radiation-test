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

bool hc_jsonl_rsp_build_get_sw_version_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          const char *sw_version)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (sw_version == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"sw_version\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       sw_version);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_ltc3901_cmd_ok(char *out,
                                           size_t out_size,
                                           uint32_t hc_id,
                                           uint32_t msg,
                                           const char *ts,
                                           const char *command)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (command == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"ltc3901_cmd\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       command);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_lt8316_cmd_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          const char *command)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (command == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"lt8316_cmd\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       command);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_manager_cmds_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            const char *ltc3901_command,
                                            const char *lt8316_command)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) ||
        (ltc3901_command == NULL) || (lt8316_command == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"ltc3901_cmd\":\"%s\",\"lt8316_cmd\":\"%s\"}}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       ltc3901_command,
                       lt8316_command);

    return ((written > 0) && ((size_t)written < out_size));
}

bool hc_jsonl_rsp_build_set_args_ok(char *out,
                                    size_t out_size,
                                    uint32_t hc_id,
                                    uint32_t msg,
                                    const char *ts,
                                    const char *args_json)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (args_json == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":%s}",
                       (unsigned long)hc_id,
                       (unsigned long)msg,
                       ts,
                       args_json);

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

bool hc_jsonl_rsp_build_set_digital_signals_ok(char *out,
                                               size_t out_size,
                                               uint8_t hc_id,
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

bool hc_jsonl_rsp_build_event_msg(char *out,
                                  size_t out_size,
                                  uint32_t hc_id,
                                  const char *ts,
                                  const char *message)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (message == NULL))
    {
        return false;
    }

    written = snprintf(out,
                       out_size,
                       "{\"type\":\"EVT\",\"hc\":%lu,\"ts\":\"%s\",\"args\":{\"msg\":\"%s\"}}",
                       (unsigned long)hc_id,
                       ts,
                       message);

    return ((written > 0) && ((size_t)written < out_size));
}
