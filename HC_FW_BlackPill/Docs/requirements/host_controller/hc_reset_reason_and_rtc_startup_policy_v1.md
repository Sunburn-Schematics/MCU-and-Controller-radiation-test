# HC Reset Reason and RTC Startup Policy v1

## Purpose
Define where reset-cause capture lives, how it is retained for later monitoring, and how RTC startup behavior depends on reset source.

## Implemented Direction
The implementation is split into two layers:

1. **Low-level reset capture** in `Drivers_Local/reset_reason_drv.*`
2. **RTC startup policy** in CubeMX-generated `Core/Src/rtc.c` user-code sections

## Reset Reason Driver

### Location
- `Drivers_Local/reset_reason_drv.h`
- `Drivers_Local/reset_reason_drv.c`

### Responsibilities
- capture MCU reset flags from `RCC->CSR`
- decode the hardware flags into HC software flags
- retain the captured result in RAM for later use
- clear the hardware reset flags after capture
- provide getters for later status and policy decisions

### Captured software flags
- `RESET_REASON_FLAG_POWER_ON`
- `RESET_REASON_FLAG_PIN`
- `RESET_REASON_FLAG_SOFTWARE`
- `RESET_REASON_FLAG_IWDG`
- `RESET_REASON_FLAG_WWDG`
- `RESET_REASON_FLAG_LOW_POWER`
- `RESET_REASON_FLAG_BROWNOUT`

### Current API
- `reset_reason_drv_init()`
- `reset_reason_drv_get_flags()`
- `reset_reason_drv_has_flag(...)`
- `reset_reason_drv_was_power_on_reset()`
- `reset_reason_drv_get_raw_csr()`

## Startup Order
Reset-reason capture must occur early in boot, before startup policy needs the decoded result.

Current intended order:
1. `HAL_Init()`
2. `reset_reason_drv_init()`
3. `SystemClock_Config()`
4. peripheral init calls including `MX_RTC_Init()`
5. `fw_app_init()`
6. main loop

This allows `MX_RTC_Init()` to branch on the retained reset reason without directly reading and clearing RCC reset flags itself.

## RTC Startup Policy

### Requirement
The HC shall only reinitialize default RTC date/time values after a **Power On Reset**.

### Current behavior
`MX_RTC_Init()` always performs the RTC peripheral/base HAL initialization, but the generated default `HAL_RTC_SetTime()` and `HAL_RTC_SetDate()` block is only allowed to run after a Power On Reset.

Current user-code policy in `Core/Src/rtc.c`:
- if reset source indicates **not** POR, return before the generated date/time overwrite block
- if reset source indicates POR, allow the default initialization block to run

## Rationale
This keeps:
- CubeMX ownership of RTC peripheral initialization
- project regeneration compatibility
- preservation of RTC date/time contents across non-POR resets

while still allowing:
- deterministic initialization of RTC contents after a true cold power-up

## Monitoring / Status Intent
The reset reason must remain available after boot for later HC monitoring and reporting.

Planned uses:
- ongoing HC status reporting
- fault/debug analysis
- watchdog reset diagnosis
- boot-history interpretation

## Recommended Next Protocol Exposure
When HC status payloads are expanded, expose at least:
- decoded reset-reason flags
- optionally raw `RCC->CSR` capture value for engineering/debug use

Suggested status fields:
- `reset_reason_flags`
- `reset_reason_raw`

## Caveats
- STM32 reset flags may be cumulative; more than one software flag may be set from a single observed boot state.
- Policy for selecting a single “primary” reset cause has **not** been defined yet.
- Current implementation uses flag retention, not a reduced single-cause enum.

## Summary
The adopted design is:
- capture and clear reset flags once in a dedicated low-level driver
- retain the decoded reason in RAM
- use the retained reason in RTC startup policy
- only allow default RTC date/time initialization after POR
- preserve reset reason for later system monitoring
