#ifndef HC_COMMS_TX_H_
#define HC_COMMS_TX_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool hc_comms_tx_send_line(const char *line);

#ifdef __cplusplus
}
#endif

#endif /* HC_COMMS_TX_H_ */