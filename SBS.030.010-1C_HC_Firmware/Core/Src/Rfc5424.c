#include "Rfc5424.h"
#include "TimeSinceBoot.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define RFC5424_VERSION 1
#define RFC5424_HOSTNAME_MAX 32
#define RFC5424_APPNAME_MAX 32
#define RFC5424_PROCID_MAX 16
#define RFC5424_MSGID_MAX 32
#define RFC5424_BUFFER_MAX 384

static char s_hostname[RFC5424_HOSTNAME_MAX] = "blackpill";
static char s_appname[RFC5424_APPNAME_MAX] = "test-host";
static char s_procid[RFC5424_PROCID_MAX] = "-";
static char s_default_msgid[RFC5424_MSGID_MAX] = "-";
static uint32_t s_sequence = 0U;

static void copy_field(char *dst, size_t dst_sz, const char *src)
{
    if ((dst == NULL) || (dst_sz == 0U)) {
        return;
    }
    if ((src == NULL) || (src[0] == '\0')) {
        dst[0] = '-';
        if (dst_sz > 1U) {
            dst[1] = '\0';
        }
        return;
    }
    snprintf(dst, dst_sz, "%s", src);
}

void RFC5424_Init(const char *hostname, const char *app_name, const char *procid, const char *msgid)
{
    copy_field(s_hostname, sizeof(s_hostname), hostname);
    copy_field(s_appname, sizeof(s_appname), app_name);
    copy_field(s_procid, sizeof(s_procid), procid);
    copy_field(s_default_msgid, sizeof(s_default_msgid), msgid);
    s_sequence = 0U;
}

uint32_t RFC5424_GetSequence(void)
{
    return s_sequence;
}

void RFC5424_Log(SyslogFacility_t facility, SyslogSeverity_t severity, const char *msg)
{
    RFC5424_Logf(facility, severity, s_default_msgid, "%s", (msg != NULL) ? msg : "");
}

void RFC5424_Logf(SyslogFacility_t facility, SyslogSeverity_t severity, const char *msgid, const char *fmt, ...)
{
    char text[192];
    char line[RFC5424_BUFFER_MAX];
    const char *effective_msgid = ((msgid != NULL) && (msgid[0] != '\0')) ? msgid : s_default_msgid;
    uint32_t pri = ((uint32_t)facility * 8U) + (uint32_t)severity;
    int n;
    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    s_sequence++;

    n = snprintf(line,
                 sizeof(line),
                 "<%lu>%d - %s %s %s %s\r\n",
                 (unsigned long)pri,
                 RFC5424_VERSION,
                 s_appname,
                 s_procid,
                 effective_msgid,
                 text);

    if (n > 0) {
        printf("%s", line);
    }
}
