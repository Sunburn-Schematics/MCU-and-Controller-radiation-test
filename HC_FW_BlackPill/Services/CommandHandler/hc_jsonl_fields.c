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
    const char *current_date_time;

    if ((request == NULL) || (rsp_buf == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    switch (hc_jsonl_parse_set_date_time(line, tokens, request, date_time, sizeof(date_time)))
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
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "BAD_FIELD",
                                          "SET currently supports only args.date_time"))
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

    if (!hc_datetime_set(date_time))
    {
        if (!hc_jsonl_rsp_build_error(rsp_buf,
                                      rsp_buf_size,
                                      HC_CMD_HOST_CONTROLLER_ID,
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
