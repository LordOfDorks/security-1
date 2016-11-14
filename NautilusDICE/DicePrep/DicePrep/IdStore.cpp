#include "stdafx.h"

#define USE_TPM_SIMULATOR (1)

#ifdef USE_TPM_SIMULATOR
BOOL tpmReProvision = TRUE;
TpmTcpDevice tcpDev;
#else
TpmTbsDevice tbsDev;
#endif

Tpm2 myTpm;
TPM_HANDLE idStorageKey = { 0 };
vector<BYTE> NullVec;
TPM_HANDLE storageKey = TPM_HANDLE((UINT32)0x81000001); // SRK
//TPM_HANDLE storageKey = TPM_HANDLE::NullHandle();
//TPM_HANDLE migrationAuthority = TPM_HANDLE::NullHandle();

//NTSTATUS LoadMigrationAuthorityPub()
//{
//    NTSTATUS retVal = S_OK;
//    HANDLE hCertFile = INVALID_HANDLE_VALUE;
//    BCRYPT_KEY_HANDLE hKey = INVALID_HANDLE_VALUE;
//    PCCERT_CONTEXT authCert = NULL;
//    DWORD keySize = 0;
//    vector<BYTE> pubKey;
//    vector<BYTE> modulus;
//    BCRYPT_RSAKEY_BLOB* rsaPubKey = NULL;
//    TPMT_PUBLIC tpmPubKey;
//
//    // Open the certificate we are using as migration authority
//    if ((hCertFile = CreateFile(L"MigrationAuthority.cer", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
//    {
//        wprintf(L"MigrationAuthority.cer not found.\n");
//        return GetLastError();
//        goto Cleanup;
//    }
//    DWORD read = 0;
//    DWORD fileSize = GetFileSize(hCertFile, NULL);
//    vector<BYTE> fileContent(fileSize);
//    if (!ReadFile(hCertFile, fileContent.data(), fileContent.size(), &read, NULL))
//    {
//        retVal = GetLastError();
//        goto Cleanup;
//    }
//    CloseHandle(hCertFile);
//    hCertFile = INVALID_HANDLE_VALUE;
//
//    // Open the certificate context
//    authCert = (PCCERT_CONTEXT)CertCreateContext(CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, fileContent.data(), fileContent.size(), 0, NULL);
//    if (authCert == NULL)
//    {
//        retVal = GetLastError();
//        goto Cleanup;
//    }
//
//    // Get the public key handle
//    if (!CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, &authCert->pCertInfo->SubjectPublicKeyInfo, CRYPT_OID_INFO_PUBKEY_SIGN_KEY_FLAG, NULL, &hKey))
//    {
//        retVal = GetLastError();
//        goto Cleanup;
//    }
//    CertFreeCertificateContext(authCert);
//    authCert = NULL;
//
//    // Export the public key from the handle
//    if ((retVal = BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &keySize, 0)) != STATUS_SUCCESS)
//    {
//        goto Cleanup;
//    }
//    pubKey = vector<BYTE>(keySize);
//    if ((retVal = BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, pubKey.data(), pubKey.size(), &keySize, 0)) != STATUS_SUCCESS)
//    {
//        goto Cleanup;
//    }
//    BCryptDestroyKey(hKey);
//    hKey = INVALID_HANDLE_VALUE;
//    rsaPubKey = (BCRYPT_RSAKEY_BLOB*)pubKey.data();
//    modulus = vector<BYTE>(rsaPubKey->cbModulus);
//    memcpy(modulus.data(), &((BYTE*)pubKey.data())[sizeof(BCRYPT_RSAKEY_BLOB) + rsaPubKey->cbPublicExp], modulus.size());
//
//    // Prepare the template
//    tpmPubKey = TPMT_PUBLIC(TPM_ALG_ID::SHA256,
//        TPMA_OBJECT::sign |
//        TPMA_OBJECT::userWithAuth |
//        TPMA_OBJECT::noDA,
//        NullVec,  // No policy
//        TPMS_RSA_PARMS(
//            TPMT_SYM_DEF_OBJECT(TPM_ALG_ID::_NULL, 0, TPM_ALG_ID::_NULL),
//            TPMS_SCHEME_RSAPSS(TPM_ALG_ID::SHA256),
//            (UINT16)rsaPubKey->BitLength,
//            0),
//        TPM2B_PUBLIC_KEY_RSA(modulus));
//
//    // Load the external key to get the name and make sure that it works, then flush it again
//    migrationAuthority = myTpm.LoadExternal(TPMT_SENSITIVE::NullObject(), tpmPubKey, TPM_HANDLE((UINT32)TPM_RH::OWNER));
//    myTpm.FlushContext(migrationAuthority);
//    migrationAuthority.handle = (UINT32)TPM_RH::_NULL;
//
//Cleanup:
//    if (hCertFile != INVALID_HANDLE_VALUE)
//    {
//        CloseHandle(hCertFile);
//        hCertFile = INVALID_HANDLE_VALUE;
//    }
//    if (authCert != NULL)
//    {
//        CertFreeCertificateContext(authCert);
//        authCert = NULL;
//    }
//    if (hKey != INVALID_HANDLE_VALUE)
//    {
//        BCryptDestroyKey(hKey);
//        hKey = INVALID_HANDLE_VALUE;
//    }
//
//    return retVal;
//}

//NTSTATUS LoadParentStorageKey()
//{
//    HANDLE hKeyFile = INVALID_HANDLE_VALUE;
//    TPM2B_PUBLIC pubKey;
//    TPM2B_PRIVATE prvKey;
//
//    if ((tpmReProvision == TRUE) ||
//        ((hKeyFile = CreateFile(L"IdStorgeKey.TPMKEY", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE))
//    {
//        // File/Key does not exisit
//        TPMT_PUBLIC inPublic(TPM_ALG_ID::SHA256,
//                             TPMA_OBJECT::decrypt | TPMA_OBJECT::restricted |
//                             TPMA_OBJECT::sensitiveDataOrigin |
//                             TPMA_OBJECT::userWithAuth |
//                             TPMA_OBJECT::noDA |
//                             TPMA_OBJECT::fixedParent | TPMA_OBJECT::fixedTPM,
//                             NullVec,  // No policy
//                             TPMS_RSA_PARMS(
//                                TPMT_SYM_DEF_OBJECT(TPM_ALG_ID::AES, 128, TPM_ALG_ID::CFB),
//                                TPMS_NULL_ASYM_SCHEME(),
//                                2048,
//                                0),
//                             TPM2B_PUBLIC_KEY_RSA(NullVec));
//        // Create a new storage key under the SRK
//        CreateResponse createResponse = myTpm.Create(TPM_HANDLE((UINT32)0x81000001),
//                                                     TPMS_SENSITIVE_CREATE(NullVec, NullVec),
//                                                     inPublic,
//                                                     NullVec,
//                                                     vector<TPMS_PCR_SELECTION>());
//        pubKey = TPM2B_PUBLIC(createResponse.outPublic);
//        prvKey = createResponse.outPrivate;
//
//        // Persist it for the next time
//        if ((hKeyFile = CreateFile(L"IdStorgeKey.TPMKEY", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
//        {
//            DWORD written = 0;
//            OutByteBuf buf;
//            buf << prvKey << pubKey;
//            if (!WriteFile(hKeyFile, buf.GetBuf().data(), buf.GetBuf().size(), &written, NULL))
//            {
//                return ERROR_WRITE_FAULT;
//            }
//            CloseHandle(hKeyFile);
//            hKeyFile = INVALID_HANDLE_VALUE;
//        }
//    }
//    else
//    {
//        DWORD read = 0;
//        DWORD fileSize = GetFileSize(hKeyFile, NULL);
//        vector<BYTE> fileContent(fileSize);
//        if (!ReadFile(hKeyFile, fileContent.data(), fileContent.size(), &read, NULL))
//        {
//            return ERROR_READ_FAULT;
//        }
//        InByteBuf buf(fileContent);
//        buf >> prvKey >> pubKey;
//    }
//    storageKey = myTpm.Load(TPM_HANDLE((UINT32)0x81000001), prvKey, pubKey.publicArea);
//    return S_OK;
//}

NTSTATUS CreateAesIdentity(BYTE* keyIn, UINT32 keyInSize, TPM_HANDLE* hKeyOut)
{
    HANDLE hKeyFile = INVALID_HANDLE_VALUE;
    TPM2B_PUBLIC pubKey;
    TPM2B_PRIVATE prvKey;
    vector<BYTE> keyVector(keyInSize);
    BYTE keyInName[16] = { 0 };
    WCHAR fileName[MAX_PATH] = L"";
    memcpy(keyVector.data(), keyIn, keyVector.size());

    // Claculate the name of the key
    if (DiceCalculateCMAC(DICE_CMAC_ENCRYPT,
        keyInName,
        sizeof(keyInName),
        keyIn,
        NULL,
        keyInName) != 0)
    {
        return ERROR_INVALID_PARAMETER;
    }
    wcscpy_s(fileName, storagePrefix);
    for (UINT32 n = 0; n < sizeof(keyInName); n++)
    {
        UINT32 cursor = wcslen(fileName);
        swprintf_s(&fileName[cursor], (MAX_PATH - cursor), L"%02x", keyInName[n]);
    }
    wcscat_s(fileName, L".TPMKEY");

    TPMT_PUBLIC inPublic(TPM_ALG_ID::SHA256,
        TPMA_OBJECT::decrypt | TPMA_OBJECT::encrypt |
        TPMA_OBJECT::userWithAuth |
        TPMA_OBJECT::noDA,
        NullVec, // No policy
        TPMS_SYMCIPHER_PARMS(
            TPMT_SYM_DEF_OBJECT(TPM_ALG_ID::AES, 128, TPM_ALG_ID::CBC)),
        TPM2B_DIGEST_Symcipher());

    TPMS_SENSITIVE_CREATE inSensitive(NullVec, keyVector);

    // Create the new identity under the storage key
    CreateResponse createResponse = myTpm.Create(storageKey,
        inSensitive,
        inPublic,
        NullVec,
        vector<TPMS_PCR_SELECTION>());
    pubKey = TPM2B_PUBLIC(createResponse.outPublic);
    prvKey = createResponse.outPrivate;

    // Persist it for the next time
    if ((hKeyFile = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        OutByteBuf buf;
        buf << prvKey << pubKey;
        if (!WriteFile(hKeyFile, buf.GetBuf().data(), buf.GetBuf().size(), &written, NULL))
        {
            return ERROR_WRITE_FAULT;
        }
        CloseHandle(hKeyFile);
        hKeyFile = INVALID_HANDLE_VALUE;
//        wprintf(L"Created TPM bound ID: %s", fileName);
    }

    if (hKeyOut != NULL)
    {
        *hKeyOut = myTpm.Load(storageKey, prvKey, pubKey.publicArea);
    }

    return S_OK;
}

NTSTATUS LoadAesIdentity(BYTE* nameIn, TPM_HANDLE* hKeyOut)
{
    HANDLE hKeyFile = INVALID_HANDLE_VALUE;
    WCHAR fileName[MAX_PATH] = L"";

    // Create the filename
    wcscpy_s(fileName, storagePrefix);
    for (UINT32 n = 0; n < 16; n++)
    {
        UINT32 cursor = wcslen(fileName);
        swprintf_s(&fileName[cursor], (MAX_PATH - cursor), L"%02x", nameIn[n]);
    }
    wcscat_s(fileName, L".TPMKEY");

    if ((hKeyFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
    {
        TPM2B_PUBLIC pubKey;
        TPM2B_PRIVATE prvKey;
        DWORD read = 0;
        DWORD fileSize = GetFileSize(hKeyFile, NULL);
        vector<BYTE> fileContent(fileSize);
        if (!ReadFile(hKeyFile, fileContent.data(), fileContent.size(), &read, NULL))
        {
            return ERROR_READ_FAULT;
        }
        InByteBuf buf(fileContent);
        buf >> prvKey >> pubKey;
        CloseHandle(hKeyFile);
        myTpm._AllowErrors();
        *hKeyOut = myTpm.Load(storageKey, prvKey, pubKey.publicArea);
        if (myTpm._GetLastError() != TPM_RC::SUCCESS)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;
}

void TpmCalculateCMAC(TPM_HANDLE* hKey, uint8_t encrypt, uint8_t* dataPtr, uint32_t dataSize, uint8_t* digestIn, uint8_t* digestOut)
{
    NTSTATUS status = S_OK;
    vector<BYTE> iv = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    vector<BYTE> dataIn = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    if (digestIn != NULL)
    {
        memcpy(iv.data(), digestIn, iv.size());
    }

    if ((dataPtr != NULL) && (dataSize != 0))
    {
        dataIn = vector<BYTE>(dataSize);
        memcpy(dataIn.data(), dataPtr, dataIn.size());
    }

    EncryptDecryptResponse EncDecResponse = myTpm.EncryptDecrypt(*hKey, (encrypt) ? (BYTE)0 : (BYTE)1, TPM_ALG_ID::CBC, iv, dataIn);

    if (digestOut != NULL)
    {
        memcpy(digestOut, EncDecResponse.ivOut.data(), EncDecResponse.ivOut.size());
    }
}

NTSTATUS OpenMyTpm(void)
{
    NTSTATUS retVal = S_OK;

#ifdef USE_TPM_SIMULATOR
    if (!tcpDev.Connect("127.0.0.1", 2321)) {
        throw runtime_error("Could not connect to TPM device.");
    }
    myTpm._SetDevice(tcpDev);
    tcpDev.PowerOff();
    tcpDev.PowerOn();
    myTpm.Startup(TPM_SU::CLEAR);
    myTpm._AllowErrors();
    ReadPublicResponse readPublicResponse = myTpm.ReadPublic(TPM_HANDLE((UINT32)0x81000001));
    if (myTpm._GetLastError() == TPM_RC::HANDLE)
    {
        // Delete all old key files
        WIN32_FIND_DATA findData = { 0 };
        HANDLE hFind = FindFirstFileW(L"*.TPMKEY", &findData);
        while(hFind != INVALID_HANDLE_VALUE)
        {
            DeleteFile(findData.cFileName);
            if (!FindNextFile(hFind, &findData))
            {
                FindClose(hFind);
                hFind = INVALID_HANDLE_VALUE;
            }
        }
        // Create the SRK
        TPMT_PUBLIC inPublic(TPM_ALG_ID::SHA256,
            TPMA_OBJECT::decrypt | TPMA_OBJECT::restricted |
            TPMA_OBJECT::sensitiveDataOrigin |
            TPMA_OBJECT::userWithAuth |
            TPMA_OBJECT::noDA |
            TPMA_OBJECT::fixedParent | TPMA_OBJECT::fixedTPM,
            NullVec,  // No policy
            TPMS_RSA_PARMS(
                TPMT_SYM_DEF_OBJECT(TPM_ALG_ID::AES, 128, TPM_ALG_ID::CFB),
                TPMS_NULL_ASYM_SCHEME(),
                2048,
                0),
            TPM2B_PUBLIC_KEY_RSA(NullVec));
        CreatePrimaryResponse createPrimaryResponse = myTpm.CreatePrimary(TPM_HANDLE((UINT32)TPM_RH::OWNER),
            TPMS_SENSITIVE_CREATE(NullVec, NullVec),
            inPublic,
            NullVec,
            vector<TPMS_PCR_SELECTION>());
        // Install it in NV
        myTpm.EvictControl(TPM_HANDLE((UINT32)TPM_RH::OWNER), createPrimaryResponse.objectHandle, TPM_HANDLE((UINT32)0x81000001));
        myTpm.FlushContext(createPrimaryResponse.objectHandle);
        createPrimaryResponse.objectHandle.handle = (UINT32)TPM_RH::_NULL;
    }
    else if (myTpm._GetLastError() != TPM_RC::SUCCESS)
    {
        // Some unexpected error
        retVal = (NTSTATUS)myTpm._GetLastError();
        goto Cleanup;
    }
    else
    {
        tpmReProvision = FALSE;
    }
#else
    tbsDev.Connect();
    myTpm._SetDevice(tbsDev);
#endif

    //if ((retVal = LoadMigrationAuthorityPub()) != S_OK)
    //{
    //    goto Cleanup;
    //}

    //if ((retVal =LoadParentStorageKey()) != S_OK)
    //{
    //    goto Cleanup;
    //}

Cleanup:
    return retVal;
}

void CloseMyTpm(void)
{
    myTpm.FlushContext(storageKey);
}
