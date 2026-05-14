#include "hc_cmd_types.h"

#include "hc_app_status.h"

uint32_t hc_cmd_get_host_controller_id(void)
{
    const hc_app_status_t *status = hc_app_status_get_const();

    if (status == 0)
    {
        return 1U;
    }

    return status->HcId;
}
