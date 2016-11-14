#include "stdafx.h"

DICE_VOLATILE_DATA_TABLE diceVolatileDataBuffer = { 0 };
PDICE_VOLATILE_DATA_TABLE diceVolatileData = (PDICE_VOLATILE_DATA_TABLE)&diceVolatileDataBuffer;
DICE_PROTECTED_PERSISTED_DATA_CONTAINER diceProtectedDataBuffer = { 0 };
PDICE_PROTECTED_PERSISTED_DATA_CONTAINER diceProtectedData = (PDICE_PROTECTED_PERSISTED_DATA_CONTAINER)&diceProtectedDataBuffer;
DICE_APP_PAYLOAD_INFO diceAppPayloadInfoBuffer = { 0 };
PDICE_APP_PAYLOAD_INFO diceAppPayloadInfo = (PDICE_APP_PAYLOAD_INFO)&diceAppPayloadInfoBuffer;

uint8_t DiceChkInitialized(uint8_t* dataPtr, uint8_t dataByte, uint32_t dataSize)
{
    for (uint32_t n = 0; n < dataSize; n++)
    {
        if (dataPtr[n] != dataByte)
        {
            return 0;
        }
    }
    return 1;
}

void DicePrintData(char* label, UINT32 fill, uint8_t* dataPtr, uint32_t dataSize)
{
    uint32_t dataCursor = 0;
    uint32_t outputCursor = 0;
    uint32_t leadingSpaces = 0;
    const size_t rowSize = 16 * 6;
    char row[rowSize] = { 0 };
    size_t rowRemain = rowSize;
    char* rowCursor = row;
    if (StringCbPrintfExA(rowCursor, rowRemain, &rowCursor, &rowRemain, 0, "%s[%d]: ", label, dataSize) != S_OK)
    {
        return;
    }
    leadingSpaces = strlen(row);
    for (INT32 n = 0; n < ((INT32)fill - (INT32)leadingSpaces); n++)
    {
        if (StringCbPrintfExA(rowCursor, rowRemain, &rowCursor, &rowRemain, 0, " ") != S_OK)
        {
            return;
        }
    }
    leadingSpaces = strlen(row);
    printf("%s", row);
    while (dataSize > dataCursor)
    {
        rowCursor = row;
        rowRemain = rowSize;
        if (dataCursor > 0)
        {
            for (uint32_t n = 0; n < leadingSpaces; n++) printf(" ");
        }
        for (uint32_t n = 0; n < MIN(dataSize, 16); n++)
        {
            if (StringCbPrintfExA(rowCursor, rowRemain, &rowCursor, &rowRemain, 0, "%02x", dataPtr[dataCursor++]) != S_OK)
            {
                return;
            }
        }
        printf("%s\r\n", row);
        memset(row, 0x00, sizeof(row));
    }
}

void DicePrintData(char* label, uint8_t* dataPtr, uint32_t dataSize)
{
    DicePrintData(label, 0, dataPtr, dataSize);
}


char timeStr[256] = { 0 };
char* asctime(UINT32 timeIn)
{
    LARGE_INTEGER convert = { 0 };
    FILETIME fTime = { 0 };
    SYSTEMTIME sTime = { 0 };
    convert.LowPart = timeIn;
    convert.QuadPart *= (UINT64)10000000;
    convert.QuadPart += (UINT64)(11644473600000 * 10000);
    fTime.dwLowDateTime = convert.LowPart;
    fTime.dwHighDateTime = convert.HighPart;
    if (!FileTimeToSystemTime(&fTime, &sTime))
    {
        return NULL;
    }
    sprintf_s(timeStr, "%d.%d.%d-%d:%d:%d", sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
    return timeStr;
}

void DiceDumpProtectedData(void)
{
    if (diceVolatileData->firewallArmed == 0)
    {
        printf("---- ProtectedData ------------------------------------------------\r\n");
        if (diceProtectedData->tag == DICE_PROTECTED_PERSISTED_DATA_TAG)
        {
            printf("Tag:            %c%c%c%c\r\n",
                (diceProtectedData->tag & 0x000000ff),
                ((diceProtectedData->tag >> 8) & 0x000000ff),
                ((diceProtectedData->tag >> 16) & 0x000000ff),
                ((diceProtectedData->tag >> 24) & 0x000000ff));
            printf("Size:           %d\r\n", diceProtectedData->size);
            printf("Version:        %d.%d\r\n",
                ((diceProtectedData->version >> 16) & 0x0000ffff),
                (diceProtectedData->version & 0x0000ffff));
            printf("Date:           %s\r\n", asctime(diceProtectedData->date));
            printf("Model:          %s\r\n", diceProtectedData->model);
            printf("Serial:         %s\r\n", diceProtectedData->serial);
            printf("Manufacturer:   %s\r\n", diceProtectedData->manufacturer);
            printf("URL:            %s\r\n", diceProtectedData->url);
            printf("---- Identities & Names -------------------------------------------\r\n");
            if (!DiceChkZero(diceProtectedData->deviceID, sizeof(diceProtectedData->deviceID)))
            {
                // Calculate the DeviceName
                DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                    diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS],
                    sizeof(diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]),
                    diceProtectedData->deviceID,
                    NULL,
                    diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]);
                printf("DeviceID:                 PROVISIONED\r\n");
                DicePrintData("DeviceRootName", 26, diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]));
            }
            else
            {
                printf("DeviceID:                 DEVICE GENERATED\r\n");
            }

#ifdef DICE_INCLUDE_MANUFACTURERID
            if (!DiceChkZero(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)) &&
                !DiceChkFF(diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID)))
            {
                // Calculate the ManufacturerName
                DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                    diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS],
                    sizeof(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]),
                    diceProtectedData->manufacturerID,
                    NULL,
                    diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]);
                printf("ManufacturerID:           LOCKED\r\n");
                DicePrintData("ManufacturerRootName", 26, diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]));
            }
            else
            {
                printf("ManufacturerID:           UNLOCKED\r\n");
            }
#endif
            if (!DiceChkZero(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)) &&
                !DiceChkFF(diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName)))
            {
                printf("AppStrongName:            LOCKED\r\n");
                DicePrintData("AppStrongName", 30, diceProtectedData->softwareStrongName, sizeof(diceProtectedData->softwareStrongName));
            }
            else
            {
                printf("AppStrongName:            UNLOCKED\r\n");
            }
        }
        else
        {
            printf("DEVICE UNITIALIZED!\r\n");
        }
        printf("-------------------------------------------------------------------\r\n");
    }
    else
    {
        printf("XXXX ProtectedData: LOCKED XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n");
    }
}

void DiceDumpAppInfo(void)
{
    printf("---- ApplicationInfo ----------------------------------------------\r\n");
    if (diceAppPayloadInfo->tag == DICE_APPPAYLOAD_INFO_TAG)
    {
        printf("Tag:          %c%c%c%c\r\n",
            (diceAppPayloadInfo->tag & 0x000000ff),
            ((diceAppPayloadInfo->tag >> 8) & 0x000000ff),
            ((diceAppPayloadInfo->tag >> 16) & 0x000000ff),
            ((diceAppPayloadInfo->tag >> 24) & 0x000000ff));
        printf("Size:         %d\r\n", diceAppPayloadInfo->appSize);
        printf("Version:      %d.%d\r\n",
            ((diceAppPayloadInfo->appVersion >> 16) & 0x0000ffff),
            (diceAppPayloadInfo->appVersion & 0x0000ffff));
        printf("Date:         %s\r\n", asctime(diceAppPayloadInfo->appDate));
        printf("Name:         %s\r\n", diceAppPayloadInfo->appName);
        printf("Manufacturer: %s\r\n", diceAppPayloadInfo->manufacturer);
        printf("-------------------------------------------------------------------\r\n");
        DicePrintData("AppStrongName", 30, diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName));
#ifdef DICE_INCLUDE_MANUFACTURERID
        DicePrintData("ManufacturerRootName", 30, diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]));
        DicePrintData("ManufacturerSalt", 30, diceAppPayloadInfo->manufacturerSalt, sizeof(diceAppPayloadInfo->manufacturerSalt));
        DicePrintData("ManufacturerCompoundName", 30, diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS]));
#endif
#ifdef DICE_INCLUDE_OPERATORID
        DicePrintData("EncOperatorId", 30, diceAppPayloadInfo->encOperatorId, sizeof(diceAppPayloadInfo->encOperatorId));
        DicePrintData("OperatorName", 30, diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS]));
#endif
        printf("-------------------------------------------------------------------\r\n");
        DicePrintData("Authorization", 30, diceAppPayloadInfo->authorization, sizeof(diceAppPayloadInfo->authorization));
    }
    else
    {
        printf("APPLICATION UNITIALIZED!\r\n");
    }
    printf("-------------------------------------------------------------------\r\n");
}

void DiceVolatileData(void)
{
    printf("---- VolatileData -------------------------------------------------\r\n");
    DicePrintData("            DeviceRootId", diceProtectedData->deviceID, sizeof(diceProtectedData->deviceID));
    DicePrintData("          DeviceRootName", diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]));
    DicePrintData("        DeviceCompoundId", diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS]));
    DicePrintData("      DeviceCompoundName", diceVolatileData->info.diceCompoundNames[DICE_DEVICE_CLASS], sizeof(diceVolatileData->info.diceCompoundNames[DICE_DEVICE_CLASS]));
#ifdef DICE_INCLUDE_MANUFACTURERID
    DicePrintData("      ManufacturerRootId", diceProtectedData->manufacturerID, sizeof(diceProtectedData->manufacturerID));
    DicePrintData("    ManufacturerRootName", diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]));
    DicePrintData("  ManufacturerCompoundId", diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS]));
    DicePrintData("ManufacturerCompoundName", diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS], sizeof(diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS]));
#endif
    DicePrintData("           AppStrongName", diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName));
#ifdef DICE_INCLUDE_OPERATORID
    DicePrintData("              OperatorId", diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]));
    DicePrintData("            OperatorName", diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS]));
#endif
    printf("-------------------------------------------------------------------\r\n");
}


HAL_StatusTypeDef DiceCalculateCMAC(uint8_t encrypt, uint8_t* dataPtr, uint32_t dataSize, uint8_t* keyIn, uint8_t* digestIn, uint8_t* digestOut)
{
    NTSTATUS status = S_OK;
    BYTE iv[16] = { 0 };
    BYTE key[16] = { 0 };
    BYTE zeroBuf[16] = { 0 };
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    BYTE* cipherBuf = NULL;
    BYTE* dataInPtr = dataPtr;
    UINT32 dataInSize = dataSize;

    if (keyIn != NULL)
    {
        memcpy(key, keyIn, sizeof(key));
    }

    if (digestIn != NULL)
    {
        memcpy(iv, digestIn, sizeof(iv));
    }

    if ((dataInPtr == NULL) || (dataInSize == 0))
    {
        dataInPtr = zeroBuf;
        dataInSize = sizeof(zeroBuf);
    }

    if ((status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0)) != 0)
    {
        goto Cleanup;
    }

    if ((status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, sizeof(key), 0)) != 0)
    {
        goto Cleanup;
    }

    if ((status = BCryptSetProperty(hKey, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0)) != 0)
    {
        goto Cleanup;
    }

    if ((cipherBuf = (BYTE*)malloc(dataInSize)) == NULL)
    {
        goto Cleanup;
    }

    if (((encrypt) && ((status = BCryptEncrypt(hKey, dataInPtr, dataInSize, NULL, iv, sizeof(iv), cipherBuf, dataInSize, (ULONG*)&dataInSize, 0)) != 0)) ||
        ((!encrypt) && ((status = BCryptDecrypt(hKey, dataInPtr, dataInSize, NULL, iv, sizeof(iv), cipherBuf, dataInSize, (ULONG*)&dataInSize, 0)) != 0)))
    {
        goto Cleanup;
    }

    if (digestOut != NULL)
    {
        memcpy(digestOut, iv, sizeof(iv));
    }

Cleanup:
    if (hAlg != NULL) BCryptCloseAlgorithmProvider(hAlg, 0);
    if (hKey != NULL) BCryptDestroyKey(hKey);
    if (cipherBuf != NULL) free(cipherBuf);
    return status;
}

HAL_StatusTypeDef DiceDeriveData(void)
{
    UINT32 retVal = 0;
#ifdef DICE_INCLUDE_OPERATORID
    uint8_t operatorIdKey[16];
#endif
    if (diceProtectedData->tag == DICE_PROTECTED_PERSISTED_DATA_TAG)
    {
        // Calculate the DeviceName
        if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
            diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS],
            sizeof(diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]),
            diceProtectedData->deviceID,
            NULL,
            diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS])) != 0)
        {
            goto Cleanup;
        }

#ifdef DICE_INCLUDE_MANUFACTURERID
        // Calculate the ManufacturerName
        if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
            diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS],
            sizeof(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]),
            diceProtectedData->manufacturerID,
            NULL,
            diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS])) != 0)
        {
            goto Cleanup;
        }
#endif

        if (diceAppPayloadInfo->tag == DICE_APPPAYLOAD_INFO_TAG)
        {
#ifdef DICE_INCLUDE_OPERATORID
            // Copy the SoftwareName
            memcpy(diceVolatileData->info.diceRootNames[DICE_OPERATOR_CLASS],
                diceAppPayloadInfo->strongName,
                sizeof(diceVolatileData->info.diceRootNames[DICE_OPERATOR_CLASS]));
#endif

            // Calculate the DeviceCompoundId
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceAppPayloadInfo->strongName,
                sizeof(diceAppPayloadInfo->strongName),
                diceProtectedData->deviceID,
                NULL,
                diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS])) != 0)
            {
                goto Cleanup;
            }

#ifdef DICE_INCLUDE_MANUFACTURERID
            // Calculate the ManufacturerCompoundId
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceAppPayloadInfo->manufacturerSalt,
                sizeof(diceAppPayloadInfo->manufacturerSalt),
                diceProtectedData->manufacturerID,
                NULL,
                diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS])) != 0)
            {
                goto Cleanup;
            }
#endif
#ifdef DICE_INCLUDE_OPERATORID
            // Decrypt the secret OperatorID
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceAppPayloadInfo->strongName,
                sizeof(diceAppPayloadInfo->strongName),
#ifdef DICE_INCLUDE_MANUFACTURERID
                diceProtectedData->manufacturerID,
#else
                diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS],
#endif
                NULL,
                operatorIdKey)) != 0)
            {
                goto Cleanup;
            }
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS],
                sizeof(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]),
                operatorIdKey,
                NULL,
                diceAppPayloadInfo->encOperatorId)) != 0)
            {
                goto Cleanup;
            }
#endif

            // Derive the DeviceCompoundName
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundNames[DICE_DEVICE_CLASS],
                sizeof(diceVolatileData->info.diceCompoundNames[DICE_DEVICE_CLASS]),
                diceVolatileData->info.diceCompoundIds[DICE_DEVICE_CLASS],
                NULL,
                diceVolatileData->info.diceCompoundNames[DICE_DEVICE_CLASS])) != 0)
            {
                goto Cleanup;
            }

#ifdef DICE_INCLUDE_MANUFACTURERID
            // Derive the ManfacturerCompoundName
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS],
                sizeof(diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS]),
                diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS],
                NULL,
                diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS])) != 0)
            {
                goto Cleanup;
            }
#endif

#ifdef DICE_INCLUDE_OPERATORID
            // Derive the OperatorCompoundName
            if ((retVal = DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS],
                sizeof(diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS]),
                diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS],
                NULL,
                diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS])) != 0)
            {
                goto Cleanup;
            }
#endif
        }
    }
Cleanup:
    return retVal;
}

