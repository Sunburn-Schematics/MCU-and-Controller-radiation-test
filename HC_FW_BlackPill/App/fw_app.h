#ifndef FW_APP_H
#define FW_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_vcp_drv.h"


void fw_app_init(void);
void fw_app_run(void);

#ifdef __cplusplus
}
#endif

#endif /* FW_APP_H */
