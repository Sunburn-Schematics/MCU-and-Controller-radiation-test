#include "fw_app.h"

#include "bsp_board.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include "main.h"

#include <assert.h>

#define FW_APP_HEARTBEAT_PERIOD_MS    (250U)

/** Describes a simple board-owned input. */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} bsp_gpio_in_t;

/** Describes a simple board-owned input. */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    bool active_high;
} bsp_gpio_out_t;



/**
 * LED map.
 *
 * Blue LED on the Black Pill module is active-low.
 * Red and green target-board LEDs are treated as active-high.
 */
static const bsp_gpio_out_t s_led_map[] =
{
    [BSP_LED_BLUE]  = { LED_Blue_GPIO_Port,  LED_Blue_Pin,  false },
    [BSP_LED_RED]   = { LED_Red_GPIO_Port,   LED_Red_Pin,   true  },
    [BSP_LED_GREEN] = { LED_Green_GPIO_Port, LED_Green_Pin, true  },
};

/**
 * Power-domain map.
 *
 * LT8316_HV_Pwr_Enable is active-high.
 * nLTC3901_Pwr_Enable is active-low.
 */
static const bsp_gpio_out_t s_power_map[] =
{
    [BSP_POWER_LT8316]  = { LT8316_HV_Pwr_Enable_GPIO_Port,  LT8316_HV_Pwr_Enable_Pin,  true  },
    [BSP_POWER_LTC3901] = { nLTC3901_Pwr_Enable_GPIO_Port,   nLTC3901_Pwr_Enable_Pin,   false },
};

/**
 * Board ID input map.
 *
 * Array index equals returned ID bit position.
 */
static const bsp_gpio_in_t s_id_map[] =
{
    { ID0_GPIO_Port, ID0_Pin },
    { ID1_GPIO_Port, ID1_Pin },
    { ID2_GPIO_Port, ID2_Pin },
    { ID3_GPIO_Port, ID3_Pin },
    { ID4_GPIO_Port, ID4_Pin },
    { ID5_GPIO_Port, ID5_Pin },
};

/** Convert a normalized asserted/deasserted state into the physical pin level. */
static GPIO_PinState prv_bool_to_pin_state(bool asserted, bool active_high)
{
    if (asserted)
    {
        return active_high ? GPIO_PIN_SET : GPIO_PIN_RESET;
    }

    return active_high ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

/** Convert a physical pin level into a normalized asserted/deasserted state. */
static bool prv_pin_state_to_bool(GPIO_PinState state, bool active_high)
{
    if (active_high)
    {
        return (state == GPIO_PIN_SET);
    }

    return (state == GPIO_PIN_RESET);
}

/** Validate a public LED enum before indexing the lookup table. */
static bool prv_is_valid_led(bsp_led_t led)
{
    return ((unsigned)led < (sizeof(s_led_map) / sizeof(s_led_map[0])));
}

/** Validate a public power-domain enum before indexing the lookup table. */
static bool prv_is_valid_power_domain(bsp_power_domain_t domain)
{
    return ((unsigned)domain < (sizeof(s_power_map) / sizeof(s_power_map[0])));
}

void bsp_init(void)
{
    /* CubeMX/HAL performs hardware init. BSP applies board policy only. */
    bsp_enter_safe_state();
}

void bsp_enter_safe_state(void)
{
    /* Disable controllable DUT power first, then force indicators off. */
    bsp_power_disable(BSP_POWER_LT8316);
    bsp_power_disable(BSP_POWER_LTC3901);

    bsp_led_off(BSP_LED_BLUE);
    bsp_led_off(BSP_LED_RED);
    bsp_led_off(BSP_LED_GREEN);
}

uint8_t bsp_get_id_raw(void)
{
    uint8_t id_raw = 0U;
    uint32_t bit_index;

    for (bit_index = 0U; bit_index < (sizeof(s_id_map) / sizeof(s_id_map[0])); ++bit_index)
    {
        if (HAL_GPIO_ReadPin(s_id_map[bit_index].port, s_id_map[bit_index].pin) == GPIO_PIN_SET)
        {
            id_raw |= (uint8_t)(1U << bit_index);
        }
    }

    return id_raw;
}

bool bsp_is_beam_on(void)
{
    return (HAL_GPIO_ReadPin(BeamOn_GPIO_Port, BeamOn_Pin) == GPIO_PIN_SET);
}

void bsp_get_status(bsp_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    status->id_raw = bsp_get_id_raw();
    status->beam_on = bsp_is_beam_on();
}

void bsp_led_write(bsp_led_t led, bool on)
{
    if (!prv_is_valid_led(led))
    {
        return;
    }

    HAL_GPIO_WritePin(s_led_map[led].port,
                      s_led_map[led].pin,
                      prv_bool_to_pin_state(on, s_led_map[led].active_high));
}

void bsp_led_on(bsp_led_t led)
{
    bsp_led_write(led, true);
}

void bsp_led_off(bsp_led_t led)
{
    bsp_led_write(led, false);
}

void bsp_led_toggle(bsp_led_t led)
{
    if (!prv_is_valid_led(led))
    {
        return;
    }

    /* Raw toggle is acceptable because the GPIO output level is the source of truth. */
    HAL_GPIO_TogglePin(s_led_map[led].port, s_led_map[led].pin);
}

void bsp_power_write(bsp_power_domain_t domain, bool enable)
{
    if (!prv_is_valid_power_domain(domain))
    {
        return;
    }

    HAL_GPIO_WritePin(s_power_map[domain].port,
                      s_power_map[domain].pin,
                      prv_bool_to_pin_state(enable, s_power_map[domain].active_high));
}

void bsp_power_enable(bsp_power_domain_t domain)
{
    bsp_power_write(domain, true);
}

void bsp_power_disable(bsp_power_domain_t domain)
{
    bsp_power_write(domain, false);
}

bool bsp_power_is_enabled(bsp_power_domain_t domain)
{
    assert(prv_is_valid_power_domain(domain));

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(s_power_map[domain].port, s_power_map[domain].pin);
    return prv_pin_state_to_bool(pin_state, s_power_map[domain].active_high);
}
