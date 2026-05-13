#ifndef LT8316_MANAGER_H_
#define LT8316_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LT8316_MGR_STATE_RESET = 0,
    LT8316_MGR_STATE_FAULT,
    LT8316_MGR_STATE_POWERED,
} lt8316_manager_state_t;

typedef enum
{
    LT8316_MGR_REQUEST_NONE = 0,
    LT8316_MGR_REQUEST_RUN,
    LT8316_MGR_REQUEST_RESET,
} lt8316_manager_request_t;

typedef enum
{
    LT8316_MGR_EVT_NONE = 0,
    LT8316_MGR_EVT_ENTERING_POWERED,
    LT8316_MGR_EVT_RETRYING_POWER_UP,
    LT8316_MGR_EVT_GATE_STOPPED,
} lt8316_manager_event_t;

typedef struct
{
    uint32_t power_retry_delay_ms;
    uint32_t power_fault_max;
    uint32_t power_on_stabilization_time_ms;
} lt8316_manager_config_t;

typedef struct
{
    uint32_t now_ms;
    lt8316_manager_request_t request;
    bool gate_freq_valid;
} lt8316_manager_inputs_t;

typedef struct
{
    bool hv_power_enable;
    lt8316_manager_state_t reported_state;
    lt8316_manager_event_t event;
    bool event_pending;
} lt8316_manager_outputs_t;

typedef struct
{
    lt8316_manager_state_t state;
    uint32_t entered_ms;
    uint32_t power_fault_count;
    lt8316_manager_outputs_t outputs;
} lt8316_manager_t;

void lt8316_manager_init(lt8316_manager_t *manager, uint32_t now_ms);
void lt8316_manager_task(lt8316_manager_t *manager,
                         const lt8316_manager_inputs_t *inputs,
                         const lt8316_manager_config_t *config,
                         lt8316_manager_outputs_t *outputs);
const char *lt8316_manager_state_to_string(lt8316_manager_state_t state);
const char *lt8316_manager_event_to_string(lt8316_manager_event_t event);
bool lt8316_manager_state_is_fault(lt8316_manager_state_t state);
bool lt8316_manager_state_is_powered(lt8316_manager_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* LT8316_MANAGER_H_ */
