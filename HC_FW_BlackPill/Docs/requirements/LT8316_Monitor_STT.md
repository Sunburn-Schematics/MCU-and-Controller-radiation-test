# LT8316_Monitor_STT

## Sheet1

| State   | On First Entry                                             | Event (While in State)               | (Guard) Condition                       | Next State | (Action) On Exit | Reported Status            | Requirement / Fault Reference | Notes |
| ------- | ---------------------------------------------------------- | ------------------------------------ | --------------------------------------- | ---------- | ---------------- | -------------------------- | ----------------------------- | ----- |
| RESET   | De-Assert HV_Pwr_En                                        | RUN command                          |                                         | POWERED    |                  | EVT_MSG: Entering POWERED  |                               |       |
|         |                                                            |                                      |                                         |            |                  |                            |                               |       |
| FAULT   | InState_tmr = 0<br>De-assert HV_Pwr_En<br>Fault_Count += 1 | RESET command                        |                                         | RESET      |                  |                            |                               | RESET clears fault counters. |
|         |                                                            | RUN command                          |                                         | POWERED    |                  | EVT_MSG: Entering POWERED  |                               |       |
|         |                                                            | InState_tmr > Power_Retry_Delay      | Fault_Count < Fault_MAX                 | POWERED    |                  | EVT_MSG: Retrying Power Up |                               |       |
|         |                                                            |                                      |                                         |            |                  |                            |                               |       |
| POWERED | InState_tmr = 0<br>Assert HV_Pwr_En                        | Gate_Freq == null                    | InState_tmr > Pwr_On_Stabilization_Time | FAULT      |                  | EVT_MSG: GATE Stopped      |                               |       |

## External command transitions

The LT8316 manager accepts these external commands:

- `RUN`: transition from the current state to `POWERED`.
- `RESET`: transition from the current state to `RESET`. RESET disables `HV_Pwr_En` and clears `Fault_Count`.
