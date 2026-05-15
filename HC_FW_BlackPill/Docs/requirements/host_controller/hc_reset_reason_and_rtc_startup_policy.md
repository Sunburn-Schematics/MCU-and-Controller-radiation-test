# HC Reset Reason and RTC Startup Policy v1

## Purpose

Define the implemented reset-cause capture and RTC startup policy.

## Implementation

Reset reason capture is implemented in:

- `Drivers_Local/reset_reason_drv.h`
- `Drivers_Local/reset_reason_drv.c`

RTC startup policy is implemented in CubeMX-generated RTC initialization user
code:

- `Core/Src/rtc.c`

## Reset Reason Driver Responsibilities

The reset reason driver:

- captures MCU reset flags from `RCC->CSR`
- decodes hardware flags into HC software flags
- stores the decoded value in RAM
- stores the raw CSR capture in RAM
- clears hardware reset flags after capture
- exposes getters for application and policy code

Captured software flags:

- `RESET_REASON_FLAG_POWER_ON`
- `RESET_REASON_FLAG_PIN`
- `RESET_REASON_FLAG_SOFTWARE`
- `RESET_REASON_FLAG_IWDG`
- `RESET_REASON_FLAG_WWDG`
- `RESET_REASON_FLAG_LOW_POWER`
- `RESET_REASON_FLAG_BROWNOUT`

Current API:

- `reset_reason_drv_init()`
- `reset_reason_drv_get_flags()`
- `reset_reason_drv_has_flag(...)`
- `reset_reason_drv_was_power_on_reset()`
- `reset_reason_drv_get_raw_csr()`

## Startup Order

Reset reason capture occurs before RTC startup policy needs the decoded result.

Implemented startup order:

1. `HAL_Init()`
2. `reset_reason_drv_init()`
3. `SystemClock_Config()`
4. peripheral initialization, including `MX_RTC_Init()`
5. `fw_app_init()`
6. main loop

## RTC Startup Policy

`MX_RTC_Init()` always performs RTC peripheral/base HAL initialization.

The generated default `HAL_RTC_SetTime()` and `HAL_RTC_SetDate()` block is only
allowed to run after a power-on reset. On non-power-on resets, RTC date/time
contents are preserved by returning before the default overwrite block.

Runtime date/time is exposed through:

- `SET args.date_time`
- `GET args:["date_time"]`

## Caveats

STM32 reset flags may be cumulative, so more than one decoded software flag may
be set from a single observed boot state. The current implementation retains
flags rather than reducing them to a single primary reset cause.
