// DicePrep.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define diceDeviceMajorVersion              (0)
#define diceDeviceMinorVersion              (1)
#define diceDeviceModel                     "Nautilus-STM32L082KZ"
#define diceDeviceSerial                    "SN12345678"
#define diceDeviceManufacturer              "StefanTh@Microsoft.com"
#define diceDeviceUrl                       "http://WindowsOnDevices.com"
BYTE diceDeviceId[DICE_DIGEST_SIZE] =       { 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,  0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5 };
#ifdef DICE_INCLUDE_MANUFACTURERID
BYTE diceManufacturerId[DICE_DIGEST_SIZE] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
#endif
//#define DICE_LOCK_STRONG_NAME (1)

#define dicePayloadMajorVersion               (0)
#define dicePayloadMinorVersion               (1)
#define dicePayloadManufacturer               "StefanTh@Microsoft.com"
#ifdef DICE_INCLUDE_MANUFACTURERID
BYTE diceManufacturerSalt[DICE_DIGEST_SIZE] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
#endif
#ifdef DICE_INCLUDE_OPERATORID
BYTE diceOperatorId[DICE_DIGEST_SIZE] =       { 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55 };
#endif

#define diceDeviceVid                         0x0483
#define diceDevicePid                         0xDF11
#define diceDeviceVer                         0x0200
#define diceIdentityPackageFile               "DiceIdPkg.dfu"
#define diceFirmwarePackageFile               "DiceFwPkg.dfu"

TPM_HANDLE hManufacturerID;
TPM_HANDLE hDeviceID;
WCHAR storagePrefix[MAX_PATH] = L"";

void MakeTimeStamp(UINT32* timeStamp)
{
    FILETIME now = { 0 };
    LARGE_INTEGER convert = { 0 };

    // Get the current timestamp
    GetSystemTimeAsFileTime(&now);
    convert.LowPart = now.dwLowDateTime;
    convert.HighPart = now.dwHighDateTime;
    convert.QuadPart = (convert.QuadPart - (UINT64)(11644473600000 * 10000)) / 10000000;
    *timeStamp = convert.LowPart;
}

NTSTATUS WriteDeviceIdPackage(void)
{
    DWORD dfuRetVal = 0;
//    HANDLE hDfuImg = INVALID_HANDLE_VALUE;
    HANDLE hDfuId = INVALID_HANDLE_VALUE;
    HANDLE hDfuFile = INVALID_HANDLE_VALUE;
    DFUIMAGEELEMENT dfuImageElement = { 0 };
    CHAR outFile[MAX_PATH] = "";

    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, storagePrefix, wcslen(storagePrefix), outFile, sizeof(outFile), NULL, NULL))
    {
        printf("WideCharToMultiByte failed with 0x%08x\n", GetLastError());
        goto Cleanup;
    }
    strcat_s(outFile, MAX_PATH, diceIdentityPackageFile);

    if ((dfuRetVal = STDFUFILES_CreateImage(&hDfuId, 0)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CreateImage failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    dfuImageElement.dwAddress = DICE_PROTECTED_PERSISTED_DATA_ADDRESS;
    dfuImageElement.dwDataLength = DICE_PROTECTED_PERSISTED_DATA_LEN;
    if ((dfuImageElement.Data = (PBYTE)malloc(dfuImageElement.dwDataLength)) == NULL)
    {
        wprintf(L"Get memory for the identity failed\n");
        dfuRetVal = STDFU_MEMORY;
        goto Cleanup;
    }
    memcpy(dfuImageElement.Data, (PBYTE)diceProtectedData, dfuImageElement.dwDataLength);
    if ((dfuRetVal = STDFUFILES_SetImageElement(hDfuId, 0, TRUE, dfuImageElement)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_SetImageElement failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_SetImageName(hDfuId, "Dice")) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_SetImageName failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_CreateNewDFUFile(outFile, &hDfuFile, diceDeviceVid, diceDevicePid, diceDeviceVer)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CreateNewDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_AppendImageToDFUFile(hDfuFile, hDfuId)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_AppendImageToDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_CloseDFUFile(hDfuFile)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CloseDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_DestroyImage(&hDfuId)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_DestroyImage failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    free(dfuImageElement.Data);
    memset(&dfuImageElement, 0x00, sizeof(dfuImageElement));
//    printf("Created: %s\n", diceIdentityPackageFile);

Cleanup:
    if (dfuImageElement.Data != NULL) free(dfuImageElement.Data);
//    if (hDfuImg != INVALID_HANDLE_VALUE) STDFUFILES_DestroyImage(&hDfuImg);
    if (hDfuId != INVALID_HANDLE_VALUE) STDFUFILES_DestroyImage(&hDfuId);
    return (dfuRetVal != STDFU_NOERROR) ? ERROR_WRITE_FAULT : S_OK;
}

NTSTATUS WriteAppPayloadPackage(WCHAR* hexFileName)
{
    DWORD dfuRetVal = STDFU_NOERROR;
    char fileName[MAX_PATH] = { 0 };
    HANDLE hDfuImg = INVALID_HANDLE_VALUE;
//    HANDLE hDfuId = INVALID_HANDLE_VALUE;
    HANDLE hDfuFile = INVALID_HANDLE_VALUE;
    DFUIMAGEELEMENT dfuImageElement = { 0 };
    PDICE_APP_PAYLOAD_INFO dfuImageElementApplicationInfo = NULL;
    BYTE compoundId[DICE_DIGEST_SIZE] = { 0 };
    CHAR outFile[MAX_PATH] = "";

    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, storagePrefix, wcslen(storagePrefix), outFile, sizeof(outFile), NULL, NULL))
    {
        printf("WideCharToMultiByte failed with 0x%08x\n", GetLastError());
        goto Cleanup;
    }
    strcat_s(outFile, MAX_PATH, diceFirmwarePackageFile);

    // Open up the payload image file
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, hexFileName, wcslen(hexFileName), fileName, sizeof(fileName), NULL, NULL))
    {
        printf("WideCharToMultiByte failed with 0x%08x\n", GetLastError());
        dfuRetVal = GetLastError();
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_ImageFromFile(fileName, &hDfuImg, 0)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CreateImage failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_GetImageElement(hDfuImg, 0, &dfuImageElement)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_GetImageElement failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    // Make sure we fit into the 4K sectors
    diceAppPayloadInfo->appSize = ((((dfuImageElement.dwDataLength - sizeof(DICE_APP_PAYLOAD_INFO)) + 4095) / 4096) * 4096);
    dfuImageElement.dwDataLength = diceAppPayloadInfo->appSize + sizeof(DICE_APP_PAYLOAD_INFO);
    if ((dfuImageElement.Data = (PBYTE)malloc(dfuImageElement.dwDataLength)) == NULL)
    {
        wprintf(L"Get memory for the payload failed\n");
        dfuRetVal = STDFU_MEMORY;
        goto Cleanup;
    }
    memset(dfuImageElement.Data, 0x00, dfuImageElement.dwDataLength);
    if ((dfuRetVal = STDFUFILES_GetImageElement(hDfuImg, 0, &dfuImageElement)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_GetImageElement failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }

    // Claculate the digest of the code
    if (DiceCalculateCMAC(TRUE, &dfuImageElement.Data[sizeof(DICE_APP_PAYLOAD_INFO)], diceAppPayloadInfo->appSize, NULL, NULL, diceAppPayloadInfo->strongName) != 0)
    {
        dfuRetVal = STDFU_BADPARAMETER;
        goto Cleanup;
    }

#ifdef DICE_INCLUDE_OPERATORID
#ifdef DICE_INCLUDE_MANUFACTURERID
    // Protect the operatorID ENC(operatorID, ENC(StrongName, ManufacturerId))
    TpmCalculateCMAC(&hManufacturerID, TRUE, diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName), NULL, compoundId);
#else
    // Protect the operatorID ENC(operatorID, ENC(StrongName, DeviceId))
    TpmCalculateCMAC(&hDeviceID, TRUE, diceAppPayloadInfo->strongName, sizeof(diceAppPayloadInfo->strongName), NULL, compoundId);
#endif
    if (DiceCalculateCMAC(TRUE, diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]), compoundId, NULL, diceAppPayloadInfo->encOperatorId) != 0)
    {
        dfuRetVal = STDFU_BADPARAMETER;
        goto Cleanup;
    }
#endif

#ifdef DICE_INCLUDE_MANUFACTURERID
    // Finally sign the payload package
    TpmCalculateCMAC(&hManufacturerID, TRUE, (BYTE*)diceAppPayloadInfo, sizeof(*diceAppPayloadInfo) - sizeof(diceAppPayloadInfo->authorization), NULL, diceAppPayloadInfo->authorization);
#else
    // Finally claculate the payload integrity value
    if (DiceCalculateCMAC(TRUE, (BYTE*)diceAppPayloadInfo, sizeof(*diceAppPayloadInfo) - sizeof(diceAppPayloadInfo->authorization), NULL, NULL, diceAppPayloadInfo->authorization) != 0)
    {
        dfuRetVal = STDFU_BADPARAMETER;
        goto Cleanup;
    }
#endif
    // Insert the payload info and write the element back into the image
    dfuImageElementApplicationInfo = (PDICE_APP_PAYLOAD_INFO)dfuImageElement.Data;
    *dfuImageElementApplicationInfo = *diceAppPayloadInfo;
    if ((dfuRetVal = STDFUFILES_SetImageElement(hDfuImg, 0, FALSE, dfuImageElement)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_SetImageElement failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_ImageToFile(fileName, hDfuImg)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_ImageToFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    free(dfuImageElement.Data);
    memset(&dfuImageElement, 0x00, sizeof(dfuImageElement));
//    printf("Created: %s\n", fileName);

// Create the firmware package file
    if ((dfuRetVal = STDFUFILES_SetImageName(hDfuImg, "Dice")) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_SetImageName failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_CreateNewDFUFile(outFile, &hDfuFile, diceDeviceVid, diceDevicePid, diceDeviceVer)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CreateNewDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_AppendImageToDFUFile(hDfuFile, hDfuImg)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_AppendImageToDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }
    if ((dfuRetVal = STDFUFILES_CloseDFUFile(hDfuFile)) != STDFUFILES_NOERROR)
    {
        wprintf(L"STDFUFILES_CloseDFUFile failed with %x\n", (~STDFUFILES_ERROR_OFFSET) & dfuRetVal);
        goto Cleanup;
    }

Cleanup:
    if (dfuImageElement.Data != NULL) free(dfuImageElement.Data);
    if (hDfuImg != INVALID_HANDLE_VALUE) STDFUFILES_DestroyImage(&hDfuImg);
//    if (hDfuImg != INVALID_HANDLE_VALUE) STDFUFILES_DestroyImage(&hDfuId);
    return (dfuRetVal != STDFU_NOERROR) ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
}

void ReadID(WCHAR* strIn, BYTE* idOut)
{
    WCHAR hexdigit[5] = L"0x00";
    UINT32 strInLen = wcslen(strIn);

    for (UINT32 n = 0; n < min((strInLen / 2), 16); n++)
    {
        UINT32 scannedDigit = 0;
        hexdigit[2] = strIn[n * 2];
        hexdigit[3] = strIn[n * 2 + 1];
        if (swscanf_s(hexdigit, L"%x", &scannedDigit) != 1)
        {
            break;
        }
        idOut[n] = (BYTE)(scannedDigit & 0x000000FF);
    }
}

void CmdHelp(void)
{
    WCHAR help[] =
        L"DicePrep.exe [command]:\n"
        L"  Generate random ID:\n"
        L"    GID \n"
        L"  Create DeviceID Package:\n"
        L"    CDP [DeviceID]"
#ifdef DICE_INCLUDE_MANUFACTURERID
        L" [ManufacturerID]"
#endif
        L"\n"
        L"  Create Application Package:\n"
        L"    CAP [HEX code file]"
#ifdef DICE_INCLUDE_MANUFACTURERID
        L" [ManufacturerRootName] [ManufacturerSalt]"
#else
        L" [DeviceRootName]"
#endif
#ifdef DICE_INCLUDE_OPERATORID
        L" [OperatorID]"
#endif
        L"\n"
//        L"  Interlocked DeviceID and Application Package\n"
//        L"    CLP [DeviceID]"
//#ifdef DICE_INCLUDE_MANUFACTURERID
//        L" [ManufacturerID] [ManufacturerSalt]"
//#endif
//#ifdef DICE_INCLUDE_OPERATORID
//        L" [OperatorID]"
//#endif
//        L"\n"
        L"  Derive TPM Bound Identity\n"
        L"    DTI [RootName] [DerivationID] \n";
    wprintf(help);
}

int wmain(int argc, const wchar_t** argv)
{
    NTSTATUS status;
    UINT32 argNeeded = 3;
    HANDLE hDfuImg = INVALID_HANDLE_VALUE;
    HANDLE hDfuId = INVALID_HANDLE_VALUE;
    HANDLE hDfuFile = INVALID_HANDLE_VALUE;
    UINT32 timeStamp = 0;
    UINT32 nameStart = 0;
    UINT32 nameEnd = 0;
    UINT32 cursor = 0;
    WCHAR appName[MAX_PATH] = { 0 };
    DFUIMAGEELEMENT dfuImageElement = { 0 };
    PDICE_APP_PAYLOAD_INFO dfuImageElementApplicationInfo = NULL;
    TPM_HANDLE aesKey;
    WCHAR toolFolder[MAX_PATH] = L"";

    // Get the TPM operational
    if ((status = OpenMyTpm()) != S_OK)
    {
        printf("TPM error.");
        goto Cleanup;
    }

    // Make timestamp
    MakeTimeStamp(&timeStamp);

    // Find the location of the tool
    if (_wsplitpath_s(argv[0], storagePrefix, MAX_PATH, toolFolder, MAX_PATH, NULL, 0, NULL, 0) != 0)
    {
        printf("_wsplitpath_s failed.");
        goto Cleanup;
    }
    wcscat_s(storagePrefix, toolFolder);
    if (wcslen(storagePrefix) == 0)
    {
        wcscpy_s(storagePrefix, L".\\");
    }
    wprintf(L"Storage Prefix: %s\n", storagePrefix);

    if (argc <= 1)
    {
        CmdHelp();
        goto Cleanup;
    }
    else if (!_wcsicmp(argv[1], L"gid"))
    {
        BYTE newId[DICE_DIGEST_SIZE] = { 0 };
        wprintf(L"NewID = ");
        srand(GetTickCount());
        for (UINT32 n = 0; n < sizeof(newId); n++)
        {
            newId[n] = (rand() & 0xff);
            wprintf(L"%02x", newId[n]);
        }
        wprintf(L"\n");
    }
    else if (argc <= 2)
    {
        CmdHelp();
        goto Cleanup;
    }
    else if (!_wcsicmp(argv[1], L"cdp"))
    {
#ifdef DICE_INCLUDE_MANUFACTURERID
        argNeeded += 1;
#endif
        if (argc == argNeeded)
        {
            BYTE deviceID[DICE_DIGEST_SIZE] = { 0 };
#ifdef DICE_INCLUDE_MANUFACTURERID
            BYTE manufacturerID[DICE_DIGEST_SIZE] = { 0 };
            ReadID((WCHAR*)argv[3], manufacturerID);
#endif
            ReadID((WCHAR*)argv[2], deviceID);

            // Fill the dice persistent data structure
            diceProtectedData->tag = DICE_PROTECTED_PERSISTED_DATA_TAG;
            diceProtectedData->size = sizeof(DICE_PROTECTED_PERSISTED_DATA_CONTAINER);
            diceProtectedData->version = (UINT32)((((UINT16)diceDeviceMajorVersion) << 16) | ((UINT16)diceDeviceMinorVersion));
            diceProtectedData->date = timeStamp;
            strcpy_s(diceProtectedData->model, diceDeviceModel);
            strcpy_s(diceProtectedData->serial, diceDeviceSerial);
            strcpy_s(diceProtectedData->manufacturer, diceDeviceManufacturer);
            strcpy_s(diceProtectedData->url, diceDeviceUrl);
            memcpy(diceProtectedData->deviceID, deviceID, sizeof(diceProtectedData->deviceID));
            if ((!DiceChkZero(deviceID, sizeof(deviceID))) &&
                ((status = CreateAesIdentity(deviceID, sizeof(deviceID), NULL)) != S_OK))
            {
                wprintf(L"CreateAesIdentity failed.\n");
                goto Cleanup;
            }

#ifdef DICE_INCLUDE_MANUFACTURERID
            memcpy(diceProtectedData->manufacturerID, manufacturerID, sizeof(diceProtectedData->manufacturerID));
            if ((!DiceChkZero(manufacturerID, sizeof(manufacturerID))) &&
                ((status = CreateAesIdentity(manufacturerID, sizeof(manufacturerID), NULL)) != S_OK))
            {
                wprintf(L"CreateAesIdentity failed.\n");
                goto Cleanup;
            }
#endif

            if ((status = WriteDeviceIdPackage()) != S_OK)
            {
                wprintf(L"WriteDeviceIdPackage failed.\n");
                goto Cleanup;
            }

            DiceDumpProtectedData();
        }
        else
        {
            CmdHelp();
            goto Cleanup;
        }
    }
    else if (!_wcsicmp(argv[1], L"cap"))
    {
        UINT32 argCursor = 2;
#ifdef DICE_INCLUDE_MANUFACTURERID
        argNeeded += 2;
#else
#ifdef DICE_INCLUDE_OPERATORID
        argNeeded += 1;
#endif
#endif
#ifdef DICE_INCLUDE_OPERATORID
        argNeeded += 1;
#endif
        if (argc == argNeeded)
        {
            DWORD dfuRetVal = 0;
            WCHAR* hexFileName = (WCHAR*)argv[argCursor++];
            WCHAR appName[MAX_PATH] = { 0 };
            BYTE compoundId[DICE_DIGEST_SIZE] = { 0 };
#ifdef DICE_INCLUDE_MANUFACTURERID
            BYTE manufacturerSalt[DICE_DIGEST_SIZE] = { 0 };
#else
#ifdef DICE_INCLUDE_OPERATORID
            BYTE deviceName[16] = { 0 };
#endif
#endif
#ifdef DICE_INCLUDE_MANUFACTURERID
            ReadID((WCHAR*)argv[argCursor++], diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS]);
            ReadID((WCHAR*)argv[argCursor++], manufacturerSalt);
            if ((status = LoadAesIdentity(diceVolatileData->info.diceRootNames[DICE_MANUFACTURER_CLASS], &hManufacturerID)) != ERROR_SUCCESS)
            {
                wprintf(L"LoadAesIdentity failed.\n");
                goto Cleanup;
            }
            // Calculate the manufactuter ID and Name
            TpmCalculateCMAC(&hManufacturerID, TRUE, manufacturerSalt, sizeof(manufacturerSalt), NULL, diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS]);
            if (DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS],
                sizeof(diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS]),
                diceVolatileData->info.diceCompoundIds[DICE_MANUFACTURER_CLASS],
                NULL,
                diceVolatileData->info.diceCompoundNames[DICE_MANUFACTURER_CLASS]) != 0)
            {
                status = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
#else
#ifdef DICE_INCLUDE_OPERATORID
            ReadID((WCHAR*)argv[argCursor++], diceVolatileData->info.diceRootNames[DICE_DEVICE_CLASS]);
            if ((status = LoadAesIdentity(deviceName, &hDeviceID)) != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
#endif
#endif
#ifdef DICE_INCLUDE_OPERATORID
            ReadID((WCHAR*)argv[argCursor++], diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]);
            if (DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
                diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS],
                sizeof(diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS]),
                diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS],
                NULL,
                diceVolatileData->info.diceCompoundNames[DICE_OPERATOR_CLASS]) != 0)
            {
                status = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
#endif

            // Fill out the payload Info
            diceAppPayloadInfo->tag = DICE_APPPAYLOAD_INFO_TAG;
            diceAppPayloadInfo->appVersion = (UINT32)((((UINT16)dicePayloadMajorVersion) << 16) | ((UINT16)dicePayloadMinorVersion));
            diceAppPayloadInfo->appDate = timeStamp;
            if (_wsplitpath_s(hexFileName, NULL, 0, NULL, 0, appName, MAX_PATH, NULL, 0) != 0)
            {
                printf("_wsplitpath_s failed.");
                goto Cleanup;
            }
            if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, appName, wcslen(appName), diceAppPayloadInfo->appName, sizeof(diceAppPayloadInfo->appName), NULL, NULL))
            {
                printf("WideCharToMultiByte failed with 0x%08x\n", GetLastError());
                goto Cleanup;
            }
            strcpy_s(diceAppPayloadInfo->manufacturer, dicePayloadManufacturer);
#ifdef DICE_INCLUDE_MANUFACTURERID
            memcpy(diceAppPayloadInfo->manufacturerSalt, manufacturerSalt, sizeof(diceAppPayloadInfo->manufacturerSalt));
#endif

            if (WriteAppPayloadPackage(hexFileName) != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            if ((status = CreateAesIdentity(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS], sizeof(diceVolatileData->info.diceCompoundIds[DICE_OPERATOR_CLASS]), NULL)) != S_OK)
            {
                goto Cleanup;
            }

            DiceDumpAppInfo();
        }
        else
        {
            CmdHelp();
            goto Cleanup;
        }
    }
    else if (!_wcsicmp(argv[1], L"dti"))
    {
        UINT32 argCursor = 2;
        argNeeded += 1;
        if (argc == argNeeded)
        {
            BYTE compoundId[DICE_DIGEST_SIZE] = { 0 };
            TPM_HANDLE hRootID;
            TPM_HANDLE hDerivedID;
            ReadID((WCHAR*)argv[argCursor++], compoundId);
            if ((status = LoadAesIdentity(compoundId, &hRootID)) != ERROR_SUCCESS)
            {
                wprintf(L"LoadAesIdentity failed.\n");
                goto Cleanup;
            }
            ReadID((WCHAR*)argv[argCursor++], compoundId);
            TpmCalculateCMAC(&hRootID, TRUE, compoundId, sizeof(compoundId), NULL, compoundId);
            if ((status = CreateAesIdentity(compoundId, sizeof(compoundId), &hDerivedID)) != S_OK)
            {
                wprintf(L"CreateAesIdentity failed.\n");
                goto Cleanup;
            }
            TpmCalculateCMAC(&hDerivedID, TRUE, NULL, 0, NULL, compoundId);
            DicePrintData("NewCompoundName", compoundId, sizeof(compoundId));
        }
    }
    else
    {
        CmdHelp();
        goto Cleanup;
    }

Cleanup:
    return 0;
}

