# Target pin connections

## Summary

This document captures the current STM32F401 Black Pill target pin assignments for the Host Controller board.

## Pin map

| Signal Name | Pin | Peripheral | Description |
|---|---|---|---|
| `LED_Blue` | `PC13` | GPIO | Output - Blue LED - On BlackPill Module |
| `LED_Red` | `PB14` | GPIO | Output - Red LED - On Target PCBA |
| `LED_Green` | `PB13` | GPIO | Output - Green LED - On Target PCBA |
| `BeamOn` | `PB12` | GPIO | Input - Sense when Ion Beam is Activated |
| `LT8316_HV_Pwr_Enable` | `PA8` | GPIO | Output - Assert High to enable HV Power to LT8316 |
| `nLTC3901_Pwr_Enable` | `PB15` | GPIO | Output - Assert Low to enable Power to LTC3901 |
| `ID[5..0]` | `PA6`, `PA7`, `PB0`, `PB1`, `PB2`, `PB10` | GPIO | Input - 6-bit Address Identifier |
| `USB_OTG_FS` | `PA12`, `PA11` | USB | USB CDC (Virtual Serial Port) |
| `DEBUG_RX` | `PA10` | USART | Input - Serial Debug |
| `DEBUG_TX` | `PA9` | USART | Output - Serial Debug |
| `SDRA` | `PB4` | `TIM3_CH1` | Sync Output Generator - Combine with `SDRB` to generate a 1 kHz quadrature signal |
| `SDRB` | `PB5` | `TIM3_CH2` | Companion quadrature output for `SDRA` |
| `LTC3901_ME_Tmr` | `PB6` | `TIM4_CH1` input with paired `TIM4_CH2` capture | Timer input capture - measure frequency and pulse width from one external input using the timer's internal paired channel path |
| `LTC3901_MF_Tmr` | `PA15` | `TIM2_CH1` input with paired `TIM2_CH2` capture | Timer input capture - measure frequency and pulse width from one external input using the timer's internal paired channel path |
| `LT8316_Gate_Tmr` | `PB8` | `TIM4_CH3` | Timer input capture - measure frequency from rising-edge timestamps; duty-cycle capture deferred in the current implementation |
| `VUpstream_Anlg` | `PA0` | `ADC1_IN0` | Sense LTC3901 voltage on the high side of the current sense resistor |
| `LTC3901_Vcc_Anlg` | `PA1` | `ADC1_IN1` | Sense LTC3901 voltage on the low side of the current sense resistor |
| `LT8316_Vout_Anlg` | `PA2` | `ADC1_IN2` | Sense LT8316 regulated output voltage |
| `LTC3901_ME_Anlg` | `PA3` | `ADC1_IN3` | LPF analog sense of `LTC3901_ME` signal |
| `LTC3901_MF_Anlg` | `PA4` | `ADC1_IN4` | LPF analog sense of `LTC3901_MF` signal |
| `LT8316_Gate_Anlg` | `PA5` | `ADC1_IN5` | LPF analog sense of `LT8316_Gate` signal |
| `VTemp` | Internal | `ADC1_IN16` | Internal temperature sensor |
| `VRefInt` | Internal | `ADC1_IN17` | Internal reference voltage |

## Notes

- `LT8316_Gate_Anlg` is assigned to `PA5`, which correctly matches `ADC1_IN5` on STM32F401.
- Timer naming in this document uses the STM32 `TIMx_CHy` convention for clarity, even where shorthand such as `T3_Ch1` or `T4_Ch4` was originally used.
- For `LTC3901_ME_Tmr` and `LTC3901_MF_Tmr`, the timer's second channel is used as an internal paired capture path. It does not require a second external pad carrying a duplicated signal.
