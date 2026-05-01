#include "rtc_drv.h"

#include "rtc.h"
#include "stm32f4xx_hal.h"

static bool s_rtc_drv_initialized = false;

static bool rtc_drv_is_leap_year(uint16_t year)
{
    if ((year % 400u) == 0u)
    {
        return true;
    }

    if ((year % 100u) == 0u)
    {
        return false;
    }

    return ((year % 4u) == 0u);
}

static uint8_t rtc_drv_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t s_days_in_month[12] = {
        31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u
    };

    if ((month < 1u) || (month > 12u))
    {
        return 0u;
    }

    if ((month == 2u) && rtc_drv_is_leap_year(year))
    {
        return 29u;
    }

    return s_days_in_month[month - 1u];
}

static uint8_t rtc_drv_compute_weekday(const rtc_drv_datetime_t *date_time)
{
    (void)date_time;

    return RTC_WEEKDAY_MONDAY;
}

void rtc_drv_init(void)
{
    s_rtc_drv_initialized = true;
}

bool rtc_drv_is_initialized(void)
{
    return s_rtc_drv_initialized;
}

bool rtc_drv_is_valid_datetime(const rtc_drv_datetime_t *date_time)
{
    uint8_t dim;

    if (date_time == NULL)
    {
        return false;
    }

    if (date_time->year < 2000u)
    {
        return false;
    }

    if ((date_time->month < 1u) || (date_time->month > 12u))
    {
        return false;
    }

    dim = rtc_drv_days_in_month(date_time->year, date_time->month);
    if ((date_time->day < 1u) || (date_time->day > dim))
    {
        return false;
    }

    if (date_time->hour > 23u)
    {
        return false;
    }

    if (date_time->minute > 59u)
    {
        return false;
    }

    if (date_time->second > 59u)
    {
        return false;
    }

    return true;
}

rtc_drv_status_t rtc_drv_set_datetime(const rtc_drv_datetime_t *date_time)
{
    RTC_TimeTypeDef hal_time = {0};
    RTC_DateTypeDef hal_date = {0};

    if (!s_rtc_drv_initialized)
    {
        return RTC_DRV_ERR_NOT_INITIALIZED;
    }

    if (date_time == NULL)
    {
        return RTC_DRV_ERR_INVALID_ARG;
    }

    if (!rtc_drv_is_valid_datetime(date_time))
    {
        return RTC_DRV_ERR_INVALID_VALUE;
    }

    hal_time.Hours = date_time->hour;
    hal_time.Minutes = date_time->minute;
    hal_time.Seconds = date_time->second;
    hal_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    hal_time.StoreOperation = RTC_STOREOPERATION_RESET;

    hal_date.Year = (uint8_t)(date_time->year - 2000u);
    hal_date.Month = date_time->month;
    hal_date.Date = date_time->day;
    hal_date.WeekDay = rtc_drv_compute_weekday(date_time);

    if (HAL_RTC_SetTime(&hrtc, &hal_time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_DRV_ERR_HAL;
    }

    if (HAL_RTC_SetDate(&hrtc, &hal_date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_DRV_ERR_HAL;
    }

    return RTC_DRV_OK;
}

rtc_drv_status_t rtc_drv_get_datetime(rtc_drv_datetime_t *date_time)
{
    RTC_TimeTypeDef hal_time = {0};
    RTC_DateTypeDef hal_date = {0};

    if (!s_rtc_drv_initialized)
    {
        return RTC_DRV_ERR_NOT_INITIALIZED;
    }

    if (date_time == NULL)
    {
        return RTC_DRV_ERR_INVALID_ARG;
    }

    if (HAL_RTC_GetTime(&hrtc, &hal_time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_DRV_ERR_HAL;
    }

    if (HAL_RTC_GetDate(&hrtc, &hal_date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return RTC_DRV_ERR_HAL;
    }

    date_time->year = (uint16_t)(2000u + hal_date.Year);
    date_time->month = hal_date.Month;
    date_time->day = hal_date.Date;
    date_time->hour = hal_time.Hours;
    date_time->minute = hal_time.Minutes;
    date_time->second = hal_time.Seconds;

    return RTC_DRV_OK;
}
