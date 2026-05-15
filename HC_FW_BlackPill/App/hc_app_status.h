#ifndef HC_APP_STATUS_H_
#define HC_APP_STATUS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *ManagerState;
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
} hc_app_ltc3901_status_t;

typedef struct
{
    const char *ManagerState;
    bool PowerEnabled;
    int32_t GateFreq_Hz;
    int32_t GateAnlg_mV;
    int32_t VOut_mV;
} hc_app_lt8316_status_t;

typedef struct
{
    uint32_t HcId;
    bool BeamOn;
    hc_app_ltc3901_status_t Ltc3901;
    hc_app_lt8316_status_t Lt8316;
} hc_app_status_t;

void hc_app_status_init(void);
void hc_app_status_refresh_from_bsp(void);
void hc_app_status_set_ltc3901_manager_state(const char *manager_state,
                                             bool sync_enabled);
void hc_app_status_set_lt8316_manager_state(const char *manager_state);
hc_app_status_t *hc_app_status_get(void);
const hc_app_status_t *hc_app_status_get_const(void);
bool hc_app_status_format_sts_json(char *buffer, size_t buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* HC_APP_STATUS_H_ */
