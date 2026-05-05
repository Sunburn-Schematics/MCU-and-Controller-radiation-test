#ifndef HC_JSONL_PARSE_H_
#define HC_JSONL_PARSE_H_

#include "hc_cmd_types.h"
#include "../jsmn/jsmn.h"
#include "hc_debug_telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool Requested;
    bool HasChannel;
    uint32_t Channel;
} hc_jsonl_get_raw_adc_request_t;

typedef struct
{
    uint32_t PeriodMs;
} hc_jsonl_set_sts_period_request_t;

typedef struct
{
    bool HasPeriodMs;
    uint32_t PeriodMs;
    bool HasSignals;
    uint8_t SignalCount;
    uint8_t SignalIds[HC_DEBUG_TELEMETRY_MAX_SIGNALS];
} hc_jsonl_set_debug_request_t;

typedef struct
{
    bool RequestConfig;
    bool RequestSample;
    uint8_t SignalCount;
    uint8_t SignalIds[HC_DEBUG_TELEMETRY_MAX_SIGNALS];
} hc_jsonl_get_debug_request_t;

typedef struct
{
    uint32_t Channel;
    int32_t SlopeScaled;
    int32_t Offset;
    bool Valid;
} hc_jsonl_set_adc_cal_request_t;

typedef struct
{
    bool Requested;
    uint32_t Channel;
} hc_jsonl_get_adc_cal_request_t;

hc_cmd_status_t hc_jsonl_parse_request(const char *line,
                                       jsmntok_t *tokens,
                                       size_t max_tokens,
                                       hc_cmd_request_t *request);

hc_cmd_status_t hc_jsonl_parse_set_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request,
                                             char *date_time_out,
                                             size_t date_time_out_size);

hc_cmd_status_t hc_jsonl_parse_set_sts_period_ms(const char *line,
                                                 const jsmntok_t *tokens,
                                                 const hc_cmd_request_t *request,
                                                 hc_jsonl_set_sts_period_request_t *sts_period_request_out);

hc_cmd_status_t hc_jsonl_parse_set_debug_config(const char *line,
                                                const jsmntok_t *tokens,
                                                const hc_cmd_request_t *request,
                                                hc_jsonl_set_debug_request_t *debug_request_out);

hc_cmd_status_t hc_jsonl_parse_set_adc_calibration(const char *line,
                                                   const jsmntok_t *tokens,
                                                   const hc_cmd_request_t *request,
                                                   hc_jsonl_set_adc_cal_request_t *adc_cal_request_out);

hc_cmd_status_t hc_jsonl_parse_get_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request);

hc_cmd_status_t hc_jsonl_parse_get_raw_adc(const char *line,
                                           const jsmntok_t *tokens,
                                           const hc_cmd_request_t *request,
                                           hc_jsonl_get_raw_adc_request_t *raw_adc_request_out);

hc_cmd_status_t hc_jsonl_parse_get_debug_config(const char *line,
                                                const jsmntok_t *tokens,
                                                const hc_cmd_request_t *request,
                                                hc_jsonl_get_debug_request_t *debug_request_out);

hc_cmd_status_t hc_jsonl_parse_get_adc_calibration(const char *line,
                                                   const jsmntok_t *tokens,
                                                   const hc_cmd_request_t *request,
                                                   hc_jsonl_get_adc_cal_request_t *adc_cal_request_out);

#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_PARSE_H_ */
