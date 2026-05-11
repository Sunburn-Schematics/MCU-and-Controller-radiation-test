# Debug Signal Dictionary

This document defines the currently supported `dbg_signals` names for:

- periodic `DBG` streaming via `SET {"dbg_period_ms":...,"dbg_signals":[...]}`
- one-shot sampled reads via `GET {"dbg_signals":[...]}`
- digital signal control via `SET {"dbg_signals":{"signal_name":boolean_value}}`

## Behavior

- Signal names are case-sensitive.
- Unknown signal names are rejected by the command parser.
- Requested signals are returned in the same order they were requested after duplicate removal.
- Unavailable values are emitted as `null`.
- ADC `.eng` values depend on per-channel `adc_cal` configuration.
- Only settable digital signals can be controlled via `SET {"dbg_signals":{...}}`; attempting to set read-only signals will result in an error.

## Types

- `int`: signed integer JSON number
- `bool`: JSON `true` or `false`
- `string`: JSON string

## ADC Signals

### ADC Channel 0: `VUpstream_Anlg`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.vupstream.raw` | `int` | counts | raw ADC sample for channel `0` |
| `adc.vupstream.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.vupstream.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":100,"args":{"dbg_signals":["adc.vupstream.raw","adc.vupstream.mv","adc.vupstream.eng"]}}
```

```json
{"type":"SET","msg":101,"args":{"dbg_period_ms":100,"dbg_signals":["adc.vupstream.raw","adc.vupstream.mv","adc.vupstream.eng"]}}
```

### ADC Channel 1: `LTC3901_Vcc_Anlg`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.ltc3901_vcc.raw` | `int` | counts | raw ADC sample for channel `1` |
| `adc.ltc3901_vcc.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.ltc3901_vcc.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":110,"args":{"dbg_signals":["adc.ltc3901_vcc.raw","adc.ltc3901_vcc.mv","adc.ltc3901_vcc.eng"]}}
```

```json
{"type":"SET","msg":111,"args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_vcc.raw","adc.ltc3901_vcc.mv","adc.ltc3901_vcc.eng"]}}
```

### ADC Channel 2: `LT8316_Vout_Anlg`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.lt8316_vout.raw` | `int` | counts | raw ADC sample for channel `2` |
| `adc.lt8316_vout.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.lt8316_vout.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":120,"args":{"dbg_signals":["adc.lt8316_vout.raw","adc.lt8316_vout.mv","adc.lt8316_vout.eng"]}}
```

```json
{"type":"SET","msg":121,"args":{"dbg_period_ms":100,"dbg_signals":["adc.lt8316_vout.raw","adc.lt8316_vout.mv","adc.lt8316_vout.eng"]}}
```

### ADC Channel 3: `LTC3901_ME_Anlg`

| Signal               | Type  | Units        | Source                              |
| -------------------- | ----- | ------------ | ----------------------------------- |
| `adc.ltc3901_me.raw` | `int` | counts       | raw ADC sample for channel `3`      |
| `adc.ltc3901_me.mv`  | `int` | mV           | nominal pin-level millivolts        |
| `adc.ltc3901_me.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":130,"args":{"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","adc.ltc3901_me.eng"]}}
```

```json
{"type":"SET","msg":131,"args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","adc.ltc3901_me.eng"]}}
```

### ADC Channel 4: `LTC3901_MF_Anlg`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.ltc3901_mf.raw` | `int` | counts | raw ADC sample for channel `4` |
| `adc.ltc3901_mf.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.ltc3901_mf.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":140,"args":{"dbg_signals":["adc.ltc3901_mf.raw","adc.ltc3901_mf.mv","adc.ltc3901_mf.eng"]}}
```

```json
{"type":"SET","msg":141,"args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_mf.raw","adc.ltc3901_mf.mv","adc.ltc3901_mf.eng"]}}
```

### ADC Channel 5: `LT8316_Gate_Anlg`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.lt8316_gate.raw` | `int` | counts | raw ADC sample for channel `5` |
| `adc.lt8316_gate.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.lt8316_gate.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":150,"args":{"dbg_signals":["adc.lt8316_gate.raw","adc.lt8316_gate.mv","adc.lt8316_gate.eng"]}}
```

```json
{"type":"SET","msg":151,"args":{"dbg_period_ms":100,"dbg_signals":["adc.lt8316_gate.raw","adc.lt8316_gate.mv","adc.lt8316_gate.eng"]}}
```

### ADC Channel 6: `VTemp`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.temp.raw` | `int` | counts | raw ADC sample for channel `6` |
| `adc.temp.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.temp.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":160,"args":{"dbg_signals":["adc.temp.raw","adc.temp.mv","adc.temp.eng"]}}
```

```json
{"type":"SET","msg":161,"args":{"dbg_period_ms":100,"dbg_signals":["adc.temp.raw","adc.temp.mv","adc.temp.eng"]}}
```

### ADC Channel 7: `VRefInt`

| Signal | Type | Units | Source |
| --- | --- | --- | --- |
| `adc.vrefint.raw` | `int` | counts | raw ADC sample for channel `7` |
| `adc.vrefint.mv` | `int` | mV | nominal pin-level millivolts |
| `adc.vrefint.eng` | `int` | user-defined | `y = mx + c` engineering conversion |

JSON Example:

```json
{"type":"GET","msg":170,"args":{"dbg_signals":["adc.vrefint.raw","adc.vrefint.mv","adc.vrefint.eng"]}}
```

```json
{"type":"SET","msg":171,"args":{"dbg_period_ms":100,"dbg_signals":["adc.vrefint.raw","adc.vrefint.mv","adc.vrefint.eng"]}}
```

## PWM Signals

| Signal             | Type  | Units | Source                      |
| ------------------ | ----- | ----- | --------------------------- |
| `pwm.me.freq_hz`   | `int` | Hz    | `LTC3901_ME_Tmr` frequency  |
| `pwm.me.duty_pct`  | `int` | %     | `LTC3901_ME_Tmr` duty cycle |
| `pwm.mf.freq_hz`   | `int` | Hz    | `LTC3901_MF_Tmr` frequency  |
| `pwm.mf.duty_pct`  | `int` | %     | `LTC3901_MF_Tmr` duty cycle |
| `pwm.gate.freq_hz` | `int` | Hz    | `LT8316_Gate_Tmr` frequency |

### PWM Gate Capture Diagnostics

| Signal                  | Type   | Units | Source                                                                     |
| ----------------------- | ------ | ----- | -------------------------------------------------------------------------- |
| `pwm.gate.valid`        | `bool` | n/a   | Last `LT8316_Gate_Tmr` result validity                                     |
| `pwm.gate.armed`        | `bool` | n/a   | Current `LT8316_Gate_Tmr` capture armed state                              |
| `pwm.gate.start_ok`     | `bool` | n/a   | Last TIM4_CH3 DMA start result                                             |
| `pwm.gate.start_status` | `int`  | n/a   | Last HAL status from TIM4_CH3 DMA start; `0` is `HAL_OK`                   |
| `pwm.gate.process_ok`   | `bool` | n/a   | Last sample processing result                                              |
| `pwm.gate.rise_count`   | `int`  | count | Rising-edge samples captured in the last completed gate burst              |
| `pwm.gate.done_mask`    | `int`  | mask  | Last completed gate burst done mask; `1` means rising DMA completed        |
| `pwm.gate.tick_hz`      | `int`  | Hz    | Capture timer tick used for the last gate calculation                      |
| `pwm.gate.dma_error`    | `bool` | n/a   | Last gate burst DMA error state                                            |
| `pwm.gate.dma_done_cnt` | `int`  | count | Number of gate rising DMA-complete callbacks since capture start           |
| `pwm.gate.timeout_cnt`  | `int`  | count | Number of gate bursts finalized by timeout rather than full DMA completion |

JSON Example:

```json
{"type":"GET","msg":200,"args":{"dbg_signals":["pwm.me.freq_hz","pwm.me.duty_pct","pwm.mf.freq_hz","pwm.mf.duty_pct","pwm.gate.freq_hz","pwm.gate.valid","pwm.gate.rise_count","pwm.gate.process_ok"]}}
```

```json
{"type":"SET","msg":201,"args":{"dbg_period_ms":2000,"dbg_signals":["pwm.me.freq_hz","pwm.me.duty_pct","pwm.mf.freq_hz","pwm.mf.duty_pct","pwm.gate.freq_hz","pwm.gate.valid","pwm.gate.rise_count","pwm.gate.process_ok"]}}
```

Notes:

- `pwm.gate.freq_hz` is frequency-only at present.
- Any unavailable PWM result is emitted as `null`.

## Digital / State Signals

| Signal           | Type     | Units | Settable | Source                     |
| ---------------- | -------- | ----- | -------- | -------------------------- |
| `beam_on`        | `bool`   | n/a   | No       | board BeamOn digital input |
| `ltc3901.pwr_en` | `bool`   | n/a   | Yes      | LTC3901 power-enable state |
| `lt8316.pwr_en`  | `bool`   | n/a   | Yes      | LT8316 power-enable state  |
| `led.blue`       | `bool`   | n/a   | Yes      | Blue LED state             |
| `led.red`        | `bool`   | n/a   | Yes      | Red LED state              |
| `led.green`      | `bool`   | n/a   | Yes      | Green LED state            |
| `sync.enable`    | `bool`   | n/a   | Yes      | SYNC output enable state   |
| `hc.state`       | `string` | n/a   | No       | top-level HC state         |
| `ltc3901.state`  | `string` | n/a   | No       | LTC3901 DUT state          |
| `lt8316.state`   | `string` | n/a   | No       | LT8316 DUT state           |

JSON Example:

```json
{"type":"GET","msg":220,"args":{"dbg_signals":["beam_on","ltc3901.pwr_en","lt8316.pwr_en","led.blue","led.red","led.green","sync.enable","hc.state","ltc3901.state","lt8316.state"]}}
```

```json
{"type":"SET","msg":221,"args":{"dbg_period_ms":100,"dbg_signals":["beam_on","ltc3901.pwr_en","lt8316.pwr_en","led.blue","led.red","led.green","sync.enable","hc.state","ltc3901.state","lt8316.state"]}}
```

```json
{"type":"SET","msg":222,"args":{"dbg_signals":{"ltc3901.pwr_en":true,"lt8316.pwr_en":false,"led.blue":true,"led.red":false,"led.green":true,"sync.enable":true}}}
```

Notes:

- Settable digital signals (`ltc3901.pwr_en`, `lt8316.pwr_en`, `led.blue`, `led.red`, `led.green`, `sync.enable`) can be controlled using `SET` with `dbg_signals` as an object containing signal-value pairs.
- Read-only signals (`beam_on`, `hc.state`, etc.) cannot be set and will return an error if attempted.

## Calibration Notes

- Engineering conversion uses:
  - `engineering = ((SlopeScaled * raw_counts) / 1000000) + Offset`
- Calibration is independent per ADC channel.
- `adc_cal.channel` for `SET`, and `args.adc_cal` for `GET`, may use either a numeric ADC channel index or one of these names:
  - `vupstream`
  - `ltc3901_vcc`
  - `lt8316_vout`
  - `ltc3901_me`
  - `ltc3901_mf`
  - `lt8316_gate`
  - `temp`
  - `vrefint`
- `adc_cal.valid = false` disables `.eng` output for that channel and the `.eng` signal returns `null`.
- Calibration values are currently stored in RAM only.


## Common Startup Pattern for Debugging
Disable Periodic Status Message
Enable Debug 

```json
{"type":"SET","msg":18,"args":{"sts_period_ms":0}}

{"type":"SET","msg":101,"args":{"dbg_period_ms":5000,"dbg_signals":["adc.vupstream.raw","adc.vupstream.mv","adc.vupstream.eng"]}}

{"type":"GET","msg":1,"args":{"dbg_period_ms":true,"dbg_signals":true}}

{"type":"SET","msg":222,"args":{"dbg_signals":{"led.red":true}}}

{"type":"SET","msg":222,"args":{"dbg_signals":{"ltc3901.pwr_en":true}}}
```


### Turn on LTC3901 after Bootup
Explanation:
1. Disable Periodic Status Messages
2. Enable periodic Debug Messages to display key LTC3901 values.
3. Enable power to the LTC3901
4. Enable the Sync Signal to the LTC3901
```json
# Status OFF: {"type":"SET","msg":18,"args":{"sts_period_ms":0}}

# Debug Loop (5 second refresh)
{"type":"SET","msg":101,"args":{"dbg_period_ms":5000,"dbg_signals":["adc.vupstream.mv","ltc3901.pwr_en","adc.ltc3901_vcc.mv","pwm.me.freq_hz","adc.ltc3901_me.mv"]}}

{"type":"SET","msg":124,"args":{"dbg_signals":{"ltc3901.pwr_en":true}}}{"type":"SET","msg":124,"args":{"dbg_signals":{"sync.enable":true}}}

# Monitor LTC3901 Signals
{"type":"SET","msg":101,"args":{"dbg_period_ms":5000,"dbg_signals":["adc.vupstream.raw","adc.ltc3901_vcc.raw","adc.ltc3901_me.raw","adc.ltc3901_mf.raw","pwm.me.freq_hz","pwm.mf.freq_hz"]}}

# Monitor LT8316 Signals
{"type":"SET","msg":101,"args":{"dbg_period_ms":1000,"dbg_signals":["pwm.gate.freq_hz","pwm.gate.valid","pwm.gate.armed","pwm.gate.start_ok","pwm.gate.start_status","pwm.gate.process_ok","pwm.gate.rise_count","pwm.gate.done_mask","pwm.gate.tick_hz","pwm.gate.dma_error","pwm.gate.dma_done_cnt","pwm.gate.timeout_cnt"]}}

# Debug OFF
{"type":"SET","msg":21,"args":{"dbg_period_ms":0}}

```
`vupstream`
  - `ltc3901_vcc`
  - `lt8316_vout`
  - `ltc3901_me`
  - `ltc3901_mf`
  - `lt8316_gate`
