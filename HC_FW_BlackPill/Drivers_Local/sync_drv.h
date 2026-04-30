#ifndef SYNC_DRV_H
#define SYNC_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @file sync_drv.h
 * @brief Synchronized SDRA/SDRB square-wave driver.
 *
 * This driver owns the coordinated generation of the SDRA and SDRB outputs - i.e. the SYNC output.
 * The public contract is intentionally simple:
 * - SDRA is a 50% duty-cycle square wave at the requested base frequency.
 * - SDRB is the same 50% duty-cycle square wave, delayed relative to SDRA.
 *
 * The driver hides all STM32 TIM details from upper layers.
 * Invalid configurations or unrealizable timing requests are rejected.
 */

#if 0
 typedef struct
{
    /** Base square-wave frequency shared by SDRA and SDRB. Must be > 0. */
    uint32_t frequency_hz;

    /** Delay of SDRB rising edge relative to SDRA rising edge, in ns. */
    uint32_t sdrb_delay_ns;
} sync_drv_config_t;    
#endif

typedef struct
{
    uint32_t ARR;
    uint32_t CCR2;
} sync_drv_raw_config_t;

/** Initialize driver state and force the outputs disabled. */
void sync_drv_init(void);

/** Disable the synchronized outputs. Safe to call multiple times. */
void sync_drv_disable(void);

/** Enable the synchronized outputs using the most recently applied configuration. */
bool sync_drv_enable(void);

/** Validate and apply a synchronized output configuration. */
bool sync_drv_configure(const sync_drv_raw_config_t *raw_cfg);

/** Convenience helper that configures and then enables the outputs. */
bool sync_drv_configure_and_enable(const sync_drv_raw_config_t *raw_cfg);

/** Return true when the synchronized outputs are currently enabled. */
bool sync_drv_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* SYNC_DRV_H */
