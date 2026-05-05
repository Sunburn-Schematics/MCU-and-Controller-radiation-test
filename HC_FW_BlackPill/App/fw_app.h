#ifndef FW_APP_H
#define FW_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "hc_debug_telemetry.h"
#include "usb_vcp_drv.h"


void fw_app_init(void);
void fw_app_run(void);
bool fw_app_set_sts_period_ms(uint32_t period_ms);
uint32_t fw_app_get_sts_period_ms(void);
bool fw_app_set_debug_config(const hc_debug_telemetry_config_t *config);
bool fw_app_get_debug_config(hc_debug_telemetry_config_t *config_out);
bool fw_app_debug_lookup_signal_id(const char *name, size_t name_len, uint8_t *signal_id_out);
const char *fw_app_debug_get_signal_name(uint8_t signal_id);
bool fw_app_debug_format_signals_json(const uint8_t *signal_ids,
                                      uint8_t signal_count,
                                      char *buffer,
                                      size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* FW_APP_H */
