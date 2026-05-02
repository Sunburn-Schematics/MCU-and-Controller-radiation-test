#include "reset_reason_drv.h"

#include "stm32f4xx_hal.h"

static reset_reason_flags_t s_reset_reason_flags;
static uint32_t s_reset_reason_raw_csr;
static bool s_reset_reason_initialized;

static reset_reason_flags_t reset_reason_drv_decode_flags(uint32_t csr)
{
    reset_reason_flags_t flags = RESET_REASON_FLAG_NONE;

    if ((csr & RCC_CSR_PORRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_POWER_ON;
    }

    if ((csr & RCC_CSR_PINRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_PIN;
    }

    if ((csr & RCC_CSR_SFTRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_SOFTWARE;
    }

    if ((csr & RCC_CSR_IWDGRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_IWDG;
    }

    if ((csr & RCC_CSR_WWDGRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_WWDG;
    }

    if ((csr & RCC_CSR_LPWRRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_LOW_POWER;
    }

#if defined(RCC_CSR_BORRSTF)
    if ((csr & RCC_CSR_BORRSTF) != 0U)
    {
        flags |= RESET_REASON_FLAG_BROWNOUT;
    }
#endif

    return flags;
}

void reset_reason_drv_init(void)
{
    s_reset_reason_raw_csr = RCC->CSR;
    s_reset_reason_flags = reset_reason_drv_decode_flags(s_reset_reason_raw_csr);
    s_reset_reason_initialized = true;

    __HAL_RCC_CLEAR_RESET_FLAGS();
}

reset_reason_flags_t reset_reason_drv_get_flags(void)
{
    return s_reset_reason_flags;
}

bool reset_reason_drv_has_flag(reset_reason_flags_t flag)
{
    return ((s_reset_reason_flags & flag) != 0U);
}

bool reset_reason_drv_was_power_on_reset(void)
{
    if (!s_reset_reason_initialized)
    {
        return false;
    }

    return reset_reason_drv_has_flag(RESET_REASON_FLAG_POWER_ON);
}

uint32_t reset_reason_drv_get_raw_csr(void)
{
    return s_reset_reason_raw_csr;
}
