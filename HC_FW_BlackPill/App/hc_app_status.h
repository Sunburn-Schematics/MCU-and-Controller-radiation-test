#ifndef HC_APP_STATUS_H_
#define HC_APP_STATUS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HC_APP_STATE_TABLE(X) \
    X(HC_APP_STATE_BOOT,   "BOOT") \
    X(HC_APP_STATE_FAULT,  "FAULT") \
    X(HC_APP_STATE_NORMAL, "NORMAL") \
    X(HC_APP_STATE_SLAVE,  "SLAVE")

typedef enum
{
#define X(name, str) name,
    HC_APP_STATE_TABLE(X)
#undef X
} hc_app_state_t;

#define HC_DUT_STATE_TABLE(X) \
    X(HC_DUT_STATE_NORMAL,    "NORMAL") \
    X(HC_DUT_STATE_RECOVERED, "RECOVERED") \
    X(HC_DUT_STATE_ISOLATED,  "ISOLATED") \
    X(HC_DUT_STATE_FAULT,     "FAULT")

typedef enum
{
#define X(name, str) name,
    HC_DUT_STATE_TABLE(X)
#undef X
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
void hc_app_status_refresh_from_bsp(void);
hc_app_status_t *hc_app_status_get(void);
const hc_app_status_t *hc_app_status_get_const(void);
bool hc_app_status_format_sts_json(char *buffer, size_t buffer_len);

const char *hc_app_state_to_string(hc_app_state_t state);
const char *hc_dut_state_to_string(hc_dut_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* HC_APP_STATUS_H_ */
