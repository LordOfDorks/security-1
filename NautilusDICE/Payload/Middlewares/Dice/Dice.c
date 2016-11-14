#include <string.h>
#include <time.h>
#include "stm32l0xx_hal.h"
#include "Dice.h"

PDICE_VOLATILE_DATA_TABLE diceVolatileData = (PDICE_VOLATILE_DATA_TABLE)DICE_VOLATILE_DATA_TABLE_ADDRESS;
PDICE_PROTECTED_PERSISTED_DATA_CONTAINER diceProtectedData = (PDICE_PROTECTED_PERSISTED_DATA_CONTAINER)DICE_PROTECTED_PERSISTED_DATA_ADDRESS;
PDICE_APP_PAYLOAD_INFO diceAppPayloadInfo = (PDICE_APP_PAYLOAD_INFO) DICE_APP_PAYLOAD_INFO_ADDRESS;

void DiceInitialize(void)
{
    if(diceVolatileData->tag == DICE_MAILBOX_COMMAND_TAG)
    {
        // TODO: Validate and execute requested mailbox OPS afterwards sanitize and initialize area
    }
    else if (diceVolatileData->tag == DICE_VOLATILE_DATA_TAG)
    {
        // Already initialized
        return;
    }
    // Initialize area
    memset(diceVolatileData, 0x00, sizeof(DICE_VOLATILE_DATA_TABLE));
    diceVolatileData->tag = DICE_VOLATILE_DATA_TAG;
}

uint8_t DiceChkInitialized(uint8_t* dataPtr, uint8_t dataByte, uint32_t dataSize)
{
    for(uint32_t n = 0; n < dataSize; n++)
    {
        if(dataPtr[n] != dataByte)
        {
            return 0;
        }
    }
    return 1;
}

#define DiceChkZero(__ptr, __size) DiceChkInitialized(__ptr, 0x00, __size)
#define DiceChkFF(__ptr, __size) DiceChkInitialized(__ptr, 0xFF, __size)

void DicePrintData(FILE* f, char* label, uint8_t* dataPtr, uint32_t dataSize)
{
    uint32_t dataCursor = 0;
    uint32_t outputCursor = 0;
    uint32_t leadingSpaces = 0;
    char row[16 * 6] = {0};
    sprintf(row, "%s[%d]: ", label, dataSize);
    leadingSpaces = strlen(row);
    fprintf(f, "%s", row);
    while(dataSize > dataCursor)
    {
        if(dataCursor > 0)
        {
            for(uint32_t n = 0; n < leadingSpaces; n++) fprintf(f, " ");
        }
        for(uint32_t n = 0; n < MIN(dataSize, 16); n++)
        {
            outputCursor += sprintf(&row[outputCursor], "%02x", dataPtr[dataCursor++]);
        }
        fprintf(f, "%s\r\n", row);
        memset(row, 0x00, sizeof(row));
    }
}

//void DicePrintData(char* label, uint8_t* dataPtr, uint32_t dataSize)
//{
//    uint32_t dataCursor = 0;
//    uint32_t outputCursor = 0;
//    char row[16 * 6] = {0};
//    printf("uint8_t %s[%d] = {\r\n", label, dataSize);
//    while(dataSize > dataCursor)
//    {
//        for(uint32_t n = 0; n < MIN(dataSize, 16); n++)
//        {
//            outputCursor += sprintf(&row[outputCursor], "0x%02x, ", dataPtr[dataCursor++]);
//        }
//        if(dataSize == dataCursor)
//        {
//            row[outputCursor - 2] = 0;
//            row[outputCursor - 1] = 0;
//        }
//        printf("%s\r\n", row);
//        memset(row, 0x00, sizeof(row));
//    }
//    printf("};\r\n");
//}

void DiceDumpProtectedData(FILE* f)
{
    if(diceVolatileData->firewallArmed == 0)
    {
        fprintf(f, "---- ProtectedData ------------------------------------------------\r\n");
        if(diceProtectedData->tag == DICE_PROTECTED_PERSISTED_DATA_TAG)
        {
            fprintf(f, "Tag:            %c%c%c%c\r\n",
                    (diceProtectedData->tag & 0x000000ff),
                    ((diceProtectedData->tag >> 8) & 0x000000ff),
                    ((diceProtectedData->tag >> 16) & 0x000000ff),
                    ((diceProtectedData->tag >> 24) & 0x000000ff));
            fprintf(f, "Size:           %d\r\n", diceProtectedData->size);
            fprintf(f, "Version:        %d.%d\r\n", 
                    ((diceProtectedData->version >> 16) & 0x0000ffff),
                    (diceProtectedData->version & 0x0000ffff));
            fprintf(f, "Date:           %s\r", asctime(localtime((time_t*)&diceProtectedData->date)));
            fprintf(f, "Model:          %s\r\n", diceProtectedData->model);
            fprintf(f, "Serial:         %s\r\n", diceProtectedData->serial);
            fprintf(f, "Manufacturer:   %s\r\n", diceProtectedData->manufacturer);
            fprintf(f, "URL:            %s\r\n", diceProtectedData->url);
            if(!DiceChkZero(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)) &&
               !DiceChkFF(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)))
            {
                fprintf(f, "ManufacturerID: LOCKED\r\n");
            }
            else
            {
                fprintf(f, "ManufacturerID: UNLOCKED\r\n");
            }
            if(!DiceChkZero(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)) &&
               !DiceChkFF(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)))
            {
                fprintf(f, "AppStrongName:  LOCKED\r\n");
            }
            else
            {
                fprintf(f, "AppStrongName:  UNLOCKED\r\n");
            }
        }
        else
        {
            printf("DEVICE UNITIALIZED!\r\n");
        }
        fprintf(f, "-------------------------------------------------------------------\r\n");
    }
    else
    {
        fprintf(f, "XXXX ProtectedData: LOCKED XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n");
    }
}

void DiceDumpAppInfo(FILE* f)
{
    fprintf(f, "---- ApplicationInfo ----------------------------------------------\r\n");
    if(diceAppPayloadInfo->tag == DICE_APPPAYLOAD_INFO_TAG)
    {
        fprintf(f, "         Tag: %c%c%c%c\r\n",
                (diceAppPayloadInfo->tag & 0x000000ff),
                ((diceAppPayloadInfo->tag >> 8) & 0x000000ff),
                ((diceAppPayloadInfo->tag >> 16) & 0x000000ff),
                ((diceAppPayloadInfo->tag >> 24) & 0x000000ff));
        fprintf(f, "        Size: %d\r\n", diceAppPayloadInfo->appSize);
        fprintf(f, "     Version: %d.%d\r\n", 
                ((diceAppPayloadInfo->appVersion >> 16) & 0x0000ffff),
                (diceAppPayloadInfo->appVersion & 0x0000ffff));
        fprintf(f, "        Date: %s\r", asctime(localtime((time_t*)&diceAppPayloadInfo->appDate)));
        fprintf(f, "        Name: %s\r\n", diceAppPayloadInfo->appName);
        fprintf(f, "Manufacturer: %s\r\n", diceAppPayloadInfo->manufacturer);
        DicePrintData(f, "      strongName", diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName));
        DicePrintData(f, "manufacturerSalt", diceAppPayloadInfo->manufacturerSalt, sizeof(diceAppPayloadInfo->manufacturerSalt));
        DicePrintData(f, "   encOperatorId", diceAppPayloadInfo->encOperatorId, sizeof(diceAppPayloadInfo->encOperatorId));
        DicePrintData(f, "   authorization", diceAppPayloadInfo->authorization, sizeof(diceAppPayloadInfo->authorization));
    }
    else
    {
        printf("APPLICATION UNITIALIZED!\r\n");
    }
    fprintf(f, "-------------------------------------------------------------------\r\n");
}

void DiceVolatileData(FILE* f)
{
    fprintf(f, "---- VolatileData -------------------------------------------------\r\n");
    DicePrintData(f, "          DeviceRootName", diceVolatileData->info.tables.diceRootNames[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.tables.diceRootNames[DICE_DEVICE_CLASS]));
//    DicePrintData(f, "        DeviceCompoundId", diceVolatileData->info.tables.diceCompoundIds[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundIds[DICE_DEVICE_CLASS]));
    DicePrintData(f, "      DeviceCompoundName", diceVolatileData->info.tables.diceCompoundNames[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_DEVICE_CLASS]));
    DicePrintData(f, "    ManufacturerRootName", diceVolatileData->info.tables.diceRootNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.tables.diceRootNames[DICE_MANUFACTURER_CLASS]));
//    DicePrintData(f, "  ManufacturerCompoundId", diceVolatileData->info.tables.diceCompoundIds[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundIds[DICE_MANUFACTURER_CLASS]));
    DicePrintData(f, "ManufacturerCompoundName", diceVolatileData->info.tables.diceCompoundNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_MANUFACTURER_CLASS]));
    DicePrintData(f, "           AppStrongName", diceVolatileData->info.tables.diceRootNames[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.tables.diceRootNames[DICE_OPERATOR_CLASS]));
//    DicePrintData(f, "              OperatorId", diceVolatileData->info.tables.diceCompoundIds[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundIds[DICE_OPERATOR_CLASS]));
    DicePrintData(f, "            OperatorName", diceVolatileData->info.tables.diceCompoundNames[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_OPERATOR_CLASS]));
    fprintf(f, "-------------------------------------------------------------------\r\n");
}

HAL_StatusTypeDef DiceCalculateCMAC(uint8_t encrypt, uint8_t* dataPtr, uint32_t dataSize, uint8_t* keyIn, uint8_t* digestIn, uint8_t* digestOut)
{
    HAL_StatusTypeDef retVal = HAL_OK;
    uint8_t iv[4 * sizeof(uint32_t)] = {0};
    uint8_t key[4 * sizeof(uint32_t)] = {0};
    uint32_t aesOutput = 0;
    
    if((dataSize % 16) != 0)
    {
        retVal = HAL_ERROR;
        goto Cleanup;
    }

    // Setup the engine
    HAL_CRYP_DeInit(&hcryp);
    hcryp.Instance = AES;
    hcryp.Init.DataType = CRYP_DATATYPE_8B;
    hcryp.Init.pKey = (keyIn != NULL) ? keyIn : key;
    hcryp.Init.pInitVect = (digestIn != NULL) ? digestIn : iv;
    if((retVal = HAL_CRYP_Init(&hcryp)) != HAL_OK)
    {
        goto Cleanup;
    }

    if(dataSize == 16)
    {
        // For a single block just run the engine
        if(((encrypt) && ((retVal = HAL_CRYP_AESCBC_Encrypt(&hcryp, dataPtr, dataSize, digestOut, HAL_MAX_DELAY)) != HAL_OK)) ||
           ((!encrypt) && ((retVal = HAL_CRYP_AESCBC_Decrypt(&hcryp, dataPtr, dataSize, digestOut, HAL_MAX_DELAY)) != HAL_OK)))
        {
            goto Cleanup;
        }
    }
    else
    {
        // For multiple blocks use DMA
        if(((encrypt) && ((retVal = HAL_CRYP_AESCBC_Encrypt_DMA(&hcryp, dataPtr, dataSize, (uint8_t*)&aesOutput)) != HAL_OK)) ||
           ((!encrypt) && ((retVal = HAL_CRYP_AESCBC_Decrypt_DMA(&hcryp, dataPtr, dataSize, (uint8_t*)&aesOutput)) != HAL_OK)))
        {
            goto Cleanup;
        }
        while(HAL_CRYP_GetState(&hcryp) != HAL_CRYP_STATE_READY); // Wait for the DMA to complete

        // Retrieve the IV that contains the *last* output data block that aka. the CMAC
        ((uint32_t*)digestOut)[0] = hcryp.Instance->IVR0;
        ((uint32_t*)digestOut)[1] = hcryp.Instance->IVR1;
        ((uint32_t*)digestOut)[2] = hcryp.Instance->IVR2;
        ((uint32_t*)digestOut)[3] = hcryp.Instance->IVR3;
    }

    // Wipe the key from the engine
    hcryp.Instance->KEYR3 = 0;
    hcryp.Instance->KEYR2 = 0;
    hcryp.Instance->KEYR1 = 0;
    hcryp.Instance->KEYR0 = 0;
    HAL_CRYP_DeInit(&hcryp);

Cleanup:
    return retVal;
}

HAL_StatusTypeDef DiceDeriveData(void)
{
    HAL_StatusTypeDef retVal = HAL_OK;
    uint8_t operatorIdKey[16];
    if(diceProtectedData->tag == DICE_PROTECTED_PERSISTED_DATA_TAG)
    {
        // Calculate the DeviceName
        if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                       diceVolatileData->info.tables.diceRootNames[DICE_DEVICE_CLASS],
                                       sizeof(diceVolatileData->info.tables.diceRootNames[DICE_DEVICE_CLASS]),
                                       diceProtectedData->deviceID,
                                       NULL,
                                       diceVolatileData->info.tables.diceRootNames[DICE_DEVICE_CLASS])) != HAL_OK)
        {
            goto Cleanup;
        }

        // Calculate the ManufacturerName
        if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                       diceVolatileData->info.tables.diceRootNames[DICE_MANUFACTURER_CLASS],
                                       sizeof(diceVolatileData->info.tables.diceRootNames[DICE_MANUFACTURER_CLASS]),
                                       diceProtectedData->manufacturerID,
                                       NULL,
                                       diceVolatileData->info.tables.diceRootNames[DICE_MANUFACTURER_CLASS])) != HAL_OK)
        {
            goto Cleanup;
        }
        
        if(diceAppPayloadInfo->tag == DICE_APPPAYLOAD_INFO_TAG)
        {
            // Copy the SoftwareName
            memcpy(diceVolatileData->info.tables.diceRootNames[DICE_OPERATOR_CLASS],
                   diceAppPayloadInfo->strongName,
                   sizeof(diceVolatileData->info.tables.diceRootNames[DICE_OPERATOR_CLASS]));

            // Calculate the DeviceCompoundId
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceAppPayloadInfo->strongName,
                                           sizeof(diceAppPayloadInfo->strongName),
                                           diceProtectedData->deviceID,
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_DEVICE_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }
            
            // Calculate the ManufacturerCompoundId
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceAppPayloadInfo->manufacturerSalt,
                                           sizeof(diceAppPayloadInfo->manufacturerSalt),
                                           diceProtectedData->manufacturerID,
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_MANUFACTURER_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }
            
            // Decrypt the secret OperatorID
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceAppPayloadInfo->strongName,
                                           sizeof(diceAppPayloadInfo->strongName),
                                           diceProtectedData->manufacturerID,
                                           NULL,
                                           operatorIdKey)) != HAL_OK)
            {
                goto Cleanup;
            }
            if((retVal = DiceCalculateCMAC(DICE_CMAC_DECRYPT,
                                           diceAppPayloadInfo->encOperatorId,
                                           sizeof(diceAppPayloadInfo->encOperatorId),
                                           operatorIdKey,
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_OPERATOR_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }

            // Derive the DeviceCompoundName
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_DEVICE_CLASS],
                                           sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_DEVICE_CLASS]),
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_DEVICE_CLASS],
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_DEVICE_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }

            // Derive the ManfacturerCompoundName
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_MANUFACTURER_CLASS],
                                           sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_MANUFACTURER_CLASS]),
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_MANUFACTURER_CLASS],
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_MANUFACTURER_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }

            // Derive the SoftwareCompoundName
            if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_OPERATOR_CLASS],
                                           sizeof(diceVolatileData->info.tables.diceCompoundNames[DICE_OPERATOR_CLASS]),
                                           diceVolatileData->info.tables.diceCompoundIds[DICE_OPERATOR_CLASS],
                                           NULL,
                                           diceVolatileData->info.tables.diceCompoundNames[DICE_OPERATOR_CLASS])) != HAL_OK)
            {
                goto Cleanup;
            }
        }
    }
Cleanup:
    return retVal;
}

HAL_StatusTypeDef DiceLockOutPersistedData(void)
{
    HAL_StatusTypeDef retVal = HAL_OK;
    if((diceVolatileData->firewallArmed == 0) && (diceProtectedData->tag == DICE_PROTECTED_PERSISTED_DATA_TAG))
    {
        fw_init.NonVDataSegmentStartAddress = (uint32_t)diceProtectedData;
        fw_init.NonVDataSegmentLength = sizeof(DICE_PROTECTED_PERSISTED_DATA_CONTAINER);
        if((retVal = HAL_FIREWALL_Config(&fw_init)) == HAL_OK)
        {
            HAL_FIREWALL_EnableFirewall();
            diceVolatileData->firewallArmed = 1;
        }
    }

//Cleanup:
    return retVal;
}

HAL_StatusTypeDef DiceAuthorizeAppPayload(void)
{
    HAL_StatusTypeDef retVal = HAL_ERROR;
    uint8_t digest[DICE_DIGEST_SIZE] = {0};
    
    // Verify the strongName in the appInfo with the flashed application
    if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                   (uint8_t*)DICE_APP_PAYLOAD_ADDRESS,
                                   diceAppPayloadInfo->appSize,
                                   NULL,
                                   NULL,
                                   digest)) != HAL_OK)
    {
        goto Cleanup;
    }
    if(memcmp(digest, diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName)))
    {
        retVal = HAL_ERROR;
        goto Cleanup;
    }
    
    // If the device is locked to a particular applicationName ensure that this is it
    if((DiceChkZero(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)) != 0) ||
       (DiceChkFF(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)) != 0) ||
       (memcmp(diceProtectedData->softwareStrongName, diceAppPayloadInfo->strongName, sizeof(diceProtectedData->softwareStrongName))))
    {
        retVal = HAL_ERROR;
        goto Cleanup;
    }
    
    // Check if the integrity protection on the application package
    if((DiceChkZero(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)) != 0) ||
       (DiceChkFF(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)) != 0))
    {
        // Verify the application info integrity
        if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                       (uint8_t*)diceAppPayloadInfo,
                                       sizeof(DICE_APP_PAYLOAD_INFO) - sizeof(diceAppPayloadInfo->authorization),
                                       NULL,
                                       NULL,
                                       digest)) != HAL_OK)
        {
            goto Cleanup;
        }
    }
    else
    {
        // Verify the application info signature
        if((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                                       (uint8_t*)diceAppPayloadInfo,
                                       sizeof(DICE_APP_PAYLOAD_INFO) - sizeof(diceAppPayloadInfo->authorization),
                                       diceProtectedData->manufacturerID,
                                       NULL,
                                       digest)) != HAL_OK)
        {
            goto Cleanup;
        }
    }
    if(memcmp(diceAppPayloadInfo->authorization, digest, sizeof(diceAppPayloadInfo->authorization)))
    {
        retVal = HAL_ERROR;
        goto Cleanup;
    }

Cleanup:
    return retVal;
}
