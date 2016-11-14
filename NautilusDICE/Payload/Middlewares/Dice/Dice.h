
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

extern CRYP_HandleTypeDef hcryp;
extern FIREWALL_InitTypeDef fw_init;

#define DICE_CODE_START (0x08000000)
#define DICE_CODE_LEN   (0x00008000)
#define IS_DICE_CODE(__add) ((__add >= DICE_CODE_START) && (__add < DICE_CODE_START + DICE_CODE_LEN))
//#define SECRETHWDEVICEID_START (DICECODE_START + DICECODE_LEN) // 0x08008000
//#define SECRETHWDEVICEID_LEN (256) // 0x100
//#define IS_SECRETHWDEVICEID(__add) ((__add >= SECRETHWDEVICEID_START) && (__add < SECRETHWDEVICEID_START + SECRETHWDEVICEID_LEN))
//#define BOOTPOLICYTABLE_START (SECRETHWDEVICEID_START + SECRETHWDEVICEID_LEN) // 0x08008100
//#define BOOTPOLICY_ENTRIES (8)
//#define BOOTPOLICYTABLE_LEN (BOOTPOLICY_ENTRIES * 16) // 0x80
//#define IS_BOOTPOLICYTABLE(__add) ((__add >= BOOTPOLICYTABLE_START) && (__add < BOOTPOLICYTABLE_START + BOOTPOLICYTABLE_LEN))
//#define APPPAYLOADINFO_START (BOOTPOLICYTABLE_START + BOOTPOLICYTABLE_LEN) // 0x08008180
//#define APPPAYLOADINFO_LEN (128) // 0x80
//#define IS_APPPAYLOADINFO(__add) ((__add >= APPPAYLOADINFO_START) && (__add < APPPAYLOADINFO_START + APPPAYLOADINFO_LEN))
//#define IS_APPPAYLOAD(__add) ((__add >= APPPAYLOAD_START) && (__add < APPPAYLOAD_START + APPPAYLOAD_LEN))
//#define DATAEEPROM_START (0x08080000)
//#define DATAEEPROM_LEN (6144) // 0x1800
//#define IS_DATAEEPROM(__add) ((__add >= DATAEEPROM_START) && (__add < DATAEEPROM_START + DATAEEPROM_LEN))

#define DICE_DIGEST_SIZE (16)
typedef enum
{
    DICE_DEVICE_CLASS = 0,
    DICE_MANUFACTURER_CLASS,
    DICE_OPERATOR_CLASS,
    DICE_VOLATILE_NUM_CLASSES
} DICE_VOLATILE_DATA_CLASSES;

typedef enum
{
    DICE_MAILBOX_COMMAND_NONE = 0,
    DICE_MAILBOX_COMMAND_SELFDESTRUCT,
    DICE_MAILBOX_COMMAND_LOCK_APPPAYLOAD,
    DICE_MAILBOX_COMMAND_LOCK_MANUFACTURER,
    DICE_MAILBOX_COMMAND_NUM_COMMANDS
} DICE_MAILBOX_COMMAND;

#define DICE_VOLATILE_DATA_TABLE_ADDRESS (0x20000000)
#define DICE_MAILBOX_COMMAND_TAG (0x43424D44)
#define DICE_VOLATILE_DATA_TAG (0x444F5644)
typedef struct
{
    uint32_t tag;
    uint32_t firewallArmed;
    union
    {
        struct
        {
            DICE_MAILBOX_COMMAND command;
            uint8_t data[124];
            uint8_t integrity[DICE_DIGEST_SIZE];
        } mailbox;
        struct
        {
            // deviceName = ENC(0, deviceId), manufacturerName = ENC(0, manufacturerId, softwareStrongName
            uint8_t diceRootNames[DICE_VOLATILE_NUM_CLASSES][DICE_DIGEST_SIZE];
            // deviceCompoundName = ENC(0, deviceCompoundId), manufacturerCompoundName = ENC(0, manufacturerCompoundId), softwareCompoundName = ENC(0, softwareCompoundId)
            uint8_t diceCompoundNames[DICE_VOLATILE_NUM_CLASSES][DICE_DIGEST_SIZE];
            // deviceCompoundId = ENC(softwareStrongName, deviceId), manufacturerCompoundId = ENC(manufacturerSalt, manufacturerId), softwareCompoundId = softwareId
            uint8_t diceCompoundIds[DICE_VOLATILE_NUM_CLASSES][DICE_DIGEST_SIZE];
        } tables;
    } info;
} DICE_VOLATILE_DATA_TABLE, *PDICE_VOLATILE_DATA_TABLE; // 148 bytes
extern PDICE_VOLATILE_DATA_TABLE diceVolatileData;

#define DICE_PROTECTED_PERSISTED_DATA_ADDRESS (0x08008000)
#define DICE_PROTECTED_PERSISTED_DATA_LEN (256)
#define IS_PROTECTED_PERSISTED_DATA(__add) ((__add >= DICE_PROTECTED_PERSISTED_DATA_ADDRESS) && (__add < DICE_PROTECTED_PERSISTED_DATA_ADDRESS + DICE_PROTECTED_PERSISTED_DATA_LEN))
#define DICE_PROTECTED_PERSISTED_DATA_TAG (0x45434944)
typedef struct
{
    uint32_t tag;
    uint32_t size;
    uint32_t version;
    uint32_t date;
    char model[32];
    char serial[32];
    char manufacturer[32];
    char url[96];
    uint8_t deviceID[DICE_DIGEST_SIZE];            // Device specific entropy
    uint8_t manufacturerID[DICE_DIGEST_SIZE];      // ManufacturerId policy - if set appPayloads have to be authorized with this ID
    uint8_t softwareStrongName[DICE_DIGEST_SIZE];  // softwareStrongName policy - if set nonNULL, appPayload to match with this with (-1 wildcard policy)
} DICE_PROTECTED_PERSISTED_DATA_CONTAINER, *PDICE_PROTECTED_PERSISTED_DATA_CONTAINER; // Has to be 256 bytes to be protected with the firewall
extern PDICE_PROTECTED_PERSISTED_DATA_CONTAINER diceProtectedData;

#define DICE_APP_PAYLOAD_INFO_ADDRESS (0x08008180)
#define DICE_APP_PAYLOAD_INFO_LEN (128)
#define IS_APP_PAYLOAD_INFO(__add) ((__add >= DICE_APP_PAYLOAD_INFO_ADDRESS) && (__add < DICE_APP_PAYLOAD_INFO_ADDRESS + DICE_APP_PAYLOAD_INFO_LEN))
#define DICE_APP_PAYLOAD_ADDRESS (0x08008200)
#define DICE_APP_PAYLOAD_LEN (0x08030000 - DICE_APP_PAYLOAD_ADDRESS) // 0x27E00
#define IS_APP_PAYLOAD(__add) ((__add >= DICE_APP_PAYLOAD_ADDRESS) && (__add < DICE_APP_PAYLOAD_ADDRESS + DICE_APP_PAYLOAD_LEN))
#define DICE_APPPAYLOAD_INFO_TAG (0x50504144)
typedef struct
{
    uint32_t tag;
    uint32_t appVersion;
    uint32_t appSize;
    uint32_t appDate;
    char appName[24];
    char manufacturer[24];
    uint8_t strongName[DICE_DIGEST_SIZE];       // Digest of the code DeviceCompoundId = ENC(strongName, deviceID)
    uint8_t manufacturerSalt[DICE_DIGEST_SIZE]; // Entropy to calculate ManufacturerCompoundId = ENC(manufacturerSalt, manufacturerID)
    uint8_t encOperatorId[DICE_DIGEST_SIZE];    // Encryped secret OperatorId = DEC(encOperatorId, ENC(strongName, manufacturerID))
    uint8_t authorization[DICE_DIGEST_SIZE];    // Integrity protection authorization = ENC((DICE_APPPAYLOADINFO - authorization), manufacturerID)
} DICE_APP_PAYLOAD_INFO, *PDICE_APP_PAYLOAD_INFO; // 128 bytes
extern PDICE_APP_PAYLOAD_INFO diceAppPayloadInfo;

#define DICE_CMAC_ENCRYPT (1)
#define DICE_CMAC_DECRYPT (0)

void DiceInitialize(void);
uint8_t DiceChkInitialized(uint8_t* dataPtr, uint8_t dataByte, uint32_t dataSize);
#define DiceChkZero(__ptr, __size) DiceChkInitialized(__ptr, 0x00, __size)
#define DiceChkFF(__ptr, __size) DiceChkInitialized(__ptr, 0xFF, __size)
void DiceDumpProtectedData(FILE* f);
void DiceDumpAppInfo(FILE* f);
void DiceVolatileData(FILE* f);
HAL_StatusTypeDef DiceCalculateCMAC(uint8_t encrypt, uint8_t* dataPtr, uint32_t dataSize, uint8_t* keyIn, uint8_t* digestIn, uint8_t* digestOut);
HAL_StatusTypeDef DiceDeriveData(void);
HAL_StatusTypeDef DiceLockOutPersistedData(void);
HAL_StatusTypeDef DiceAuthorizeAppPayload(void);
