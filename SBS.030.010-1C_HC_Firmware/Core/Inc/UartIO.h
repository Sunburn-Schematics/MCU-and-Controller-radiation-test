/*
 * UartIO.h
 * Imported from DrMarty/STM32Utils
 */

#ifndef INC_UARTIO_H_
#define INC_UARTIO_H_

#include "main.h"
#include <stdio.h>

#ifndef EOF
#define EOF (-1)
#endif

#define TX_MODE_INTERRUPT				

// Set by PRD/DevAssist to 256 bytes
#define TX_BUFF_SZ	128
#define RX_BUFF_SZ	128

void UartIO_Init(UART_HandleTypeDef *huart);
char UartIO_getch(void);

#endif /* INC_UARTIO_H_ */
