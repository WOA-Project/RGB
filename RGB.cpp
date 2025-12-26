//  enumdevices.c - enumerate and open device(s)

#pragma warning( disable : 4090 )
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#include <INITGUID.H>

#pragma comment(lib, "setupapi.lib")

#define FILE_DEVICE_PMIC_RGB_LED    32783

DEFINE_GUID(GUID_DEVINTERFACE_PMIC_RGB_LED,
    0x2f706348, 0x47c5, 0x4873, 0xa6, 0x6c, 0x6b, 0x6b, 0xc6, 0xb0, 0x16, 0x98);

typedef enum
{
    LED_SRC_GND,           // ground(no power
    LED_SRC_VINRGB_VBOOST, // SRC_VINRGB_VBOOST/ Vboost_bypass
    LED_SRC_RESERVED,      // reserved (not used)
    LED_SRC_VSYS           //-> ~4.2V, internally clamped by charger module
} RGBLED_SRC_TYPE;

#define IOCTL_PM_RGB_LED_ENABLE \
	CTL_CODE(FILE_DEVICE_PMIC_RGB_LED, 1000, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Deprecated in PM855x */
#define IOCTL_PM_RGB_LED_SRC_SEL \
	CTL_CODE(FILE_DEVICE_PMIC_RGB_LED, 1001, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PM_RGB_LED_FIRST IOCTL_PM_RGB_LED_ENABLE
#define IOCTL_PM_RGB_LED_LAST IOCTL_PM_RGB_LED_SRC_SEL

typedef enum {
    PM_RGB_LED1,	// LED1 = 0x80 = bit 7 of register 0x46
    PM_RGB_LED2,	// LED2 = 0x40 = bit 6 of register 0x46
    PM_RGB_LED3,	// LED3 = 0x20 = bit 5 of register 0x46
    PM_RGB_LED_MAX
}PM_RGB_LED_TYPE;

typedef enum _pm_register_number_type
{
    PMIC_NUMBER1,
    PMIC_NUMBER2,
    PMIC_NUMBER3,
    PMIC_NUMBER4,
    PMIC_NUMBER5,
    PMIC_NUMBER6,
    PMIC_NUMBER_MAX,
    PMIC_NUMBER_INVALID = PMIC_NUMBER_MAX,
} pm_register_number_type;

// IOCTL_PM_RGB_LED_ENABLE
typedef struct _PM_RGB_LED_ENABLE
{
    UINT32 rgb_led_type;   	// R: 0x80; G: 0x40; B; 0x20
    // DEPRECATED in PM855x onwards, use input as 'led_type'
    BOOLEAN bEnabled;       // 0/1: disable/enable the (rgb_led_type) LED driver
    BOOLEAN bAtcEnabled;	// DEPRECATED in PM855x
    // 0: do not allow ATC_LED_EN from SMBC to enable the LED
    // 1: allow ATC_LED_EN to force on the LED with 4mA current
    PM_RGB_LED_TYPE led_type;
    pm_register_number_type pmic_number;
} PM_RGB_LED_ENABLE_TYPE, * PPM_RGB_LED_ENABLE_TYPE;



// IOCTL_PM_RGB_LED_SRC_SEL
typedef struct _PM_RGB_LED_SRC_SEL
{
    UINT8 led_src_sel;   // 0x0 : SRC_GND -> ground(no power)
    // 0x1 : SRC_VINRGB_VBOOST -> VIN_RGB / Vboost_bypass
    // 0x2 : reserved
    // 0x3 : SRC_VSYS -> ~4.2V, internally clamped by charger module
    pm_register_number_type pmic_number;
} PM_RGB_LED_SRC_SEL_TYPE, * PPM_RGB_LED_SRC_SEL_TYPE;



/**
 * These are all the error codes returned by PMIC APIs
 *
 * PMIC APIs most commonly return errors if the caller has tried
 * to use parameters that are out of range or if the feature is
 * not supported by the PMIC model detected at run-time.
 */
typedef enum
{
    /**
     * The API completed successfully
     */
    PM_ERR_FLAG__SUCCESS,
    /**
     * The SBI operation Failed
     * extra lines are to support += error codes
     */
    PM_ERR_FLAG__SPMI_OPT_ERR,
    PM_ERR_FLAG__SPMI_OPT2_ERR,
    PM_ERR_FLAG__SPMI_OPT3_ERR,
    PM_ERR_FLAG__SPMI_OPT4_ERR,

    PM_ERR_FLAG__I2C_OPT_ERR,
    PM_ERR_FLAG__I2C_OPT2_ERR,
    PM_ERR_FLAG__I2C_OPT3_ERR,
    PM_ERR_FLAG__I2C_OPT4_ERR,

    PM_ERR_FLAG__USB_OPT_ERR,
    PM_ERR_FLAG__USB_OPT2_ERR,
    PM_ERR_FLAG__USB_OPT3_ERR,
    PM_ERR_FLAG__USB_OPT4_ERR,

    /**
     *  Input Parameter one is out of range
     */
    PM_ERR_FLAG__PAR1_OUT_OF_RANGE,
    /**
     *  Input Parameter two is out of range
     */
    PM_ERR_FLAG__PAR2_OUT_OF_RANGE,
    /**
     *  Input Parameter three is out of range
     */
    PM_ERR_FLAG__PAR3_OUT_OF_RANGE,
    /**
     *  Input Parameter four is out of range
     */
    PM_ERR_FLAG__PAR4_OUT_OF_RANGE,
    /**
     *  Input Parameter five is out of range
     */
    PM_ERR_FLAG__PAR5_OUT_OF_RANGE,
    /**
     *  Input Parameter six is out of range
     */
    PM_ERR_FLAG__PAR6_OUT_OF_RANGE,
    /**
    *  Input Parameter seven is out of range
    */
    PM_ERR_FLAG__PAR7_OUT_OF_RANGE,
    /**
    *  Input Parameter eight is out of range
    */
    PM_ERR_FLAG__PAR8_OUT_OF_RANGE,
    /**
     *  Input Parameter is out of range
     */
    PM_ERR_FLAG__PAR_OUT_OF_RANGE,
    PM_ERR_FLAG__VLEVEL_OUT_OF_RANGE,
    PM_ERR_FLAG__VREG_ID_OUT_OF_RANGE,
    /**
     *  This feature is not supported by this PMIC
     */
    PM_ERR_FLAG__FEATURE_NOT_SUPPORTED,
    PM_ERR_FLAG__INVALID_PMIC_MODEL,
    PM_ERR_FLAG__SECURITY_ERR,
    PM_ERR_FLAG__IRQ_LIST_ERR,
    PM_ERR_FLAG__DEV_MISMATCH,
    PM_ERR_FLAG__ADC_INVALID_RES,
    PM_ERR_FLAG__ADC_NOT_READY,
    PM_ERR_FLAG__ADC_SIG_NOT_SUPPORTED,
    /**
     *  The RTC displayed mode read from the PMIC is invalid
     */
    PM_ERR_FLAG__RTC_BAD_DIS_MODE,
    /**
     *  Failed to read the time from the PMIC RTC
     */
    PM_ERR_FLAG__RTC_READ_FAILED,
    /**
     *  Failed to write the time to the PMIC RTC
     */
    PM_ERR_FLAG__RTC_WRITE_FAILED,
    /**
     *  RTC not running
     */
    PM_ERR_FLAG__RTC_HALTED,
    /**
     *  The DBUS is already in use by another MPP.
     */
    PM_ERR_FLAG__DBUS_IS_BUSY_MODE,
    /**
     *  The ABUS is already in use by another MPP.
     */
    PM_ERR_FLAG__ABUS_IS_BUSY_MODE,
    /**
     *  The error occurs illegal value that isn't in the
     *  macro_type enum is used
     */
    PM_ERR_FLAG__MACRO_NOT_RECOGNIZED,
    /**
     *  This error occurs if the data read from a register does
     *  not match the setting data
     */
    PM_ERR_FLAG__DATA_VERIFY_FAILURE,
    /**
     *  The error occurs illegal value that isn't in the
     *  pm_register_type enum is used
     */
    PM_ERR_FLAG__SETTING_TYPE_NOT_RECOGNIZED,
    /**
     * The error occurs illegal value that isn't in the
     * pm_mode_group enum is used
     */
    PM_ERR_FLAG__MODE_NOT_DEFINED_FOR_MODE_GROUP,
    /**
     *  The error occurs illegal value that isn't in the pm_mode
     *  enum is used
     */
    PM_ERR_FLAG__MODE_GROUP_NOT_DEFINED,
    /**
     *  This error occurs if the PRESTUB function returns a false
     */
    PM_ERR_FLAG__PRESTUB_FAILURE,
    /**
     *  This error occurs if the POSTSTUB function returns a
     *  false
     */
    PM_ERR_FLAG__POSTSTUB_FAILURE,
    /**
     *  When modes are set for a modegroup, they are recorded and
     *  checked for success
     */
    PM_ERR_FLAG__MODE_NOT_RECORDED_CORRECTLY,
    /**
     *  Unable to find the mode group in the mode group recording
     *  structure. Fatal memory problem
     */
    PM_ERR_FLAG__MODE_GROUP_STATE_NOT_FOUND,
    /**
     *  This error occurs if the SUPERSTUB function return a
     *  false
     */
    PM_ERR_FLAG__SUPERSTUB_FAILURE,
    /* Processor does not have access to this resource */
    PM_ERR_FLAG__TARGET_PROCESSOR_CAN_NOT_ACCESS_RESOURCE,
    /* Resource is invalid. Resource index was pass
     * into router that is not defined in the build
     */
    PM_ERR_FLAG__INVALID_RESOURCE_ACCESS_ATTEMPTED,

    /*! Non-Zero means unsuccessful call. Here it means registration failed
         because driver has ran out of memory.MAX_CLIENTS_ALLOWED needs to be
         increased and the code needs to be recompiled */
    PM_ERR_FLAG__VBATT_CLIENT_TABLE_FULL,
    /*! One of the parameters to the function call was invalid. */
    PM_ERR_FLAG__VBATT_REG_PARAMS_WRONG,
    /*! Client could not be deregistered because perhaps it does not exist */
    PM_ERR_FLAG__VBATT_DEREGISTRATION_FAILED,
    /*! Client could not be modified because perhaps it does not exist */
    PM_ERR_FLAG__VBATT_MODIFICATION_FAILED,
    /*!< Client could not be queried because perhaps it does not exist */
    PM_ERR_FLAG__VBATT_INTERROGATION_FAILED,
    /*!< Client's filetr could not be set because perhaps it does not exist */
    PM_ERR_FLAG__VBATT_SET_FILTER_FAILED,
    /*! Keeps the count of all errors. Any error code equal to or greater
         than this one means an unknown error. VBATT_LAST_ERROR should be
         the last error code always with the highest value */
    PM_ERR_FLAG__VBATT_LAST_ERROR,
    PM_ERR_FLAG__PMIC_NOT_SUPPORTED,

    /* Non Vibrator Module is being indexed */
    PM_ERR_FAG__INVALID_VIBRATOR_INDEXED,

    /* Non PWM generator is being indexed */
    PM_ERR_FLAG__INVALID_PWM_GENERATOR_INDEXED,

    /* Non Chg Module is being indexed */
    PM_ERR_FLAG__INVALID_CHG_INDEXED,

    PM_ERR_FAG__INVALID_CLK_INDEXED,

    PM_ERR_FAG__INVALID_XO_INDEXED,

    PM_ERR_FAG__INVALID_XOADC_INDEXED,

    PM_ERR_FAG__INVALID_TCXO_INDEXED,

    PM_ERR_FAG__INVALID_RTC_INDEXED,

    PM_ERR_FLAG__API_NOT_IMPLEMENTED,

    PM_ERR_FAG__INVALID_PAONCNTRL_INDEXED,

    /* Non Coincell Module is being indexed */
    PM_ERR_FLAG__INVALID_COINCELL_INDEXED,

    /* Non Flash Module is being indexed */
    PM_ERR_FLAG__INVALID_FLASH_INDEXED,

    /* Non OVP Module is being indexed */
    PM_ERR_FLAG__INVALID_OVP_INDEXED,

    /* Non KEYPAD Module is being indexed */
    PM_ERR_FLAG__INVALID_KEYPAD_INDEXED,

    /* Non LVS Module is being indexed */
    PM_ERR_FLAG__INVALID_LVS_INDEXED,

    /* Non HSED Module is being indexed */
    PM_ERR_FLAG__INVALID_HSED_INDEXED,

    /* Non TALM Module is being indexed */
    PM_ERR_FLAG__INVALID_TALM_INDEXED,

    /* Non NCP Module is being indexed */
    PM_ERR_FLAG__INVALID_NCP_INDEXED,

    /* Non MVS Module is being indexed */
    PM_ERR_FLAG__INVALID_MVS_INDEXED,

    /* Non HDMI Module is being indexed */
    PM_ERR_FLAG__INVALID_HDMI_INDEXED,

    /* Non UVLO Module is being indexed */
    PM_ERR_FLAG__INVALID_UVLO_INDEXED,

    /* Non AMUX Module is being indexed */
    PM_ERR_FLAG__INVALID_AMUX_INDEXED,

    PM_ERR_FLAG__INVALID_KEYPAD_EVENT_COUNTER,

    PM_ERR_FLAG__INVALID_BATTERY_CELL_NUMBER,

    PM_ERR_FLAG__INVALID_PWRON_INDEXED,

    PM_ERR_FLAG__INVALID_VBATT_INDEXED,

    PM_ERR_FLAG__SHADOW_REGISTER_INIT_FAILED,

    /* PSDTE Error Functionality */
    PM_ERR_FLAG__PSDTE_ENV_FAILURE,

    PM_ERR_FLAG__PSDTE_PMIC_POWERUP_FAILED,

    PM_ERR_FLAG__PSDTE_PMIC_POWERDOWN_FAILED,

    PM_ERR_FLAG__FTS_CALCULATION_ERROR,

    PM_ERR_FLAG__API_DEPRICATED_ERROR,

    PM_ERR_FLAG__RPC_PROCESSOR_NOT_RECOGNIZED_ERROR,

    /* VREG Errors */
    PM_ERR_FLAG__VREG_VOTE_DEREGISTER_ERROR,

    PM_ERR_FLAG__VREG_VOTE_REGISTER_ERROR,

    PM_ERR_FLAG__VREG_INTERFACE_INITIALIZATION_ERROR,

    PM_ERR_FLAG__VREG_INTERFACE_CMD_ERROR,

    PM_ERR_FLAG__OUT_OF_MEMORY,

    PM_ERR_FLAG__MEMORY_ERROR,

    PM_ERR_FLAG__INVALID_DATA,

    PM_ERR_FLAG__INVALID_PARAM,

    PM_ERR_FLAG__INVALID,

    /* PMIC SMBC Errors */
    PM_ERR_FLAG__CHG_REG_BASEADDR_NOT_SET,		// Register base address was not yet set

    /* PMIC PSI Errors */
    PM_ERR_FLAG__PSI_ERROR_CLOSED,		// PSI communications is closed

    PM_ERR_FLAG__PSI_ERROR_IN_USE,		// PSI is in use

    PM_ERR_FLAG__PSI_ERROR_DISABLED,    // PSI is disabled

    PM_ERR_FLAG__PSI_ERROR_NACK,        // PSI error if remote device communication error was detected.

    PM_ERR_FLAG__PSI_ERROR_RECEIVE,     // PSI error if there is a problem receiving data

    PM_ERR_FLAG__PSI_ERROR_OVERFLOW,    // PSI error if receive buffer overflowed

    /*Keypad Errors*/
    PM_ERR_FLAG__KEYPAD_NOT_CONFIGURED,

    PM_ERR_FLAG__KEYPAD_SCAN_ERROR,

    /**
     * The API failed
     */
    PM_ERR_FLAG__FAILURE

} pm_err_flag_type, * ppm_err_flag_type;


int
main(int argc, char** argv)
{
#define ALLOC(size)  GlobalAlloc( GPTR, size)

    SP_DEVINFO_DATA* devData;
    HANDLE devSet;
    HANDLE hDev;
    SP_DEVICE_INTERFACE_DATA* devIfData;
    SP_DEVICE_INTERFACE_DETAIL_DATA* Details;
    const GUID* devGuid;
    DWORD needed;
    DWORD unused;
    int count;          // count of enumerated devices
    DWORD idev;         // device index
    DWORD iface;        // interface index
    WCHAR deviceID[200]; // device id string
    ULONG IDSize;
    BOOL ok;

    devData = (SP_DEVINFO_DATA*)ALLOC(sizeof(SP_DEVINFO_DATA));
    devData->cbSize = sizeof(SP_DEVINFO_DATA);

    // GET SET OF DEVICE INTERFACES PRESENT OF SPECIFIED devGuid
    devGuid = &GUID_DEVINTERFACE_PMIC_RGB_LED;     // set dev class guid to enumerate
    devSet = SetupDiGetClassDevs(devGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    // OUTER LOOP
    idev = 0;
    count = 0;
    while (TRUE) {
        // GET DEVICE INFO DATA
        ok = SetupDiEnumDeviceInfo(devSet, idev, devData);
        if (!ok) break;

        // GET ID SIZE
        devData->cbSize = sizeof(SP_DEVINFO_DATA);
        CM_Get_Device_ID_Size(&IDSize, devData->DevInst, 0);

        // GET DEVICE ID
        CM_Get_Device_ID(devData->DevInst, deviceID, 200, 0);
        wprintf(L"Device Instance #%d: deviceId = \"%s\"\n", devData->DevInst, deviceID); // print it
        count++;

        devIfData = (SP_DEVICE_INTERFACE_DATA*)ALLOC(sizeof(SP_DEVICE_INTERFACE_DATA));
        devIfData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        iface = 0;                                  // init index
        while (TRUE) {                            // loop over all interfaces in set
            // GET DEVICE INTERFACE DATA index=iface
            ok = SetupDiEnumDeviceInterfaces(
                devSet,                 // handle to interface set
                devData,
                devGuid, //&GUID_DEVINTERFACE_USB_DEVICE, 
                iface,                              // interface index
                devIfData);
            if (!ok) break;

            // GET NEEDED BUFFER SIZE
            devIfData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            ok = SetupDiGetDeviceInterfaceDetail(
                devSet,
                devIfData,
                NULL,
                0,
                &needed,
                0);

            Details = (SP_INTERFACE_DEVICE_DETAIL_DATA*)ALLOC(needed);
            Details->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);  // IMPORTANT!

            // GET DEVICE DETAILS
            ok = SetupDiGetDeviceInterfaceDetail(
                devSet,                 // device set
                devIfData,              // device info data
                Details,                // detail data
                needed,                 // size of Details
                &unused,                // unused
                NULL);                 // device info data (can be NULL)
            wprintf(L"%s\n", Details->DevicePath);        // announce

            // OPEN DEVICE
            hDev = CreateFile(Details->DevicePath,
                GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
            if (hDev != INVALID_HANDLE_VALUE) {
                wprintf(L"Device successfully opened\n");
                // DO SOMETHING WITH DEVICE HANDLE (e.g., DeviceIoControl)...

                PM_RGB_LED_ENABLE_TYPE rgbInput;
                pm_err_flag_type result;
                DWORD bytesReturned = 0;

                rgbInput.led_type = PM_RGB_LED1;
                rgbInput.bEnabled = TRUE;
                rgbInput.pmic_number = PMIC_NUMBER2;

				wprintf(L"Sending IOCTL_PM_RGB_LED_ENABLE\n");

				BOOL successSent = DeviceIoControl(
                    hDev, 
                    IOCTL_PM_RGB_LED_ENABLE, 
                    &rgbInput,
                    sizeof(PM_RGB_LED_ENABLE_TYPE), 
                    &result, 
                    sizeof(pm_err_flag_type), 
                    &bytesReturned,
                    NULL
                );
                
				wprintf(L"IOCTL_PM_RGB_LED_ENABLE sent\n");
				wprintf(L"DeviceIoControl returned %d\n", successSent);
				wprintf(L"returnedBytes = %d\n", bytesReturned);
				wprintf(L"result = %d\n", result);

                fwprintf(stderr, L"Press any key to continue...\n");
                _getch();

                rgbInput.led_type = PM_RGB_LED2;
                rgbInput.bEnabled = TRUE;
                rgbInput.pmic_number = PMIC_NUMBER2;

                wprintf(L"Sending IOCTL_PM_RGB_LED_ENABLE\n");

                successSent = DeviceIoControl(
                    hDev,
                    IOCTL_PM_RGB_LED_ENABLE,
                    &rgbInput,
                    sizeof(PM_RGB_LED_ENABLE_TYPE),
                    &result,
                    sizeof(pm_err_flag_type),
                    &bytesReturned,
                    NULL
                );

                wprintf(L"IOCTL_PM_RGB_LED_ENABLE sent\n");
                wprintf(L"DeviceIoControl returned %d\n", successSent);
                wprintf(L"returnedBytes = %d\n", bytesReturned);
                wprintf(L"result = %d\n", result);

                fwprintf(stderr, L"Press any key to continue...\n");
                _getch();

                rgbInput.led_type = PM_RGB_LED3;
                rgbInput.bEnabled = TRUE;
                rgbInput.pmic_number = PMIC_NUMBER2;

                wprintf(L"Sending IOCTL_PM_RGB_LED_ENABLE\n");

                successSent = DeviceIoControl(
                    hDev,
                    IOCTL_PM_RGB_LED_ENABLE,
                    &rgbInput,
                    sizeof(PM_RGB_LED_ENABLE_TYPE),
                    &result,
                    sizeof(pm_err_flag_type),
                    &bytesReturned,
                    NULL
                );

                wprintf(L"IOCTL_PM_RGB_LED_ENABLE sent\n");
                wprintf(L"DeviceIoControl returned %d\n", successSent);
                wprintf(L"returnedBytes = %d\n", bytesReturned);
                wprintf(L"result = %d\n", result);

                CloseHandle(hDev);
            }

            iface++;
        }

        idev++;     // next device
    }

    wprintf(L"\nenumerated %d device interfaces\n", count);
    fwprintf(stderr, L"Press any key to exit...\n");
    _getch();
}