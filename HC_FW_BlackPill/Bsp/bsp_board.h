/**
 * @file bsp_board.h
 * @brief Consolidated board-support API for the STM32F401 Black Pill host controller.
 *
 * CubeMX/HAL owns peripheral configuration and generated pin setup.
 * This BSP layer owns only simple board-level controls and reads:
 * - board LEDs
 * - DUT power enables
 * - BeamOn input
 * - 6-bit board ID input
 * - deterministic safe-state application
 *
 * This module deliberately does not own ADC acquisition, timer-capture logic,
 * waveform generation, protocol handling, or fault policy.
 */
#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/** Board-owned LED identifiers. */
typedef enum
{
    BSP_LED_BLUE = 0,
    BSP_LED_RED,
    BSP_LED_GREEN,
} bsp_led_t;

/**
 * Board power-enable domains.
 *
 * Polarity is normalized internally:
 * - BSP_POWER_LT8316: enable == asserted high on LT8316_HV_Pwr_Enable
 * - BSP_POWER_LTC3901: enable == asserted low on nLTC3901_Pwr_Enable
 */
typedef enum
{
    BSP_POWER_LT8316 = 0,
    BSP_POWER_LTC3901,
} bsp_power_domain_t;

/** Snapshot of simple board-readable state. */
typedef struct
{
    uint8_t id_raw;
    bool beam_on;
} bsp_status_t;

/**
 * Initialize the BSP policy layer after CubeMX peripheral initialization.
 *
 * This function currently applies the board safe state immediately.
 */
void bsp_init(void);

/**
 * Apply deterministic safe outputs for the board.
 *
 * Current safe state:
 * - LT8316 disabled
 * - LTC3901 disabled
 * - Blue LED off
 * - Red LED off
 * - Green LED off
 */
void bsp_enter_safe_state(void);

/**
 * Read the raw 6-bit board ID.
 *
 * Bit packing is defined as:
 * - bit0 = ID0 (PB10)
 * - bit1 = ID1 (PB2)
 * - bit2 = ID2 (PB1)
 * - bit3 = ID3 (PB0)
 * - bit4 = ID4 (PA7)
 * - bit5 = ID5 (PA6)
 */
uint8_t bsp_get_id_raw(void);

/** Read the normalized BeamOn input state. */
bool bsp_is_beam_on(void);

/** Fill a caller-provided status snapshot structure. */
void bsp_get_status(bsp_status_t *status);

/** Write a normalized on/off state to a board-owned LED. */
void bsp_led_write(bsp_led_t led, bool on);

/** Turn on a board-owned LED. */
void bsp_led_on(bsp_led_t led);

/** Turn off a board-owned LED. */
void bsp_led_off(bsp_led_t led);

/** Toggle a board-owned LED. */
void bsp_led_toggle(bsp_led_t led);

/** Write a normalized enable state to a board power domain. */
void bsp_power_write(bsp_power_domain_t domain, bool enable);

/** Enable a board power domain using normalized polarity handling. */
void bsp_power_enable(bsp_power_domain_t domain);

/** Disable a board power domain using normalized polarity handling. */
void bsp_power_disable(bsp_power_domain_t domain);

/** Read back a normalized enabled/disabled state for a board power domain. */
bool bsp_power_is_enabled(bsp_power_domain_t domain);

#ifdef __cplusplus
}
#endif

#endif /* BSP_BOARD_H */
