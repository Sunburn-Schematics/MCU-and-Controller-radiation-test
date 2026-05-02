#ifndef RESET_REASON_DRV_H
#define RESET_REASON_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t reset_reason_flags_t;

enum
{
    RESET_REASON_FLAG_NONE       = 0U,
    RESET_REASON_FLAG_POWER_ON   = (1U << 0),
    RESET_REASON_FLAG_PIN        = (1U << 1),
    RESET_REASON_FLAG_SOFTWARE   = (1U << 2),
    RESET_REASON_FLAG_IWDG       = (1U << 3),
    RESET_REASON_FLAG_WWDG       = (1U << 4),
    RESET_REASON_FLAG_LOW_POWER  = (1U << 5),
    RESET_REASON_FLAG_BROWNOUT   = (1U << 6)
};

void reset_reason_drv_init(void);
reset_reason_flags_t reset_reason_drv_get_flags(void);
bool reset_reason_drv_has_flag(reset_reason_flags_t flag);
bool reset_reason_drv_was_power_on_reset(void);
uint32_t reset_reason_drv_get_raw_csr(void);

#ifdef __cplusplus
}
#endif

#endif /* RESET_REASON_DRV_H */
