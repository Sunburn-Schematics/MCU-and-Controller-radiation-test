/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "UartIO.h"
#include "TimeSinceBoot.h"
#include "Rfc5424.h"
#include "Stimulus.h"
#include "terminal.h"
#include <stdio.h>
/* USER CODE END Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t last_tsb_report_ms = 0U;
static uint8_t g_device_id = 0U;
static volatile const char *g_error_context = "reset";
static volatile uint8_t g_uart1_ready = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void FormatTsb(char *out, size_t out_size, uint32_t tsb_ms);
int __io_getchar(void);
int __io_putchar(int ch);

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void FormatTsb(char *out, size_t out_size, uint32_t tsb_ms)
{
  uint32_t total_seconds = tsb_ms / 1000U;
  uint32_t ms = tsb_ms % 1000U;
  uint32_t days = total_seconds / 86400U;
  uint32_t hours = (total_seconds % 86400U) / 3600U;
  uint32_t minutes = (total_seconds % 3600U) / 60U;
  uint32_t seconds = total_seconds % 60U;

  snprintf(out, out_size, "%08lu %02lu:%02lu:%02lu.%03lu",
           (unsigned long)days,
           (unsigned long)hours,
           (unsigned long)minutes,
           (unsigned long)seconds,
           (unsigned long)ms);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  char rfc5424_app_name[8];
  char tsb_text[32];
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  g_uart1_ready = 1U;
  UartIO_Init(&huart1);
    printf("\r\nUART1 Initialized\r\n");
//  TSB_Init();
  Terminal_DebugCheckpoint("TSB_Init_done", &g_error_context);

  while (1) {
    HAL_GPIO_TogglePin(LED_BlackPill_GPIO_Port, LED_BlackPill_Pin);
    HAL_Delay(500U);
  }





  snprintf(rfc5424_app_name, sizeof(rfc5424_app_name), "ID=%02u", (unsigned)(g_device_id & 0x3FU));
  RFC5424_Init("-", rfc5424_app_name, "-", "BOOT");
  Terminal_DebugCheckpoint("RFC5424_Init_done", &g_error_context);

  if (ADC1_StartContinuousDma() != HAL_OK)
  {
    g_error_context = "ADC1_StartContinuousDma_failed";
    Error_Handler();
  }
  Terminal_DebugCheckpoint("ADC1_StartContinuousDma_done", &g_error_context);

  if (Stimulus_Init() != HAL_OK)
  {
    g_error_context = "Stimulus_Init_failed";
    Error_Handler();
  }
  Terminal_DebugCheckpoint("Stimulus_Init_done", &g_error_context);

  if (Stimulus_SetFrequencyHz(1000U) != HAL_OK)
  {
    g_error_context = "Stimulus_SetFrequency_failed";
    Error_Handler();
  }
  Terminal_DebugCheckpoint("Stimulus_SetFrequency_done", &g_error_context);

  if (Stimulus_SetMode(STIMULUS_MODE_OPPOSED) != HAL_OK)
  {
    g_error_context = "Stimulus_SetMode_failed";
    Error_Handler();
  }
  Terminal_DebugCheckpoint("Stimulus_SetMode_done", &g_error_context);

  if (Stimulus_Start() != HAL_OK)
  {
    g_error_context = "Stimulus_Start_failed";
    Error_Handler();
  }
  Terminal_DebugCheckpoint("Stimulus_Start_done", &g_error_context);

  FormatTsb(tsb_text, sizeof(tsb_text), TSB_GetMs());
  RFC5424_Logf(SYSLOG_FACILITY_LOCAL0,
               SYSLOG_SEV_INFO,
               "BOOT",
               "%s - BOOTING... SDRA/SDRB stimulus started at %lu Hz mode=%u ADC[%u,%u,%u,%u,%u,%u]",
               tsb_text,
               (unsigned long)Stimulus_GetFrequencyHz(),
               (unsigned)Stimulus_GetMode(),
               g_adc1_samples[0],
               g_adc1_samples[1],
               g_adc1_samples[2],
               g_adc1_samples[3],
               g_adc1_samples[4],
               g_adc1_samples[5]);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t tsb_ms = TSB_GetMs();
    int c = __io_getchar();

    if (c != EOF)
    {
      __io_putchar((uint8_t)c);
    }

    if ((tsb_ms - last_tsb_report_ms) >= 1000U)
    {
      last_tsb_report_ms = tsb_ms;
      FormatTsb(tsb_text, sizeof(tsb_text), tsb_ms);
      RFC5424_Logf(SYSLOG_FACILITY_LOCAL0,
                   SYSLOG_SEV_INFO,
                   "BOOT",
                   "%s - STIM f=%luHz mode=%u ADC[%u,%u,%u,%u,%u,%u]",
                   tsb_text,
                   (unsigned long)Stimulus_GetFrequencyHz(),
                   (unsigned)Stimulus_GetMode(),
                   g_adc1_samples[0],
                   g_adc1_samples[1],
                   g_adc1_samples[2],
                   g_adc1_samples[3],
                   g_adc1_samples[4],
                   g_adc1_samples[5]);
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    HAL_Delay(100);
    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    g_error_context = "HAL_RCC_OscConfig_failed";
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    g_error_context = "HAL_RCC_ClockConfig_failed";
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  if (g_uart1_ready != 0U)
  {
    Terminal_ErrorWrite("\r\nERROR_HANDLER:");
    Terminal_ErrorWriteVolatile(g_error_context);
    Terminal_ErrorWrite("\r\n");
  }

  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    if (g_uart1_ready != 0U)
    {
      Terminal_ErrorWrite("ERROR_HANDLER:alive\r\n");
    }
    HAL_Delay(250U);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
