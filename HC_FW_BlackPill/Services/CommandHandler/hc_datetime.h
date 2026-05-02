#ifndef HC_DATETIME_H_
#define HC_DATETIME_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HC_DATETIME_STRING_LEN 17u
#define HC_DATETIME_BUFFER_LEN (HC_DATETIME_STRING_LEN + 1u)

typedef enum
{
    HC_DATETIME_OK = 0,
    HC_DATETIME_ERR_BAD_FORMAT,
    HC_DATETIME_ERR_BAD_VALUE,
    HC_DATETIME_ERR_RTC_READ,
    HC_DATETIME_ERR_RTC_WRITE
} hc_datetime_status_t;

void hc_datetime_init(void);
bool hc_datetime_is_valid_string(const char *value);
hc_datetime_status_t hc_datetime_set(const char *value);
const char *hc_datetime_get(void);
bool hc_datetime_get_now(char *buffer, size_t buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* HC_DATETIME_H_ */
