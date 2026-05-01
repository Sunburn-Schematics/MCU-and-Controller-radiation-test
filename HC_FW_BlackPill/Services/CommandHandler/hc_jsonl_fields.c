#include "hc_jsonl_fields.h"

#include "hc_datetime.h"
#include "hc_jsonl_parse.h"
#include "hc_jsonl_rsp.h"

hc_cmd_status_t hc_jsonl_handle_set(const char *line,
                                    const jsmntok_t *tokens,
                                    const hc_cmd_request_t *request,
                                    char *rsp_buf,
                                    size_t rsp_buf_size)
{
    char date_time[HC_CMD_MAX_DATE_TIME_LEN] = {0};

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (hc_jsonl_parse_set_date_time(line, tokens, request, date_time, sizeof(date_time)) != HC_CMD_OK)
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      false,
                                      0u,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "NOT_IMPLEMENTED",
                                      "SET field parsing not wired yet"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_NOT_SUPPORTED;
    }

    if (!hc_datetime_set(date_time))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      false,
                                      0u,
                                      request->has_msg,
                                      request->msg,
                                      hc_datetime_get(),
                                      "BAD_VALUE",
                                      "Invalid date_time value"))
        {
            return HC_CMD_ERR_INTERNAL;
        }
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (!hc_jsonl_rsp_build_set_datetime_ok(rsp_buf,
                                            rsp_buf_size,
                                            false,
                                            0u,
                                            request->has_msg,
                                            request->msg,
                                            hc_datetime_get(),
                                            hc_datetime_get()))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return HC_CMD_OK;
}