#include "ltc3901_manager.h"

static const char *const s_state_strings[] = {
    "RESET",
    "HALT",
    "POWER_UP",
    "POWER_FAULT",
    "POWERED",
    "POWERED_SYNC_ON",
    "POWERED_SYNC_OFF",
    "POWERED_SYNC_FAULT",
};

static const char *const s_event_strings[] = {
    "",
    "Entering POWER_UP",
    "Isupply Current too high",
    "Power Up Timeout",
    "Powered",
    "Retrying Power Up",
    "VUpstream Too Low",
    "LTC3901 VCC Too Low",
    "Cycle Sync ON",
    "Cycle Sync OFF",
    "ME Stopped",
    "MF Stopped",
};

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

static bool power_retry_allowed(const ltc3901_manager_t *manager,
                                const ltc3901_manager_config_t *config)
{
    return (config->power_fault_max == 0U) ||
           (manager->power_fault_count < config->power_fault_max);
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
    uint32_t in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);

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
        power_retry_allowed(manager, config))
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

const char *ltc3901_manager_state_to_string(ltc3901_manager_state_t state)
{
    if (((unsigned int)state) >= (sizeof(s_state_strings) / sizeof(s_state_strings[0])))
    {
        return "RESET";
    }

    return s_state_strings[state];
}

const char *ltc3901_manager_event_to_string(ltc3901_manager_event_t event)
{
    if (((unsigned int)event) >= (sizeof(s_event_strings) / sizeof(s_event_strings[0])))
    {
        return "";
    }

    return s_event_strings[event];
}

bool ltc3901_manager_state_is_fault(ltc3901_manager_state_t state)
{
    return (state == LTC3901_MGR_STATE_POWER_FAULT) ||
           (state == LTC3901_MGR_STATE_POWERED_SYNC_FAULT);
}

bool ltc3901_manager_state_is_powered(ltc3901_manager_state_t state)
{
    return (state == LTC3901_MGR_STATE_POWER_UP) ||
           (state == LTC3901_MGR_STATE_POWERED) ||
           (state == LTC3901_MGR_STATE_POWERED_SYNC_ON) ||
           (state == LTC3901_MGR_STATE_POWERED_SYNC_OFF) ||
           (state == LTC3901_MGR_STATE_POWERED_SYNC_FAULT);
}
