#include "hc_jsonl_rsp.h"

#include <stdio.h>

bool hc_jsonl_rsp_build_set_datetime_ok(char *out,
                                        size_t out_size,
                                        bool include_hc,
                                        uint32_t hc_id,
                                        bool include_msg,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time)
{
    int written;

    if ((out == NULL) || (out_size == 0u) || (ts == NULL) || (date_time == NULL))
    {
        return false;
    }

    if (include_hc && include_msg)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}\n",
                           (unsigned long)hc_id,
                           (unsigned long)msg,
                           ts,
                           date_time);
    }
    else if (include_hc)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}\n",
                           (unsigned long)hc_id,
                           ts,
                           date_time);
    }
    else if (include_msg)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"msg\":%lu,\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}\n",
                           (unsigned long)msg,
                           ts,
                           date_time);
    }
    else
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"ts\":\"%s\",\"args\":{\"date_time\":\"%s\"}}\n",
                           ts,
                           date_time);
    }

    return (written > 0) && ((size_t)written < out_size);
}

bool hc_jsonl_rsp_build_error(char *out,
                              size_t out_size,
                              bool include_hc,
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

    if (include_hc && include_msg)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"msg\":%lu,\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}\n",
                           (unsigned long)hc_id,
                           (unsigned long)msg,
                           ts,
                           code,
                           message);
    }
    else if (include_hc)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"hc\":%lu,\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}\n",
                           (unsigned long)hc_id,
                           ts,
                           code,
                           message);
    }
    else if (include_msg)
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"msg\":%lu,\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}\n",
                           (unsigned long)msg,
                           ts,
                           code,
                           message);
    }
    else
    {
        written = snprintf(out, out_size,
                           "{\"type\":\"RSP\",\"ts\":\"%s\",\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}\n",
                           ts,
                           code,
                           message);
    }

    return (written > 0) && ((size_t)written < out_size);
}