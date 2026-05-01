#ifndef HC_JSONL_CMD_H_
#define HC_JSONL_CMD_H_

#include "hc_cmd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void hc_jsonl_cmd_init(void);
void hc_jsonl_cmd_process_line(const char *line);

void hc_jsonl_process_command(const char *cmd, uint32_t len);


#ifdef __cplusplus
}
#endif

#endif /* HC_JSONL_CMD_H_ */
