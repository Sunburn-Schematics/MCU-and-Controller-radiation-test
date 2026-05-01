

/* This function is called by printf() to print a character. It is implemented here to
 * send the character over USB VCP. The function returns the number of characters
 * transmitted.
 */
#include "usb_vcp_drv.h"
#include "usbd_cdc_if.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/_intsup.h>

#include "../Services/CommandHandler/hc_jsonl_cmd.h"

/* Defined in usbd_cdc_if.h 
    These buffers are used by the USB CDC interface to store incoming and outgoing data. The VCP driver can read from the Rx buffer and write to the Tx buffer as needed. 
    The actual transmission and reception over USB is handled by the USB CDC interface, which will call into the VCP driver to get data to send or to provide received data.
*/
extern char UserRxBufferFS[];    //Data received over USB is stored in this buffer
extern char UserTxBufferFS[];    //Data to send over USB CDC are stored in this buffer

static char usb_vcp_TxBuffer[VCP_TX_BUFFER_SIZE];    // Local buffer for data to be transmitted; will be copied to UserTxBufferFS when data is ready to be sent. This allows the VCP driver to manage its own buffer indices without interfering with the USB CDC interface's buffer management.

static uint32_t TxInsertIdx = 0; // Index for inserting data into the Tx buffer
static uint32_t TxExtractIdx = 0; // Index for extracting data from the Tx buffer

static VCP_HandleTypeDef hVcp = {0};    // Global instance of the VCP handle, defined in usb_vcp_drv.h and initialized in usb_vcp_drv_init()
static VCP_MetricsTypeDef vcp_metrics = {0}; // Structure to hold various metrics about VCP operation, such as packet counts and dropped packet counts



/* Called on reception of each USB packet */
/* TREAT THIS LIKE ITS INSIDE AN ISR - i.e. NO printf() calls */
int8_t usb_vcp_buffer_rx_pkt(uint8_t* Buf, uint32_t Len)
{
    vcp_metrics.rx_pkt_count += 1; // Increment total received packet count
    // printf("usb_vcp_buffer_rx_pkt: Received packet %lu with length %lu bytes\n", pkt_count, Len);

    if (hVcp.vcp_rx_status != VCP_READY)
    {
        // Previous data has not been processed yet, so we should drop this packet to avoid overwriting the buffer before it's been read. Increment dropped packet count and return.
        vcp_metrics.dropped_rx_pkt_count += 1;
        return (VCP_FAIL);
    }

    hVcp.Lock = HAL_LOCKED;                           // Lock the VCP handle to protect against concurrent access while processing the received data
    uint8_t copy_len = ( (hVcp.vcp_rx_msg_len + Len) < VCP_RX_BUFFER_SIZE) ? Len : (VCP_RX_BUFFER_SIZE - hVcp.vcp_rx_msg_len - 1); // Calculate how many bytes can be copied while leaving room for '\0' null terminator; if the incoming packet would overflow the buffer, only copy as much as can fit and leave the rest to be dropped
    memcpy(hVcp.vcp_rx_msg_buffer + hVcp.vcp_rx_msg_len, Buf, copy_len); // Copy received data to the VCP driver's local buffer at the current insert index
    hVcp.vcp_rx_msg_len += copy_len; // Update total bytes received count
    hVcp.vcp_rx_msg_buffer[hVcp.vcp_rx_msg_len] = '\0'; // Null-terminate the buffer for safety
    vcp_metrics.rx_byte_count += copy_len; // Update total bytes received metric

    /* Conditions for passing on for further processing are: 
        1) Received packet is less than the maximum USB packet size, which indicates it's the final packet of a transmission, or
        2) Received packet is exactly the maximum USB packet size but ends with a newline '\n', which also indicates it's the end of a transmission in this context.
        3) We're at the limit of our RxBuffer size.
    */
    if (    (Len != MAX_USB_PACKET_SIZE) ||
            ((Len == MAX_USB_PACKET_SIZE) && (hVcp.vcp_rx_msg_buffer[hVcp.vcp_rx_msg_len - 1] == '\n')) || 
            (hVcp.vcp_rx_msg_len >= VCP_RX_BUFFER_SIZE - 1) )
    {
        hVcp.vcp_rx_status = VCP_RECEIVED_DATA_AVAILABLE;   // Update status to indicate new data is available for processing
    }
    hVcp.Lock = HAL_UNLOCKED;                         // Unlock the VCP handle after processing
    return (VCP_OK);
}


VCP_StatusTypeDef usb_vcp_drv_init(void)
{
    assert_param(TX_BUFFER_SIZE & (TX_BUFFER_SIZE - 1) == 0U);      // Ensures buffer size is a power of 2
    assert_param(RX_BUFFER_SIZE & (RX_BUFFER_SIZE - 1) == 0U);      // Ensures buffer size is a power of 2

    hVcp.Lock = HAL_UNLOCKED;                     // Initialize lock to unlocked state
    hVcp.vcp_rx_status = VCP_READY;                // Initialize receive status to ready
    hVcp.vcp_rx_msg_buffer[0] = '\0';             // Initialize receive buffer to empty string
    hVcp.vcp_rx_msg_len = 0;                      // Initialize receive message length to 0

    setvbuf(stdin, NULL, _IONBF, 0);    // Disable buffering for stdin to ensure immediate reception over USB VCP
    setvbuf(stdout, NULL, _IONBF, 0);   // Disable buffering for stdout to ensure immediate transmission over USB VCP
    return (VCP_OK);
}


int _write(int file, char *ptr, int len)
{
	int32_t DataIdx = 0;
	int32_t BlockAddr = (TxExtractIdx - 1) & (VCP_TX_BUFFER_SIZE - 1);

	while ( (DataIdx < len) && (TxInsertIdx != BlockAddr) )
	{
		usb_vcp_TxBuffer[TxInsertIdx++] = *ptr++;
		TxInsertIdx &= (VCP_TX_BUFFER_SIZE - 1);
		DataIdx++;
	}
	return DataIdx;
}


VCP_StatusTypeDef usb_vcp_drv_task(void) 
{
    /* Process incoming messages */
    if (hVcp.vcp_rx_status == VCP_RECEIVED_DATA_AVAILABLE)
    {
        printf("usb_vcp_drv_task: Processing msg[%ld](%d): \"%s\"\n", hVcp.vcp_rx_msg_len, strlen(hVcp.vcp_rx_msg_buffer), hVcp.vcp_rx_msg_buffer);
        hVcp.vcp_rx_status = VCP_BUSY;
        hc_jsonl_process_command(hVcp.vcp_rx_msg_buffer, hVcp.vcp_rx_msg_len);
        hVcp.vcp_rx_msg_buffer[0] = '\0'; // Clear the buffer after processing
        hVcp.vcp_rx_msg_len = 0; // Reset message length after processing
        hVcp.vcp_rx_status = VCP_READY;
    }

    /* Push outgoing messages */
    if (TxInsertIdx != TxExtractIdx)
    {
        uint32_t DataLength = (VCP_TX_BUFFER_SIZE + TxInsertIdx - TxExtractIdx) & (VCP_TX_BUFFER_SIZE - 1);
        uint32_t ChunkSize = (DataLength < MAX_USB_PACKET_SIZE) ? DataLength : MAX_USB_PACKET_SIZE; // Transmit in chunks of MAX_USB_PACKET_SIZE bytes or less

        char chunk[MAX_USB_PACKET_SIZE];
        for (uint32_t i = 0; i < ChunkSize; i++)
        {
            chunk[i] = usb_vcp_TxBuffer[(TxExtractIdx+i) & (VCP_TX_BUFFER_SIZE - 1)];
        }

        if (CDC_Transmit_FS((uint8_t*)chunk, (uint16_t)ChunkSize) == USBD_OK)
        { 
            TxExtractIdx = (TxExtractIdx + ChunkSize) & (VCP_TX_BUFFER_SIZE - 1); // Move extract index forward by chunk size, wrapping around if necessary            
        } else {
            vcp_metrics.blocked_tx_pkt_count += 1; // Increment dropped packet count
        }
    }

    return VCP_OK;
}


