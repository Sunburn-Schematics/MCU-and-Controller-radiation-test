#include "hc_jsonl_cmd.h"

#include "hc_comms_tx.h"
#include "hc_datetime.h"
#include "hc_jsonl_dispatch.h"
#include "hc_jsonl_parse.h"
#include "jsmn.h"

#include <stdio.h>

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
        (void)hc_comms_tx_send_line("{\"type\":\"RSP\",\"error\":{\"code\":\"NOT_IMPLEMENTED\",\"message\":\"parse error response path not wired yet\"}}\n");
        return;
    }

    status = hc_jsonl_dispatch_request(line, s_tokens, &request, rsp_buf, sizeof(rsp_buf));
    if (status != HC_CMD_OK)
    {
        (void)hc_comms_tx_send_line("{\"type\":\"RSP\",\"error\":{\"code\":\"NOT_IMPLEMENTED\",\"message\":\"dispatch error response path not wired yet\"}}\n");
        return;
    }

    (void)hc_comms_tx_send_line(rsp_buf);
}


void hc_jsonl_process_command(const char *cmd, uint32_t len) {
//    if (preprocess_command(cmd, len) == HC_CMD_OK)
    {
        printf("{\"type\":\"RSP\",\"status\":\"OK\"}\n");
//        hc_comms_tx_send_line("{\"type\":\"RSP\",\"error\":{\"code\":\"NOT_IMPLEMENTED\",\"message\":\"preprocess error response path not wired yet\"}}\n");
        return;
    }
}