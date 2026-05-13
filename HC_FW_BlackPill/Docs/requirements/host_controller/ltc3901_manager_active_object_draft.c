/*
 * LTC3901 manager active-object draft
 *
 * Source: Docs/requirements/LTC3901_Monitor_STT.xlsx
 *
 * Purpose:
 *   Review-only draft of a cooperative, non-event-driven "active object"
 *   style manager for the LTC3901 state transition table.
 *
 * Build status:
 *   This file is intentionally placed under Docs/ and is not included in the
 *   firmware build. It avoids project headers so the approach can be reviewed
 *   before integration.
 *
 * Architecture intent:
 *   - One stateful object owns the LTC3901 manager state.
 *   - The main loop calls ltc3901_manager_task() periodically.
 *   - Command/event inputs are represented as sampled request flags, not an
 *     event queue.
 *   - Entry actions are centralized in enter_state().
 *   - Hardware effects are returned as output intents so integration can decide
 *     how to call BSP/power/sync/fault/reporting services.
 */

#include <stdbool.h>
#include <stdint.h>

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

static uint32_t elapsed_ms(uint32_t now_ms, uint32_t then_ms)
{
    return now_ms - then_ms;
}

static bool measurement_valid(int32_t value)
{
    return value >= 0;
}

static bool isupply_too_high(const ltc3901_manager_inputs_t *inputs,
                             const ltc3901_manager_config_t *config)
{
    return measurement_valid(inputs->isupply_ma) &&
           (inputs->isupply_ma >= config->isupply_ma_max);
}

static bool vupstream_low(const ltc3901_manager_inputs_t *inputs,
                          const ltc3901_manager_config_t *config)
{
    return !measurement_valid(inputs->vupstream_mv) ||
           (inputs->vupstream_mv < config->vupstream_mv_min);
}

static bool vcc_low(const ltc3901_manager_inputs_t *inputs,
                    const ltc3901_manager_config_t *config)
{
    return !measurement_valid(inputs->ltc3901_vcc_mv) ||
           (inputs->ltc3901_vcc_mv < config->ltc3901_vcc_mv_min);
}

static bool power_good(const ltc3901_manager_inputs_t *inputs,
                       const ltc3901_manager_config_t *config)
{
    return measurement_valid(inputs->vupstream_mv) &&
           measurement_valid(inputs->ltc3901_vcc_mv) &&
           (inputs->vupstream_mv > config->vupstream_mv_min) &&
           (inputs->ltc3901_vcc_mv > config->ltc3901_vcc_mv_min) &&
           !isupply_too_high(inputs, config);
}

static void emit(ltc3901_manager_t *manager,
                 ltc3901_manager_event_t event)
{
    manager->outputs.event = event;
    manager->outputs.event_pending = (event != LTC3901_MGR_EVT_NONE);
}

static void set_output_intents_for_state(ltc3901_manager_t *manager)
{
    switch (manager->state)
    {
    case LTC3901_MGR_STATE_POWER_UP:
    case LTC3901_MGR_STATE_POWERED:
    case LTC3901_MGR_STATE_POWERED_SYNC_OFF:
    case LTC3901_MGR_STATE_POWERED_SYNC_FAULT:
        manager->outputs.ltc3901_power_enable = true;
        manager->outputs.sdra_drv_enable = false;
        manager->outputs.sdrb_drv_enable = false;
        break;

    case LTC3901_MGR_STATE_POWERED_SYNC_ON:
        manager->outputs.ltc3901_power_enable = true;
        manager->outputs.sdra_drv_enable = true;
        manager->outputs.sdrb_drv_enable = true;
        break;

    case LTC3901_MGR_STATE_RESET:
    case LTC3901_MGR_STATE_HALT:
    case LTC3901_MGR_STATE_POWER_FAULT:
    default:
        manager->outputs.ltc3901_power_enable = false;
        manager->outputs.sdra_drv_enable = false;
        manager->outputs.sdrb_drv_enable = false;
        break;
    }
}

static void enter_state(ltc3901_manager_t *manager,
                        ltc3901_manager_state_t next_state,
                        uint32_t now_ms,
                        ltc3901_manager_event_t event)
{
    manager->state = next_state;
    manager->entered_ms = now_ms;
    manager->outputs.reported_state = next_state;
    manager->outputs.event = LTC3901_MGR_EVT_NONE;
    manager->outputs.event_pending = false;

    switch (next_state)
    {
    case LTC3901_MGR_STATE_RESET:
        manager->power_fault_count = 0U;
        manager->sync_fault_count = 0U;
        break;

    case LTC3901_MGR_STATE_POWER_FAULT:
        manager->power_fault_count++;
        break;

    case LTC3901_MGR_STATE_POWERED_SYNC_FAULT:
        manager->sync_fault_count++;
        break;

    default:
        break;
    }

    set_output_intents_for_state(manager);
    emit(manager, event);
}

void ltc3901_manager_init(ltc3901_manager_t *manager, uint32_t now_ms)
{
    if (manager == 0)
    {
        return;
    }

    manager->state = LTC3901_MGR_STATE_RESET;
    manager->entered_ms = now_ms;
    manager->power_fault_count = 0U;
    manager->sync_fault_count = 0U;
    manager->outputs.ltc3901_power_enable = false;
    manager->outputs.sdra_drv_enable = false;
    manager->outputs.sdrb_drv_enable = false;
    manager->outputs.reported_state = LTC3901_MGR_STATE_RESET;
    manager->outputs.event = LTC3901_MGR_EVT_NONE;
    manager->outputs.event_pending = false;
}

static void state_reset(ltc3901_manager_t *manager,
                        const ltc3901_manager_inputs_t *inputs)
{
    if (inputs->request == LTC3901_MGR_REQUEST_RUN)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_UP,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ENTERING_POWER_UP);
    }
}

static void state_power_up(ltc3901_manager_t *manager,
                           const ltc3901_manager_inputs_t *inputs,
                           const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);

    if (isupply_too_high(inputs, config))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ISUPPLY_TOO_HIGH);
        return;
    }

    if (in_state_ms >= config->power_up_timeout_ms)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_POWER_UP_TIMEOUT);
        return;
    }

    if (power_good(inputs, config))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_POWERED);
    }
}

static void state_power_fault(ltc3901_manager_t *manager,
                              const ltc3901_manager_inputs_t *inputs,
                              const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (inputs->request == LTC3901_MGR_REQUEST_RESET)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_RESET,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_NONE);
        return;
    }

    if (inputs->request == LTC3901_MGR_REQUEST_RUN)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_UP,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ENTERING_POWER_UP);
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);
    if ((in_state_ms > config->power_retry_delay_ms) &&
        (manager->power_fault_count < config->power_fault_max))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_UP,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_RETRYING_POWER_UP);
    }
}

static bool common_power_fault_transition(ltc3901_manager_t *manager,
                                          const ltc3901_manager_inputs_t *inputs,
                                          const ltc3901_manager_config_t *config)
{
    if (isupply_too_high(inputs, config))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ISUPPLY_TOO_HIGH);
        return true;
    }

    if (vupstream_low(inputs, config))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_VUPSTREAM_TOO_LOW);
        return true;
    }

    if (vcc_low(inputs, config))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_LTC3901_VCC_TOO_LOW);
        return true;
    }

    return false;
}

static void state_powered(ltc3901_manager_t *manager,
                          const ltc3901_manager_inputs_t *inputs,
                          const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (common_power_fault_transition(manager, inputs, config))
    {
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);
    if (in_state_ms > config->sync_on_delay_ms)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_ON,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_CYCLE_SYNC_ON);
    }
}

static void state_powered_sync_on(ltc3901_manager_t *manager,
                                  const ltc3901_manager_inputs_t *inputs,
                                  const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (common_power_fault_transition(manager, inputs, config))
    {
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);

    if ((in_state_ms > config->sync_stabilization_time_ms) && !inputs->me_freq_valid)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ME_STOPPED);
        return;
    }

    if ((in_state_ms > config->sync_stabilization_time_ms) && !inputs->mf_freq_valid)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_FAULT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_MF_STOPPED);
        return;
    }

    if (in_state_ms > config->sync_hold_on_time_ms)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_OFF,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_CYCLE_SYNC_OFF);
    }
}

static void state_powered_sync_off(ltc3901_manager_t *manager,
                                   const ltc3901_manager_inputs_t *inputs,
                                   const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (common_power_fault_transition(manager, inputs, config))
    {
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);
    if (in_state_ms > config->sync_hold_off_time_ms)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_ON,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_CYCLE_SYNC_ON);
    }
}

static void state_powered_sync_fault(ltc3901_manager_t *manager,
                                     const ltc3901_manager_inputs_t *inputs,
                                     const ltc3901_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (common_power_fault_transition(manager, inputs, config))
    {
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);
    if (in_state_ms > config->sync_fault_delay_ms)
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWERED_SYNC_ON,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_CYCLE_SYNC_ON);
    }
}

void ltc3901_manager_task(ltc3901_manager_t *manager,
                          const ltc3901_manager_inputs_t *inputs,
                          const ltc3901_manager_config_t *config,
                          ltc3901_manager_outputs_t *outputs)
{
    if ((manager == 0) || (inputs == 0) || (config == 0) || (outputs == 0))
    {
        return;
    }

    manager->outputs.event = LTC3901_MGR_EVT_NONE;
    manager->outputs.event_pending = false;
    set_output_intents_for_state(manager);

    if ((inputs->request == LTC3901_MGR_REQUEST_RUN) &&
        (manager->state != LTC3901_MGR_STATE_POWER_UP))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_POWER_UP,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_ENTERING_POWER_UP);
        *outputs = manager->outputs;
        return;
    }

    if ((inputs->request == LTC3901_MGR_REQUEST_HALT) &&
        (manager->state != LTC3901_MGR_STATE_HALT))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_HALT,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_NONE);
        *outputs = manager->outputs;
        return;
    }

    if ((inputs->request == LTC3901_MGR_REQUEST_RESET) &&
        (manager->state != LTC3901_MGR_STATE_RESET))
    {
        enter_state(manager,
                    LTC3901_MGR_STATE_RESET,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_NONE);
        *outputs = manager->outputs;
        return;
    }

    switch (manager->state)
    {
    case LTC3901_MGR_STATE_RESET:
        state_reset(manager, inputs);
        break;

    case LTC3901_MGR_STATE_HALT:
        break;

    case LTC3901_MGR_STATE_POWER_UP:
        state_power_up(manager, inputs, config);
        break;

    case LTC3901_MGR_STATE_POWER_FAULT:
        state_power_fault(manager, inputs, config);
        break;

    case LTC3901_MGR_STATE_POWERED:
        state_powered(manager, inputs, config);
        break;

    case LTC3901_MGR_STATE_POWERED_SYNC_ON:
        state_powered_sync_on(manager, inputs, config);
        break;

    case LTC3901_MGR_STATE_POWERED_SYNC_OFF:
        state_powered_sync_off(manager, inputs, config);
        break;

    case LTC3901_MGR_STATE_POWERED_SYNC_FAULT:
        state_powered_sync_fault(manager, inputs, config);
        break;

    default:
        enter_state(manager,
                    LTC3901_MGR_STATE_RESET,
                    inputs->now_ms,
                    LTC3901_MGR_EVT_NONE);
        break;
    }

    manager->outputs.reported_state = manager->state;
    *outputs = manager->outputs;
}

/*
 * Current production integration notes:
 *
 * 1. Production module pair:
 *      App/ltc3901_manager.c
 *      App/ltc3901_manager.h
 *
 * 2. fw_app.c owns one static ltc3901_manager_t instance and calls
 *    ltc3901_manager_init() from fw_app_init().
 *
 * 3. fw_app_run() refreshes ADC/PWM/status data, samples any pending
 *    ltc3901_cmd request, and calls:
 *      ltc3901_manager_task(&manager, &inputs, &config, &outputs)
 *
 * 4. fw_app.c maps outputs to existing hardware controls:
 *      outputs.ltc3901_power_enable -> bsp_power_enable/disable(BSP_POWER_LTC3901)
 *      outputs.sdra_drv_enable and outputs.sdrb_drv_enable -> sync_drv policy
 *
 * 5. SET args.ltc3901_cmd accepts RUN, HALT, and RESET.
 *    RUN enters POWER_UP, HALT enters HALT without clearing fault counters,
 *    and RESET enters RESET while clearing fault counters.
 *
 * 6. Add named configuration variables for all STT timing and threshold fields:
 *      Isupply_MAX, Vupstream_Min, LTC3901_VCC_Min, PWR_UP_TIMEOUT,
 *      Power_Retry_Delay, Power_Fault_MAX, Sync_ON_Delay,
 *      Sync_Hold_ON_Time, Sync_HOLD_OFF_Time, Sync_Stabilization_Time,
 *      Sync_Fault_Delay.
 *
 * 7. Decide how events become TE-visible:
 *      - debug-only trace,
 *      - EVT JSONL records,
 *      - fault-manager records,
 *      - or status-only reporting.
 *
 * 8. Decide how POWER_FAULT and POWERED_SYNC_FAULT map to existing HLF IDs.
 *    The draft currently emits events but does not assert project fault records.
 *
 * 9. Extend hc_app_status_refresh_from_bsp() to report the manager's DUT-local
 *    state rather than deriving LTC3901 state only from BSP power-enable state.
 *
 * 10. Add unit-style host tests for transition priority and timeout behavior
 *     before connecting the manager to live hardware outputs.
 */
