#include "hc_jsonl_parse.h"

#include "jsmn.h"
#include "fw_app.h"

#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
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

static hc_cmd_status_t hc_jsonl_get_args_object(const char *line,
                                                const jsmntok_t *tokens,
                                                int *args_index_out)
{
    int args_index;

    if ((line == NULL) || (tokens == NULL) || (args_index_out == NULL))
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

    *args_index_out = args_index;
    return HC_CMD_OK;
}

static hc_cmd_status_t hc_jsonl_parse_u32_primitive(const char *line,
                                                    const jsmntok_t *token,
                                                    uint32_t *value_out)
{
    size_t value_len;
    char value_text[16];
    size_t i;

    if ((line == NULL) || (token == NULL) || (value_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (!hc_jsonl_token_is_primitive(token))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(token->end - token->start);
    if ((value_len == 0U) || (value_len >= sizeof(value_text)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(value_text, &line[token->start], value_len);
    value_text[value_len] = '\0';

    for (i = 0U; i < value_len; ++i)
    {
        if (!isdigit((unsigned char)value_text[i]))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }
    }

    *value_out = (uint32_t)strtoul(value_text, NULL, 10);
    return HC_CMD_OK;
}

static hc_cmd_status_t hc_jsonl_parse_i32_primitive(const char *line,
                                                    const jsmntok_t *token,
                                                    int32_t *value_out)
{
    size_t value_len;
    char value_text[16];
    char *end_ptr;
    long parsed_value;

    if ((line == NULL) || (token == NULL) || (value_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (!hc_jsonl_token_is_primitive(token))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(token->end - token->start);
    if ((value_len == 0U) || (value_len >= sizeof(value_text)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(value_text, &line[token->start], value_len);
    value_text[value_len] = '\0';

    parsed_value = strtol(value_text, &end_ptr, 10);
    if ((end_ptr == value_text) || (*end_ptr != '\0'))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    if ((parsed_value > (long)INT32_MAX) || (parsed_value < (long)INT32_MIN))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    *value_out = (int32_t)parsed_value;
    return HC_CMD_OK;
}

static hc_cmd_status_t hc_jsonl_parse_bool_primitive(const char *line,
                                                     const jsmntok_t *token,
                                                     bool *value_out)
{
    if ((line == NULL) || (token == NULL) || (value_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (!hc_jsonl_token_is_primitive(token))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (hc_jsonl_token_equals(line, token, "true"))
    {
        *value_out = true;
        return HC_CMD_OK;
    }

    if (hc_jsonl_token_equals(line, token, "false"))
    {
        *value_out = false;
        return HC_CMD_OK;
    }

    return HC_CMD_ERR_BAD_VALUE;
}

static bool hc_jsonl_lookup_adc_channel_name(const char *name,
                                             size_t name_len,
                                             uint32_t *channel_out)
{
    typedef struct
    {
        const char *Name;
        uint32_t Channel;
    } hc_jsonl_adc_channel_name_t;

    static const hc_jsonl_adc_channel_name_t s_adc_channel_names[] = {
        { "vupstream",    0U },
        { "ltc3901_vcc",  1U },
        { "lt8316_vout",  2U },
        { "ltc3901_me",   3U },
        { "ltc3901_mf",   4U },
        { "lt8316_gate",  5U },
        { "temp",         6U },
        { "vrefint",      7U },
    };
    uint8_t i;

    if ((name == NULL) || (channel_out == NULL))
    {
        return false;
    }

    for (i = 0U; i < (uint8_t)(sizeof(s_adc_channel_names) / sizeof(s_adc_channel_names[0])); ++i)
    {
        size_t entry_len;

        entry_len = strlen(s_adc_channel_names[i].Name);
        if ((entry_len == name_len) && (strncmp(name, s_adc_channel_names[i].Name, name_len) == 0))
        {
            *channel_out = s_adc_channel_names[i].Channel;
            return true;
        }
    }

    return false;
}

static hc_cmd_status_t hc_jsonl_parse_adc_channel_selector(const char *line,
                                                           const jsmntok_t *token,
                                                           uint32_t *channel_out)
{
    size_t value_len;

    if ((line == NULL) || (token == NULL) || (channel_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (hc_jsonl_token_is_primitive(token))
    {
        return hc_jsonl_parse_u32_primitive(line, token, channel_out);
    }

    if (!hc_jsonl_token_is_string(token))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(token->end - token->start);
    if (!hc_jsonl_lookup_adc_channel_name(&line[token->start], value_len, channel_out))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    return HC_CMD_OK;
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

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
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

hc_cmd_status_t hc_jsonl_parse_set_sts_period_ms(const char *line,
                                                 const jsmntok_t *tokens,
                                                 const hc_cmd_request_t *request,
                                                 hc_jsonl_set_sts_period_request_t *sts_period_request_out)
{
    int args_index;
    int sts_period_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (sts_period_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    sts_period_request_out->PeriodMs = 0U;

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    sts_period_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "sts_period_ms");
    if (sts_period_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    return hc_jsonl_parse_u32_primitive(line, &tokens[sts_period_index], &sts_period_request_out->PeriodMs);
}

hc_cmd_status_t hc_jsonl_parse_set_debug_config(const char *line,
                                                const jsmntok_t *tokens,
                                                const hc_cmd_request_t *request,
                                                hc_jsonl_set_debug_request_t *debug_request_out)
{
    int args_index;
    int dbg_period_index;
    int dbg_signals_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (debug_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(debug_request_out, 0, sizeof(*debug_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    dbg_period_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "dbg_period_ms");
    if (dbg_period_index >= 0)
    {
        hc_cmd_status_t parse_status;

        parse_status = hc_jsonl_parse_u32_primitive(line, &tokens[dbg_period_index], &debug_request_out->PeriodMs);
        if (parse_status != HC_CMD_OK)
        {
            return parse_status;
        }

        debug_request_out->HasPeriodMs = true;
    }

    dbg_signals_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "dbg_signals");
    if (dbg_signals_index >= 0)
    {
        int i;

        if (tokens[dbg_signals_index].type != JSMN_ARRAY)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (tokens[dbg_signals_index].size > (int)HC_DEBUG_TELEMETRY_MAX_SIGNALS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        debug_request_out->HasSignals = true;
        for (i = 0; i < tokens[dbg_signals_index].size; ++i)
        {
            int element_index;
            size_t name_len;
            uint8_t signal_id;
            bool duplicate_found;
            uint8_t j;

            element_index = dbg_signals_index + 1 + i;
            if (!hc_jsonl_token_is_string(&tokens[element_index]))
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            name_len = (size_t)(tokens[element_index].end - tokens[element_index].start);
            if (!fw_app_debug_lookup_signal_id(&line[tokens[element_index].start], name_len, &signal_id))
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            duplicate_found = false;
            for (j = 0U; j < debug_request_out->SignalCount; ++j)
            {
                if (debug_request_out->SignalIds[j] == signal_id)
                {
                    duplicate_found = true;
                    break;
                }
            }

            if (!duplicate_found)
            {
                debug_request_out->SignalIds[debug_request_out->SignalCount] = signal_id;
                debug_request_out->SignalCount++;
            }
        }
    }

    if (!debug_request_out->HasPeriodMs && !debug_request_out->HasSignals)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_adc_calibration(const char *line,
                                                   const jsmntok_t *tokens,
                                                   const hc_cmd_request_t *request,
                                                   hc_jsonl_set_adc_cal_request_t *adc_cal_request_out)
{
    int args_index;
    int adc_cal_index;
    int channel_index;
    int slope_index;
    int offset_index;
    int valid_index;
    hc_cmd_status_t parse_status;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (adc_cal_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(adc_cal_request_out, 0, sizeof(*adc_cal_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    adc_cal_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "adc_cal");
    if (adc_cal_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (tokens[adc_cal_index].type != JSMN_OBJECT)
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    channel_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, adc_cal_index, "channel");
    slope_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, adc_cal_index, "slope_scaled");
    offset_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, adc_cal_index, "offset");
    valid_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, adc_cal_index, "valid");
    if ((channel_index < 0) || (slope_index < 0) || (offset_index < 0) || (valid_index < 0))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    parse_status = hc_jsonl_parse_adc_channel_selector(line, &tokens[channel_index], &adc_cal_request_out->Channel);
    if (parse_status != HC_CMD_OK)
    {
        return parse_status;
    }

    parse_status = hc_jsonl_parse_i32_primitive(line, &tokens[slope_index], &adc_cal_request_out->SlopeScaled);
    if (parse_status != HC_CMD_OK)
    {
        return parse_status;
    }

    parse_status = hc_jsonl_parse_i32_primitive(line, &tokens[offset_index], &adc_cal_request_out->Offset);
    if (parse_status != HC_CMD_OK)
    {
        return parse_status;
    }

    return hc_jsonl_parse_bool_primitive(line, &tokens[valid_index], &adc_cal_request_out->Valid);
}

hc_cmd_status_t hc_jsonl_parse_get_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request)
{
    int args_index;
    int date_time_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    date_time_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "date_time");
    if (date_time_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_token_is_primitive(&tokens[date_time_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    if (!hc_jsonl_token_equals(line, &tokens[date_time_index], "true"))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_get_raw_adc(const char *line,
                                           const jsmntok_t *tokens,
                                           const hc_cmd_request_t *request,
                                           hc_jsonl_get_raw_adc_request_t *raw_adc_request_out)
{
    int args_index;
    int raw_adc_index;
    size_t value_len;
    char value_text[16];

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (raw_adc_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    raw_adc_request_out->Requested = false;
    raw_adc_request_out->HasChannel = false;
    raw_adc_request_out->Channel = 0u;

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    raw_adc_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "raw_adc");
    if (raw_adc_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    raw_adc_request_out->Requested = true;

    if (hc_jsonl_token_is_primitive(&tokens[raw_adc_index]) && hc_jsonl_token_equals(line, &tokens[raw_adc_index], "true"))
    {
        return HC_CMD_OK;
    }

    if (!hc_jsonl_token_is_primitive(&tokens[raw_adc_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(tokens[raw_adc_index].end - tokens[raw_adc_index].start);
    if ((value_len == 0u) || (value_len >= sizeof(value_text)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(value_text, &line[tokens[raw_adc_index].start], value_len);
    value_text[value_len] = '\0';

    raw_adc_request_out->Channel = (uint32_t)strtoul(value_text, NULL, 10);
    raw_adc_request_out->HasChannel = true;
    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_get_debug_config(const char *line,
                                                const jsmntok_t *tokens,
                                                const hc_cmd_request_t *request,
                                                hc_jsonl_get_debug_request_t *debug_request_out)
{
    int args_index;
    int dbg_period_index;
    int dbg_signals_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (debug_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(debug_request_out, 0, sizeof(*debug_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    dbg_period_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "dbg_period_ms");
    if (dbg_period_index >= 0)
    {
        if (!hc_jsonl_token_is_primitive(&tokens[dbg_period_index]) ||
            !hc_jsonl_token_equals(line, &tokens[dbg_period_index], "true"))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        debug_request_out->RequestConfig = true;
    }

    dbg_signals_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "dbg_signals");
    if (dbg_signals_index >= 0)
    {
        if (tokens[dbg_signals_index].type == JSMN_ARRAY)
        {
            int i;

            if (tokens[dbg_signals_index].size > (int)HC_DEBUG_TELEMETRY_MAX_SIGNALS)
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            debug_request_out->RequestSample = true;
            for (i = 0; i < tokens[dbg_signals_index].size; ++i)
            {
                int element_index;
                size_t name_len;
                uint8_t signal_id;
                bool duplicate_found;
                uint8_t j;

                element_index = dbg_signals_index + 1 + i;
                if (!hc_jsonl_token_is_string(&tokens[element_index]))
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }

                name_len = (size_t)(tokens[element_index].end - tokens[element_index].start);
                if (!fw_app_debug_lookup_signal_id(&line[tokens[element_index].start], name_len, &signal_id))
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }

                duplicate_found = false;
                for (j = 0U; j < debug_request_out->SignalCount; ++j)
                {
                    if (debug_request_out->SignalIds[j] == signal_id)
                    {
                        duplicate_found = true;
                        break;
                    }
                }

                if (!duplicate_found)
                {
                    debug_request_out->SignalIds[debug_request_out->SignalCount] = signal_id;
                    debug_request_out->SignalCount++;
                }
            }
        }
        else
        {
            if (!hc_jsonl_token_is_primitive(&tokens[dbg_signals_index]) ||
                !hc_jsonl_token_equals(line, &tokens[dbg_signals_index], "true"))
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            debug_request_out->RequestConfig = true;
        }
    }

    if (!debug_request_out->RequestConfig && !debug_request_out->RequestSample)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_get_adc_calibration(const char *line,
                                                   const jsmntok_t *tokens,
                                                   const hc_cmd_request_t *request,
                                                   hc_jsonl_get_adc_cal_request_t *adc_cal_request_out)
{
    int args_index;
    int adc_cal_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (adc_cal_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    adc_cal_request_out->Requested = false;
    adc_cal_request_out->Channel = 0U;

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    adc_cal_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "adc_cal");
    if (adc_cal_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    adc_cal_request_out->Requested = true;
    return hc_jsonl_parse_adc_channel_selector(line, &tokens[adc_cal_index], &adc_cal_request_out->Channel);
}
