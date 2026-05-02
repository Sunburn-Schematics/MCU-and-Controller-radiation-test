#include "hc_jsonl_dispatch.h"

#include "hc_datetime.h"
#include "hc_jsonl_fields.h"
#include "hc_jsonl_rsp.h"

hc_cmd_status_t hc_jsonl_dispatch_request(const char *line,
                                          const jsmntok_t *tokens,
                                          const hc_cmd_request_t *request,
                                          char *rsp_buf,
                                          size_t rsp_buf_size)
{
    if ((request == NULL) || (rsp_buf == NULL) || (rsp_buf_size == 0u))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    switch (request->type)
    {
        case HC_PKT_SET:
            return hc_jsonl_handle_set(line, tokens, request, rsp_buf, rsp_buf_size);

        case HC_PKT_GET:
            return hc_jsonl_handle_get(line, tokens, request, rsp_buf, rsp_buf_size);

        case HC_PKT_EXC:
        default:
            if (!hc_jsonl_rsp_build_error(rsp_buf,
                                          rsp_buf_size,
                                          HC_CMD_HOST_CONTROLLER_ID,
                                          request->has_msg,
                                          request->msg,
                                          hc_datetime_get(),
                                          "NOT_SUPPORTED",
                                          "Packet type not implemented yet"))
            {
                return HC_CMD_ERR_INTERNAL;
            }
            return HC_CMD_ERR_NOT_SUPPORTED;
    }
}
