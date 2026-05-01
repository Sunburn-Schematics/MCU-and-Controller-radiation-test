#ifndef HC_JSONL_RSP_H_
#define HC_JSONL_RSP_H_

#include "hc_cmd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool hc_jsonl_rsp_build_set_datetime_ok(char *out,
                                        size_t out_size,
                                        uint32_t hc_id,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time);

bool hc_jsonl_rsp_build_error(char *out,
                              size_t out_size,
                              uint32_t hc_id,
                              bool include_msg,
                              uint32_t msg,
                              const char *ts,
                              const char *code,
                              const char *message);

#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_RSP_H_ */
