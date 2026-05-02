#include "hc_app_status.h"

static hc_app_status_t s_hc_app_status;

static void hc_app_fault_summary_init(hc_fault_summary_t *faults)
{
    if (faults == 0)
    {
        return;
    }

    faults->Count = 0U;
    faults->Summary = "NONE";
    faults->IdsJson = "[]";
}

void hc_app_status_init(void)
{
    s_hc_app_status.HcId = 1U;
    s_hc_app_status.State = HC_APP_STATE_NORMAL;
    s_hc_app_status.BeamOn = false;

    s_hc_app_status.Ltc3901.State = HC_DUT_STATE_NORMAL;
    s_hc_app_status.Ltc3901.PowerEnabled = false;
    s_hc_app_status.Ltc3901.SyncEnabled = true;
    s_hc_app_status.Ltc3901.VSupply_mV = -1;
    s_hc_app_status.Ltc3901.VShunt_mV = -1;
    s_hc_app_status.Ltc3901.ISupply_mA = -1;
    s_hc_app_status.Ltc3901.MeFreq_Hz = -1;
    s_hc_app_status.Ltc3901.MeRatio_Pct = -1;
    s_hc_app_status.Ltc3901.MeAnlg_mV = -1;
    s_hc_app_status.Ltc3901.MfFreq_Hz = -1;
    s_hc_app_status.Ltc3901.MfRatio_Pct = -1;
    s_hc_app_status.Ltc3901.MfAnlg_mV = -1;
    hc_app_fault_summary_init(&s_hc_app_status.Ltc3901.Faults);

    s_hc_app_status.Lt8316.State = HC_DUT_STATE_NORMAL;
    s_hc_app_status.Lt8316.PowerEnabled = false;
    s_hc_app_status.Lt8316.GateFreq_Hz = -1;
    s_hc_app_status.Lt8316.GateRatio_Pct = -1;
    s_hc_app_status.Lt8316.GateAnlg_mV = -1;
    s_hc_app_status.Lt8316.VOut_mV = -1;
    hc_app_fault_summary_init(&s_hc_app_status.Lt8316.Faults);
}

hc_app_status_t *hc_app_status_get(void)
{
    return &s_hc_app_status;
}

const hc_app_status_t *hc_app_status_get_const(void)
{
    return &s_hc_app_status;
}

const char *hc_app_state_to_string(hc_app_state_t state)
{
    switch (state)
    {
        case HC_APP_STATE_BOOT:
            return "BOOT";

        case HC_APP_STATE_FAULT:
            return "FAULT";

        case HC_APP_STATE_NORMAL:
            return "NORMAL";

        case HC_APP_STATE_SLAVE:
            return "SLAVE";

        default:
            return "FAULT";
    }
}

const char *hc_dut_state_to_string(hc_dut_state_t state)
{
    switch (state)
    {
        case HC_DUT_STATE_NORMAL:
            return "NORMAL";

        case HC_DUT_STATE_RECOVERED:
            return "RECOVERED";

        case HC_DUT_STATE_ISOLATED:
            return "ISOLATED";

        case HC_DUT_STATE_FAULT:
            return "FAULT";

        default:
            return "FAULT";
    }
}
