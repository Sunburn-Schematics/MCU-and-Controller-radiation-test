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

bool hc_jsonl_rsp_build_set_ltc3901_cmd_ok(char *out,
                                           size_t out_size,
                                           uint32_t hc_id,
                                           uint32_t msg,
                                           const char *ts,
                                           const char *command);
bool hc_jsonl_rsp_build_set_lt8316_cmd_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          const char *command);
bool hc_jsonl_rsp_build_set_manager_cmds_ok(char *out,
                                            size_t out_size,
                                            uint32_t hc_id,
                                            uint32_t msg,
                                            const char *ts,
                                            const char *ltc3901_command,
                                            const char *lt8316_command);
bool hc_jsonl_rsp_build_set_args_ok(char *out,
                                    size_t out_size,
                                    uint32_t hc_id,
                                    uint32_t msg,
                                    const char *ts,
                                    const char *args_json);
bool hc_jsonl_rsp_build_set_digital_signals_ok(char *out,
                                               size_t out_size,
                                               uint8_t hc_id,
                                               uint32_t msg,
                                               const char *ts,
                                               const char *signals_json);
bool hc_jsonl_rsp_build_get_datetime_ok(char *out,
                                        size_t out_size,
                                        uint32_t hc_id,
                                        uint32_t msg,
                                        const char *ts,
                                        const char *date_time);

bool hc_jsonl_rsp_build_get_sw_version_ok(char *out,
                                          size_t out_size,
                                          uint32_t hc_id,
                                          uint32_t msg,
                                          const char *ts,
                                          const char *sw_version);

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

bool hc_jsonl_rsp_build_error(char *out,
                              size_t out_size,
                              uint32_t hc_id,
                              bool include_msg,
                              uint32_t msg,
                              const char *ts,
                              const char *code,
                              const char *message);

bool hc_jsonl_rsp_build_event_msg(char *out,
                                  size_t out_size,
                                  uint32_t hc_id,
                                  const char *ts,
                                  const char *message);

#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_RSP_H_ */
