#ifndef HC_JSONL_PARSE_H_
#define HC_JSONL_PARSE_H_

#include "hc_cmd_types.h"
#include "../jsmn/jsmn.h"

#ifdef __cplusplus
extern "C" {
#endif

hc_cmd_status_t hc_jsonl_parse_request(const char *line,
                                       jsmntok_t *tokens,
                                       size_t max_tokens,
                                       hc_cmd_request_t *request);

hc_cmd_status_t hc_jsonl_parse_set_date_time(const char *line,
                                             const jsmntok_t *tokens,
                                             const hc_cmd_request_t *request,
                                             char *date_time_out,
                                             size_t date_time_out_size);

#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_PARSE_H_ */
