#ifndef HC_JSONL_DISPATCH_H_
#define HC_JSONL_DISPATCH_H_

#include "hc_cmd_types.h"
#include "../jsmn/jsmn.h"

#ifdef __cplusplus
extern "C" {
#endif

hc_cmd_status_t hc_jsonl_dispatch_request(const char *line,
                                          const jsmntok_t *tokens,
                                          const hc_cmd_request_t *request,
                                          char *rsp_buf,
                                          size_t rsp_buf_size);

#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_DISPATCH_H_ */
