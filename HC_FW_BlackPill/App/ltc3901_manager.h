#ifndef LTC3901_MANAGER_H_
#define LTC3901_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LTC3901_MGR_STATE_RESET = 0,
    LTC3901_MGR_STATE_HALT,
    LTC3901_MGR_STATE_POWER_UP,
    LTC3901_MGR_STATE_POWER_FAULT,
    LTC3901_MGR_STATE_POWERED,
    LTC3901_MGR_STATE_POWERED_SYNC_ON,
    LTC3901_MGR_STATE_POWERED_SYNC_OFF,
    LTC3901_MGR_STATE_POWERED_SYNC_FAULT,
} ltc3901_manager_state_t;

typedef enum
{
    LTC3901_MGR_REQUEST_NONE = 0,
    LTC3901_MGR_REQUEST_RUN,
    LTC3901_MGR_REQUEST_HALT,
    LTC3901_MGR_REQUEST_RESET,
} ltc3901_manager_request_t;

typedef enum
{
    LTC3901_MGR_EVT_NONE = 0,
    LTC3901_MGR_EVT_ENTERING_POWER_UP,
    LTC3901_MGR_EVT_ISUPPLY_TOO_HIGH,
    LTC3901_MGR_EVT_POWER_UP_TIMEOUT,
    LTC3901_MGR_EVT_POWERED,
    LTC3901_MGR_EVT_RETRYING_POWER_UP,
    LTC3901_MGR_EVT_VUPSTREAM_TOO_LOW,
    LTC3901_MGR_EVT_LTC3901_VCC_TOO_LOW,
    LTC3901_MGR_EVT_CYCLE_SYNC_ON,
    LTC3901_MGR_EVT_CYCLE_SYNC_OFF,
    LTC3901_MGR_EVT_ME_STOPPED,
    LTC3901_MGR_EVT_MF_STOPPED,
} ltc3901_manager_event_t;

typedef struct
{
    int32_t isupply_ma_max;
    int32_t vupstream_mv_min;
    int32_t ltc3901_vcc_mv_min;
    uint32_t power_up_timeout_ms;
    uint32_t power_retry_delay_ms;
    uint32_t power_fault_max;
    uint32_t sync_on_delay_ms;
    uint32_t sync_hold_on_time_ms;
    uint32_t sync_hold_off_time_ms;
    uint32_t sync_stabilization_time_ms;
    uint32_t sync_fault_delay_ms;
} ltc3901_manager_config_t;

typedef struct
{
    uint32_t now_ms;
    ltc3901_manager_request_t request;

    int32_t isupply_ma;
    int32_t vupstream_mv;
    int32_t ltc3901_vcc_mv;

    bool me_freq_valid;
    bool mf_freq_valid;
} ltc3901_manager_inputs_t;

typedef struct
{
    bool ltc3901_power_enable;
    bool sdra_drv_enable;
    bool sdrb_drv_enable;
    ltc3901_manager_state_t reported_state;
    ltc3901_manager_event_t event;
    bool event_pending;
} ltc3901_manager_outputs_t;

typedef struct
{
    ltc3901_manager_state_t state;
    uint32_t entered_ms;
    uint32_t power_fault_count;
    uint32_t sync_fault_count;
    ltc3901_manager_outputs_t outputs;
} ltc3901_manager_t;

void ltc3901_manager_init(ltc3901_manager_t *manager, uint32_t now_ms);
void ltc3901_manager_task(ltc3901_manager_t *manager,
                          const ltc3901_manager_inputs_t *inputs,
                          const ltc3901_manager_config_t *config,
                          ltc3901_manager_outputs_t *outputs);
const char *ltc3901_manager_state_to_string(ltc3901_manager_state_t state);
const char *ltc3901_manager_event_to_string(ltc3901_manager_event_t event);
bool ltc3901_manager_state_is_fault(ltc3901_manager_state_t state);
bool ltc3901_manager_state_is_powered(ltc3901_manager_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* LTC3901_MANAGER_H_ */
