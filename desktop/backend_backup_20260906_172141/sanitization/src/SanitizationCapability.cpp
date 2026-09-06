#include "SanitizationCapability.h"
#include "StorageDevice.h"
#include "NvmeCapability.h"
#include "AtaCapability.h"
#include <Windows.h>
#include <winioctl.h>
#include <ntddscsi.h>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

// SCSI INQUIRY Request

struct ScsiInquiryRequest
{
    SCSI_PASS_THROUGH spt;

    ULONG filler;

    UCHAR senseBuffer[32];

    UCHAR dataBuffer[96];
};

// Decode fixed-width SCSI text

static std::string decodeScsiText(
    const UCHAR *buffer,
    std::size_t length)
{
    std::string result;

    for (std::size_t i = 0;
         i < length;
         ++i)
    {
        char character =
            static_cast<char>(buffer[i]);

        if (character == '\0')
            continue;

        result += character;
    }

    while (!result.empty() &&
           result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

// Decode SCSI INQUIRY response

static void decodeInquiryResponse(
    const UCHAR *dataBuffer)
{
    int peripheralDeviceType =
        dataBuffer[0] & 0x1F;

    bool removable =
        (dataBuffer[1] & 0x80) != 0;

    int scsiVersion =
        dataBuffer[2] & 0x07;

    int responseDataFormat =
        dataBuffer[3] & 0x0F;

    int additionalLength =
        static_cast<int>(
            dataBuffer[4]);

    std::string vendor =
        decodeScsiText(
            &dataBuffer[8],
            8);

    std::string product =
        decodeScsiText(
            &dataBuffer[16],
            16);

    std::string revision =
        decodeScsiText(
            &dataBuffer[32],
            4);

    std::cout
        << "\nDecoded INQUIRY Information\n";

    std::cout
        << "Peripheral Device Type : "
        << peripheralDeviceType
        << '\n';

    std::cout
        << "Removable Medium       : "
        << (removable ? "YES" : "NO")
        << '\n';

    std::cout
        << "SCSI Version           : "
        << scsiVersion
        << '\n';

    std::cout
        << "Response Data Format   : "
        << responseDataFormat
        << '\n';

    std::cout
        << "Additional Length      : "
        << additionalLength
        << '\n';

    std::cout
        << "Vendor                 : "
        << vendor
        << '\n';

    std::cout
        << "Product                : "
        << product
        << '\n';

    std::cout
        << "Revision               : "
        << revision
        << '\n';
}

// Query Windows Storage Properties
//
// Purpose:
// Determine how Windows exposes the storage device.
//
// This does NOT perform sanitization.

static bool queryStorageProperties(
    HANDLE deviceHandle)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
        return false;

    STORAGE_PROPERTY_QUERY query{};

    query.PropertyId =
        StorageDeviceProperty;

    query.QueryType =
        PropertyStandardQuery;

    BYTE buffer[1024]{};

    DWORD bytesReturned = 0;

    BOOL success =
        DeviceIoControl(
            deviceHandle,

            IOCTL_STORAGE_QUERY_PROPERTY,

            &query,
            sizeof(query),

            buffer,
            sizeof(buffer),

            &bytesReturned,

            nullptr);

    if (!success)
    {
        std::cout
            << "\nStorage property query failed.\n";

        std::cout
            << "Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }

    if (bytesReturned <
        sizeof(STORAGE_DEVICE_DESCRIPTOR))
    {
        std::cout
            << "\nStorage property response is too small.\n";

        return false;
    }

    STORAGE_DEVICE_DESCRIPTOR *descriptor =
        reinterpret_cast<
            STORAGE_DEVICE_DESCRIPTOR *>(
            buffer);

    std::cout
        << "\nWindows Storage Property Information\n";

    // --------------------------------------------------------
    // Bus Type
    // --------------------------------------------------------

    std::cout
        << "Bus Type: ";

    switch (descriptor->BusType)
    {
    case BusTypeUsb:

        std::cout
            << "USB";

        break;

    case BusTypeScsi:

        std::cout
            << "SCSI";

        break;

    case BusTypeAta:

        std::cout
            << "ATA";

        break;

    case BusTypeSata:

        std::cout
            << "SATA";

        break;

    case BusTypeNvme:

        std::cout
            << "NVMe";

        break;

    default:

        std::cout
            << "Other / Unknown";

        break;
    }

    std::cout
        << '\n';

    // --------------------------------------------------------
    // Device Type
    // --------------------------------------------------------

    std::cout
        << "Device Type: "
        << static_cast<int>(
               descriptor->DeviceType)
        << '\n';

    // --------------------------------------------------------
    // Removable media
    // --------------------------------------------------------

    std::cout
        << "Removable Media: "
        << (descriptor->RemovableMedia
                ? "YES"
                : "NO")
        << '\n';

    // --------------------------------------------------------
    // Vendor ID
    // --------------------------------------------------------

    if (descriptor->VendorIdOffset != 0 &&
        descriptor->VendorIdOffset < bytesReturned)
    {
        const char *vendor =
            reinterpret_cast<const char *>(
                buffer +
                descriptor->VendorIdOffset);

        std::cout
            << "Windows Vendor ID: "
            << vendor
            << '\n';
    }

    // --------------------------------------------------------
    // Product ID
    // --------------------------------------------------------

    if (descriptor->ProductIdOffset != 0 &&
        descriptor->ProductIdOffset < bytesReturned)
    {
        const char *product =
            reinterpret_cast<const char *>(
                buffer +
                descriptor->ProductIdOffset);

        std::cout
            << "Windows Product ID: "
            << product
            << '\n';
    }

    return true;
}

// Print SCSI Sense Data

static void printSenseData(
    const UCHAR *senseBuffer,
    UCHAR senseLength)
{
    if (senseBuffer == nullptr ||
        senseLength == 0)
    {
        std::cout
            << "No SCSI sense data available.\n";

        return;
    }

    std::cout
        << "\nSCSI Sense Data\n";

    UCHAR responseCode =
        senseBuffer[0] & 0x7F;

    if (responseCode == 0x70 ||
        responseCode == 0x71)
    {
        if (senseLength >= 14)
        {
            UCHAR senseKey =
                senseBuffer[2] & 0x0F;

            UCHAR asc =
                senseBuffer[12];

            UCHAR ascq =
                senseBuffer[13];

            std::cout
                << "Sense Format : Fixed\n";

            std::cout
                << "Response Code: 0x"
                << std::hex
                << static_cast<int>(
                       responseCode)
                << std::dec
                << '\n';

            std::cout
                << "Sense Key    : 0x"
                << std::hex
                << static_cast<int>(
                       senseKey)
                << std::dec
                << '\n';

            std::cout
                << "ASC          : 0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(
                       asc)
                << std::dec
                << '\n';

            std::cout
                << "ASCQ         : 0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(
                       ascq)
                << std::dec
                << '\n';
        }
    }
    else if (responseCode == 0x72 ||
             responseCode == 0x73)
    {
        if (senseLength >= 4)
        {
            UCHAR senseKey =
                senseBuffer[1] & 0x0F;

            UCHAR asc =
                senseBuffer[2];

            UCHAR ascq =
                senseBuffer[3];

            std::cout
                << "Sense Format : Descriptor\n";

            std::cout
                << "Response Code: 0x"
                << std::hex
                << static_cast<int>(
                       responseCode)
                << std::dec
                << '\n';

            std::cout
                << "Sense Key    : 0x"
                << std::hex
                << static_cast<int>(
                       senseKey)
                << std::dec
                << '\n';

            std::cout
                << "ASC          : 0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(
                       asc)
                << std::dec
                << '\n';

            std::cout
                << "ASCQ         : 0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(
                       ascq)
                << std::dec
                << '\n';
        }
    }
    else
    {
        std::cout
            << "Unknown or invalid sense format.\n";
    }

    std::cout
        << "Raw Sense Data:\n";

    for (int i = 0;
         i < static_cast<int>(
                 senseLength);
         ++i)
    {
        std::cout
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(
                   senseBuffer[i])
            << ' ';

        if ((i + 1) % 16 == 0)
        {
            std::cout << '\n';
        }
    }

    std::cout
        << std::dec
        << '\n';
}

// SCSI INQUIRY

static bool testScsiPassThrough(
    HANDLE deviceHandle)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
        return false;

    ScsiInquiryRequest request{};

    request.spt.Length =
        sizeof(SCSI_PASS_THROUGH);

    request.spt.CdbLength =
        6;

    request.spt.DataIn =
        SCSI_IOCTL_DATA_IN;

    request.spt.DataTransferLength =
        sizeof(request.dataBuffer);

    request.spt.TimeOutValue =
        10;

    request.spt.DataBufferOffset =
        offsetof(
            ScsiInquiryRequest,
            dataBuffer);

    request.spt.SenseInfoLength =
        sizeof(request.senseBuffer);

    request.spt.SenseInfoOffset =
        offsetof(
            ScsiInquiryRequest,
            senseBuffer);

    // --------------------------------------------------------
    // SCSI INQUIRY
    // --------------------------------------------------------

    request.spt.Cdb[0] =
        0x12;

    request.spt.Cdb[4] =
        sizeof(request.dataBuffer);

    DWORD bytesReturned = 0;

    BOOL success =
        DeviceIoControl(
            deviceHandle,

            IOCTL_SCSI_PASS_THROUGH,

            &request,
            sizeof(request),

            &request,
            sizeof(request),

            &bytesReturned,

            nullptr);

    if (!success)
    {
        std::cout
            << "SCSI INQUIRY failed.\n";

        std::cout
            << "Windows error: "
            << GetLastError()
            << '\n';

        return false;
    }

    if (request.spt.ScsiStatus != 0)
    {
        std::cout
            << "SCSI INQUIRY returned non-zero "
               "SCSI status: "
            << static_cast<int>(
                   request.spt.ScsiStatus)
            << '\n';

        printSenseData(
            request.senseBuffer,
            request.spt.SenseInfoLength);

        return false;
    }

    std::cout
        << "\nSCSI INQUIRY successful.\n";

    decodeInquiryResponse(
        request.dataBuffer);

    return true;
}

// Detect Sanitization Capability

SanitizationCapability detectSanitizationCapability(const StorageDevice &device)
{
    SanitizationCapability capability;

    // --------------------------------------------------------
    // Step 1: Existing interface information
    // Device interface information
    // --------------------------------------------------------

    if (device.getInterfaceType() == "USB")
    {
        capability.isUsbDevice = true;
    }

    // --------------------------------------------------------
    // Step 2: Open physical device
    // Open physical storage device
    // --------------------------------------------------------

    HANDLE deviceHandle =
        CreateFileA(
            device.getDeviceId().c_str(),

            GENERIC_READ |
                GENERIC_WRITE,

            FILE_SHARE_READ |
                FILE_SHARE_WRITE,

            nullptr,

            OPEN_EXISTING,

            0,

            nullptr);

    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        DWORD error =
            GetLastError();

        std::cout
            << "Unable to open storage device.\n";

        std::cout
            << "Windows error: "
            << error
            << '\n';

        std::cout
            << "Unable to open storage device.\n";

        std::cout
            << "Windows error: "
            << GetLastError()
            << '\n';

        return capability;
    }

    // --------------------------------------------------------
    // Step 3: Query Windows storage properties
    //
    // This is non-destructive.
    // Windows Storage Properties
    // --------------------------------------------------------

    capability.storagePropertyQueryAvailable =
        queryStorageProperties(
            deviceHandle);

    // ========================================================
    // Step 4: Protocol-specific capability detection
    //
    // USB:
    //     Use SCSI capability path.
    //
    // NVMe:
    //     Use dedicated NVMe capability path.
    //
    // Other interfaces:
    //     No protocol-specific capability test yet.
    // ========================================================

    // --------------------------------------------------------
    // USB
    // --------------------------------------------------------

    if (device.getInterfaceType() == "USB")
    {
        std::cout << "\nUSB device detected.\n";

        std::cout << "Running SCSI capability detection...\n";

        capability.scsiPathAvailable = testScsiPassThrough(deviceHandle);
    }

    // --------------------------------------------------------
    // NVMe
    // --------------------------------------------------------

    else if (device.getInterfaceType() == "NVMe")
    {
        std::cout << "\nNVMe device detected.\n";

        std::cout << "Running NVMe capability detection...\n";

        bool nvmeSuccess = detectNvmeCapability(deviceHandle, capability);

        if (!nvmeSuccess)
        {
            std::cout << "NVMe capability detection failed.\n";
        }
    }

    // --------------------------------------------------------
    // Other interface
    // --------------------------------------------------------

    else if (device.getInterfaceType() == "SATA")
    {
        AtaCapability ata = detectAtaCapability(device);

        capability.ataIdentifyAvailable = ata.identifyAvailable;
        capability.ataSecuritySupported = ata.securitySupported;
        capability.ataEnhancedEraseSupported = ata.enhancedEraseSupported;
        capability.ataSecurityEnabled = ata.securityEnabled;
        capability.ataSecurityLocked = ata.securityLocked;
        capability.ataSecurityFrozen = ata.securityFrozen;
        capability.atasanitizeSupported = ata.sanitizeSupported;
        capability.atacryptoScrambleSupported = ata.cryptoScrambleSupported;
        capability.atablockEraseSupported = ata.blockEraseSupported;
        capability.ataoverwriteSupported = ata.overwriteSupported;
    }
    else
    {
        std::cout << "\nNo protocol-specific capability "
                     "detector available for interface: "
                  << device.getInterfaceType()
                  << '\n';
    }

    // ========================================================
    // Step 5: Close device
    // ========================================================

    CloseHandle(
        deviceHandle);

    return capability;

    // --------------------------------------------------------
    // Step 5:
    //
    // Do NOT execute REPORT SUPPORTED OPERATION CODES here.
    //
    // Your HP USB device previously timed out with error 121.
    // Therefore we do not repeatedly send the same failing
    // command while detecting the USB transport.
    //
    // Native sanitize remains UNKNOWN until a reliable
    // capability mechanism is established.
    // Native sanitization
    //
    // NVMe capability detection will update this later.
    // --------------------------------------------------------

    capability.nativeSanitizeSupported =
        NativeSanitizeSupport::UNKNOWN;

    // --------------------------------------------------------
    // Step 6: Close device
    // Close device
    // --------------------------------------------------------

    CloseHandle(
        deviceHandle);

    return capability;
}