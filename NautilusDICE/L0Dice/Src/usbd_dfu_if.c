/**
  ******************************************************************************
  * @file           : usbd_dfu_if.c
  * @brief          :
  ******************************************************************************
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
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
#include "usbd_dfu_if.h"
/* USER CODE BEGIN INCLUDE */
#include "Dice.h"
extern RNG_HandleTypeDef hrng;
/* USER CODE END INCLUDE */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */
/** @defgroup USBD_DFU 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_DFU_Private_TypesDefinitions
  * @{
  */ 
/* USER CODE BEGIN PRIVATE_TYPES */
/* USER CODE END PRIVATE_TYPES */ 
/**
  * @}
  */ 

/** @defgroup USBD_DFU_Private_Defines
  * @{
  */ 
//#define FLASH_DESC_STR "@DiceFlash/0x08000000/63*001Ka,01*256Bc,01*256Bd,32*016Be,32*004Kg/0x08080000/96*064Bg"
/* USER CODE BEGIN PRIVATE_DEFINES */
uint8_t flashDescrStr[512];

#define FLASH_ERASE_TIME    (uint16_t)50
#define FLASH_PROGRAM_TIME  (uint16_t)50

void GenerateDfuDescriptionString(void)
{
    uint32_t cursor = 0;
    cursor += sprintf((char*)&flashDescrStr[cursor], "@Dice");
    if(diceVolatileData->firewallArmed == 0)
    {
        cursor += sprintf((char*)&flashDescrStr[cursor], "/0x%08x/01*%03dBd", DICE_PROTECTED_PERSISTED_DATA_ADDRESS, DICE_PROTECTED_PERSISTED_DATA_LEN);
    }
    cursor += sprintf((char*)&flashDescrStr[cursor], "/0x%08x/01*128Bg,%02d*004Kg", DICE_APP_PAYLOAD_INFO_ADDRESS, (DICE_APP_PAYLOAD_LEN / 4096));
}
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */ 

/** @defgroup USBD_DFU_Private_Macros
  * @{
  */ 
/* USER CODE BEGIN PRIVATE_MACRO */
/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */ 

/** @defgroup USBD_AUDIO_IF_Private_Variables
  * @{
  */
/* USER CODE BEGIN PRIVATE_VARIABLES */
/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */ 
  
/** @defgroup USBD_DFU_IF_Exported_Variables
  * @{
  */ 
  extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE BEGIN EXPORTED_VARIABLES */
/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */ 
  
/** @defgroup USBD_DFU_Private_FunctionPrototypes
  * @{
  */
static uint16_t MEM_If_Init_FS(void);
static uint16_t MEM_If_Erase_FS (uint32_t Add);
static uint16_t MEM_If_Write_FS (uint8_t *src, uint8_t *dest, uint32_t Len);
static uint8_t *MEM_If_Read_FS  (uint8_t *src, uint8_t *dest, uint32_t Len);
static uint16_t MEM_If_DeInit_FS(void);
static uint16_t MEM_If_GetStatus_FS (uint32_t Add, uint8_t Cmd, uint8_t *buffer);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */ 
  
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4   
#endif
__ALIGN_BEGIN USBD_DFU_MediaTypeDef USBD_DFU_fops_FS __ALIGN_END =
{
   (uint8_t*)flashDescrStr,
    MEM_If_Init_FS,
    MEM_If_DeInit_FS,
    MEM_If_Erase_FS,
    MEM_If_Write_FS,
    MEM_If_Read_FS,
    MEM_If_GetStatus_FS,   
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  MEM_If_Init_FS
  *         Memory initialization routine.
  * @param  None
  * @retval 0 if operation is successful, MAL_FAIL else.
  */
uint16_t MEM_If_Init_FS(void)
{ 
  /* USER CODE BEGIN 0 */
  uint16_t result = USBD_FAIL;
  if(HAL_FLASH_Unlock() == HAL_OK)
  {
    result = USBD_OK;
  }
  fprintf(&__stderr, "MEM_If_Init_FS() = %s\r\n", (result==USBD_OK) ? "USBD_OK" : "USBD_FAIL");
  return result;
  /* USER CODE END 0 */ 
}

/**
  * @brief  MEM_If_DeInit_FS
  *         De-Initializes Memory.
  * @param  None
  * @retval 0 if operation is successful, MAL_FAIL else.
  */
uint16_t MEM_If_DeInit_FS(void)
{ 
  /* USER CODE BEGIN 1 */
  uint16_t result = USBD_FAIL;
  if(HAL_FLASH_Lock() == HAL_OK)
  {
    result = USBD_OK;
  }
  fprintf(&__stderr, "MEM_If_DeInit_FS() = %s\r\n", (result==USBD_OK) ? "USBD_OK" : "USBD_FAIL");
  return result;
  /* USER CODE END 1 */ 
}

/**
  * @brief  MEM_If_Erase_FS
  *         Erase sector.
  * @param  Add: Address of sector to be erased.
  * @retval 0 if operation is successful, MAL_FAIL else.
  */
uint16_t MEM_If_Erase_FS(uint32_t Add)
{
  /* USER CODE BEGIN 2 */
  uint16_t result = USBD_FAIL;

  if(IS_APP_PAYLOAD_INFO(Add))
  {
      uint32_t PageError = 0;
      FLASH_EraseInitTypeDef erase = {FLASH_TYPEERASE_PAGES, Add, 1}; // One 128byte page
      if(HAL_FLASHEx_Erase(&erase, &PageError) == HAL_OK)
      {
          result = USBD_OK;
          goto Cleanup;
      }
  }
  else if(IS_APP_PAYLOAD(Add))
  {
      uint32_t PageError = 0;
      FLASH_EraseInitTypeDef erase = {FLASH_TYPEERASE_PAGES, Add, 32}; // One 4K Sector is 32 pages
      if(HAL_FLASHEx_Erase(&erase, &PageError) == HAL_OK)
      {
          result = USBD_OK;
          goto Cleanup;
      }
  }
//  else if(IS_DATAEEPROM(Add))
//  {
//      for(uint32_t n = 0; n < 64; n++) // One sector is 64byte
//      {
//        if(HAL_FLASHEx_DATAEEPROM_Erase(Add + n) != HAL_OK)
//        {
//            goto Cleanup;
//        }
//      }
//      result = USBD_OK;
//  }

Cleanup:
  fprintf(&__stderr, "MEM_If_Erase_FS(0x%08x) = %s\r\n", Add, (result==USBD_OK) ? "USBD_OK" : "USBD_FAIL");
  return result;
  /* USER CODE END 2 */ 
}

/**
  * @brief  MEM_If_Write_FS
  *         Memory write routine.
  * @param  src: Pointer to the source buffer. Address to be written to.
  * @param  dest: Pointer to the destination buffer.
  * @param  Len: Number of data to be written (in bytes).
  * @retval 0 if operation is successful, MAL_FAIL else.
  */
uint16_t MEM_If_Write_FS(uint8_t *src, uint8_t *dest, uint32_t Len)
{
  /* USER CODE BEGIN 3 */ 
  uint16_t result = USBD_FAIL;

  if(IS_PROTECTED_PERSISTED_DATA((unsigned int)dest))
  {
      PDICE_PROTECTED_PERSISTED_DATA_CONTAINER diceInfo = (PDICE_PROTECTED_PERSISTED_DATA_CONTAINER)src;
      if((diceVolatileData->firewallArmed == 0) && (diceInfo->tag == DICE_PROTECTED_PERSISTED_DATA_TAG))
      {
          uint8_t copyData = 0;
          
          // This is only for the development phase so we can overwrite a provisioned device
          // We definitely do not want this code path in the retail version
          uint32_t PageError = 0;
          FLASH_EraseInitTypeDef erase = {FLASH_TYPEERASE_PAGES, (uint32_t)dest, 2};
          do
          {
            HAL_FLASHEx_Erase(&erase, &PageError);
          }
          while(PageError != 0xffffffff);

          // Write the first part unconditionally
          for(uint32_t i = 0; i < Len - (DICE_DIGEST_SIZE * 3); i += 4)
          {
              if((HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(&dest[i]), *((uint32_t*)(&src[i]))) != HAL_OK) ||
                 (*(uint32_t*)(&src[i]) != *(uint32_t*)(&dest[i])))
              {
                  goto Cleanup;
              }
          }

          // Write the deviceID
          copyData = !DiceChkZero(diceInfo->deviceID, sizeof(diceInfo->deviceID));
          for(uint32_t i = 0; i < sizeof(diceInfo->deviceID); i += 4)
          {
              uint32_t dataWord = *((uint32_t*)(&diceInfo->deviceID[i]));

              // Are we making up a random device ID from the internal RNG?
              if((!copyData) &&
                 (HAL_RNG_GenerateRandomNumber(&hrng, &dataWord) != HAL_OK))
              {
                  goto Cleanup;
              }
              if((HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(&diceProtectedData->deviceID[i]), dataWord) != HAL_OK) ||
                 (dataWord != *(uint32_t*)(&diceProtectedData->deviceID[i])))
              {
                  goto Cleanup;
              }
          }
          
#ifdef DICE_INCLUDE_MANUFACTURERID
          // Write the deviceID if specified otherwise leave writable
          if(!DiceChkZero(diceInfo->manufacturerID, sizeof(diceInfo->manufacturerID)))
          {
              for(uint32_t i = 0; i < sizeof(diceInfo->manufacturerID); i += 4)
              {
                  if((HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(&diceProtectedData->manufacturerID[i]), *((uint32_t*)(&diceInfo->manufacturerID[i]))) != HAL_OK) ||
                     (*((uint32_t*)(&diceInfo->manufacturerID[i])) != *((uint32_t*)(&diceProtectedData->manufacturerID[i]))))
                  {
                      goto Cleanup;
                  }
              }
          }
#endif

          // Write the applicationName if specified otherwise leave writable
          if(!DiceChkZero(diceInfo->softwareStrongName, sizeof(diceInfo->softwareStrongName)))
          {
              for(uint32_t i = 0; i < sizeof(diceInfo->softwareStrongName); i += 4)
              {
                  if((HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(&diceProtectedData->softwareStrongName[i]), *((uint32_t*)(&diceInfo->softwareStrongName[i]))) != HAL_OK) ||
                     (*((uint32_t*)(&diceInfo->softwareStrongName[i])) != *((uint32_t*)(&diceProtectedData->softwareStrongName[i]))))
                  {
                      goto Cleanup;
                  }
              }
          }

          // ToDo: Lock the flashlock bits for the two pages so it can be never overwritten
      }
      result = USBD_OK;
  }
  else if((IS_APP_PAYLOAD_INFO((unsigned int)dest)) ||
          (IS_APP_PAYLOAD((unsigned int)dest)))
  {
    for(uint32_t i = 0; i < Len; i += 4)
    {
        if((HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(dest + i), *(uint32_t*)(src + i)) != HAL_OK) ||
           (*(uint32_t *)(&src[i]) != *(uint32_t*)(&dest[i])))
        {
            goto Cleanup;
        }
    }
    result = USBD_OK;
  }
//  else if(IS_DATAEEPROM((unsigned int)dest))
//  {
//    for(uint32_t i = 0; i < Len; i += 4)
//    {
//      if((HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(dest + i), *(uint32_t*)(src + i)) != HAL_OK) ||
//         (*(uint32_t *)(&src[i]) != *(uint32_t*)(&dest[i])))
//      {
//          goto Cleanup;
//      }
//    }
//    result = USBD_OK;
//  }

Cleanup:
  fprintf(&__stderr, "MEM_If_Write_FS(0x%08x, 0x%08x, %d) = %s\r\n", (unsigned int)src, (unsigned int)dest, Len, (result==USBD_OK) ? "USBD_OK" : "USBD_FAIL");
  return result;
  /* USER CODE END 3 */ 
}

/**
  * @brief  MEM_If_Read_FS
  *         Memory read routine.
  * @param  src: Pointer to the source buffer. Address to be written to.
  * @param  dest: Pointer to the destination buffer.
  * @param  Len: Number of data to be read (in bytes).
  * @retval Pointer to the physical address where data should be read.
  */
uint8_t *MEM_If_Read_FS (uint8_t *src, uint8_t *dest, uint32_t Len)
{
  /* Return a valid address to avoid HardFault */
  /* USER CODE BEGIN 4 */ 
  uint16_t result = USBD_FAIL;

  if((IS_APP_PAYLOAD_INFO((unsigned int)src)) ||
     (IS_APP_PAYLOAD((unsigned int)src)))
  {
    memcpy(dest, src, Len);
    result = USBD_OK;
  }
  printf("MEM_If_Read_FS(0x%08x, 0x%08x, %d) = %s\r\n", (unsigned int)src, (unsigned int)dest, Len, (result==USBD_OK) ? "USBD_OK" : "USBD_FAIL");
  return (uint8_t*)(USBD_OK);
  /* USER CODE END 4 */ 
}

/**
  * @brief  Flash_If_GetStatus_FS
  *         Get status routine.
  * @param  Add: Address to be read from.
  * @param  Cmd: Number of data to be read (in bytes).
  * @param  buffer: used for returning the time necessary for a program or an erase operation
  * @retval 0 if operation is successful
  */
uint16_t MEM_If_GetStatus_FS(uint32_t Add, uint8_t Cmd, uint8_t *buffer)
{
  /* USER CODE BEGIN 5 */ 
  switch (Cmd)
  {
  case DFU_MEDIA_PROGRAM:
    buffer[1] = (uint8_t)FLASH_PROGRAM_TIME;
    buffer[2] = (uint8_t)(FLASH_PROGRAM_TIME << 8);
    buffer[3] = 0;  
    break;
    
  case DFU_MEDIA_ERASE:
  default:
    buffer[1] = (uint8_t)FLASH_ERASE_TIME;
    buffer[2] = (uint8_t)(FLASH_ERASE_TIME << 8);
    buffer[3] = 0;
    break;
  } 
  fprintf(&__stderr, "MEM_If_GetStatus_FS(0x%08x, %d, 0x%08x)\r\n", Add, Cmd, (unsigned int)buffer);
  return  (USBD_OK);
  /* USER CODE END 5 */  
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */
/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */ 

/**
  * @}
  */  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

