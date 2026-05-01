#include "hc_jsonl_parse.h"

#include "jsmn.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static bool hc_jsonl_token_is_string(const jsmntok_t *token)
{
    return ((token != NULL) && (token->type == JSMN_STRING));
}

static bool hc_jsonl_token_is_primitive(const jsmntok_t *token)
{
    return ((token != NULL) && (token->type == JSMN_PRIMITIVE));
}

static bool hc_jsonl_token_equals(const char *json, const jsmntok_t *token, const char *text)
{
    size_t text_len;

    if ((json == NULL) || (token == NULL) || (text == NULL))
    {
        return false;
    }

    text_len = strlen(text);
    if ((token->start < 0) || (token->end < token->start))
    {
        return false;
    }

    if (((size_t)(token->end - token->start)) != text_len)
    {
        return false;
    }

    return (strncmp(&json[token->start], text, text_len) == 0);
}

static int hc_jsonl_find_object_value(const char *json,
                                      const jsmntok_t *tokens,
                                      size_t token_count,
                                      int object_index,
                                      const char *key)
{
    int i;
    int pair_count;

    if ((json == NULL) || (tokens == NULL) || (key == NULL) || (object_index < 0))
    {
        return -1;
    }

    if (tokens[object_index].type != JSMN_OBJECT)
    {
        return -1;
    }

    pair_count = tokens[object_index].size;
    i = object_index + 1;

    while ((pair_count > 0) && ((size_t)i < token_count))
    {
        if (hc_jsonl_token_equals(json, &tokens[i], key))
        {
            if (((size_t)(i + 1)) < token_count)
            {
                return (i + 1);
            }
            return -1;
        }

        i += 2;
        pair_count--;
    }

    return -1;
}

static hc_pkt_type_t hc_jsonl_parse_type(const char *json,
                                         const jsmntok_t *tokens,
                                         size_t token_count)
{
    int type_index;

    type_index = hc_jsonl_find_object_value(json, tokens, token_count, 0, "type");
    if (type_index < 0)
    {
        return HC_PKT_UNKNOWN;
    }

    if (!hc_jsonl_token_is_string(&tokens[type_index]))
    {
        return HC_PKT_UNKNOWN;
    }

    if (hc_jsonl_token_equals(json, &tokens[type_index], "SET"))
    {
        return HC_PKT_SET;
    }

    if (hc_jsonl_token_equals(json, &tokens[type_index], "GET"))
    {
        return HC_PKT_GET;
    }

    if (hc_jsonl_token_equals(json, &tokens[type_index], "EXC"))
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
    jsmn_parser parser;
    int parsed_tokens;
    int msg_index;
    char msg_text[16];
    size_t msg_len;

    if ((line == NULL) || (tokens == NULL) || (request == NULL) || (max_tokens == 0u))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(request, 0, sizeof(*request));

    jsmn_init(&parser);
    parsed_tokens = jsmn_parse(&parser, line, strlen(line), tokens, (unsigned int)max_tokens);
    if (parsed_tokens < 0)
    {
        return HC_CMD_ERR_BAD_JSON;
    }

    if ((parsed_tokens == 0) || (tokens[0].type != JSMN_OBJECT))
    {
        return HC_CMD_ERR_BAD_JSON;
    }

    request->type = hc_jsonl_parse_type(line, tokens, (size_t)parsed_tokens);
    if (request->type == HC_PKT_UNKNOWN)
    {
        return HC_CMD_ERR_BAD_TYPE;
    }

    msg_index = hc_jsonl_find_object_value(line, tokens, (size_t)parsed_tokens, 0, "msg");
    if (msg_index < 0)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (!hc_jsonl_token_is_primitive(&tokens[msg_index]))
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    msg_len = (size_t)(tokens[msg_index].end - tokens[msg_index].start);
    if ((msg_len == 0u) || (msg_len >= sizeof(msg_text)))
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    memcpy(msg_text, &line[tokens[msg_index].start], msg_len);
    msg_text[msg_len] = '\0';

    request->msg = (uint32_t)strtoul(msg_text, NULL, 10);
    request->has_msg = true;

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request,
                                             char *date_time_out,
                                             size_t date_time_out_size)
{
    int args_index;
    int date_time_index;
    size_t value_len;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (date_time_out == NULL) || (date_time_out_size < HC_CMD_MAX_DATE_TIME_LEN))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    args_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, 0, "args");
    if (args_index < 0)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (tokens[args_index].type != JSMN_OBJECT)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    date_time_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "date_time");
    if (date_time_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_token_is_string(&tokens[date_time_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(tokens[date_time_index].end - tokens[date_time_index].start);
    if ((value_len + 1u) > date_time_out_size)
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(date_time_out, &line[tokens[date_time_index].start], value_len);
    date_time_out[value_len] = '\0';

    return HC_CMD_OK;
}
