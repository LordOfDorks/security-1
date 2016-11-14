#pragma once

extern WCHAR storagePrefix[MAX_PATH];

NTSTATUS OpenMyTpm(void);
NTSTATUS CreateAesIdentity(BYTE* keyIn, UINT32 keyInSize, TPM_HANDLE* hKeyOut);
void TpmCalculateCMAC(TPM_HANDLE* hKey, uint8_t encrypt, uint8_t* dataPtr, uint32_t dataSize, uint8_t* digestIn, uint8_t* digestOut);
NTSTATUS LoadAesIdentity(BYTE* nameIn, TPM_HANDLE* hKeyOut);
