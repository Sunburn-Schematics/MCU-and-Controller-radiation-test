#ifndef RFC5424_H
#define RFC5424_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    SYSLOG_SEV_EMERG = 0,
    SYSLOG_SEV_ALERT = 1,
    SYSLOG_SEV_CRIT = 2,
    SYSLOG_SEV_ERR = 3,
    SYSLOG_SEV_WARNING = 4,
    SYSLOG_SEV_NOTICE = 5,
    SYSLOG_SEV_INFO = 6,
    SYSLOG_SEV_DEBUG = 7
} SyslogSeverity_t;

typedef enum {
    SYSLOG_FACILITY_KERN = 0,
    SYSLOG_FACILITY_USER = 1,
    SYSLOG_FACILITY_LOCAL0 = 16,
    SYSLOG_FACILITY_LOCAL1 = 17,
    SYSLOG_FACILITY_LOCAL2 = 18,
    SYSLOG_FACILITY_LOCAL3 = 19,
    SYSLOG_FACILITY_LOCAL4 = 20,
    SYSLOG_FACILITY_LOCAL5 = 21,
    SYSLOG_FACILITY_LOCAL6 = 22,
    SYSLOG_FACILITY_LOCAL7 = 23
} SyslogFacility_t;

void RFC5424_Init(const char *hostname, const char *app_name, const char *procid, const char *msgid);
void RFC5424_Log(SyslogFacility_t facility, SyslogSeverity_t severity, const char *msg);
void RFC5424_Logf(SyslogFacility_t facility, SyslogSeverity_t severity, const char *msgid, const char *fmt, ...);
uint32_t RFC5424_GetSequence(void);

#endif
