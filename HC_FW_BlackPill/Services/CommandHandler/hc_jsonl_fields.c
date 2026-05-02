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
    hc_datetime_status_t set_status;

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

hc_cmd_status_t hc_jsonl_handle_get(const char *line,
                                    const jsmntok_t *tokens,
                                    const hc_cmd_request_t *request,
                                    char *rsp_buf,
                                    size_t rsp_buf_size)
{
    const char *current_date_time;

    if ((request == NULL) || (rsp_buf == NULL))
    {
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
                                          "GET currently supports only args.date_time"))
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
