#include "hc_datetime.h"

#include <ctype.h>
#include <string.h>

static hc_datetime_state_t s_datetime = {
    .current = "19700101 00:00:00.00",
    .valid = false
};

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

void hc_datetime_init(void)
{
    (void)strncpy(s_datetime.current, "19700101 00:00:00.00", sizeof(s_datetime.current) - 1u);
    s_datetime.current[sizeof(s_datetime.current) - 1u] = '\0';
    s_datetime.valid = false;
}

bool hc_datetime_is_valid_string(const char *value)
{
    if (value == NULL)
    {
        return false;
    }

    if (strlen(value) != 20u)
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
    if (value[17] != '.') return false;
    if (!hc_datetime_check_digit_range(value, 18u, 19u)) return false;

    return true;
}

bool hc_datetime_set(const char *value)
{
    if (!hc_datetime_is_valid_string(value))
    {
        return false;
    }

    (void)strncpy(s_datetime.current, value, sizeof(s_datetime.current) - 1u);
    s_datetime.current[sizeof(s_datetime.current) - 1u] = '\0';
    s_datetime.valid = true;
    return true;
}

const char *hc_datetime_get(void)
{
    return s_datetime.current;
}