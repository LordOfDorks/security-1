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
#include "dice.h"
#define DEFAULT_FILE_HANDLE_STDOUT (1)
#define DEFAULT_FILE_HANDLE_STDIN (2)
#define DEFAULT_FILE_HANDLE_STDERR (3)
struct __FILE { int handle; };

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
RNG_HandleTypeDef hrng;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
CRYP_HandleTypeDef hcryp ={0};
FIREWALL_InitTypeDef fw_init = {0};
FILE __stdout = {DEFAULT_FILE_HANDLE_STDOUT};
FILE __stdin = {DEFAULT_FILE_HANDLE_STDIN};
FILE __stderr = {DEFAULT_FILE_HANDLE_STDERR};
uint8_t WipeIdentityAndPayloadHeader = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_RNG_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
extern void AppKicker(void);

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    if(f->handle == DEFAULT_FILE_HANDLE_STDOUT)
    {
//        while(CDC_Transmit_FS((uint8_t*)&ch, sizeof(uint8_t)) == USBD_BUSY);
//        return ch;
    }
    else if(f->handle == DEFAULT_FILE_HANDLE_STDERR)
    {
        while(HAL_UART_Transmit(&huart2, (uint8_t*)&ch, sizeof(uint8_t), HAL_MAX_DELAY) == HAL_BUSY);
        return ch;
    }
    return -1;
}

int fgetc(FILE *f)
{
  if(f->handle == DEFAULT_FILE_HANDLE_STDIN)
  {
//      while((!CDC_RX_Data) && (CDC_DTR));
//      if(CDC_DTR)
//      {
//          int ch = *CDC_RX_Data;
//          CDC_RX_Data = NULL;
//          return ch;
//      }
  }
  return -1;
}

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t appPayloadInvalid = 0;
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
//  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN 2 */
	if(WipeIdentityAndPayloadHeader != 0)
	{
		  uint32_t PageError = 0;
      FLASH_EraseInitTypeDef erase = {FLASH_TYPEERASE_PAGES, DICE_PROTECTED_PERSISTED_DATA_ADDRESS, 3};
      if((HAL_FLASH_Unlock() != HAL_OK) ||
				 (HAL_FLASHEx_Erase(&erase, &PageError) != HAL_OK) ||
			   (HAL_FLASH_Lock() != HAL_OK))
      {
				fprintf(&__stderr, "WipeIdentityAndPayloadHeader failed.\r\n");
				for(;;);
      }
			WipeIdentityAndPayloadHeader = 0;
	}
  DiceInitialize();

  // Secure the platform before we interact with the outside world
  DiceDumpProtectedData(&__stderr);
  if(DiceDeriveData() != HAL_OK)
  {
      fprintf(&__stderr, "DiceDeriveData() failed.\r\n");
      for(;;);
  }
  DiceDumpAppInfo(&__stderr);
  DiceVolatileData(&__stderr);
  appPayloadInvalid = (DiceAuthorizeAppPayload() != HAL_OK);
  fprintf(&__stderr, "App payload is *%s*\r\n", (!appPayloadInvalid) ? "AUTHORIZED" : "INVALID");
  if(DiceLockOutPersistedData() != HAL_OK)
  {
      fprintf(&__stderr, "DiceLockOutPersistedData() failed.\r\n");
      for(;;);
  }
  fprintf(&__stderr, "Persisted device data is *%s*\r\n", (diceVolatileData->firewallArmed != 0) ? "SECURED" : "UNINITIALIZED");

  // Is USB DFU mode needed/requested?
  if((appPayloadInvalid) ||
     (HAL_GPIO_ReadPin(FWBut_GPIO_Port, FWBut_Pin) == GPIO_PIN_SET))
  {
    fprintf(&__stderr, "==== Starting USB DFU Mode ==========================================\r\n");
    fprintf(&__stderr, "Scrubing derived identities from memory...\r\n");
    memset(diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS], 0x00, sizeof(diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS]));
#ifdef DICE_INCLUDE_MANUFACTURERID
    memset(diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS], 0x00, sizeof(diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS]));
#endif
#ifdef DICE_INCLUDE_OPERATORID
    memset(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS], 0x00, sizeof(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]));
#endif
    fprintf(&__stderr, "Starting USB DFU stack...\r\n");
    MX_USB_DEVICE_Init();
    fprintf(&__stderr, "Reboot to exit DFU...\r\n");
    for(;;)
    {
      // Only a reset gets us out from here
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
      HAL_Delay(100);
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
      HAL_Delay(100);
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
      HAL_Delay(100);
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
      HAL_Delay(750);
    }
  }

  fprintf(&__stderr, "==== Action: Application Launch =====================================\r\n");
  // Transition to app payload
  fprintf(&__stderr, "DeInitializing HAL\r\n");
  // Release the clock source
  HAL_RCC_DeInit();
  // Release the HAL
  HAL_DeInit();
  // Redirect the vector table to the application vector table
  SCB->VTOR = DICE_APP_PAYLOAD_ADDRESS;
  // Wipe all RAM after the identity table to make sure we didn't leave anything around
  for(uint32_t n = 0; n < 0x4F00; n++) ((uint8_t*)0x20000100)[n] = 0x00;
  // Set the stackpointer to where the application expects it
  __set_MSP(*((uint32_t*)DICE_APP_PAYLOAD_ADDRESS));
  // Jump into the abyss
  ((void (*)(void)) *((uint32_t*)(DICE_APP_PAYLOAD_ADDRESS + sizeof(uint32_t))))();
  // We will never return from this call!!
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // We should never get here...
  while (1)
  {
  /* USER CODE END WHILE */
  /* USER CODE BEGIN 3 */
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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
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

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
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

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : FWBut_Pin */
  GPIO_InitStruct.Pin = FWBut_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(FWBut_GPIO_Port, &GPIO_InitStruct);

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
