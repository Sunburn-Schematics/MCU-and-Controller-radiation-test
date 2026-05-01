#ifndef USB_VCP_DRV_H
#define USB_VCP_DRV_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "usb_device.h"

#define VCP_RX_BUFFER_SIZE 1024U    // Size of the VCP receive buffer; must be large enough to hold expected incoming data and should be a power of 2 for the current implementation
#define VCP_TX_BUFFER_SIZE 1024U    // Size of the VCP transmit buffer; must be large enough to hold expected outgoing data and should be a power of 2 for the current implementation

/* VCP Device status */
typedef enum
{
  VCP_OK = 0U,
  VCP_READY,
  VCP_RECEIVED_DATA_AVAILABLE,
  VCP_BUSY,
  VCP_FAIL,
} VCP_StatusTypeDef;

typedef struct 
{
    uint32_t tx_pkt_count;
    uint32_t tx_byte_count;
    uint32_t rx_pkt_count;
    uint32_t rx_byte_count;
    uint32_t blocked_tx_pkt_count;
    uint32_t dropped_rx_pkt_count;
} VCP_MetricsTypeDef;

typedef struct
{
    // Need something here to track the connection / availability of the USB transport layer
    HAL_LockTypeDef Lock;                           // Lock to protect concurrent access to the VCP handle from different contexts (e.g., main loop, USB interrupt handlers, etc.)
    //USBD_HandleTypeDef* hUsbDevice;               // Pointer to the USB device handle, used for transmitting data over USB. This is set during initialization and can be used by the VCP driver to send data without needing to pass the handle around in function parameters.
    VCP_StatusTypeDef vcp_rx_status;                // Status of the VCP receive buffer, used to indicate when new data has been received and is available for processing
    char vcp_rx_msg_buffer[VCP_RX_BUFFER_SIZE];     // Buffer where received data is stored
    uint32_t vcp_rx_msg_len;                        // Size of data currently in the receive buffer
//    char* vcp_tx_msg_buffer;                      // Pointer to the buffer where data to be transmitted is stored
//    uint32_t vcp_tx_msg_len;         // Size of data currently in the transmit buffer
//    char* vcp_tx_msg_ptr;                // Pointer to the current position in the transmit buffer for ongoing transmissions
} VCP_HandleTypeDef;

//extern VCP_HandleTypeDef hVcp;    // Global instance of the VCP handle


VCP_StatusTypeDef usb_vcp_drv_init(void);
VCP_StatusTypeDef usb_vcp_drv_task(void);     //Periodically tickle the VCP to process any pending transmissions or receptions.
int _write(int file, char *ptr, int len);

int8_t usb_vcp_buffer_rx_pkt(uint8_t* Buf, uint32_t Len);


#ifdef __cplusplus
}
#endif

#endif /* USB_VCP_DRV_H */