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

bool hc_jsonl_rsp_build_set_sts_period_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          uint32_t sts_period_ms);

bool hc_jsonl_rsp_build_set_debug_config_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            uint32_t dbg_period_ms,
                                            const char *dbg_signals_json);

bool hc_jsonl_rsp_build_set_adc_cal_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       uint32_t channel,
                                       int32_t slope_scaled,
                                       int32_t offset,
                                       bool valid);

bool hc_jsonl_rsp_build_get_datetime_ok(char *out,
                                        size_t out_size,
                                        uint32_t hc_id,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time);

bool hc_jsonl_rsp_build_get_debug_config_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            uint32_t dbg_period_ms,
                                            const char *dbg_signals_json);

bool hc_jsonl_rsp_build_get_debug_signals_ok(char *out,
                                             size_t out_size,
                                             uint32_t hc_id,
                                             uint32_t msg,
                                             const char *ts,
                                             const char *signals_json);

bool hc_jsonl_rsp_build_get_adc_cal_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       uint32_t channel,
                                       int32_t slope_scaled,
                                       int32_t offset,
                                       bool valid);

bool hc_jsonl_rsp_build_get_raw_adc_ok(char *out,
                                       size_t out_size,
                                       uint32_t hc_id,
                                       uint32_t msg,
                                       const char *ts,
                                       bool include_channel,
                                       uint32_t channel,
                                       uint16_t raw_adc);

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
