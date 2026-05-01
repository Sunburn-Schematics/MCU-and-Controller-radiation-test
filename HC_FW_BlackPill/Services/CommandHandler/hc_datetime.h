#ifndef HC_DATETIME_H_
#define HC_DATETIME_H_

#include "hc_cmd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hc_datetime_init(void);
bool hc_datetime_is_valid_string(const char *value);
bool hc_datetime_set(const char *value);
const char *hc_datetime_get(void);

#ifdef __cplusplus
}
#endif

#endif /* HC_DATETIME_H_ */