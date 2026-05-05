#ifndef PWM_CAPTURE_DRV_H
#define PWM_CAPTURE_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PWM_CAPTURE_SIGNAL_LTC3901_ME = 0,
    PWM_CAPTURE_SIGNAL_LTC3901_MF,
    PWM_CAPTURE_SIGNAL_LT8316_GATE,
    PWM_CAPTURE_SIGNAL_COUNT
} pwm_capture_signal_t;

typedef struct
{
    uint32_t frequency_hz;
    uint32_t period_ticks_avg;
    uint32_t high_ticks_avg;
    uint16_t duty_pct_x100;
    uint8_t valid_samples;
    bool has_duty_cycle;
    bool data_valid;
} pwm_capture_result_t;

void pwm_capture_drv_init(void);
void pwm_capture_drv_task(void);
bool pwm_capture_drv_is_initialized(void);
bool pwm_capture_drv_is_busy(void);
bool pwm_capture_drv_is_complete(void);
bool pwm_capture_drv_start_burst(void);
void pwm_capture_drv_abort(void);
bool pwm_capture_drv_get_result(pwm_capture_signal_t signal, pwm_capture_result_t *result_out);

#ifdef __cplusplus
}
#endif

#endif /* PWM_CAPTURE_DRV_H */
