#include "lt8316_manager.h"

static const char *const s_state_strings[] = {
    "RESET",
    "FAULT",
    "POWERED",
};

static const char *const s_event_strings[] = {
    "",
    "Entering POWERED",
    "Retrying Power Up",
    "GATE Stopped",
};

static uint32_t elapsed_ms(uint32_t now_ms, uint32_t then_ms)
{
    return now_ms - then_ms;
}

static bool power_retry_allowed(const lt8316_manager_t *manager,
                                const lt8316_manager_config_t *config)
{
    return (config->power_fault_max == 0U) ||
           (manager->power_fault_count < config->power_fault_max);
}

static void emit(lt8316_manager_t *manager,
                 lt8316_manager_event_t event)
{
    manager->outputs.event = event;
    manager->outputs.event_pending = (event != LT8316_MGR_EVT_NONE);
}

static void set_output_intents_for_state(lt8316_manager_t *manager)
{
    switch (manager->state)
    {
    case LT8316_MGR_STATE_POWERED:
        manager->outputs.hv_power_enable = true;
        break;

    case LT8316_MGR_STATE_RESET:
    case LT8316_MGR_STATE_FAULT:
    default:
        manager->outputs.hv_power_enable = false;
        break;
    }
}

static void enter_state(lt8316_manager_t *manager,
                        lt8316_manager_state_t next_state,
                        uint32_t now_ms,
                        lt8316_manager_event_t event)
{
    manager->state = next_state;
    manager->entered_ms = now_ms;
    manager->outputs.reported_state = next_state;
    manager->outputs.event = LT8316_MGR_EVT_NONE;
    manager->outputs.event_pending = false;

    switch (next_state)
    {
    case LT8316_MGR_STATE_RESET:
        manager->power_fault_count = 0U;
        break;

    case LT8316_MGR_STATE_FAULT:
        manager->power_fault_count++;
        break;

    default:
        break;
    }

    set_output_intents_for_state(manager);
    emit(manager, event);
}

void lt8316_manager_init(lt8316_manager_t *manager, uint32_t now_ms)
{
    if (manager == 0)
    {
        return;
    }

    manager->state = LT8316_MGR_STATE_RESET;
    manager->entered_ms = now_ms;
    manager->power_fault_count = 0U;
    manager->outputs.hv_power_enable = false;
    manager->outputs.reported_state = LT8316_MGR_STATE_RESET;
    manager->outputs.event = LT8316_MGR_EVT_NONE;
    manager->outputs.event_pending = false;
}

static void state_reset(lt8316_manager_t *manager,
                        const lt8316_manager_inputs_t *inputs)
{
    if (inputs->request == LT8316_MGR_REQUEST_RUN)
    {
        enter_state(manager,
                    LT8316_MGR_STATE_POWERED,
                    inputs->now_ms,
                    LT8316_MGR_EVT_ENTERING_POWERED);
    }
}

static void state_fault(lt8316_manager_t *manager,
                        const lt8316_manager_inputs_t *inputs,
                        const lt8316_manager_config_t *config)
{
    uint32_t in_state_ms;

    if (inputs->request == LT8316_MGR_REQUEST_RUN)
    {
        enter_state(manager,
                    LT8316_MGR_STATE_POWERED,
                    inputs->now_ms,
                    LT8316_MGR_EVT_ENTERING_POWERED);
        return;
    }

    in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);
    if ((in_state_ms > config->power_retry_delay_ms) &&
        power_retry_allowed(manager, config))
    {
        enter_state(manager,
                    LT8316_MGR_STATE_POWERED,
                    inputs->now_ms,
                    LT8316_MGR_EVT_RETRYING_POWER_UP);
    }
}

static void state_powered(lt8316_manager_t *manager,
                          const lt8316_manager_inputs_t *inputs,
                          const lt8316_manager_config_t *config)
{
    uint32_t in_state_ms = elapsed_ms(inputs->now_ms, manager->entered_ms);

    if ((in_state_ms > config->power_on_stabilization_time_ms) &&
        !inputs->gate_freq_valid)
    {
        enter_state(manager,
                    LT8316_MGR_STATE_FAULT,
                    inputs->now_ms,
                    LT8316_MGR_EVT_GATE_STOPPED);
    }
}

void lt8316_manager_task(lt8316_manager_t *manager,
                         const lt8316_manager_inputs_t *inputs,
                         const lt8316_manager_config_t *config,
                         lt8316_manager_outputs_t *outputs)
{
    if ((manager == 0) || (inputs == 0) || (config == 0) || (outputs == 0))
    {
        return;
    }

    manager->outputs.event = LT8316_MGR_EVT_NONE;
    manager->outputs.event_pending = false;
    set_output_intents_for_state(manager);

    if ((inputs->request == LT8316_MGR_REQUEST_RUN) &&
        (manager->state != LT8316_MGR_STATE_POWERED))
    {
        enter_state(manager,
                    LT8316_MGR_STATE_POWERED,
                    inputs->now_ms,
                    LT8316_MGR_EVT_ENTERING_POWERED);
        *outputs = manager->outputs;
        return;
    }

    if ((inputs->request == LT8316_MGR_REQUEST_RESET) &&
        (manager->state != LT8316_MGR_STATE_RESET))
    {
        enter_state(manager,
                    LT8316_MGR_STATE_RESET,
                    inputs->now_ms,
                    LT8316_MGR_EVT_NONE);
        *outputs = manager->outputs;
        return;
    }

    switch (manager->state)
    {
    case LT8316_MGR_STATE_RESET:
        state_reset(manager, inputs);
        break;

    case LT8316_MGR_STATE_FAULT:
        state_fault(manager, inputs, config);
        break;

    case LT8316_MGR_STATE_POWERED:
        state_powered(manager, inputs, config);
        break;

    default:
        enter_state(manager,
                    LT8316_MGR_STATE_RESET,
                    inputs->now_ms,
                    LT8316_MGR_EVT_NONE);
        break;
    }

    manager->outputs.reported_state = manager->state;
    *outputs = manager->outputs;
}

const char *lt8316_manager_state_to_string(lt8316_manager_state_t state)
{
    if (((unsigned int)state) >= (sizeof(s_state_strings) / sizeof(s_state_strings[0])))
    {
        return "RESET";
    }

    return s_state_strings[state];
}

const char *lt8316_manager_event_to_string(lt8316_manager_event_t event)
{
    if (((unsigned int)event) >= (sizeof(s_event_strings) / sizeof(s_event_strings[0])))
    {
        return "";
    }

    return s_event_strings[event];
}

bool lt8316_manager_state_is_fault(lt8316_manager_state_t state)
{
    return (state == LT8316_MGR_STATE_FAULT);
}

bool lt8316_manager_state_is_powered(lt8316_manager_state_t state)
{
    return (state == LT8316_MGR_STATE_POWERED);
}
