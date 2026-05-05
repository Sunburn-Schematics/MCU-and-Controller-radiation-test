#ifndef HC_DEBUG_TELEMETRY_H
#define HC_DEBUG_TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HC_DEBUG_TELEMETRY_MAX_SIGNALS (16U)

typedef struct
{
    uint32_t PeriodMs;
    uint8_t SignalCount;
    uint8_t SignalIds[HC_DEBUG_TELEMETRY_MAX_SIGNALS];
} hc_debug_telemetry_config_t;

void hc_debug_telemetry_init(void);
void hc_debug_telemetry_task(void);
bool hc_debug_telemetry_set_config(const hc_debug_telemetry_config_t *config);
bool hc_debug_telemetry_get_config(hc_debug_telemetry_config_t *config_out);
bool hc_debug_telemetry_lookup_signal_id(const char *name, size_t name_len, uint8_t *signal_id_out);
const char *hc_debug_telemetry_get_signal_name(uint8_t signal_id);
bool hc_debug_telemetry_format_signals_json(const uint8_t *signal_ids,
                                            uint8_t signal_count,
                                            char *buffer,
                                            size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* HC_DEBUG_TELEMETRY_H */
