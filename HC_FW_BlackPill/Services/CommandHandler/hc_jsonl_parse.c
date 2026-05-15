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

static int hc_jsonl_skip_token(const jsmntok_t *tokens, size_t token_count, int token_index)
{
    int next_index;
    int child_count;
    int i;

    if ((tokens == NULL) || (token_index < 0) || ((size_t)token_index >= token_count))
    {
        return token_index + 1;
    }

    next_index = token_index + 1;
    if ((tokens[token_index].type != JSMN_OBJECT) && (tokens[token_index].type != JSMN_ARRAY))
    {
        return next_index;
    }

    child_count = tokens[token_index].size;
    if (tokens[token_index].type == JSMN_OBJECT)
    {
        child_count *= 2;
    }

    for (i = 0; (i < child_count) && ((size_t)next_index < token_count); ++i)
    {
        next_index = hc_jsonl_skip_token(tokens, token_count, next_index);
    }

    return next_index;
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
        int value_index = i + 1;

        if ((size_t)value_index >= token_count)
        {
            return -1;
        }

        if (hc_jsonl_token_equals(json, &tokens[i], key))
        {
            return value_index;
        }

        i = hc_jsonl_skip_token(tokens, token_count, value_index);
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

static hc_cmd_status_t hc_jsonl_get_args_token(const char *line,
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

    *args_index_out = args_index;
    return HC_CMD_OK;
}

static bool hc_jsonl_args_array_contains_string(const char *line,
                                                const jsmntok_t *tokens,
                                                int array_index,
                                                const char *field_name)
{
    int element_index;
    int i;

    if ((line == NULL) || (tokens == NULL) || (field_name == NULL) || (array_index < 0))
    {
        return false;
    }

    if (tokens[array_index].type != JSMN_ARRAY)
    {
        return false;
    }

    element_index = array_index + 1;
    for (i = 0; i < tokens[array_index].size; ++i)
    {
        if (hc_jsonl_token_is_string(&tokens[element_index]) &&
            hc_jsonl_token_equals(line, &tokens[element_index], field_name))
        {
            return true;
        }

        element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
    }

    return false;
}

static bool hc_jsonl_token_contains_char(const char *line,
                                         const jsmntok_t *token,
                                         char value)
{
    int i;

    if ((line == NULL) || (token == NULL) || (token->start < 0) || (token->end < token->start))
    {
        return false;
    }

    for (i = token->start; i < token->end; ++i)
    {
        if (line[i] == value)
        {
            return true;
        }
    }

    return false;
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

typedef enum
{
    HC_JSONL_ADC_CAL_KEY_NO_MATCH = 0,
    HC_JSONL_ADC_CAL_KEY_MATCH,
    HC_JSONL_ADC_CAL_KEY_BAD_VALUE
} hc_jsonl_adc_cal_key_status_t;

typedef enum
{
    HC_JSONL_ADC_CAL_FIELD_SLOPE_SCALED = 0,
    HC_JSONL_ADC_CAL_FIELD_OFFSET,
    HC_JSONL_ADC_CAL_FIELD_VALID
} hc_jsonl_adc_cal_field_t;

static hc_jsonl_adc_cal_key_status_t hc_jsonl_parse_adc_cal_key(const char *line,
                                                                 const jsmntok_t *token,
                                                                 uint32_t *channel_out,
                                                                 hc_jsonl_adc_cal_field_t *field_out)
{
    const char *key;
    size_t key_len;
    size_t channel_start;
    size_t channel_len;
    size_t field_start;
    size_t field_len;
    size_t i;

    if ((line == NULL) || (token == NULL) || (channel_out == NULL) || (field_out == NULL))
    {
        return HC_JSONL_ADC_CAL_KEY_BAD_VALUE;
    }

    if (!hc_jsonl_token_is_string(token) || (token->start < 0) || (token->end < token->start))
    {
        return HC_JSONL_ADC_CAL_KEY_BAD_VALUE;
    }

    key = &line[token->start];
    key_len = (size_t)(token->end - token->start);
    if ((key_len < strlen("adc.x.valid")) || (strncmp(key, "adc.", 4U) != 0))
    {
        return HC_JSONL_ADC_CAL_KEY_NO_MATCH;
    }

    channel_start = 4U;
    field_start = 0U;
    for (i = channel_start; i < key_len; ++i)
    {
        if (key[i] == '.')
        {
            field_start = i + 1U;
            break;
        }
    }

    if ((field_start == 0U) || (field_start >= key_len))
    {
        return HC_JSONL_ADC_CAL_KEY_NO_MATCH;
    }

    channel_len = field_start - channel_start - 1U;
    field_len = key_len - field_start;
    if ((channel_len == 0U) || (field_len == 0U))
    {
        return HC_JSONL_ADC_CAL_KEY_BAD_VALUE;
    }

    for (i = field_start; i < key_len; ++i)
    {
        if (key[i] == '.')
        {
            return HC_JSONL_ADC_CAL_KEY_BAD_VALUE;
        }
    }

    if (!hc_jsonl_lookup_adc_channel_name(&key[channel_start], channel_len, channel_out))
    {
        return HC_JSONL_ADC_CAL_KEY_BAD_VALUE;
    }

    if ((field_len == strlen("slope_scaled")) &&
        (strncmp(&key[field_start], "slope_scaled", field_len) == 0))
    {
        *field_out = HC_JSONL_ADC_CAL_FIELD_SLOPE_SCALED;
        return HC_JSONL_ADC_CAL_KEY_MATCH;
    }

    if ((field_len == strlen("offset")) &&
        (strncmp(&key[field_start], "offset", field_len) == 0))
    {
        *field_out = HC_JSONL_ADC_CAL_FIELD_OFFSET;
        return HC_JSONL_ADC_CAL_KEY_MATCH;
    }

    if ((field_len == strlen("valid")) &&
        (strncmp(&key[field_start], "valid", field_len) == 0))
    {
        *field_out = HC_JSONL_ADC_CAL_FIELD_VALID;
        return HC_JSONL_ADC_CAL_KEY_MATCH;
    }

    return HC_JSONL_ADC_CAL_KEY_NO_MATCH;
}

static hc_cmd_status_t hc_jsonl_parse_get_bool_selector(const char *line,
                                                        const jsmntok_t *tokens,
                                                        const char *field_name,
                                                        bool *requested_out)
{
    int args_index;

    if ((line == NULL) || (tokens == NULL) || (field_name == NULL) || (requested_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    *requested_out = false;

    if (hc_jsonl_get_args_token(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (tokens[args_index].type == JSMN_ARRAY)
    {
        if (!hc_jsonl_args_array_contains_string(line, tokens, args_index, field_name))
        {
            return HC_CMD_ERR_BAD_FIELD;
        }

        *requested_out = true;
        return HC_CMD_OK;
    }

    return HC_CMD_ERR_BAD_ARGS;
}

static bool hc_jsonl_token_starts_with(const char *line,
                                       const jsmntok_t *token,
                                       const char *prefix)
{
    size_t prefix_len;
    size_t token_len;

    if ((line == NULL) || (token == NULL) || (prefix == NULL) ||
        (token->start < 0) || (token->end < token->start))
    {
        return false;
    }

    prefix_len = strlen(prefix);
    token_len = (size_t)(token->end - token->start);
    if (token_len <= prefix_len)
    {
        return false;
    }

    return (strncmp(&line[token->start], prefix, prefix_len) == 0);
}

static bool hc_jsonl_token_suffix_equals(const char *line,
                                         const jsmntok_t *token,
                                         const char *prefix,
                                         const char *suffix)
{
    size_t prefix_len;
    size_t suffix_len;
    size_t token_len;

    if ((line == NULL) || (token == NULL) || (prefix == NULL) || (suffix == NULL) ||
        (token->start < 0) || (token->end < token->start))
    {
        return false;
    }

    prefix_len = strlen(prefix);
    suffix_len = strlen(suffix);
    token_len = (size_t)(token->end - token->start);
    if (token_len != (prefix_len + suffix_len))
    {
        return false;
    }

    return (strncmp(&line[token->start], prefix, prefix_len) == 0) &&
           (strncmp(&line[token->start + (int)prefix_len], suffix, suffix_len) == 0);
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
        if (tokens[dbg_signals_index].type == JSMN_ARRAY)
        {
            int i;

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
        // Direct digital output control is handled by top-level signal fields in SET args.
        // dbg_signals remains reserved for the periodic debug telemetry signal array.
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
    int key_index;
    uint32_t selected_channel = 0U;
    bool has_selected_channel = false;

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

    key_index = args_index + 1;
    for (int i = 0; i < tokens[args_index].size; ++i)
    {
        int value_index = key_index + 1;
        uint32_t key_channel;
        hc_jsonl_adc_cal_field_t field;
        hc_jsonl_adc_cal_key_status_t key_status;
        hc_cmd_status_t parse_status;

        if (value_index >= (int)HC_CMD_MAX_TOKENS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        key_status = hc_jsonl_parse_adc_cal_key(line,
                                                &tokens[key_index],
                                                &key_channel,
                                                &field);
        if (key_status == HC_JSONL_ADC_CAL_KEY_BAD_VALUE)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (key_status == HC_JSONL_ADC_CAL_KEY_NO_MATCH)
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }

        if (has_selected_channel && (selected_channel != key_channel))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }
        selected_channel = key_channel;
        has_selected_channel = true;

        switch (field)
        {
            case HC_JSONL_ADC_CAL_FIELD_SLOPE_SCALED:
                if (adc_cal_request_out->HasSlopeScaled)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                parse_status = hc_jsonl_parse_i32_primitive(line,
                                                            &tokens[value_index],
                                                            &adc_cal_request_out->SlopeScaled);
                if (parse_status != HC_CMD_OK)
                {
                    return parse_status;
                }
                adc_cal_request_out->HasSlopeScaled = true;
                break;

            case HC_JSONL_ADC_CAL_FIELD_OFFSET:
                if (adc_cal_request_out->HasOffset)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                parse_status = hc_jsonl_parse_i32_primitive(line,
                                                            &tokens[value_index],
                                                            &adc_cal_request_out->Offset);
                if (parse_status != HC_CMD_OK)
                {
                    return parse_status;
                }
                adc_cal_request_out->HasOffset = true;
                break;

            case HC_JSONL_ADC_CAL_FIELD_VALID:
                if (adc_cal_request_out->HasValid)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                parse_status = hc_jsonl_parse_bool_primitive(line,
                                                             &tokens[value_index],
                                                             &adc_cal_request_out->Valid);
                if (parse_status != HC_CMD_OK)
                {
                    return parse_status;
                }
                adc_cal_request_out->HasValid = true;
                break;

            default:
                return HC_CMD_ERR_BAD_VALUE;
        }

        key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
    }

    if (!has_selected_channel)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    adc_cal_request_out->Channel = selected_channel;
    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_ltc3901_cmd(const char *line,
                                               const jsmntok_t *tokens,
                                               const hc_cmd_request_t *request,
                                               hc_jsonl_set_ltc3901_cmd_request_t *cmd_request_out)
{
    int args_index;
    int cmd_index;
    size_t value_len;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (cmd_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(cmd_request_out, 0, sizeof(*cmd_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    cmd_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "ltc3901_cmd");
    if (cmd_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_token_is_string(&tokens[cmd_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(tokens[cmd_index].end - tokens[cmd_index].start);
    if ((value_len == 0U) || (value_len >= sizeof(cmd_request_out->Command)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(cmd_request_out->Command, &line[tokens[cmd_index].start], value_len);
    cmd_request_out->Command[value_len] = '\0';

    if ((strcmp(cmd_request_out->Command, "RUN") != 0) &&
        (strcmp(cmd_request_out->Command, "HALT") != 0) &&
        (strcmp(cmd_request_out->Command, "RESET") != 0))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_lt8316_cmd(const char *line,
                                              const jsmntok_t *tokens,
                                              const hc_cmd_request_t *request,
                                              hc_jsonl_set_lt8316_cmd_request_t *cmd_request_out)
{
    int args_index;
    int cmd_index;
    size_t value_len;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (cmd_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(cmd_request_out, 0, sizeof(*cmd_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    cmd_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "lt8316_cmd");
    if (cmd_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_token_is_string(&tokens[cmd_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(tokens[cmd_index].end - tokens[cmd_index].start);
    if ((value_len == 0U) || (value_len >= sizeof(cmd_request_out->Command)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(cmd_request_out->Command, &line[tokens[cmd_index].start], value_len);
    cmd_request_out->Command[value_len] = '\0';

    if ((strcmp(cmd_request_out->Command, "RUN") != 0) &&
        (strcmp(cmd_request_out->Command, "RESET") != 0))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_hc_cmd(const char *line,
                                          const jsmntok_t *tokens,
                                          const hc_cmd_request_t *request,
                                          hc_jsonl_set_hc_cmd_request_t *cmd_request_out)
{
    int args_index;
    int cmd_index;
    size_t value_len;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (cmd_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(cmd_request_out, 0, sizeof(*cmd_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    cmd_index = hc_jsonl_find_object_value(line, tokens, HC_CMD_MAX_TOKENS, args_index, "hc_cmd");
    if (cmd_index < 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    if (!hc_jsonl_token_is_string(&tokens[cmd_index]))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    value_len = (size_t)(tokens[cmd_index].end - tokens[cmd_index].start);
    if ((value_len == 0U) || (value_len >= sizeof(cmd_request_out->Command)))
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    memcpy(cmd_request_out->Command, &line[tokens[cmd_index].start], value_len);
    cmd_request_out->Command[value_len] = '\0';

    if (strcmp(cmd_request_out->Command, "RESET") != 0)
    {
        return HC_CMD_ERR_BAD_VALUE;
    }

    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_get_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request)
{
    bool requested;
    hc_cmd_status_t status;

    (void)request;

    status = hc_jsonl_parse_get_bool_selector(line, tokens, "date_time", &requested);
    if (status != HC_CMD_OK)
    {
        return status;
    }

    return requested ? HC_CMD_OK : HC_CMD_ERR_BAD_FIELD;
}

hc_cmd_status_t hc_jsonl_parse_get_sw_version(const char *line,
                                              const jsmntok_t *tokens,
                                              const hc_cmd_request_t *request)
{
    bool requested;
    hc_cmd_status_t status;

    (void)request;

    status = hc_jsonl_parse_get_bool_selector(line, tokens, "sw_version", &requested);
    if (status != HC_CMD_OK)
    {
        return status;
    }

    return requested ? HC_CMD_OK : HC_CMD_ERR_BAD_FIELD;
}

hc_cmd_status_t hc_jsonl_parse_get_debug_config(const char *line,
                                                const jsmntok_t *tokens,
                                                const hc_cmd_request_t *request,
                                                hc_jsonl_get_debug_request_t *debug_request_out)
{
    int args_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (debug_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(debug_request_out, 0, sizeof(*debug_request_out));

    if (hc_jsonl_get_args_token(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (tokens[args_index].type == JSMN_ARRAY)
    {
        int element_index = args_index + 1;
        for (int i = 0; i < tokens[args_index].size; ++i)
        {
            size_t name_len;
            uint8_t signal_id;
            bool duplicate_found;

            if (!hc_jsonl_token_is_string(&tokens[element_index]))
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            if (hc_jsonl_token_equals(line, &tokens[element_index], "dbg_period_ms") ||
                hc_jsonl_token_equals(line, &tokens[element_index], "dbg_signals"))
            {
                debug_request_out->RequestConfig = true;
                element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
                continue;
            }

            name_len = (size_t)(tokens[element_index].end - tokens[element_index].start);
            if (!fw_app_debug_lookup_signal_id(&line[tokens[element_index].start], name_len, &signal_id))
            {
                hc_jsonl_adc_cal_field_t adc_cal_field;
                uint32_t adc_cal_channel;
                if (hc_jsonl_parse_adc_cal_key(line, &tokens[element_index], &adc_cal_channel, &adc_cal_field) == HC_JSONL_ADC_CAL_KEY_MATCH)
                {
                    element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
                    continue;
                }

                if (hc_jsonl_token_contains_char(line, &tokens[element_index], '.'))
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }

                element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
                continue;
            }

            if (debug_request_out->SignalCount >= HC_DEBUG_TELEMETRY_MAX_SIGNALS)
            {
                return HC_CMD_ERR_BAD_VALUE;
            }

            debug_request_out->RequestSample = true;
            duplicate_found = false;
            for (uint8_t j = 0U; j < debug_request_out->SignalCount; ++j)
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

            element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
        }

        if (debug_request_out->RequestConfig || debug_request_out->RequestSample)
        {
            return HC_CMD_OK;
        }

        return HC_CMD_ERR_BAD_FIELD;
    }

    return HC_CMD_ERR_BAD_ARGS;
}

hc_cmd_status_t hc_jsonl_parse_get_adc_calibration(const char *line,
                                                   const jsmntok_t *tokens,
                                                   const hc_cmd_request_t *request,
                                                   hc_jsonl_get_adc_cal_request_t *adc_cal_request_out)
{
    int args_index;
    bool matched_field = false;
    bool selected_channel_valid = false;
    uint32_t selected_channel = 0U;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (adc_cal_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    adc_cal_request_out->Requested = false;
    adc_cal_request_out->Channel = 0U;
    adc_cal_request_out->HasSlopeScaled = false;
    adc_cal_request_out->HasOffset = false;
    adc_cal_request_out->HasValid = false;

    if (hc_jsonl_get_args_token(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    if (tokens[args_index].type != JSMN_ARRAY)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    int element_index = args_index + 1;
    for (int i = 0; i < tokens[args_index].size; ++i)
    {
        uint32_t channel;
        hc_jsonl_adc_cal_field_t field;
        hc_jsonl_adc_cal_key_status_t key_status;

        if (!hc_jsonl_token_is_string(&tokens[element_index]))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        key_status = hc_jsonl_parse_adc_cal_key(line, &tokens[element_index], &channel, &field);
        if (key_status == HC_JSONL_ADC_CAL_KEY_BAD_VALUE)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (key_status == HC_JSONL_ADC_CAL_KEY_NO_MATCH)
        {
            element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
            continue;
        }

        if (selected_channel_valid && (channel != selected_channel))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        selected_channel = channel;
        selected_channel_valid = true;
        matched_field = true;

        switch (field)
        {
            case HC_JSONL_ADC_CAL_FIELD_SLOPE_SCALED:
                if (adc_cal_request_out->HasSlopeScaled)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                adc_cal_request_out->HasSlopeScaled = true;
                break;

            case HC_JSONL_ADC_CAL_FIELD_OFFSET:
                if (adc_cal_request_out->HasOffset)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                adc_cal_request_out->HasOffset = true;
                break;

            case HC_JSONL_ADC_CAL_FIELD_VALID:
                if (adc_cal_request_out->HasValid)
                {
                    return HC_CMD_ERR_BAD_VALUE;
                }
                adc_cal_request_out->HasValid = true;
                break;

            default:
                return HC_CMD_ERR_BAD_VALUE;
        }

        element_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, element_index);
    }

    if (!matched_field)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    adc_cal_request_out->Channel = selected_channel;
    adc_cal_request_out->Requested = true;
    return HC_CMD_OK;
}

hc_cmd_status_t hc_jsonl_parse_set_ltc3901_cfg(const char *line,
                                               const jsmntok_t *tokens,
                                               const hc_cmd_request_t *request,
                                               hc_jsonl_set_ltc3901_cfg_request_t *config_request_out)
{
    int args_index;
    int key_index;
    bool matched_field = false;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (config_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(config_request_out, 0, sizeof(*config_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    key_index = args_index + 1;
    for (int i = 0; i < tokens[args_index].size; ++i)
    {
        int value_index = key_index + 1;
        hc_cmd_status_t status = HC_CMD_OK;

        if (value_index >= (int)HC_CMD_MAX_TOKENS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (!hc_jsonl_token_starts_with(line, &tokens[key_index], "ltc3901."))
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }

        if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "isupply_ma_max"))
        {
            if (config_request_out->HasIsupplyMaMax) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_i32_primitive(line, &tokens[value_index], &config_request_out->Config.isupply_ma_max);
            config_request_out->HasIsupplyMaMax = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "vupstream_mv_min"))
        {
            if (config_request_out->HasVupstreamMvMin) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_i32_primitive(line, &tokens[value_index], &config_request_out->Config.vupstream_mv_min);
            config_request_out->HasVupstreamMvMin = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "ltc3901_vcc_mv_min"))
        {
            if (config_request_out->HasLtc3901VccMvMin) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_i32_primitive(line, &tokens[value_index], &config_request_out->Config.ltc3901_vcc_mv_min);
            config_request_out->HasLtc3901VccMvMin = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "power_up_timeout_ms"))
        {
            if (config_request_out->HasPowerUpTimeoutMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_up_timeout_ms);
            config_request_out->HasPowerUpTimeoutMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "power_retry_delay_ms"))
        {
            if (config_request_out->HasPowerRetryDelayMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_retry_delay_ms);
            config_request_out->HasPowerRetryDelayMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "power_fault_max"))
        {
            if (config_request_out->HasPowerFaultMax) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_fault_max);
            config_request_out->HasPowerFaultMax = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "sync_on_delay_ms"))
        {
            if (config_request_out->HasSyncOnDelayMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.sync_on_delay_ms);
            config_request_out->HasSyncOnDelayMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "sync_hold_on_time_ms"))
        {
            if (config_request_out->HasSyncHoldOnTimeMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.sync_hold_on_time_ms);
            config_request_out->HasSyncHoldOnTimeMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "sync_hold_off_time_ms"))
        {
            if (config_request_out->HasSyncHoldOffTimeMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.sync_hold_off_time_ms);
            config_request_out->HasSyncHoldOffTimeMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "sync_stabilization_time_ms"))
        {
            if (config_request_out->HasSyncStabilizationTimeMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.sync_stabilization_time_ms);
            config_request_out->HasSyncStabilizationTimeMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "sync_fault_delay_ms"))
        {
            if (config_request_out->HasSyncFaultDelayMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.sync_fault_delay_ms);
            config_request_out->HasSyncFaultDelayMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "ltc3901.", "pwr_en"))
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }
        else
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (status != HC_CMD_OK)
        {
            return status;
        }

        matched_field = true;
        key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
    }

    return matched_field ? HC_CMD_OK : HC_CMD_ERR_BAD_FIELD;
}

hc_cmd_status_t hc_jsonl_parse_get_ltc3901_cfg(const char *line,
                                               const jsmntok_t *tokens,
                                               const hc_cmd_request_t *request,
                                               hc_jsonl_get_ltc3901_cfg_request_t *config_request_out)
{
    (void)request;

    if (config_request_out == NULL)
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return hc_jsonl_parse_get_bool_selector(line, tokens, "ltc3901", &config_request_out->Requested);
}

hc_cmd_status_t hc_jsonl_parse_set_lt8316_cfg(const char *line,
                                              const jsmntok_t *tokens,
                                              const hc_cmd_request_t *request,
                                              hc_jsonl_set_lt8316_cfg_request_t *config_request_out)
{
    int args_index;
    int key_index;
    bool matched_field = false;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (config_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(config_request_out, 0, sizeof(*config_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    key_index = args_index + 1;
    for (int i = 0; i < tokens[args_index].size; ++i)
    {
        int value_index = key_index + 1;
        hc_cmd_status_t status = HC_CMD_OK;

        if (value_index >= (int)HC_CMD_MAX_TOKENS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (!hc_jsonl_token_starts_with(line, &tokens[key_index], "lt8316."))
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }

        if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "lt8316.", "power_retry_delay_ms"))
        {
            if (config_request_out->HasPowerRetryDelayMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_retry_delay_ms);
            config_request_out->HasPowerRetryDelayMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "lt8316.", "power_fault_max"))
        {
            if (config_request_out->HasPowerFaultMax) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_fault_max);
            config_request_out->HasPowerFaultMax = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "lt8316.", "power_on_stabilization_time_ms"))
        {
            if (config_request_out->HasPowerOnStabilizationTimeMs) { return HC_CMD_ERR_BAD_VALUE; }
            status = hc_jsonl_parse_u32_primitive(line, &tokens[value_index], &config_request_out->Config.power_on_stabilization_time_ms);
            config_request_out->HasPowerOnStabilizationTimeMs = (status == HC_CMD_OK);
        }
        else if (hc_jsonl_token_suffix_equals(line, &tokens[key_index], "lt8316.", "pwr_en"))
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }
        else
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (status != HC_CMD_OK)
        {
            return status;
        }

        matched_field = true;
        key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
    }

    return matched_field ? HC_CMD_OK : HC_CMD_ERR_BAD_FIELD;
}

hc_cmd_status_t hc_jsonl_parse_get_lt8316_cfg(const char *line,
                                              const jsmntok_t *tokens,
                                              const hc_cmd_request_t *request,
                                              hc_jsonl_get_lt8316_cfg_request_t *config_request_out)
{
    (void)request;

    if (config_request_out == NULL)
    {
        return HC_CMD_ERR_INTERNAL;
    }

    return hc_jsonl_parse_get_bool_selector(line, tokens, "lt8316", &config_request_out->Requested);
}

hc_cmd_status_t hc_jsonl_parse_set_digital_signals(const char *line,
                                                  const jsmntok_t *tokens,
                                                  const hc_cmd_request_t *request,
                                                  hc_jsonl_set_digital_signals_request_t *digital_signals_request_out)
{
    int args_index;
    int key_index;

    (void)request;

    if ((line == NULL) || (tokens == NULL) || (digital_signals_request_out == NULL))
    {
        return HC_CMD_ERR_INTERNAL;
    }

    memset(digital_signals_request_out, 0, sizeof(*digital_signals_request_out));

    if (hc_jsonl_get_args_object(line, tokens, &args_index) != HC_CMD_OK)
    {
        return HC_CMD_ERR_BAD_ARGS;
    }

    key_index = args_index + 1;
    for (int i = 0; i < tokens[args_index].size; ++i)
    {
        int value_index = key_index + 1;
        size_t name_len;
        uint8_t signal_id;
        bool value;
        bool duplicate_found;

        if (value_index >= (int)HC_CMD_MAX_TOKENS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (!hc_jsonl_token_is_string(&tokens[key_index]))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        name_len = (size_t)(tokens[key_index].end - tokens[key_index].start);
        if (!fw_app_debug_lookup_signal_id(&line[tokens[key_index].start], name_len, &signal_id))
        {
            key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
            continue;
        }

        if (!hc_jsonl_token_is_primitive(&tokens[value_index]))
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (hc_jsonl_token_equals(line, &tokens[value_index], "true"))
        {
            value = true;
        }
        else if (hc_jsonl_token_equals(line, &tokens[value_index], "false"))
        {
            value = false;
        }
        else
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        if (digital_signals_request_out->SignalCount >= HC_DEBUG_TELEMETRY_MAX_SIGNALS)
        {
            return HC_CMD_ERR_BAD_VALUE;
        }

        duplicate_found = false;
        for (uint8_t j = 0U; j < digital_signals_request_out->SignalCount; ++j)
        {
            if (digital_signals_request_out->SignalIds[j] == signal_id)
            {
                duplicate_found = true;
                break;
            }
        }

        if (!duplicate_found)
        {
            digital_signals_request_out->SignalIds[digital_signals_request_out->SignalCount] = signal_id;
            digital_signals_request_out->Values[digital_signals_request_out->SignalCount] = value;
            digital_signals_request_out->SignalCount++;
        }

        key_index = hc_jsonl_skip_token(tokens, HC_CMD_MAX_TOKENS, value_index);
    }

    if (digital_signals_request_out->SignalCount == 0)
    {
        return HC_CMD_ERR_BAD_FIELD;
    }

    return HC_CMD_OK;
}
