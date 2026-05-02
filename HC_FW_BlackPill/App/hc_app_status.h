#ifndef HC_APP_STATUS_H_
#define HC_APP_STATUS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HC_APP_STATE_BOOT = 0,
    HC_APP_STATE_FAULT,
    HC_APP_STATE_NORMAL,
    HC_APP_STATE_SLAVE
} hc_app_state_t;

typedef enum
{
    HC_DUT_STATE_NORMAL = 0,
    HC_DUT_STATE_RECOVERED,
    HC_DUT_STATE_ISOLATED,
    HC_DUT_STATE_FAULT
} hc_dut_state_t;

typedef struct
{
    uint32_t Count;
    const char *Summary;
    const char *IdsJson;
} hc_fault_summary_t;

typedef struct
{
    hc_dut_state_t State;
    bool PowerEnabled;
    bool SyncEnabled;
    int32_t VSupply_mV;
    int32_t VShunt_mV;
    int32_t ISupply_mA;
    int32_t MeFreq_Hz;
    int32_t MeRatio_Pct;
    int32_t MeAnlg_mV;
    int32_t MfFreq_Hz;
    int32_t MfRatio_Pct;
    int32_t MfAnlg_mV;
    hc_fault_summary_t Faults;
} hc_app_ltc3901_status_t;

typedef struct
{
    hc_dut_state_t State;
    bool PowerEnabled;
    int32_t GateFreq_Hz;
    int32_t GateRatio_Pct;
    int32_t GateAnlg_mV;
    int32_t VOut_mV;
    hc_fault_summary_t Faults;
} hc_app_lt8316_status_t;

typedef struct
{
    uint32_t HcId;
    hc_app_state_t State;
    bool BeamOn;
    hc_app_ltc3901_status_t Ltc3901;
    hc_app_lt8316_status_t Lt8316;
} hc_app_status_t;

void hc_app_status_init(void);
hc_app_status_t *hc_app_status_get(void);
const hc_app_status_t *hc_app_status_get_const(void);

const char *hc_app_state_to_string(hc_app_state_t state);
const char *hc_dut_state_to_string(hc_dut_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* HC_APP_STATUS_H_ */
