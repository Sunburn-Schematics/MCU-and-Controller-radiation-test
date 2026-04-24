/*
 * UartIO.c
 * Imported from DrMarty/STM32Utils
 */

#include "UartIO.h"

static UART_HandleTypeDef *huartio;

#ifdef TX_MODE_INTERRUPT
static uint8_t TxBuff[TX_BUFF_SZ];
#endif
static uint8_t RxBuff[RX_BUFF_SZ];
static uint32_t RxIdx;		
static uint32_t TxInsertIdx;		
static uint32_t TxExtractIdx;		

#define DMA_RX_INSERT_PTR	( (RX_BUFF_SZ - huartio->hdmarx->Instance->NDTR) & (RX_BUFF_SZ - 1) )

void UartIO_Init(UART_HandleTypeDef *huart)
{
	setbuf(stdout, NULL);	
	setbuf(stderr, NULL);
	setvbuf(stdin, NULL, _IONBF, 0);

	huartio = huart;
	HAL_UART_Receive_DMA(huartio, RxBuff, RX_BUFF_SZ);
	RxIdx = 0;
	TxInsertIdx = 0;
	TxExtractIdx = 0;
}

static int UartIO_RxEmpty(void)
{
	return (RxIdx == DMA_RX_INSERT_PTR) ? 1 : 0;
}

int __io_getchar(void)
{
	char ch;
	if (UartIO_RxEmpty())
		return (int)EOF;

	ch = RxBuff[RxIdx++];
	RxIdx &= (RX_BUFF_SZ - 1);
	return (int)ch;
}

#ifdef TX_MODE_INTERRUPT
int _write(int file, char *ptr, int len)
{
	int DataIdx = 0;
	int BlockAddr = (TxExtractIdx - 1) & (TX_BUFF_SZ - 1);

	while ( (DataIdx < len) && (TxInsertIdx != BlockAddr) )
	{
		TxBuff[TxInsertIdx++] = *ptr++;
		TxInsertIdx &= (TX_BUFF_SZ - 1);
		DataIdx++;
	}
	if (DataIdx != 0)
		HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], 1);

	return DataIdx;
}

void __io_putchar(uint8_t ch)
{
	TxBuff[TxInsertIdx++] = ch;
	TxInsertIdx &= (TX_BUFF_SZ - 1);
	HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], 1);
	return;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uint16_t Size;

	TxExtractIdx = huart->pTxBuffPtr - TxBuff;
	TxExtractIdx &= (TX_BUFF_SZ - 1);
	if (TxExtractIdx == TxInsertIdx)
		return;		

	if (TxInsertIdx > TxExtractIdx)
		Size = TxInsertIdx - TxExtractIdx;
	else
		Size = TX_BUFF_SZ - TxExtractIdx;
	HAL_UART_Transmit_IT(huartio, &TxBuff[TxExtractIdx], Size);
	return;
}
#endif
