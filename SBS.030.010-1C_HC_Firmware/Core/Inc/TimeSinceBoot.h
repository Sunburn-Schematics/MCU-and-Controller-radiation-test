#ifndef __TIME_SINCE_BOOT_H__
#define __TIME_SINCE_BOOT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void TSB_Init(void);
void TSB_Reset(void);
uint32_t TSB_GetMs(void);
uint32_t TSB_GetSeconds(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIME_SINCE_BOOT_H__ */
