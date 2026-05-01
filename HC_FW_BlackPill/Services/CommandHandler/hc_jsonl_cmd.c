#include "hc_jsonl_cmd.h"

#include "hc_comms_tx.h"
#include "hc_datetime.h"
#include "hc_jsonl_dispatch.h"
#include "hc_jsonl_parse.h"
#include "hc_jsonl_rsp.h"
#include "jsmn.h"

static jsmntok_t s_tokens[HC_CMD_MAX_TOKENS];

void hc_jsonl_cmd_init(void)
{
    hc_datetime_init();
}

void hc_jsonl_cmd_process_line(const char *line)
{
    hc_cmd_request_t request = {0};
    char rsp_buf[HC_CMD_MAX_LINE_LEN] = {0};
    hc_cmd_status_t status;

    status = hc_jsonl_parse_request(line, s_tokens, HC_CMD_MAX_TOKENS, &request);
    if (status != HC_CMD_OK)
    {
        const char *error_code = "INTERNAL";
        const char *error_message = "Command parse failure";

        if (status == HC_CMD_ERR_BAD_JSON)
        {
            error_code = "BAD_JSON";
            error_message = "Input is not valid JSON";
        }
        else if (status == HC_CMD_ERR_BAD_TYPE)
        {
            error_code = "BAD_TYPE";
            error_message = "Unsupported or missing packet type";
        }
        else if (status == HC_CMD_ERR_BAD_ARGS)
        {
            error_code = "BAD_ARGS";
            error_message = "Missing or invalid msg field";
        }

        if (hc_jsonl_rsp_build_error(rsp_buf,
                                     sizeof(rsp_buf),
                                     HC_CMD_HOST_CONTROLLER_ID,
                                     request.has_msg,
                                     request.msg,
                                     hc_datetime_get(),
                                     error_code,
                                     error_message))
        {
            (void)hc_comms_tx_send_line(rsp_buf);
        }
        return;
    }

    (void)hc_jsonl_dispatch_request(line, s_tokens, &request, rsp_buf, sizeof(rsp_buf));
    (void)hc_comms_tx_send_line(rsp_buf);
}

void hc_jsonl_process_command(const char *cmd, uint32_t len)
{
    (void)len;
    hc_jsonl_cmd_process_line(cmd);
}
