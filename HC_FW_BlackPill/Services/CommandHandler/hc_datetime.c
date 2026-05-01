#include "hc_datetime.h"

#include "rtc_drv.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char s_datetime_string[HC_DATETIME_BUFFER_LEN] = "19700101 00:00:00";

static bool hc_datetime_check_digit_range(const char *value, size_t from, size_t to)
{
    size_t i;

    for (i = from; i <= to; ++i)
    {
        if (!isdigit((unsigned char)value[i]))
        {
            return false;
        }
    }

    return true;
}

static int hc_datetime_parse_u32(const char *value, size_t from, size_t len)
{
    size_t i;
    int result = 0;

    for (i = 0u; i < len; ++i)
    {
        result *= 10;
        result += (int)(value[from + i] - '0');
    }

    return result;
}

static bool hc_datetime_parse_string(const char *value, rtc_drv_datetime_t *date_time)
{
    if ((value == NULL) || (date_time == NULL))
    {
        return false;
    }

    if (!hc_datetime_is_valid_string(value))
    {
        return false;
    }

    date_time->year = (uint16_t)hc_datetime_parse_u32(value, 0u, 4u);
    date_time->month = (uint8_t)hc_datetime_parse_u32(value, 4u, 2u);
    date_time->day = (uint8_t)hc_datetime_parse_u32(value, 6u, 2u);
    date_time->hour = (uint8_t)hc_datetime_parse_u32(value, 9u, 2u);
    date_time->minute = (uint8_t)hc_datetime_parse_u32(value, 12u, 2u);
    date_time->second = (uint8_t)hc_datetime_parse_u32(value, 15u, 2u);

    return rtc_drv_is_valid_datetime(date_time);
}

static bool hc_datetime_format_string(const rtc_drv_datetime_t *date_time, char *buffer, size_t buffer_len)
{
    int written;

    if ((date_time == NULL) || (buffer == NULL) || (buffer_len < HC_DATETIME_BUFFER_LEN))
    {
        return false;
    }

    written = snprintf(buffer,
                       buffer_len,
                       "%04u%02u%02u %02u:%02u:%02u",
                       (unsigned int)date_time->year,
                       (unsigned int)date_time->month,
                       (unsigned int)date_time->day,
                       (unsigned int)date_time->hour,
                       (unsigned int)date_time->minute,
                       (unsigned int)date_time->second);

    return (written == (int)HC_DATETIME_STRING_LEN);
}

void hc_datetime_init(void)
{
    rtc_drv_datetime_t current_time;

    rtc_drv_init();

    if (rtc_drv_get_datetime(&current_time) == RTC_DRV_OK)
    {
        (void)hc_datetime_format_string(&current_time, s_datetime_string, sizeof(s_datetime_string));
    }
    else
    {
        (void)strncpy(s_datetime_string, "19700101 00:00:00", sizeof(s_datetime_string) - 1u);
        s_datetime_string[sizeof(s_datetime_string) - 1u] = '\0';
    }
}

bool hc_datetime_is_valid_string(const char *value)
{
    if (value == NULL)
    {
        return false;
    }

    if (strlen(value) != HC_DATETIME_STRING_LEN)
    {
        return false;
    }

    if (!hc_datetime_check_digit_range(value, 0u, 7u)) return false;
    if (value[8]  != ' ') return false;
    if (!hc_datetime_check_digit_range(value, 9u, 10u)) return false;
    if (value[11] != ':') return false;
    if (!hc_datetime_check_digit_range(value, 12u, 13u)) return false;
    if (value[14] != ':') return false;
    if (!hc_datetime_check_digit_range(value, 15u, 16u)) return false;

    return true;
}

bool hc_datetime_set(const char *value)
{
    rtc_drv_datetime_t date_time;

    if (!hc_datetime_parse_string(value, &date_time))
    {
        return false;
    }

    if (rtc_drv_set_datetime(&date_time) != RTC_DRV_OK)
    {
        return false;
    }

    (void)strncpy(s_datetime_string, value, sizeof(s_datetime_string) - 1u);
    s_datetime_string[sizeof(s_datetime_string) - 1u] = '\0';
    return true;
}

const char *hc_datetime_get(void)
{
    (void)hc_datetime_get_now(s_datetime_string, sizeof(s_datetime_string));
    return s_datetime_string;
}

bool hc_datetime_get_now(char *buffer, size_t buffer_len)
{
    rtc_drv_datetime_t date_time;

    if (rtc_drv_get_datetime(&date_time) != RTC_DRV_OK)
    {
        return false;
    }

    return hc_datetime_format_string(&date_time, buffer, buffer_len);
}
