/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "usb_device.h"

/* USER CODE BEGIN Includes */
#include <time.h>
#include "usbd_cdc_if.h"
#include "dice.h"

#define DEFAULT_FILE_HANDLE_STDOUT (1)
#define DEFAULT_FILE_HANDLE_STDIN (2)
#define DEFAULT_FILE_HANDLE_STDERR (3)
struct __FILE { int handle; };

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
RNG_HandleTypeDef hrng;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
CRYP_HandleTypeDef hcryp = {0};
FIREWALL_InitTypeDef fw_init = {0};
FILE __stdout = {DEFAULT_FILE_HANDLE_STDOUT};
FILE __stdin = {DEFAULT_FILE_HANDLE_STDIN};
FILE __stderr = {DEFAULT_FILE_HANDLE_STDERR};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_RNG_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    if(f->handle == DEFAULT_FILE_HANDLE_STDOUT)
    {
        while(CDC_Transmit_FS((uint8_t*)&ch, sizeof(uint8_t)) == USBD_BUSY);
    }
    else if(f->handle == DEFAULT_FILE_HANDLE_STDERR)
    {
        while(HAL_UART_Transmit(&huart2, (uint8_t*)&ch, sizeof(uint8_t), HAL_MAX_DELAY) == HAL_BUSY);
    }
    return ch;
}

int fgetc(FILE *f)
{
  uint8_t ch;
  if(f->handle == DEFAULT_FILE_HANDLE_STDIN)
  {
      while((CDC_RX_Data == NULL) && (CDC_DTR))
      {
          HAL_Delay(0);
      };
      if(CDC_DTR)
      {
          ch = *CDC_RX_Data;
          CDC_RX_Data = NULL;
          fputc(ch, &__stdout); // Echo
          if(ch == '\r') fputc('\n', &__stdout);
      }
      else
      {
          return -1;
      }
  }
  return ch;
}

HAL_StatusTypeDef GetTimeOfDay(time_t* rawTime)
{
    HAL_StatusTypeDef retVal = HAL_OK;
    RTC_DateTypeDef date = {0};
    RTC_TimeTypeDef time = {0};

    if(((retVal = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN)) == HAL_OK) &&
       ((retVal = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN)) == HAL_OK))
    {
        struct tm timeInfo = {0};
        timeInfo.tm_sec = time.Seconds;
        timeInfo.tm_min = time.Minutes;
        timeInfo.tm_hour = time.Hours;
        timeInfo.tm_mday = date.Date - 1;
        timeInfo.tm_wday = date.WeekDay;
        timeInfo.tm_mon = date.Month - 1; // January = 0
        timeInfo.tm_year = date.Year + 100; // Base year is 1900
        *rawTime = mktime(&timeInfo);
    }
    return retVal;
}

HAL_StatusTypeDef SetTimeOfDay(time_t* rawTime)
{
    HAL_StatusTypeDef retVal = HAL_OK;
    RTC_DateTypeDef date = { 0 };
    RTC_TimeTypeDef time = { 0 };
    struct tm* timeInfo = NULL;

    timeInfo = localtime(rawTime);
    time.Seconds = timeInfo->tm_sec;
    time.Minutes = timeInfo->tm_min;
    time.Hours = timeInfo->tm_hour;
    date.WeekDay = timeInfo->tm_wday;
    date.Date = timeInfo->tm_mday + 1; // mday 0 is the first
    date.Month = timeInfo->tm_mon + 1; // January = 0
    date.Year = timeInfo->tm_year - 100; // Base year is 1900

    if(((retVal = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN)) != HAL_OK) ||
       ((retVal = HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN)) != HAL_OK))
    {
        goto Cleanup;
    }

Cleanup:
    return retVal;
}

void Morse(char* dataStr)
{
    const uint16_t morseCode[] = {
        0x0201, // A *-
        0x040E, // B -***
        0x040A, // C -*-*
        0x0306, // D -**
        0x0101, // E *
        0x040B, // F **-*
        0x0304, // G --*
        0x040F, // H ****
        0x0203, // I **
        0x0401, // J *----
        0x0302, // K -*-
        0x040D, // L *-**
        0x0200, // M --
        0x0202, // N -*
        0x0300, // O ---
        0x0409, // P *--*
        0x0404, // Q --*-
        0x0302, // R *-*
        0x0307, // S ***
        0x0100, // T -
        0x0302, // U **-
        0x0407, // V ***-
        0x0301, // W *--
        0x0406, // X -**-
        0x0402, // Y -*--
        0x040C, // Z --**
        0x0500, // 0 -----
        0x0501, // 1 *----
        0x0503, // 2 **---
        0x0507, // 3 ***--
        0x050F, // 4 ****-
        0x051F, // 5 *****
        0x051E, // 6 -****
        0x051C, // 7 --***
        0x0518, // 8 ---**
        0x0510};// 9 ----*

    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    for(uint8_t n = 0; n < strlen(dataStr); n++)
    {
        uint32_t cursor = 0;
        char send = dataStr[n];
        if((send >= 'a') && (send <= 'z')) send -= 'a' - 'A';
        if((send >= 'A') && (send <= 'Z'))
        {
            cursor = send - 'A';
        }
        else if((send >= '0') && (send <= '9'))
        {
            cursor = send - '0' + 26;
        }
        else
        {
            return;
        }
        if(n > 0) HAL_Delay(750);
        for(uint32_t m = 0; m < (morseCode[cursor] >> 8); m++)
        {
            if(m > 0) HAL_Delay(250);
            HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
            if(((morseCode[cursor] & 0x00FF) >> m) & 0x01)
            {
                HAL_Delay(100);
            }
            else
            {
                HAL_Delay(500);
            }
            HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        }
    }
}

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
    HAL_StatusTypeDef retVal = HAL_OK;
    time_t clockLast = 0;
    time_t* persistedTime = (time_t*)0x08080000;
    char userinput[256];
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RNG_Init();
  MX_USART2_UART_Init();
  MX_USB_DEVICE_Init();
  MX_RTC_Init();

  /* USER CODE BEGIN 2 */
  fprintf(&__stderr, "#####################################################################\r\n");
  fprintf(&__stderr, "##  BlinkyBlink running @ 0x%08x\r\n", SCB->VTOR);
  fprintf(&__stderr, "#####################################################################\r\n");
  DiceDumpProtectedData(&__stderr);
  DiceDumpAppInfo(&__stderr);
  DiceVolatileData(&__stderr);
  SetTimeOfDay(persistedTime);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    struct tm* timeInfo = NULL;
    time_t clockNow = {0};
    do
    {
        HAL_Delay(0);
        retVal = GetTimeOfDay(&clockNow);
    }
    while((retVal != HAL_OK) || (clockNow == clockLast));
    clockLast = clockNow;
    timeInfo = localtime(&clockLast);
    fprintf(&__stderr, "[%02d.%02d.%02d-%02d:%02d:%02d]", 1900 + timeInfo->tm_year, timeInfo->tm_mon, timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    fprintf(&__stderr, "GPIO_LD3: %s\r", (HAL_GPIO_ReadPin(LD3_GPIO_Port, LD3_Pin) == GPIO_PIN_SET) ? "ON " : "OFF");

    if(CDC_DTR)
    {
      DiceVolatileData(&__stdout);
      printf("Welcome to BlinkyBlink!\r\n");
      
      GetTimeOfDay(&clockLast);
      printf("New filetime (current %u) or N: ", clockLast);
      if(scanf("%d", &clockLast) > 0)
      {
          printf("Setting: 0x%08x\r\n", clockLast);
          if(SetTimeOfDay(&clockLast) == HAL_OK)
          {
              do
              {
                if(GetTimeOfDay(&clockNow) != HAL_OK)
                {
                    break;
                }
              }
              while(clockNow < clockLast);
              printf("SystemTime: %s\r", asctime(localtime((time_t*)&clockNow)));
          }

          // Persist that time
          if((HAL_FLASHEx_DATAEEPROM_Unlock() == HAL_OK) &&
             (HAL_FLASHEx_DATAEEPROM_Erase((uint32_t)persistedTime) == HAL_OK) &&
             (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)persistedTime, (uint32_t)clockLast) == HAL_OK) &&
             (HAL_FLASHEx_DATAEEPROM_Lock() == HAL_OK))
          {
              printf("TimeStamp persisted.\r\n");
          }
          clockLast = clockNow;
      }
      else
      {
          printf("\r\nSkipping clock set.\r\n");
      }
      do
      {
          while(GetTimeOfDay(&clockLast) != HAL_OK) HAL_Delay(10);
          printf("Time: %s\r", asctime(localtime((time_t*)&clockLast)));
          printf("Input: ");
          if(scanf("%s", userinput) > 0)
          {
              printf("Morseing '%s' with LD3...\r\n", userinput);
              Morse(userinput);
          }
      }
      while(CDC_DTR);
    }
  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_RTC
                              |RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* RNG init function */
static void MX_RNG_Init(void)
{

  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }

}

/* RTC init function */
static void MX_RTC_Init(void)
{

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  RTC_AlarmTypeDef sAlarm;

    /**Initialize RTC and set the Time and Date 
    */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

    /**Enable the Alarm A 
    */
  sAlarm.AlarmTime.Hours = 0x0;
  sAlarm.AlarmTime.Minutes = 0x0;
  sAlarm.AlarmTime.Seconds = 0x0;
  sAlarm.AlarmTime.SubSeconds = 0x0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_HalfDuplex_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
