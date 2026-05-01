#ifndef RTC_DRV_H_
#define RTC_DRV_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_drv_datetime_t;

typedef enum
{
    RTC_DRV_OK = 0,
    RTC_DRV_ERR_INVALID_ARG,
    RTC_DRV_ERR_INVALID_VALUE,
    RTC_DRV_ERR_HAL,
    RTC_DRV_ERR_NOT_INITIALIZED
} rtc_drv_status_t;

void rtc_drv_init(void);
bool rtc_drv_is_initialized(void);

rtc_drv_status_t rtc_drv_set_datetime(const rtc_drv_datetime_t *date_time);
rtc_drv_status_t rtc_drv_get_datetime(rtc_drv_datetime_t *date_time);

bool rtc_drv_is_valid_datetime(const rtc_drv_datetime_t *date_time);

#ifdef __cplusplus
}
#endif

#endif /* RTC_DRV_H_ */
