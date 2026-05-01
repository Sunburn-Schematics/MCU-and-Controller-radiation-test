#include "hc_jsonl_parse.h"

#include <stddef.h>
#include <string.h>

static hc_pkt_type_t hc_jsonl_parse_type_placeholder(const char *line)
{
    if (line == NULL)
    {
        return HC_PKT_UNKNOWN;
    }

    if (strstr(line, "\"type\":\"SET\"") != NULL)
    {
        return HC_PKT_SET;
    }

    if (strstr(line, "\"type\":\"GET\"") != NULL)
    {
        return HC_PKT_GET;
    }

    if (strstr(line, "\"type\":\"EXC\"") != NULL)
    {
        return HC_PKT_EXC;
    }

    return HC_PKT_UNKNOWN;
}

hc_cmd_status_t hc_jsonl_parse_request(const char *line,
                                       jsmntok_t *tokens,
                                       size_t max_tokens,
                                       hc_cmd_request_t *request)
{
    (void)tokens;
    (void)max_tokens;

    if ((line == NULL) || (request == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(request, 0, sizeof(*request));
    request->type = hc_jsonl_parse_type_placeholder(line);

    if (request->type == HC_PKT_UNKNOWN)
    {
        return HC_CMD_ERR_BAD_TYPE;
    }

    /* Placeholder until full jsmn-based extraction is added. */
    request->has_msg = true;
    request->msg = 0u;

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request,
                                             char *date_time_out,
                                             size_t date_time_out_size)
{
    (void)line;
    (void)tokens;
    (void)request;

    if ((date_time_out == NULL) || (date_time_out_size < HC_CMD_MAX_DATE_TIME_LEN))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    date_time_out[0] = '\0';
    return HC_CMD_ERR_NOT_SUPPORTED;
}