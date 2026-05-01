#include "hc_comms_tx.h"

bool hc_comms_tx_send_line(const char *line)
{
    (void)line;

    /*
     * Stub only.
     * Later this will forward complete JSONL lines to the USB VCP transmit path.
     */
    return false;
}