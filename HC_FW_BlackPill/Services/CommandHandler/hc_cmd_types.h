#ifndef HC_CMD_TYPES_H_
#define HC_CMD_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HC_CMD_HOST_CONTROLLER_ID       (1u)
#define HC_CMD_MAX_LINE_LEN             (256u)
#define HC_CMD_MAX_TS_LEN               (18u)
#define HC_CMD_MAX_DATE_TIME_LEN        (18u)
#define HC_CMD_MAX_ERROR_CODE_LEN       (24u)
#define HC_CMD_MAX_ERROR_MESSAGE_LEN    (96u)
#define HC_CMD_MAX_TOKENS               (48u)

typedef enum
{
    HC_PKT_UNKNOWN = 0,
    HC_PKT_GET,
    HC_PKT_SET,
    HC_PKT_EXC,
    HC_PKT_RSP,
    HC_PKT_STS,
    HC_PKT_EVT
} hc_pkt_type_t;

typedef enum
{
    HC_CMD_OK = 0,
    HC_CMD_ERR_BAD_JSON,
    HC_CMD_ERR_BAD_TYPE,
    HC_CMD_ERR_BAD_ARGS,
    HC_CMD_ERR_BAD_FIELD,
    HC_CMD_ERR_BAD_VALUE,
    HC_CMD_ERR_NOT_SUPPORTED,
    HC_CMD_ERR_INTERNAL
} hc_cmd_status_t;

typedef struct
{
    hc_pkt_type_t type;
    uint32_t msg;
    bool has_msg;
} hc_cmd_request_t;

typedef struct
{
    hc_cmd_status_t status;
    bool include_msg;
    uint32_t msg;
    const char *error_code;
    const char *error_message;
} hc_cmd_error_t;

#endif /* HC_CMD_TYPES_H_ */
